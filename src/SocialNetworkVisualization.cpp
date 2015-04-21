#include "SocialNetworkVisualization.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
SocialNetworkVisualization::~SocialNetworkVisualization() {
    // Disconnects and frees the redis context
    redisFree(redis);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Download the adjacency list from the backend Redis database and save user info to Particles
void SocialNetworkVisualization::setup()
{
    connectRedis();
    redisReply *keyReply = (redisReply*)redisCommand(redis,"KEYS list@*");
    if (keyReply->type == REDIS_REPLY_ARRAY) {
        Vec2f p1 = getWindowCenter() + Vec2f::one() * 2.5 * EDGE_LEN;
        
        for (int i = 0; i < keyReply->elements; ++i) {
            string name = string(keyReply->element[i]->str).substr(5);
            Particle *center = createParticleForUser(name, p1);
            
            cout << name << endl;
            cout << "  id: " << center->_info["id"] << endl;
            cout << "  name: " << center->_info["name"] << endl;
            cout << "  friends count: " << center->_info["friends_count"] << endl;
            cout << "  followers count: " << center->_info["followers_count"] << endl;
            
            redisReply *listReply = (redisReply*)redisCommand(redis,"LRANGE %s 0 -1", ("list@" + name).c_str());
            if (listReply->type == REDIS_REPLY_ARRAY) {
                Vec2f p2 = center->_currPosition + Vec2f::one() * EDGE_LEN;
                
                for (int j = 0; j < listReply->elements; ++j) {
                    string nameOfNeighbor = string(listReply->element[j]->str);
                    Particle *neighbor = createParticleForUser(nameOfNeighbor, p2);
                    mLinks.push_back(make_pair(center, neighbor));
                    
                    cout << nameOfNeighbor << endl;
                    cout << "  id: " << neighbor->_info["id"] << endl;
                    cout << "  name: " << neighbor->_info["name"] << endl;
                    cout << "  friends count: " << neighbor->_info["friends_count"] << endl;
                    cout << "  followers count: " << neighbor->_info["followers_count"] << endl;
                    
                    p2.rotate(2 * M_PI / listReply->elements);
                }
                freeReplyObject(listReply);
                p1.rotate(2 * M_PI / keyReply->elements);
            }
        }
    }
    freeReplyObject(keyReply);
}


void SocialNetworkVisualization::mouseDown( MouseEvent event ){
    float minDist = 20.f;
    mIsHandle = false;
    for(auto it = mParticleSystem.particles.begin(); it != mParticleSystem.particles.end(); ++it) {
        float dist = (*it)->_currPosition.distance(event.getPos());
        if(dist < minDist) {
            mIsHandle = true;
            minDist = dist;
            
            if (mIsHandle && mHandle) {
                mHandle->deselected();
            }
            
            mHandle = (*it);
            mHandle->selected();
        }
    }
}

void SocialNetworkVisualization::mouseUp(MouseEvent event){
    mIsHandle = false;
    for(auto it = mParticleSystem.particles.begin(); it != mParticleSystem.particles.end(); ++it) {
        (*it)->deselected();
    }
}

void SocialNetworkVisualization::update() {
    // calculate replusion
    for(auto it1 = mParticleSystem.particles.begin(); it1 != mParticleSystem.particles.end(); ++it1) {
        for(auto it2 = mParticleSystem.particles.begin(); it2 != mParticleSystem.particles.end(); ++it2) {
            Vec2f conVec = (*it2)->_currPosition - (*it1)->_currPosition;
            if(conVec.length() < 0.1f)
                continue;
            float distance = conVec.length();
            conVec.normalize();
            float force = (distance - EDGE_LEN * 2.0f) * 0.1f;
            force = math<float>::min(0.f, force);
            (*it1)->_forces += conVec * force * 0.5f * (*it2)->_forceFactor;
            (*it2)->_forces += -conVec * force * 0.5f * (*it1)->_forceFactor;
        }
    }
    // calculate attraction
    for(auto it = mLinks.begin(); it != mLinks.end(); ++it ) {
        Vec2f conVec = it->second->_currPosition - it->first->_currPosition;
        float distance = conVec.length();
        float diff = (distance-EDGE_LEN)/distance;
        it->first->_forces += conVec * 0.5f * diff;
        it->second->_forces -= conVec * 0.5f * diff;
    }
    if(mIsHandle) {
        mHandle->_currPosition = getMousePos() - getWindowPos();
        mHandle->_forces = Vec2f::zero();
    }
    mParticleSystem.update();
}

void SocialNetworkVisualization::draw() {
    // clear out the window with black
    gl::enableAlphaBlending();
    gl::clear( Color::white() );
    gl::setViewport(getWindowBounds());
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    gl::color( ColorA(0.f,0.f,0.f, 0.8f) );
    for(auto it = mLinks.begin(); it != mLinks.end(); ++it) {
        Vec2f conVec = it->second->_currPosition - it->first->_currPosition;
        conVec.normalize();
        
        gl::drawLine(it->first->_currPosition + conVec * ( it->first->_radius+2.f ),
                     it->second->_currPosition - conVec * ( it->second->_radius+2.f ) );
    }
    gl::color( ci::ColorA(0.f,0.f,0.f, 0.8f) );
    mParticleSystem.draw();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Connect to Redis database
void SocialNetworkVisualization::connectRedis() {
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    redis = redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    if (redis == NULL || redis->err) {
        if (redis) {
            printf("Connection error: %s\n", redis->errstr);
            redisFree(redis);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Create a particle for a user at position if there's no particle for the user yet
Particle *SocialNetworkVisualization::createParticleForUser(string name, Vec2f position) {
    Particle *particle;
    if (map.count(name) > 0)
        particle = map[name];
    else {
        particle = new Particle(position, NODE_SIZE, NODE_MASS, NODE_DRAG);
        mParticleSystem.addParticle(particle);
        map[name] = particle;
    }
    particle->_info = getInfoForUser(name);
    return particle;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Get profile information for specified user from Redis
unordered_map<string, string> SocialNetworkVisualization::getInfoForUser(string name) {
    unordered_map<string, string> info;
    redisReply *infoReply = (redisReply*)redisCommand(redis, "HKEYS %s", ("info@" + name).c_str());
    
    if (infoReply->type == REDIS_REPLY_ARRAY) {
        for (int i = 0; i < infoReply->elements; ++i) {
            string field(infoReply->element[i]->str);
            redisReply *fieldReply = (redisReply*)redisCommand(redis, "HGET %s %s", ("info@" + name).c_str(), field.c_str());
            
            if (fieldReply->type == REDIS_REPLY_STRING)
                info[field] = fieldReply->str;
            
            freeReplyObject(fieldReply);
        }
    }
    
    freeReplyObject(infoReply);
    return info;
}

CINDER_APP_NATIVE(SocialNetworkVisualization, RendererGl)
