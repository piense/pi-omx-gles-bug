// Creates an ES context to use for loading resources.
// Basically we pass this class to the SlideRenderers and they push up callbacks
// for the render loop

#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <list>

#include "../PiSignageLogging.h"

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

// Singleton pattern used:
// https://stackoverflow.com/questions/1008019/c-singleton-design-pattern

struct queueItem
{
    void (*func)( void *);
    void *arg;
} ;

class PiSlideLoader{
public:

    static PiSlideLoader& getInstance()
    {
        static PiSlideLoader instance; // Guaranteed to be destroyed.
                                        // Instantiated on first use.
        return instance;
    }

    // Returns the GL context used to load stuff
    // Needs to be added as a shared context to the SlideRenderers' contexts
    static EGLContext loadingContext();

    // This will add the function and arguement to the queue
    // to be called in the worker thread under the shared GL Context
    static void RunGLWork( void (*func)(void *), void *arg);

    ~PiSlideLoader();

private: 

	PiSlideLoader();

    static void runLoop();

    static std::mutex mQueueMutex;
    static std::list<queueItem> mCallQueue;

    static std::atomic<bool> mRunning;
    static std::atomic<bool> mStop;

    // OpenGL|ES objects
	static DISPMANX_DISPLAY_HANDLE_T dispman_display;
	static DISPMANX_ELEMENT_HANDLE_T dispman_element;
	static EGLDisplay display;
	static EGLSurface surface;
	static EGLContext context;

private:

    // C++ 11
    // =======
    // We can use the better technique of deleting the methods
    // we don't want.
public:
    PiSlideLoader(PiSlideLoader const&)  = delete;
    void operator=(PiSlideLoader const&)  = delete;

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status

};