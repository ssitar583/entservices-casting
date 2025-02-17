//
//  MiracastGraphicsDelegate.cpp
//  Copyright (C) 2020 Apple Inc. All Rights Reserved. Not to be used or disclosed without permission from Apple.
//

#include "MiracastGraphicsDelegate.hpp"
#include <stdlib.h>
#include <cassert>
#include <pthread.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include "MiracastAppLogging.hpp"

namespace MiracastApp{
namespace Graphics{

#define RED_SIZE (8)
#define GREEN_SIZE (8)
#define BLUE_SIZE (8)
#define ALPHA_SIZE (8)
#define DEPTH_SIZE (24)

NativeDisplayType display;
MiracastGraphicsDelegate* MiracastGraphicsDelegate::mInstance;

#define ESS_DISPATCH_THREAD_NAME "EssDispatchTh"
#define kESSRunLoopPeriod 16

struct Arg_EssDispatchTh{
	EssosDispatchThread * essDispTh;
	EssCtx *essCtx;
};

class EssosDispatchThread 
{
public:
	pthread_t EssDispatchT_Id;
	struct Arg_EssDispatchTh arg_DispTh;
	struct sigaction sigact;
	static void signalHandler(int signum)
	{
		MIRACASTLOG_VERBOSE("signalHandler: signum %d\n",signum);
		mEssDispTh->setRunning(false);
	}

	EssosDispatchThread();
     ~EssosDispatchThread()
    {
        // don't let the thread outlive the object
    }

    bool isRunning()
    {
        return mRunning;
    }

    void setRunning(bool running)
    {
    	pthread_mutex_lock(&mRunningMutex);
        mRunning = running;
		pthread_mutex_unlock(&mRunningMutex);
    }

private:
    static void* run(void *arg)
    {
		struct timespec tspec;
		long long delay = 0, curr_time = 0, diff_time = 0, ess_evloop_last_ts = 0;
		while(mEssDispTh->isRunning()){
            EssContextRunEventLoopOnce(MiracastGraphicsDelegate::getInstance()->getEssCtxInstance());
			clock_gettime(CLOCK_MONOTONIC,&tspec);
			curr_time = tspec.tv_sec * 1000 + tspec.tv_nsec / 1e6;
			diff_time = curr_time - ess_evloop_last_ts;
			delay = (long long)kESSRunLoopPeriod - diff_time;
			if(delay > 0 && delay <= kESSRunLoopPeriod )
			{
				usleep(delay*1000);
			}
			ess_evloop_last_ts = curr_time;
		}
		return nullptr;
    }

    bool mRunning;
	static EssosDispatchThread *mEssDispTh;
    pthread_mutex_t mRunningMutex;
};
EssosDispatchThread * EssosDispatchThread::mEssDispTh;

