/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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