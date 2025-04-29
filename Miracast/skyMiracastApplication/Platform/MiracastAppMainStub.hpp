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

#ifndef MiracastAppMainStub_hpp
#define MiracastAppMainStub_hpp

#include<mutex>

#include "ThunderUtils.h"
#include "MiracastApplication.hpp"

using namespace MiracastApp;

class MiracastAppMain{
    public:
        static MiracastAppMain * getMiracastAppMainInstance();
        static void destroyMiracastAppMainInstance();
        static bool isMiracastAppMainObjValid();
        uint32_t launchMiracastAppMain(int argc, const char **argv,void*);
	void handleStopMiracastApp();
	int getMiracastAppstate();
    void setMiracastAppVisibility(bool visible);
    private:
        static MiracastAppMain * mMiracastAppMain;
        MiracastApp::Application::Engine *mAppEngine {nullptr};
	    std::mutex mAppRunnerMutex;
        MiracastAppMain();
        ~MiracastAppMain();
		MiracastAppMain & operator=(const MiracastAppMain &) = delete;
		MiracastAppMain(const MiracastAppMain &) = delete;
};
#endif /* MiracastAppMainStub_hpp */
