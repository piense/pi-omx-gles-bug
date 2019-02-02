#include "piSlideLoader.h"
#include <assert.h>

std::mutex PiSlideLoader::mQueueMutex;
std::list<queueItem> PiSlideLoader::mCallQueue;

std::atomic<bool> PiSlideLoader::mRunning;
std::atomic<bool> PiSlideLoader::mStop;

// OpenGL|ES objects
DISPMANX_DISPLAY_HANDLE_T PiSlideLoader::dispman_display;
DISPMANX_ELEMENT_HANDLE_T PiSlideLoader::dispman_element;
EGLDisplay PiSlideLoader::display;
EGLSurface PiSlideLoader::surface;
EGLContext PiSlideLoader::context;

PiSlideLoader::PiSlideLoader()
{
	//Might be a better way than a 1x1 pixel element but it works for now so whatever

    // Setup the GLES context

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
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
	assert(context!=EGL_NO_CONTEXT);

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = 1;
	dst_rect.height = 1;
		
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = 1 << 16;
	src_rect.height = 1 << 16;        

	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );

	VC_DISPMANX_ALPHA_T alpha;  //alpha.alpha: 0->255
	alpha.flags = (DISPMANX_FLAGS_ALPHA_T)
		(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_MIX);
	alpha.opacity = 0; 
	alpha.mask = 0;
			
	dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
		- 127/*layer*/, &dst_rect, 0/*src*/,
		&src_rect, DISPMANX_PROTECTION_NONE, &alpha /*alpha*/, 0/*clamp*/, DISPMANX_NO_ROTATE/*transform*/);
		
	nativewindow.element = dispman_element;
	nativewindow.width = 1;
	nativewindow.height = 1;
	vc_dispmanx_update_submit_sync( dispman_update );
		
	surface = eglCreateWindowSurface( display, config, &nativewindow, NULL );
	assert(surface != EGL_NO_SURFACE);

    printf("Starting slideLoader!\n");

    // Setup and launch the worker thread

    mRunning = false;
    mStop = false;

    std::thread workThread(runLoop);
    workThread.detach();

    while(!mRunning){
        usleep(2000); //2 millis
    }

}

PiSlideLoader::~PiSlideLoader()
{
    mStop = true;
    while(mRunning == true)
    {
        usleep(2000); //2 millis
    }

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

EGLContext PiSlideLoader::loadingContext(){
    return context;
}

void PiSlideLoader::RunGLWork( void (*func)(void *), void *arg)
{
    mQueueMutex.lock();
    mCallQueue.push_back({func, arg});
    mQueueMutex.unlock();
}


void PiSlideLoader::runLoop()
{
    mRunning = true;

    printf("Loader is running\n");

    // connect the context to the surface
	EGLBoolean result = eglMakeCurrent(display, surface, surface, context);
	if(result != EGL_TRUE){
        pis_logMessage(PIS_LOGLEVEL_ERROR, "eglMakeCurrent failed\n");
        return;
    }

	glEnable(GL_TEXTURE_2D);

    queueItem item;

    while (!mStop) {
        mQueueMutex.lock();
        if(!mCallQueue.empty())
        {
            item = mCallQueue.front();
            mCallQueue.pop_front();
        }else{
            item.func = NULL;
        }
        mQueueMutex.unlock();

        if(item.func != NULL)
        {
            item.func(item.arg);
        }
        
        usleep(10000); //10 millis
    }

    mRunning = false;
}