    EssosDispatchThread::EssosDispatchThread()
        : mRunning(true)
    {
	    if (pthread_mutex_init(&mRunningMutex, NULL) != 0) {
        	MIRACASTLOG_VERBOSE("mutex init has failed\n");
    	}
		EssosDispatchThread::mEssDispTh = this;
        sigact.sa_handler= EssosDispatchThread::signalHandler;
        sigemptyset(&sigact.sa_mask);
        sigact.sa_flags= SA_RESETHAND;
        sigaction(SIGINT, &sigact, NULL);
		sigaction(SIGSEGV, &sigact, NULL);

		arg_DispTh.essDispTh = this;
		arg_DispTh.essCtx = MiracastGraphicsDelegate::getInstance()->getEssCtxInstance();
		if(pthread_create(&EssDispatchT_Id, nullptr, &EssosDispatchThread::run, nullptr )){
			MIRACASTLOG_VERBOSE("Failure to create Thread name\n");
		}
		else{
			if(pthread_setname_np(EssDispatchT_Id, ESS_DISPATCH_THREAD_NAME )){
				MIRACASTLOG_VERBOSE("Failure to set Thread name\n");
			}
		}
    }

static EssTerminateListener terminateListener =
{
    //terminated
    [](void* /*data*/) {
        _Exit(-1);
    }
};

void MiracastGraphicsDelegate::OnKeyPressed(unsigned int key){
	MIRACASTLOG_VERBOSE(" <com.miracastapp.graphics> OnKeyPressed() key_code: %02X translated code: ?\n", key);
}

void MiracastGraphicsDelegate::OnKeyReleased(unsigned int key){
	MIRACASTLOG_VERBOSE(" <com.miracastapp.graphics> OnKeyReleased() key_code: %02X translated code: ?\n", key);
}

void MiracastGraphicsDelegate::OnKeyRepeat(unsigned int key)
{
	MIRACASTLOG_VERBOSE(" <com.miracastapp.graphics> OnKeyRepeat() key_code: %02X translated code: ?\n", key);
}

EssKeyListener MiracastGraphicsDelegate::keyListener =
{
  // keyPressed
  [](void* data, unsigned int key) { reinterpret_cast<MiracastGraphicsDelegate*>(data)->OnKeyPressed(key); },
  // keyReleased
  [](void* data, unsigned int key) { reinterpret_cast<MiracastGraphicsDelegate*>(data)->OnKeyReleased(key); },
  // keyRepeat
  [](void* data, unsigned int key) { reinterpret_cast<MiracastGraphicsDelegate*>(data)->OnKeyRepeat(key); }
};

//this callback is invoked with the new width and height values
void displaySize(void *userData, int width, int height )
{
	EssCtx *ctx = (EssCtx*)userData;
	EssContextResizeWindow( ctx, width, height );
}

static EssSettingsListener settingsListener=
{
   displaySize
};

MiracastGraphicsDelegate::MiracastGraphicsDelegate() {
	mEssCtx = nullptr;
	if (pthread_mutex_init(&mDispatchMutex, NULL) != 0) {
    	MIRACASTLOG_ERROR("<com.miracastapp.graphics> mutex init failed\n");
	}
}
MiracastGraphicsDelegate::~MiracastGraphicsDelegate() {
	EssContextDestroy(mEssCtx);
}

MiracastGraphicsDelegate* MiracastGraphicsDelegate::getInstance() {
	if(!mInstance) {
		mInstance = new MiracastGraphicsDelegate;
	}
	return mInstance;
}

void MiracastGraphicsDelegate::destroyInstance(){
	if(mInstance != NULL){
		delete mInstance;
		mInstance = NULL;
	}
}

bool MiracastGraphicsDelegate::BuildEssosContext() {
	mEssCtx = EssContextCreate();
	bool error = false;
	if(!mEssCtx) {
		MIRACASTLOG_ERROR("<com.miracastapp.graphics> Failed to create Essos context\n");
	    return false;
	}
	else {
		if(!EssContextSetUseWayland(mEssCtx, true)){
			error = true;
		}
		if (!EssContextSetSettingsListener(mEssCtx, mEssCtx, &settingsListener))
		{
			error = true;
		}
		if ( !EssContextSetTerminateListener(mEssCtx, 0, &terminateListener) ) {
    		error = true;
  		}
		if ( !EssContextSetKeyListener(mEssCtx, 0, &keyListener) ) {
                error = true;
        }
		if ( !EssContextInit(mEssCtx) )
    	{            	
			error = true;
        } 
		
		if (!EssContextGetEGLDisplayType(mEssCtx, &display))
		{
			error = true;
		}
		mDisplay = eglGetDisplay((NativeDisplayType)display);
		assert(mDisplay != EGL_NO_DISPLAY);

		if ( !EssContextGetDisplaySize( mEssCtx, &gDisplayWidth, &gDisplayHeight ) )
		{
			error= true;
		}
		if ( !EssContextSetInitialWindowSize( mEssCtx, gDisplayWidth, gDisplayHeight) ){
			error = true;
		}
		MIRACASTLOG_VERBOSE("<com.miracastapp.graphics> display %dx%d\n", gDisplayWidth, gDisplayHeight);

		if ( !EssContextCreateNativeWindow(mEssCtx, gDisplayWidth, gDisplayHeight, &mNativewindow) ) {
			error = true;
		}
	}	
        if ( error )
        {
            const char *detail = EssContextGetLastErrorDetail(mEssCtx);
           MIRACASTLOG_VERBOSE("<com.miracastapp.graphics>BuildEssosContext(): Essos error: '%s'\n",detail);
        }	
	
	return error;
}

void MiracastGraphicsDelegate::DestroyNativeWindow() {
  if (mNativewindow == 0)
    return;

  if ( !EssContextDestroyNativeWindow(mEssCtx, mNativewindow) ) {
    const char *detail = EssContextGetLastErrorDetail(mEssCtx);
    MIRACASTLOG_VERBOSE("<com.miracastapp.graphics> DestroyNativeWindow() Essos error: '%s'\n", detail);
  }

  mNativewindow = 0;
}

bool MiracastGraphicsDelegate::initialize() {
	//Perform platform-specific initialization of the graphics system here
	bool error = BuildEssosContext();
	if(!error){
		if( !EssContextStart(mEssCtx)){
			error = true;
		}
		if(!error)
		{
			mAppEngine = MiracastApp::Application::Engine::getAppEngineInstance();
            error = InitializeEGL();
			if(!error)
				startDispatching();
		}
	}
	if(error)
		return false;
	else
		return true;
}

bool MiracastGraphicsDelegate::InitializeEGL()
{
   MIRACASTLOG_VERBOSE("MiracastGraphicsDelegate::InitializeEGL\n");
	EGLint major, minor;
	if (eglInitialize(mDisplay, &major, &minor) != EGL_TRUE)
	{
      	MIRACASTLOG_ERROR("MiracastGraphicsDelegate::InitializeEGL eglInitialize failed\n");
		return true;
	}
    if(eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
         MIRACASTLOG_VERBOSE("Unable to bind EGL API 0x%x",eglGetError());
        return true;
    }
	/*
	 * Get number of available configurations
	 */
	EGLint configCount;
	MIRACASTLOG_VERBOSE("Vendor: %s\n",eglQueryString(mDisplay, EGL_VENDOR));
    MIRACASTLOG_VERBOSE("version: %d.%d\n", major, minor);

	if (eglGetConfigs(mDisplay, nullptr, 0, &configCount))
	{

		EGLConfig eglConfigs[configCount];

		EGLint attributes[] = {
			EGL_RED_SIZE, RED_SIZE,
			EGL_GREEN_SIZE, GREEN_SIZE,
			EGL_BLUE_SIZE, BLUE_SIZE,
			EGL_ALPHA_SIZE, 8,
			EGL_DEPTH_SIZE, DEPTH_SIZE,
			EGL_STENCIL_SIZE, 0,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_BUFFER_SIZE, EGL_DONT_CARE,
			EGL_NONE};
	
		MIRACASTLOG_VERBOSE("Configs: %d\n", configCount);
		/*
		 * Get a list of configurations that meet or exceed our requirements
		 */
		if (eglChooseConfig(mDisplay, attributes, eglConfigs, configCount, &configCount))
		{	/*
			 * Choose a suitable configuration
			 */
			int index = 0;
			while (index < configCount)
			{
				EGLint redSize, greenSize, blueSize, alphaSize, depthSize;
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_RED_SIZE, &redSize);
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_GREEN_SIZE, &greenSize);
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_BLUE_SIZE, &blueSize);
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_ALPHA_SIZE, &alphaSize);
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_DEPTH_SIZE, &depthSize);
				MIRACASTLOG_VERBOSE("depthSize = %d\n",depthSize);

