//
//  ParticleSystem.cpp
//  SocialNetworkVisualization
//
//  Created by Zhi Xing on 4/20/15.
//
//

#include "ParticleSystem.h"

ParticleSystem::~ParticleSystem()
{
    for( std::vector<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it )
    {
        delete *it;
    }
    particles.clear();
}
void ParticleSystem::update()
{
    for( std::vector<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it )
    {
        (*it)->update();
    }
}
void ParticleSystem::draw()
{
    for( std::vector<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it )
    {
        (*it)->draw();
    }
}
void ParticleSystem::addParticle(Particle* particle)
{
    particles.push_back( particle );
}
void ParticleSystem::destroyParticle(Particle* particle)
{
    std::vector<Particle*>::iterator it = std::find( particles.begin(), particles.end(), particle );
    delete *it;
    particles.erase( it );
}