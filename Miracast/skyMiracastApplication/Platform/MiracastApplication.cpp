/*
        Copyright (C) 2021 Apple Inc. All Rights Reserved. Not to be used or disclosed without permission from Apple.
*/

#include "MiracastGraphicsDelegate.hpp"
#include "MiracastApplication.hpp"
#include <cstdlib>
#include <unistd.h>
#include <cassert>
#include <string.h>

#include "MiracastAppLogging.hpp"

static void redirect_std_out_err_logs()
{
    MIRACASTLOG_TRACE("Entering ...");

    const char *disable_stderr_redirect_flag = "/opt/disable_miracastapp_redirect_stderr_log";
    const char *disable_stdout_redirect_flag = "/opt/disable_miracastapp_redirect_stdout_log";

    // const char *log_file_name = "/tmp/log_redirect_msg.txt";
    const char *log_file_name = "/dev/null";

    if (access(disable_stdout_redirect_flag, F_OK) != 0)
    {
        FILE *fp_stdout = freopen(log_file_name, "w", stdout);
        if (fp_stdout)
        {
            MIRACASTLOG_INFO("By default, the stdout redirected to [%s]\n", log_file_name);
            fclose(fp_stdout);
        }
    }
    else
    {
        MIRACASTLOG_WARNING("The [%s] flag is present, so disabled stdout redirect logging to [%s]. \n", disable_stdout_redirect_flag, log_file_name);
    }

    if (access(disable_stderr_redirect_flag, F_OK) != 0)
    {
        MIRACASTLOG_WARNING("By default, stderr will redirect to [%s]. \n", log_file_name);
        FILE *fp_stderr = freopen(log_file_name, "w", stderr);
        if (fp_stderr)
        {
            MIRACASTLOG_INFO("The stderr redirected to [%s].\n", log_file_name);
            fclose(fp_stderr);
        }
    }
    else
    {
        MIRACASTLOG_WARNING("The [%s] flag is present, so disabled stderr redirect logging to [%s]. \n", disable_stderr_redirect_flag, log_file_name);
    }
    MIRACASTLOG_VERBOSE("Exiting..!!! \n");
}

namespace MiracastApp {
namespace Application {

Engine * Engine::_appEngine {nullptr};

Engine::Engine()
 : _argc(0)
 , _argv(nullptr)
 , _graphicsDelegate(nullptr)
 , _appStopRequested(false)
 , _state(STOPPED)
{
        MIRACASTLOG_INFO("Constructor() MiracastApp Application instance");
        redirect_std_out_err_logs();
}

Engine::~Engine() {
        this->_freeSavedArgs();
}

Status Engine::setGraphicsDelegate(MiracastApp::Graphics::Delegate* graphicsDelegate) {
        //TODO: lock (?)
        if (this->_state != State::STOPPED) {
                return Status::INVALID_STATE;
        }
        this->_graphicsDelegate = graphicsDelegate;
        MIRACASTLOG_VERBOSE("setGraphicsDelegate");
        return Status::OK;
}

void Engine::_freeSavedArgs() {
        if (this->_argv != nullptr) {
                for (int i = 0; i < this->_argc; i++) {
                        free((void *)this->_argv[i]);
                }
                delete[] this->_argv;
        }
        this->_argc = 0;
        this->_argv = nullptr;
        MIRACASTLOG_VERBOSE("_freeSavedArgs");
}

Status Engine::setLaunchArguments(int argc, const char **argv) {
        //TODO: lock(?)
        MIRACASTLOG_VERBOSE("Entered, args:%d", argc);
        this->_freeSavedArgs();
        this->_argc = argc;
        this->_argv = new const char*[argc];;
        for (int i = 0; i < argc; i++) {
                this->_argv[i] = strdup(argv[i]);
                MIRACASTLOG_VERBOSE("argv[%d]:%s\n",i, argv[i]);
        }
        
        return Status::OK;
}

Status Engine::start() {
        if (this->_state != State::STOPPED) {
                return Status::INVALID_STATE;
        }
        else if (!this->_readyToStart()) {
                return Status::NOT_READY;
        }
        this->_state = State::STARTED;
        return Status::OK;
}

Status Engine::tick(bool forceFrameDraw) {
        //TODO: locking
        //_appEngineMutex.lock();
        //TODO: more robust state checking
        if (this->_state == State::STOPPED) {
                MIRACASTLOG_INFO("App state is State::STOPPED, Invalid_state");
                return Status::INVALID_STATE;
        }

        if(this->_appStopRequested == true){
                MIRACASTLOG_INFO("Requested App to stop Status::APP_TERMINATED");
                //_appEngineMutex.unlock();
                return Status::APP_TERMINATED;
        }
        if (this->_state == State::STARTED) {
                // Initialize first-run
                // Force the first frame to draw
                bool didSucceed = this->_graphicsDelegate->initialize();
                if (didSucceed) 
                {
                        this->_state = State::RUNNING;
                }
                else {
                        //TODO: handle failure to initialize (displayInfo might be stale on second run)
                        MIRACASTLOG_INFO("Unable to initialize graphics delegate--stopping");
                        this->stop();
                        //_appEngineMutex.unlock();
                        return Status::ERROR;
                }
        }

        // Update dynamic properties
        if (this->_state != State::BACKGROUNDED)
        {
                //
        }

        if (this->_state == State::RUNNING) {
            bool platformShouldSwapFrame = false;
            this->_graphicsDelegate->preFrameHook();
            this->_graphicsDelegate->postFrameHook(platformShouldSwapFrame);
        } else if (this->_state == State::BACKGROUNDED) {
            MIRACASTLOG_INFO("BACKGROUNDED \n");
        }
        //_appEngineMutex.unlock();
    return Status::OK;
}

void Engine::background()
{
        MIRACASTLOG_INFO("Backgrounding app, no more js code will run");
	this->_state = State::BACKGROUNDED;
}

void Engine::foreground()
{
	if (this->_state == State::BACKGROUNDED || this->_state == State::BACKGROUNDING) {
		MIRACASTLOG_INFO("Foregrounding app, js code will run");
	}
}

void Engine::_RequestApptoStop()
{
    _appEngineMutex.lock();
    _appStopRequested = true;
    _appEngineMutex.unlock();
}

bool Engine::_isAppStopRequested(){
        return _appStopRequested;
}

Status Engine::stop() {
        if (this->_state != State::STOPPED) {
		    MIRACASTLOG_INFO("AppEngine stop() free-up resources");
            if(this->_graphicsDelegate != nullptr)
                this->_graphicsDelegate->teardown();
        }
        //Reset
        this->_state = State::STOPPED;

        return Status::OK;
}

// Private methods

bool Engine::_readyToStart() {
        bool isReady = false;
        std::string failureReason;

        if (this->_state != State::STOPPED) {
            MIRACASTLOG_ERROR(" Application engine state is not STOPPED");
        }
        else if (!this->_graphicsDelegate) {
                
			MIRACASTLOG_ERROR(" Graphics delegate not set");
        }
        else {
            MIRACASTLOG_INFO(" Application engine is ready to launch");
            isReady = true;
        }
        return isReady;
}

static bool _controlHndlr(void *context, const std::string &control, const std::string &message, uint32_t ) {
	if (control == "AXUpdateTree") {
		// This is the display tree if required for a screen reader accessibility tool
	}
	return true;
}

uint8_t Engine::_getAppstate(){
        return this->_state;
}

}
}
