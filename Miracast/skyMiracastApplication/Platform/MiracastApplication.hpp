/*
	Copyright (C) 2021 Apple Inc. All Rights Reserved. Not to be used or disclosed without permission from Apple.
*/

#ifndef MiracastApplication_hpp
#define MiracastApplication_hpp

//TODO: locking
#include <mutex>

#include "MiracastGraphicsPAL.hpp"

namespace MiracastApp {
namespace Application {

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

class Engine {
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
	
	// Call setGraphicsDelegate before start(). Required.
	Status setGraphicsDelegate(MiracastApp::Graphics::Delegate* graphicsDelegate);
	
	// Call setLaunchArguments before start() with the command line arguments passed to the application by the MiracastApp daemon
	Status setLaunchArguments(int argc, const char **argv);
	
	// Call setMediaImplementation before start()
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
	// Warning: the Luna SDK does not expose a true "stop" command--there is no way to reliably stop and restart
	// the application without restarting the process completely.
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

private:
	// TODO: locking
	//	std::mutex _lock;
	int 									_argc;
	const char **							_argv;
	MiracastApp::Graphics::Delegate *       _graphicsDelegate;
	bool                                    _appStopRequested;
	State 									_state;
	std::mutex                              _appEngineMutex;
	static Engine * 						_appEngine;
	//To free up the command line  arguments 
	void _freeSavedArgs();

	//checking for miracasapp engine readyness
	bool _readyToStart();

	bool _shouldTick();

	//TODO: Make a shared Engine instance (since Luna uses shared instances extensively)
	Engine();
	Engine & operator=(const Engine &) = delete;
	Engine(const Engine &) = delete;
	virtual ~Engine();
};
}

}

#endif /* MiracastApplication_hpp */
