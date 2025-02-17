//
//  MiracastAppMainStub.hpp
//  Copyright (C) 2020 Apple Inc. All Rights Reserved. Not to be used or disclosed without permission from Apple.
//

#ifndef MiracastAppMainStub_hpp
#define MiracastAppMainStub_hpp

#include<mutex>

#include "MiracastApplication.hpp"
#include "MiracastGraphicsDelegate.hpp"
#include "ThunderUtils.h"

using namespace MiracastApp;
using namespace MiracastApp::Graphics;

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
    	MiracastGraphicsDelegate * mGraphicsDelegate {nullptr};
        MiracastApp::Application::Engine *mAppEngine {nullptr};
	    std::mutex mAppRunnerMutex;
        MiracastAppMain();
        ~MiracastAppMain();
		MiracastAppMain & operator=(const MiracastAppMain &) = delete;
		MiracastAppMain(const MiracastAppMain &) = delete;
};
#endif /* MiracastAppMainStub_hpp */
