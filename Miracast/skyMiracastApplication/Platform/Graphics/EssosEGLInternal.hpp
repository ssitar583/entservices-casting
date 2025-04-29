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

#ifndef _ESSOS_EGL_INTERNAL_H_
#define _ESSOS_EGL_INTERNAL_H_

#include <iostream>
#include <essos.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <pthread.h>
#include <map>

namespace MiracastApp{
namespace Graphics{
    class EssosRenderThread
    {
        public:
            pthread_t EssRenderT_Id;
            pthread_t RenderT_Id;
            struct sigaction sigact;
            static void signalHandler(int signum);

            EssosRenderThread();
            ~EssosRenderThread()
            {
                // don't let the thread outlive the object
            }
            bool isRunning() { return mRunning; }
            void setRunning(bool running);

        protected:
            void OnKeyPressed(unsigned int key);
            void OnKeyReleased(unsigned int key);
            void OnKeyRepeat(unsigned int key);

        private:
            static void* run(void *arg);
            bool mRunning;
            static EssosRenderThread *_mEssRenderTh;
            static EssKeyListener keyListener;
            pthread_mutex_t mRunningMutex;

            EssCtx *mEssCtx { nullptr };
            int gDisplayWidth { 0 };
            int gDisplayHeight { 0 };
            NativeWindowType mNativewindow { 0 };
            EGLConfig mConfig;
            EGLDisplay mDisplay;
            EGLContext mContext;
            EGLSurface mSurface;

            bool BuildEssosContext();
            void DestroyNativeWindow();
            void displaySize(void *userData, int width, int height );
            bool InitializeEGL();
            void InitializeQtRendering(void);

            void fillColor(float alpha, float red, float green, float blue, bool swapBuffers );
            void displayMessage(const std::string& headerText, const std::string& bodyText, const std::string& buttonText);
    };
}
}
#endif /* _ESSOS_EGL_INTERNAL_H_ */