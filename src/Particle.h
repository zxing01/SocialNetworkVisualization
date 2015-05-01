//
//  Particle.h
//  SocialNetworkVisualization
//
//  Created by Lu Han, Zilong Jiao and Zhi Xing on 4/20/15.
//
//

#ifndef __SocialNetworkVisualization__Particle__
#define __SocialNetworkVisualization__Particle__

#include <unordered_map>
#include "cinder/Vector.h"
#include "cinder/gl/gl.h"
#include "Parameters.h"
#include "Cpp11-BlockingQueue.h"

#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"


using namespace std;
using namespace ci;

class Particle
{
public:
    Particle(const Vec2f& position, float radius, float mass, float drag);
    void update();
    void draw();
    //void selected();
    //void deselected();
    float& radius();
    float& mass();
    Vec2f& force();
    Vec2f& position();
    ColorA& color();
    unordered_map<string, string>& info();
    static void selectParticle(Particle* particle);

private:
    void drawAbstract();
    void drawFull();
    gl::Texture getImage();
    //bool _selected;
    bool _downloaded;
    float _radius;
    float _mass;
    float _drag;
    float _resistance;
    Vec2f _currPosition;
    Vec2f _prevPosition;
    Vec2f _force;
    ColorA _color;
    unordered_map<string, string> _info;
    gl::Texture _image;
    
    static Particle* selected;
    static BlockingQueue<string> requests;
    static vector<thread*> threads;
    //static thread downloadThread;
    static void downloadImage();
};

#endif /* defined(__SocialNetworkVisualization__Particle__) */
