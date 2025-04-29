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

#ifndef _MIRACAST_GRAPHICS_DELEGATE_HPP_
#define _MIRACAST_GRAPHICS_DELEGATE_HPP_

#include <iostream>
#include <essos.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <pthread.h>
#include <map>

#include "RDKPluginCore.h"

namespace MiracastApp{
namespace Graphics{

class EssosRenderThread;

class MiracastGraphicsDelegate
{
	public:
		static MiracastGraphicsDelegate* getInstance(); 
		static void destroyInstance();
		bool initialize();
		void teardown();
		void setAppScreenState(MiracastAppScreenState state, const std::string &deviceName, const std::string &errorCode);
        MiracastAppScreenState getAppScreenState() {return mCurrentAppScreenState;}
        void setFriendlyName(const std::string &friendlyName) { mFriendlyName = friendlyName; }
        void setLanguageCode(const std::string &languageCode) { mLanguageCode = languageCode; }
        std::string getFriendlyName() { return mFriendlyName; }
        std::string getLanguageCode() { return mLanguageCode; }
        std::string getWelcomePageHeader() { return mWelcomePageHeader; }
        std::string getWelcomePageDescription() { return mWelcomePageDescription; }
        std::string getConnectingPageHeader() { return mConnectingPageHeader; }
        std::string getMirroringPageHeader() { return mMirroringPageHeader; }
        std::string getErrorPageHeader() { return mErrorPageHeader; }
        std::string getErrorPageDescription() { return mErrorPageDescription; }
        std::string getButtonText() { return mButtonText; }
        void updateTTSVoiceCommand(const std::string &voiceMsg);
    private:
		bool mResize_pending { false };
		static pthread_mutex_t _mRenderMutex;
		static MiracastGraphicsDelegate* mInstance;
		EssosRenderThread*  _mRenderThread { nullptr };
        RDKTextToSpeech*    _mRDKTextToSpeech {nullptr};

        std::string mFriendlyName;
        std::string mLanguageCode;
        std::string mWelcomePageHeader;
        std::string mWelcomePageDescription;
        std::string mConnectingPageHeader;
        std::string mMirroringPageHeader;
        std::string mErrorPageHeader;
        std::string mErrorPageDescription;
        std::string mButtonText;
        MiracastAppScreenState mCurrentAppScreenState;

        MiracastGraphicsDelegate();	
		MiracastGraphicsDelegate & operator=(const MiracastGraphicsDelegate &) = delete;
		MiracastGraphicsDelegate(const MiracastGraphicsDelegate &) = delete;
		virtual ~MiracastGraphicsDelegate();
    };
}
}
#endif /* _MIRACAST_GRAPHICS_DELEGATE_HPP_ */