				if ((redSize == RED_SIZE) && (greenSize == GREEN_SIZE) && (blueSize == BLUE_SIZE) && (alphaSize == ALPHA_SIZE) && (depthSize >= DEPTH_SIZE))
				{
					break;
				}

				index++;
			}
			if (index < configCount)
			{
				mConfig = eglConfigs[index];
				EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2 /* ES2 */, EGL_NONE};
				MIRACASTLOG_VERBOSE(" Config choosen: %d\n", index);

				/*
				 * Create an EGL context
				 */
				mContext = eglCreateContext(mDisplay, mConfig, EGL_NO_CONTEXT, attributes);
				MIRACASTLOG_VERBOSE("Context created");
			}
		}
	}
	
	MIRACASTLOG_VERBOSE("Extentions: %s\n", eglQueryString(mDisplay, EGL_EXTENSIONS));


   	/*
	 * Create a window surface
	 */
	mSurface = eglCreateWindowSurface(
		mDisplay,
		mConfig,
		mNativewindow,
		nullptr);

	assert(EGL_NO_SURFACE != mSurface);

	EGLint surfaceType(0);
	eglQuerySurface(mDisplay, mSurface, EGL_WIDTH, &gDisplayWidth);
	eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &gDisplayHeight);
	eglGetConfigAttrib(mDisplay, mConfig, EGL_SURFACE_TYPE, &surfaceType);
	MIRACASTLOG_VERBOSE("EGL window surface is %dx%d\n", gDisplayWidth, gDisplayHeight);
    if(eglMakeCurrent(mDisplay, mSurface, mSurface, mContext) != EGL_TRUE) {
        MIRACASTLOG_ERROR(" Unable to make EGL context current 0x%x", eglGetError());
        return true;
    }
	eglSwapInterval(mDisplay, 1);
   return false;
}

void MiracastGraphicsDelegate::preFrameHook() {

}

void MiracastGraphicsDelegate::postFrameHook(bool flush) {
	if(flush){
		eglSwapBuffers(mDisplay, mSurface);
	}
}

void MiracastGraphicsDelegate::teardown() {

    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(mDisplay, mContext);
    mContext = 0;
    eglDestroySurface(mDisplay, mSurface);
	DestroyNativeWindow();
	stopDispatching();
	MIRACASTLOG_VERBOSE("<com.miracastapp.graphics> teardown() completed\n");
}

bool MiracastGraphicsDelegate::startDispatching()
{
    pthread_mutex_lock(&mDispatchMutex);
    if ( !mDispatchThread )
    {
        mDispatchThread = new EssosDispatchThread();
    }
	pthread_mutex_unlock(&mDispatchMutex);
	MIRACASTLOG_VERBOSE("<com.miracastapp.graphics>: startDispatching()\n");
    return true;
}

void MiracastGraphicsDelegate::stopDispatching()
{
    pthread_mutex_lock(&mDispatchMutex);
    if(mDispatchThread)
    {
        mDispatchThread->setRunning(false);
        pthread_join(mDispatchThread->EssDispatchT_Id, nullptr); 
        EssContextStop(mEssCtx);
    }
	pthread_mutex_unlock(&mDispatchMutex);
	pthread_mutex_destroy(&mDispatchMutex);
	MIRACASTLOG_VERBOSE("<com.miracastapp.graphics>: stopDispatching()\n");
}

}
}