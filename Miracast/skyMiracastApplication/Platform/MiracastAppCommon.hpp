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

#ifndef _MIRACAST_APP_COMMON_H_
#define _MIRACAST_APP_COMMON_H_

typedef enum _MiracastAppScreenStates
{
    APPSCREEN_STATE_DEFAULT,
    APPSCREEN_STATE_WELCOME,
    APPSCREEN_STATE_CONNECTING,
    APPSCREEN_STATE_MIRRORING,
    APPSCREEN_STATE_ERROR,
    APPSCREEN_STATE_CONNECTED,
    APPSCREEN_STATE_STOPPED
}
MiracastAppScreenState;

namespace MiracastApp {
namespace Graphics {

//The Graphics Delegate is used by the MiracastApp application to coordinate drawing with the platform.
class Delegate {
public:
    // Setup OpenGL. E.g. initialize framebuffers/renderbuffers. Return false upon error.
    virtual bool initialize() = 0;
    
    // Called immediately prior to MiracastApp drawing. Can be used for debugging, performance measurement, OpenGL state management, etc.
    virtual void preFrameHook() = 0;
    
    // Called after MiracastApp has finished drawing. If flush is true, a frame was drawn to the back buffer and should be swapped to the active buffer.
    virtual void postFrameHook(bool flush) = 0;
    
    // Called when the application is shut down and no longer drawing to the screen. The platform should release graphics resources here.
    virtual void teardown() = 0;
};

}
}

#endif /* _MIRACAST_APP_COMMON_H_ */