//
//  Particle.cpp
//  SocialNetworkVisualization
//
//  Created by Zhi Xing on 4/20/15.
//
//

#include "Particle.h"

Particle::Particle(const ci::Vec2f& currPosition, float radius, float mass, float drag)
{
    _currPosition =currPosition;
    _radius = radius;
    _mass = mass;
    _drag = drag;
    _prevPosition = currPosition;
    _forces = ci::Vec2f(1.0f,1.0f);
}
void Particle::update()
{
    ci::Vec2f temp = _currPosition;
    ci::Vec2f vel = ( _currPosition - _prevPosition ) * _drag;
    _currPosition += vel + _forces / _mass;
    _prevPosition = temp;
    _forces = ci::Vec2f::zero();
}
void Particle::draw()
{
    ci::gl::drawSolidCircle( _currPosition, _radius);
    //ci::gl::drawStrokedCircle( _currPosition, _radius+2.f);
}