//
//  MiracastGraphicsDelegate.hpp
//  Copyright (C) 2020 Apple Inc. All Rights Reserved. Not to be used or disclosed without permission from Apple.
//

#ifndef MIRACAST_GRAPHICS_DELEGATE_HPP
#define MIRACAST_GRAPHICS_DELEGATE_HPP

#include <iostream>
#include <essos.h>
#include <pthread.h>
#include <map>

#include "MiracastGraphicsPAL.hpp"
#include "MiracastApplication.hpp"
namespace MiracastApp{
namespace Graphics{

#define NATIVE_KEYCODE_SELECT 0x1C
#define NATIVE_KEYCODE_UP 0x67
#define NATIVE_KEYCODE_DOWN 0x6C
#define NATIVE_KEYCODE_RIGHT 0x6A
#define NATIVE_KEYCODE_LEFT 0x69
#define NATIVE_KEYCODE_BACK 0x26
#define NATIVE_KEYCODE_HOME 0x42
#define NATIVE_KEYCODE_VOLUME_UP 0x73
#define NATIVE_KEYCODE_VOLUME_DOWN 0x72
#define NATIVE_KEYCODE_MUTE 0x71
#define NATIVE_KEYCODE_STANDBY 0x57
#define NATIVE_KEYCODE_VOICE 0x66
#define NATIVE_KEYCODE_INPUT_SELECT 0x44
#define NATIVE_KEYCODE_SWKEY_BACK 0x0E
#define NATIVE_KEYCODE_SKYKEY_BACK 0x01

class EssosDispatchThread;

class MiracastGraphicsDelegate : public MiracastApp::Graphics::Delegate {
	public:

		static MiracastGraphicsDelegate* getInstance(); 
		static void destroyInstance();
		bool initialize();
		void preFrameHook();
		void postFrameHook(bool flush);
		void teardown();
		EssCtx *getEssCtxInstance() {return mEssCtx;}
    	bool startDispatching();
    	void stopDispatching();
    protected:
                void OnKeyPressed(unsigned int key);
                void OnKeyReleased(unsigned int key);
				void OnKeyRepeat(unsigned int key);
	private: 
		bool mResize_pending { false };
		EssCtx *mEssCtx { nullptr };
		int gDisplayWidth { 0 };
		int gDisplayHeight { 0 };
		NativeWindowType mNativewindow { 0 };
		EGLConfig mConfig;
		EGLDisplay mDisplay;
		EGLContext mContext;
		EGLSurface mSurface;
		pthread_mutex_t mDispatchMutex;
		static MiracastGraphicsDelegate* mInstance;
		static EssKeyListener keyListener;
		EssosDispatchThread *mDispatchThread { nullptr };
		MiracastApp::Application::Engine * mAppEngine { nullptr }; 
		MiracastGraphicsDelegate();	
		MiracastGraphicsDelegate & operator=(const MiracastGraphicsDelegate &) = delete;
		MiracastGraphicsDelegate(const MiracastGraphicsDelegate &) = delete;
		virtual ~MiracastGraphicsDelegate();
		bool BuildEssosContext();
		void DestroyNativeWindow();
		void displaySize(void *userData, int width, int height );
		bool InitializeEGL();
};
}
}
#endif /* MIRACAST_GRAPHICS_DELEGATE_HPP */