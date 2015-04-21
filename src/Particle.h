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

using namespace std;

class Particle
{
public:
    Particle(const ci::Vec2f& position, float radius, float mass, float drag);
    void update();
    void draw();
    
    ci::Vec2f _currPosition;
    ci::Vec2f _prevPosition;
    ci::Vec2f _forces;
    float _radius;
    float _mass;
    float _drag;
    unordered_map<string, string> info;
};

#endif /* defined(__SocialNetworkVisualization__Particle__) */
