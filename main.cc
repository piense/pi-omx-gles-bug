#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#include "PiSignageLogging.h"

extern "C"
{
//External libs
#include "bcm_host.h"
}

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "mediatypes/PiMediaImageGFX.h"
#include "compositor/piSlideLoader.h"

using namespace std;

uint32_t screen_width;
uint32_t screen_height;
// OpenGL|ES objects
DISPMANX_DISPLAY_HANDLE_T dispman_display;
DISPMANX_ELEMENT_HANDLE_T dispman_element;
EGLDisplay display;
EGLSurface surface;
EGLContext context;

int SignageExit = 0;

void InitGLES();
void CleanupGLES();

void stop(int s){
	printf("Caught signal %d\n",s);
	SignageExit = 1;
}

void LoadStub(pis_MediaImageGFX *g){
	g->Load();
}

int main(int argc, char *argv[])
{
	pis_loggingLevel = PIS_LOGLEVEL_ALL;

	bcm_host_init();

	PiSlideLoader& loader = PiSlideLoader::getInstance();

	InitGLES();

	// reset model position
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();


	//Load one locally
	pis_MediaImageGFX image1;
	image1.X = .25;
	image1.Y = .25;
	image1.MaxHeight = .5;
	image1.MaxWidth = .5;
	image1.Filename = "test.jpg";
	image1.SetScreenSize(screen_width, screen_height);
	image1.Load();

	pis_MediaImageGFX image2;
	image2.X = .5;
	image2.Y = .5;
	image2.MaxHeight = 1;
	image2.MaxWidth = 1;
	image2.Filename = "test.jpg";
	image2.SetScreenSize(screen_width, screen_height);
	loader.RunGLWork((void (*)(void*))LoadStub, (void*)&image2);


	while(SignageExit == 0)
	{
		// Start with a clear screen
		glClear( GL_COLOR_BUFFER_BIT );

		image1.GLDraw();
		image2.GLDraw();

		eglSwapBuffers(display, surface);
	}

	CleanupGLES();

	bcm_host_deinit();

	return 0;
}


void InitGLES()
{
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;

	static EGL_DISPMANX_WINDOW_T nativewindow;

	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	EGLConfig config;

	// get an EGL display connection
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(display!=EGL_NO_DISPLAY);

	// initialize the EGL display connection
	result = eglInitialize(display, NULL, NULL);
	assert(EGL_FALSE != result);

	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);

	// create an EGL rendering context
	context = eglCreateContext(display, config, PiSlideLoader::loadingContext(), NULL);
	assert(context!=EGL_NO_CONTEXT);

	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);
	assert( success >= 0 );

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = screen_width;
	dst_rect.height = screen_height;
		
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = screen_width << 16;
	src_rect.height = screen_height << 16;        

	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );

	VC_DISPMANX_ALPHA_T alpha;  //alpha.alpha: 0->255
	alpha.flags = (DISPMANX_FLAGS_ALPHA_T)
		(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_MIX);
	alpha.opacity = 255; 
	alpha.mask = 0;
			
	dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
		20 /*layer*/, &dst_rect, 0/*src*/,
		&src_rect, DISPMANX_PROTECTION_NONE, &alpha /*alpha*/, 0/*clamp*/, DISPMANX_NO_ROTATE/*transform*/);
		
	nativewindow.element = dispman_element;
	nativewindow.width = screen_width;
	nativewindow.height = screen_height;
	vc_dispmanx_update_submit_sync( dispman_update );
		
	surface = eglCreateWindowSurface( display, config, &nativewindow, NULL );
	assert(surface != EGL_NO_SURFACE);

	// connect the context to the surface
	result = eglMakeCurrent(display, surface, surface, context);
	assert(EGL_FALSE != result);

	// Set background color and clear buffers
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Enable back face culling.
	glEnable(GL_CULL_FACE);

	glViewport(0, 0, (GLsizei)screen_width, (GLsizei)screen_height);
		
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrthof(0.F, (float)screen_width, 0.F, (float)screen_height, -1.F, 1.F);

	glEnableClientState( GL_VERTEX_ARRAY );

	// reset model position
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

}

void CleanupGLES()
{
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   int s;
   // clear screen
   glClear( GL_COLOR_BUFFER_BIT );
   eglSwapBuffers(display, surface);

   eglDestroySurface( display, surface );

   dispman_update = vc_dispmanx_update_start( 0 );
   s = vc_dispmanx_element_remove(dispman_update, dispman_element);
   if(s != 0){
	   pis_logMessage(PIS_LOGLEVEL_WARN, "PiSlideRenderer::CleanupGLES() WARNING removing element failed %d", s);
   }
   
   vc_dispmanx_update_submit_sync( dispman_update );
   s = vc_dispmanx_display_close(dispman_display);
   if(s != 0){
	   pis_logMessage(PIS_LOGLEVEL_WARN, "PiSlideRenderer::CleanupGLES() WARNING closing display failed %d", s);
   }

   // Release OpenGL resources
   eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
   eglDestroyContext( display, context );
   eglTerminate( display );

}
