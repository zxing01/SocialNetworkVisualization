//
//  Particle.cpp
//  SocialNetworkVisualization
//
//  Created by Lu Han, Zilong Jiao and Zhi Xing on 4/20/15.
//
//

#include <fstream>
#include "Particle.h"
#include "cinder/gl/TextureFont.h"
#include <curl/curl.h>
#include <stdio.h>

using namespace ci::app;

Particle* Particle::selected = nullptr;
BlockingQueue<string> Particle::requests;
vector<thread*> Particle::threads;
//thread Particle::downloadThread(Particle::downloadImage);

////////////////////////////////////////////////////////////////////////////////////////////////
// Select a particle
void Particle::selectParticle(Particle* particle) {
    selected = particle;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function for writing data to an open file
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Image downloading function used by downloading threads
void Particle::downloadImage() {
    while (true) {
        string request = requests.deQ();
        int delimit = request.find(' ');
        string filename = request.substr(0, delimit);
        string url = request.substr(delimit + 1, request.length() - delimit - 1);
        CURL *curl;
        FILE *fp;
        CURLcode res;
    
        curl = curl_easy_init();
        if (curl) {
            fp = fopen(filename.c_str(), "wb");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            //curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
Particle::Particle(const Vec2f& currPosition, float radius, float mass, float drag):
_currPosition(currPosition),
_radius(radius),
_mass(mass),
_drag(drag),
_prevPosition(currPosition),
_force(0.f,0.f),
_resistance(2.0),
//_selected(false),
_downloaded(false),
_color(0.0f,1.0f,0.0f,0.8f) {
    if (threads.size() < 10) {
        thread *th;
        threads.push_back(th);
        th = new thread(&Particle::downloadImage);
        th->detach();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Update
void Particle::update() {
    if (selected == this) {
        _mass = NODE_MASS * SLCT_POW * SLCT_POW;
        _radius = NODE_SIZE * SLCT_POW;
    }
    else {
        _mass = NODE_MASS;
        _radius = NODE_SIZE;
    }
    
    Vec2f vel = (_currPosition - _prevPosition) * _drag; // decayed average velocity
    float distanceToCenter = _currPosition.distance(getWindowCenter());
    Vec2f gravityDirection = (getWindowCenter() - _currPosition).normalized();
    Vec2f gravity = gravityDirection * distanceToCenter * GRAV_COEFF; // new gravitational equation!
    Vec2f totalForce = _force - vel * _resistance + gravity;
    
    _prevPosition = _currPosition;
    _currPosition += (vel +  totalForce / _mass); // time is 1
    
    _force = Vec2f::zero();
    if(_color != ColorA(0.0f,0.0f,0.0f,1.0f))
        _color = _color - ColorA(0.01f,0.01f,0.01f,0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering
void Particle::draw(){
    if (selected == this)
        drawFull();
    else
        drawAbstract();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Draw all the content of the particle
void Particle::drawAbstract() {
    Vec2f topLeft = Vec2f(_currPosition.x -  _radius, _currPosition.y - _radius);
    Vec2f botRight = Vec2f(_currPosition.x + _radius, _currPosition.y + _radius);
    gl::color(Color::white());
    //gl::Texture image = getImage();
    //if (image)
        //gl::draw(image, Rectf(topLeft, botRight));
    if (_image) {
        gl::draw(_image, Rectf(topLeft, botRight));
    }
    else {
        _image = getImage();
        
        gl::color(0.5f, 0.5f, 0.5f, 0.8f);
        gl::drawSolidRect(Rectf(topLeft, botRight));
    }
    gl::color(_color);
    gl::drawStrokedRoundedRect(Rectf(topLeft - Vec2f(1,1), botRight + Vec2f(1,1)), 5);
    gl::color(Color::black());
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Draw the abstracted content of the particle
void Particle::drawFull() {
    gl::color(_color - ColorA(0.f,0.f,0.f,0.2f));
    //ci::gl::drawStrokedCircle( _currPosition, _radius);
    ci::gl::drawSolidCircle(_currPosition, _radius);

    // write info
    string info;
    info += "@" + _info["screen_name"] + "\n";
    info += "Name: " + _info["name"] + "\n";
    info += "Friends: " + _info["friends_count"] + "\n";
    info += "Followers: " + _info["followers_count"] + "\n";
    info += "Tweets: " + _info["statuses_count"] + "\n";
    
    gl::color(Color::white());
    Font mFont = Font("Helvetica", 11);
    gl::TextureFontRef mTextureFont;
    mTextureFont = gl::TextureFont::create(mFont);
    
    float offset = _radius / 1.414;
    Vec2f topLeft(_currPosition.x - offset,_currPosition.y);
    Vec2f botRight(_currPosition.x + offset,_currPosition.y + offset);
    mTextureFont->drawStringWrapped(info, Rectf(topLeft, botRight));
    
    // draw profile image
    float sideLen = _radius / (2 * GOLDEN_RATIO - 1);
    topLeft = Vec2f(_currPosition.x - sideLen / 2, _currPosition.y - _radius / 2 - sideLen / 2);
    botRight = Vec2f(_currPosition.x + sideLen / 2, _currPosition.y - _radius / 2 + sideLen / 2);
    gl::color( ColorA(1.f,1.f,1.f, 1.0f) );
    //gl::Texture image = getImage();
    //if (image)
        //gl::draw(image, Rectf(topLeft, botRight));
    if (_image) {
        gl::draw(_image, Rectf(topLeft, botRight));
    }
    else {
        _image = getImage();
        
        gl::color(0.5f, 0.5f, 0.5f, 0.8f);
        gl::drawSolidRect(Rectf(topLeft, botRight));
    }
    gl::color(Color::black());
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Get the profile image
gl::Texture Particle::getImage() {
    string filename = _info["screen_name"] + ".jpg";
    ifstream f(filename);
    gl::Texture image;

    try {
        if (f.good())
            image = gl::Texture(loadImage(filename));
        else if (!_downloaded) {
            string request = filename + " " + _info["profile_image_url_https"];
            requests.enQ(request);
            _downloaded = true;
        }
    }
    catch (exception e) {
        //cout << "Particle::getImage():" << e.what() << endl;
    }
    
    f.close();
    return image;
}

/*
////////////////////////////////////////////////////////////////////////////////////////////////
// Set states when this particle is selected
void Particle::selected() {
    _selected = true;
    _mass = NODE_MASS * SLCT_POW * SLCT_POW;
    _radius = NODE_SIZE * SLCT_POW;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Set states when this particle is no longer selected
void Particle::deselected() {
    _selected = false;
    _mass = NODE_MASS;
    _radius = NODE_SIZE;
}
*/
////////////////////////////////////////////////////////////////////////////////////////////////
// Access the radius of the particle
float& Particle::radius() {
    return _radius;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Access the mass of the particle
float& Particle::mass() {
    return _mass;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Access the total force affecting the particle
Vec2f& Particle::force() {
    return _force;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Access the current position of the particle
Vec2f& Particle::position() {
    return _currPosition;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Access the color of the particle
ColorA& Particle::color() {
    return _color;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Access the information stored on the particle
unordered_map<string, string>& Particle::info() {
    return _info;
}