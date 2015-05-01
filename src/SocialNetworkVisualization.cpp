//
//  SocialNetworkVisualization.cpp
//  SocialNetworkVisualization
//
//  Created by Lu Han, Zilong Jiao and Zhi Xing on 4/20/15.
//
//

#include "SocialNetworkVisualization.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
SocialNetworkVisualization::~SocialNetworkVisualization() {
    // Disconnects and frees the redis context
    redisFree(redis);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Download the adjacency list from the backend Redis database and save user info to Particles
void SocialNetworkVisualization::setup() {
    vector<string> args = getArgs();
    if (args.size() < 2)
        cout << " Please give me a username.\n";
    createGui();
    connectRedis();
    redisUpdate(args[1]);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Repeated update without rendering
void SocialNetworkVisualization::update() {
    userUpdate();
    // calculate replusion
    for(auto it1 = mParticleSystem.particles().begin(); it1 != mParticleSystem.particles().end(); ++it1) {
        for(auto it2 = mParticleSystem.particles().begin(); it2 != mParticleSystem.particles().end(); ++it2) {
            Vec2f conVec = (*it2)->position() - (*it1)->position();
            if(conVec.length() < 0.1f)
                continue;
            float distance = conVec.length();
            float repulsionCoeff = (distance - EDGE_LEN * 2.0f) * 0.05f / distance / NODE_MASS;
            repulsionCoeff = math<float>::min(0.f, repulsionCoeff);
            (*it1)->force() += conVec * repulsionCoeff * (*it2)->mass();
            (*it2)->force() += -conVec * repulsionCoeff * (*it1)->mass();
        }
    }
    // calculate attraction
    for(auto it = mLinks.begin(); it != mLinks.end(); ++it ) {
        Vec2f conVec = it->second->position() - it->first->position();
        float distance = conVec.length();
        float attractionCoeff = (distance - EDGE_LEN) * 0.5f / distance;
        it->first->force() += conVec * attractionCoeff;
        it->second->force() += -conVec * attractionCoeff;
    }
    
    //th.join();
    
    if(mIsHandle) {
        mHandle->position() = getMousePos() - getWindowPos();
        mHandle->force() = Vec2f::zero();
    }
    mParticleSystem.update();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// The rendering function
void SocialNetworkVisualization::draw() {
    // clear out the window with white
    gl::enableAlphaBlending();
    gl::clear(Color::white());
    
    gl::setViewport(getWindowBounds());
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    gl::lineWidth(3.0f);
    for(auto it = mLinks.begin(); it != mLinks.end(); ++it) {
        Vec2f conVec = it->second->position() - it->first->position();
        conVec.normalize();
        
        gl::drawLine(it->first->position() + conVec * (it->first->radius() + 2.f),
                     it->second->position() - conVec * (it->second->radius() + 2.f));
    }
    mParticleSystem.draw();
    mParams->draw();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Handle mouse down events
void SocialNetworkVisualization::mouseDown(MouseEvent event) {
    float minDist = 20.f;
    mIsHandle = false;
    for(auto it = mParticleSystem.particles().begin(); it != mParticleSystem.particles().end(); ++it) {
        float dist = (*it)->position().distance(event.getPos());
        if(dist < minDist) {
            minDist = dist;
            mIsHandle = true;
            mHandle = (*it);
            Particle::selectParticle((*it));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Handle mouse up events
void SocialNetworkVisualization::mouseUp(MouseEvent event) {
    mIsHandle = false;
    Particle::selectParticle(nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Handle key down events
void SocialNetworkVisualization::keyDown(KeyEvent event) {
    if(event.getCode() == KeyEvent::KEY_SPACE && mIsHandle == true) {
        string username = mHandle->info()["screen_name"];
        redisUpdate(username);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Send update request to Redis
void SocialNetworkVisualization::redisUpdate(string name) {
    if(map.count(name) > 0)
        map[name]->color() = ColorA(1.0f,0.0f,0.0f,0.8f);
    
    redisReply* updateReply = (redisReply*)redisCommand(redis, "RPUSH update@request %s", name.c_str());
    freeReplyObject(updateReply);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Collect requested user information from Redis
void SocialNetworkVisualization::userUpdate() {
    redisReply* userReply = (redisReply*)redisCommand(redis, "LPOP update@reply");
    if (userReply->type == REDIS_REPLY_STRING) {
        string name = string(userReply->str);
        Particle *center = createParticleForUser(name, getWindowCenter() + Vec2f::one()); // particle disappear if just use getWindowCenter()
        
        redisReply *listReply = (redisReply*)redisCommand(redis,"LRANGE %s 0 -1", ("list@" + name).c_str());
        if (listReply->type == REDIS_REPLY_ARRAY) {
            Vec2f r = Vec2f::one() * EDGE_LEN;
            
            for (int j = 0; j < listReply->elements; ++j) {
                string nameOfNeighbor = string(listReply->element[j]->str);
                Particle *neighbor = createParticleForUser(nameOfNeighbor, getWindowCenter() + r);
                mLinks.push_back(make_pair(center, neighbor));
                
                r.rotate(2 * M_PI / listReply->elements);
            }
            freeReplyObject(listReply);
        }
    }
    freeReplyObject(userReply);
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
    particle->info() = getInfoForUser(name);
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

////////////////////////////////////////////////////////////////////////////////////////////////
// Greate the GUI for type-in updates
void SocialNetworkVisualization::createGui() {
    mParams = params::InterfaceGl::create(getWindow(), "UPDATE", toPixels(Vec2i(190,88)), ColorA(0.5f,0.5f,0.5f,0.8f));
    mParams->addParam("USERNAME: @", &mSearchKey );
    mParams->addButton("OK", std::bind( &SocialNetworkVisualization::issueUpdate, this));
    mParams->addText("text", "label=`  `");
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Issue the update requested from the type-in GUI
void SocialNetworkVisualization::issueUpdate() {
    if (map.count(mSearchKey) > 0) {
        redisUpdate(mSearchKey);
    }
    else {
        string str = mSearchKey + " is not in the network.";
        mParams->setOptions("text", "label=`" + str + "`");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Delete all the data in Redis
void SocialNetworkVisualization::redisFlush() {
    redisReply* updateReply = (redisReply*)redisCommand(redis, "FLUSHDB");
    freeReplyObject(updateReply);
}

CINDER_APP_NATIVE(SocialNetworkVisualization, RendererGl)
