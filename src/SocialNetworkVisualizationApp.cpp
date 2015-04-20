#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SocialNetworkVisualizationApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void SocialNetworkVisualizationApp::setup()
{
}

void SocialNetworkVisualizationApp::mouseDown( MouseEvent event )
{
}

void SocialNetworkVisualizationApp::update()
{
}

void SocialNetworkVisualizationApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( SocialNetworkVisualizationApp, RendererGl )
