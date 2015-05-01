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
#include "Parameters.h"
#include "ParticleSystem.h"
#include "cinder/params/Params.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

class SocialNetworkVisualization : public AppNative {

public:
    ~SocialNetworkVisualization();
    void setup();
    void mouseDown(MouseEvent event );
    void mouseUp(MouseEvent event);
    void keyDown(KeyEvent event);
    void update();
    void draw();

private:
    void connectRedis();
    void redisUpdate(string name);
    void userUpdate();
    void issueUpdate();
    void redisFlush();
    Particle *createParticleForUser(string key, Vec2f position);
    unordered_map<string, string> getInfoForUser(string key);
    void createGui();
    
    bool mIsHandle;
    Particle* mHandle;
    ParticleSystem mParticleSystem;
    vector< pair<Particle*, Particle*> > mLinks;
    unordered_map<string, Particle*> map;
    redisContext *redis;
    params::InterfaceGlRef mParams;
    string mSearchKey;
};

#endif
