//
//  Particle.cpp
//  SocialNetworkVisualization
//
//  Created by Lu Han, Zilong Jiao and Zhi Xing on 4/20/15.
//
//

#include "Particle.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"

using namespace cinder;
using namespace ci;
using namespace ci::app;

Particle::Particle(const ci::Vec2f& currPosition, float radius, float mass, float drag)
{
    _currPosition =currPosition;
    _radius = radius;
    _mass = mass;
    _drag = drag;
    _prevPosition = currPosition;
    _forces = ci::Vec2f(1.0f,1.0f);
    _forceFactor = 1.f;
    _selected = false;
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
    if (!_selected)
        ci::gl::drawSolidCircle( _currPosition, _radius);
    else {
        //Url url(_info["profile_image_url_https"]);
        //gl::Texture image = gl::Texture(loadImage(loadUrl(url)));
        ci::gl::drawStrokedCircle( _currPosition, _radius);
        //gl::draw(image, )
    }
}

void Particle::selected() {
    _selected = true;
    _forceFactor = SLCT_POW;
    _radius = NODE_SIZE * SLCT_POW;
}

void Particle::deselected() {
    _selected = false;
    _forceFactor = 1;
    _radius = NODE_SIZE;
}