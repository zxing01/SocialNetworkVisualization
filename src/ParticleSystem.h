//
//  ParticleSystem.h
//  SocialNetworkVisualization
//
//  Created by Lu Han, Zilong Jiao and Zhi Xing on 4/20/15.
//
//

#ifndef __SocialNetworkVisualization__ParticleSystem__
#define __SocialNetworkVisualization__ParticleSystem__

#include <iostream>
#include "Parameters.h"
#include "Particle.h"
#include <vector>

class ParticleSystem
{
public:
    ~ParticleSystem();
    void update();
    void draw();
    void addParticle(Particle* particle);
    void destroyParticle(Particle* particle);
    std::vector<Particle*>& particles();
private:
    std::vector<Particle*> _particles;
};

#endif /* defined(__SocialNetworkVisualization__ParticleSystem__) */
