//
//  SocialNetworkVisualization.h
//  SocialNetworkVisualization
//
//  Created by Lu Han, Zilong Jiao and Zhi Xing on 4/20/15.
//
//

#ifndef SocialNetworkVisualization_SocialNetworkVisualization_h
#define SocialNetworkVisualization_SocialNetworkVisualization_h

#include <string>
#include <unordered_map>
#include <hiredis/hiredis.h>
#include "cinder/app/AppNative.h"
#include "ParticleSystem.h"

#define NODE_MASS 10.f
#define NODE_SIZE 5.f
#define NODE_DRAG 0.95f
#define EDGE_LEN 60.f

using namespace ci;
using namespace ci::app;
using namespace std;

class SocialNetworkVisualization : public AppNative {

public:
    ~SocialNetworkVisualization();
    void setup();
    void mouseDown( MouseEvent event );
    void mouseMove(MouseEvent event);
    void mouseUp(MouseEvent event);
    void keyDown(KeyEvent event);
    void update();
    void draw();

private:
    bool mIsHandle;
    ParticleSystem mParticleSystem;
    Particle* mHandle;
    vector< pair<Particle*, Particle*> > mLinks;
    unordered_map<string, Particle*> map;
    
    // redis related
    redisContext *redis;
    void connectRedis();
    Particle *createParticleForUser(string key, Vec2f position);
    unordered_map<string, string> getInfoForUser(string key);
};

#endif
