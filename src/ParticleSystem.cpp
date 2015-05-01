//
//  ParticleSystem.cpp
//  SocialNetworkVisualization
//
//  Created by Lu Han, Zilong Jiao and Zhi Xing on 4/20/15.
//
//

#include "ParticleSystem.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor - delete all the particles
ParticleSystem::~ParticleSystem() {
    for(auto it = _particles.begin(); it != _particles.end(); ++it)
        delete *it;
    _particles.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Update all the particles by calling update() on each one
void ParticleSystem::update() {
    for(auto it = _particles.begin(); it != _particles.end(); ++it)
        (*it)->update();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Render all the particles by calling draw() on each one
void ParticleSystem::draw() {
    for(auto it = _particles.begin(); it != _particles.end(); ++it) {
        (*it)->draw();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Add a particle to the system
void ParticleSystem::addParticle(Particle* particle) {
    _particles.push_back(particle);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Remove a particle to the system
void ParticleSystem::destroyParticle(Particle* particle) {
    auto it = std::find(_particles.begin(), _particles.end(), particle);
    delete *it;
    _particles.erase(it);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Access the particles
std::vector<Particle*>& ParticleSystem::particles() {
    return _particles;
}
