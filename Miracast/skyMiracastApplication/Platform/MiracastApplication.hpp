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

#ifndef _MIRACAST_APPLICATION_H_
#define _MIRACAST_APPLICATION_H_

//TODO: locking
#include <mutex>
#include "RDKPluginCore.h"
#include "MiracastCommon.h"

namespace MiracastApp {
namespace Application {

//class MiracastGraphicsDelegate;

typedef enum _MiracastPluginRequestEvents
{
    MIRACASTSERVICE_ON_CLIENT_CONNECTION_REQUEST,
    MIRACASTSERVICE_ON_CLIENT_CONNECTION_ERROR,
    MIRACASTSERVICE_ON_LAUNCH_REQUEST,
    MIRACASTPLAYER_ON_STATE_CHANGE,
    SELF_ABORT,
    INVALID_REQUEST
}
MiracastPluginRequestEvent;

typedef struct _miracast_plugin_request_hldr_msgq_st
{
    MiracastPluginRequestEvent eventType;
    char reason[64];
    char src_dev_name[40];
    char error_code[32];
    char src_dev_ip[24];
    char src_dev_mac[24];
    char sink_dev_ip[24];
    char state[24];
}
MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT;

#define MIRACASTPLUGIN_REQ_HANDLER_THREAD_NAME ("MIRACASTPLUGIN_REQ_HANDLER")
#define MIRACASTPLUGIN_REQ_HANDLER_THREAD_STACK ( 512 * 1024)
#define MIRACASTPLUGIN_REQ_HANDLER_MSG_COUNT (2)
#define MIRACASTPLUGIN_REQ_HANDLER_MSGQ_SIZE (sizeof(MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT))

enum Status {
	// No error
	OK = 0,
	
	//An unspecified error occured
	ERROR,
	
	// The method or operation was attempted when it was not allowed
	INVALID_STATE,
	
	// The receiver was not ready
	NOT_READY,
	
	// Returned by tick() to indicate the application has quit
	APP_TERMINATED
};

class MiracastPluginEventListener : public MiracastPlugin::IMiracastPluginListener
{
public:
    MiracastPluginEventListener() {MIRACASTLOG_INFO("MiracastPluginEventListener Constructor");};
    void MiracastPluginEventHandler_Thread(void *args);
    void startRequestHandlerThread();
    void stopRequestHandlerThread();

    void send_msgto_event_hdler_thread(MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT payload);

    void onMiracastServiceClientConnectionRequest(const string &client_mac, const string &client_name);
    void onMiracastServiceClientConnectionError(const std::string &client_mac, const std::string &client_name, const std::string &error_code, const std::string &reason );
    void onMiracastServiceLaunchRequest(const string &src_dev_ip, const string &src_dev_mac, const string &src_dev_name, const string & sink_dev_ip);

    void onMiracastPlayerStateChange(const std::string &client_mac, const std::string &client_name, const std::string &state, const std::string &reason, const std::string &reason_code);

    ~MiracastPluginEventListener() {};

private:
    MiracastThread *m_miracastplugin_event_handler_thread;
    std::string     _source_dev_ip;
    std::string     _source_dev_mac;
    std::string     _source_dev_name;
    std::string     _sink_dev_ip;
};

class Engine
{
public:
	enum State {
		// Application is not running. Initial State. Setup Engine in this State.
		STOPPED,
		
		// start() has been called, but the first tick() has not yet been executed.
		STARTED,
		
		// Application is running.
		RUNNING,
		BACKGROUNDING,
		// Application is backgrounded
		BACKGROUNDED,
		RESUMING,
		
	};

    void setCurrentAppScreenState(MiracastAppScreenState state, const std::string &deviceName, const std::string &errorCode);
	
	// Call setLaunchArguments before start() with the command line arguments passed to the application by the MiracastApp daemon
	Status setLaunchArguments(int argc, const char **argv);

	static Engine *  getAppEngineInstance(void){
		if(_appEngine == nullptr){
			_appEngine = new Engine();
		}
		return _appEngine;
	}

	static void destroyAppEngineInstance(void){
		if(_appEngine != nullptr){
			delete _appEngine;
			_appEngine = nullptr;
		}
	}

    RDKMiracastPlugin *  getRDKMiracastPluginInstance(void){
		return _mRDKMiracastPlugin;
	}

    // The main thread is the thread which owns the OpenGL context.
	// These methods should only be called from the main thread:
	//
	// start()
	// tick()
	// stop()
	Status start();
	
	// Call tick() in a loop, once per frame, from the main thread, after calling start().
	// tick() runs one iteration of the application run loop and draws a frame to the current frame buffer.
	// Once tick() returns Status::APP_TERMINATED, the executable should clean up and exit.
	// Pass true to force a frame to be drawn.
	Status tick(bool forceFrameDraw=false);
	
	// Call stop() to clean up resources, usually in response to tick() returning Status::APP_TERMINATED.
	// tick() may not be called after stop().
	//
	Status stop();
	
	// Trigger a "backgrounding" of the draw loop
	// The expectation is that it is legal for an implementation to suspend calling appEngine.tick *3* seconds after minimizing/hiding
	// the application
	void background();
	
	// Trigger a "foregrounding" of the draw loop
	// This reverses the backgrounding, so that "draw" is called
	void foreground();
        
	// Returns current Application state
	uint8_t _getAppstate();	

	// Sets Application state
	//uint8_t _setAppState(State newState);

	// Main process request to stop Application
	void _RequestApptoStop();

	//Returns current value of _appStopRequested
	bool _isAppStopRequested();

    void setVideoResolution(VideoRectangleInfo &rect);
    void getVideoResolution(VideoRectangleInfo &rect);
    void playRequest(std::string source_dev_ip, std::string source_dev_mac , std::string source_dev_name, std::string sink_dev_ip);
    void stopRequest(void);

private:
	// TODO: locking
	//	std::mutex _lock;
	int 									_argc;
	const char **							_argv;
	bool                                    _appStopRequested;
	State 									_state;
	std::mutex                              _appEngineMutex;
    VideoRectangleInfo                      _video_rect;
	static Engine * 						_appEngine;
    MiracastApp::Graphics::MiracastGraphicsDelegate*               _mGraphicsDelegate {nullptr};
    RDKMiracastPlugin * 					_mRDKMiracastPlugin {nullptr};
    MiracastPluginEventListener *          _mRDKMiracastPluginListener {nullptr};
	//To free up the command line  arguments 
	void _freeSavedArgs();

	//checking for miracasapp engine readyness
	bool _readyToStart();

	bool _shouldTick();

    std::string stateDescription(eMIRA_PLAYER_STATES e);
    std::string reasonDescription(eM_PLAYER_REASON_CODE e);

	Engine();
	Engine & operator=(const Engine &) = delete;
	Engine(const Engine &) = delete;
	virtual ~Engine();
};

}
}

#endif /* _MIRACAST_APPLICATION_H_ */