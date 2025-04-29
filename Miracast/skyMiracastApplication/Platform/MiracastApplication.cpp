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
#include "MiracastAppCommon.hpp"
#include "MiracastGraphicsDelegate.hpp"
#include "MiracastApplication.hpp"
#include <cstdlib>
#include <unistd.h>
#include <cassert>
#include <string.h>

#include "MiracastAppLogging.hpp"

using namespace MiracastApp;
using namespace MiracastApp::Graphics;

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

static void MiracastPluginEventHandlerCallback(void *args)
{
    MiracastPluginEventListener *miracastServiceListenerInstance = (MiracastPluginEventListener *)args;
    MIRACASTLOG_TRACE("Entering...");
    if ( nullptr != miracastServiceListenerInstance)
    {
        miracastServiceListenerInstance->MiracastPluginEventHandler_Thread(nullptr);
    }
    MIRACASTLOG_TRACE("Exiting...");
}

Engine::Engine()
 : _argc(0)
 , _argv(nullptr)
 , _appStopRequested(false)
 , _state(STOPPED)
{
    MiracastError ret_code = MIRACAST_OK;

    MIRACASTLOG_INFO("Constructor() MiracastApp Application instance");

    _mGraphicsDelegate = MiracastGraphicsDelegate::getInstance();

    _mRDKMiracastPlugin = RDKMiracastPlugin::getInstance();
    if (_mRDKMiracastPlugin)
    {
        _mRDKMiracastPluginListener = new MiracastPluginEventListener();
        if (nullptr == _mRDKMiracastPluginListener)
        {
            MIRACASTLOG_ERROR("Failed to create MiracastPluginEventListener instance");
        }
        else
        {
            _mRDKMiracastPluginListener->startRequestHandlerThread();
            _mRDKMiracastPlugin->registerListener(_mRDKMiracastPluginListener);
        }
    }
     //Redirect stdout and stderr logs to /dev/null
    redirect_std_out_err_logs();
}

Engine::~Engine() {
    this->_freeSavedArgs();

    if( _mRDKMiracastPlugin )
    {
        if ( nullptr != _mRDKMiracastPluginListener ){
            delete _mRDKMiracastPluginListener;
            _mRDKMiracastPluginListener = nullptr;
        }
        RDKMiracastPlugin::destroyInstance();
        _mRDKMiracastPlugin = nullptr;
    }

    if (_mGraphicsDelegate)
    {
        MiracastGraphicsDelegate::destroyInstance();
        _mGraphicsDelegate = nullptr;
    }
}

void Engine::setVideoResolution(VideoRectangleInfo &rect)
{
    MIRACASTLOG_TRACE("Entering ...");
    _video_rect.startX = rect.startX;
    _video_rect.startY = rect.startY;
    _video_rect.width = rect.width;
    _video_rect.height = rect.height;
    MIRACASTLOG_INFO("Video resolution set to: startX=%d, startY=%d, width=%d, height=%d",
                     _video_rect.startX, _video_rect.startY, _video_rect.width, _video_rect.height);
    MIRACASTLOG_TRACE("Exiting ...");
}

void Engine::getVideoResolution(VideoRectangleInfo &rect)
{
    MIRACASTLOG_TRACE("Entering ...");
    rect.startX = _video_rect.startX;
    rect.startY = _video_rect.startY;
    rect.width = _video_rect.width;
    rect.height = _video_rect.height;
    MIRACASTLOG_INFO("Video resolution retrieved: startX=%d, startY=%d, width=%d, height=%d",
                     rect.startX, rect.startY, rect.width, rect.height);
    MIRACASTLOG_TRACE("Exiting ...");
}

void Engine::setCurrentAppScreenState(MiracastAppScreenState state, const std::string &deviceName, const std::string &errorCode)
{
    MIRACASTLOG_TRACE("Entering ...");
    if (_mGraphicsDelegate)
    {
        _mGraphicsDelegate->setAppScreenState(state, deviceName, errorCode);
    }
    MIRACASTLOG_TRACE("Exiting ...");
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
        if (_mRDKMiracastPlugin)
        {
            _mRDKMiracastPlugin->setEnable(true);       
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
                bool didSucceed = this->_mGraphicsDelegate->initialize();
                if (didSucceed) 
                {
                    this->_state = State::RUNNING;
                    setCurrentAppScreenState(APPSCREEN_STATE_WELCOME, "", "");
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
            if(this->_mGraphicsDelegate != nullptr)
                this->_mGraphicsDelegate->teardown();
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
        else if (!this->_mGraphicsDelegate) {
                
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

uint8_t Engine::_getAppstate()
{
    uint8_t state = -1;
    MIRACASTLOG_TRACE("TRACE1");
    state = this->_state;
    MIRACASTLOG_TRACE("TRACE2");
    return state;
}

void MiracastPluginEventListener::startRequestHandlerThread()
{
    MiracastError error_code = MIRACAST_FAIL;
    MIRACASTLOG_TRACE("Entering...");

    m_miracastplugin_event_handler_thread = new MiracastThread( MIRACASTPLUGIN_REQ_HANDLER_THREAD_NAME,
                                                    MIRACASTPLUGIN_REQ_HANDLER_THREAD_STACK,
                                                    MIRACASTPLUGIN_REQ_HANDLER_MSG_COUNT,
                                                    MIRACASTPLUGIN_REQ_HANDLER_MSGQ_SIZE,
                                                    reinterpret_cast<void (*)(void *)>(&MiracastPluginEventHandlerCallback),
                                                    this);
    if (nullptr != m_miracastplugin_event_handler_thread)
    {
        error_code = m_miracastplugin_event_handler_thread->start();

        if ( MIRACAST_OK != error_code )
        {
            delete m_miracastplugin_event_handler_thread;
            m_miracastplugin_event_handler_thread = nullptr;
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastPluginEventListener::stopRequestHandlerThread()
{
    MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT payload;
    payload.eventType = SELF_ABORT;
    MIRACASTLOG_TRACE("Entering ...");
    if ( nullptr != m_miracastplugin_event_handler_thread )
    {
        send_msgto_event_hdler_thread(payload);
        delete m_miracastplugin_event_handler_thread;
        m_miracastplugin_event_handler_thread = nullptr;
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastPluginEventListener::send_msgto_event_hdler_thread(MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT payload)
{
    MIRACASTLOG_TRACE("Entering...");
    if (nullptr != m_miracastplugin_event_handler_thread)
    {
        m_miracastplugin_event_handler_thread->send_message(&payload, MIRACASTPLUGIN_REQ_HANDLER_MSGQ_SIZE);
    }
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastPluginEventListener::onMiracastServiceClientConnectionRequest(const string &client_mac, const string &client_name)
{
    MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT payload;

    payload.eventType = MIRACASTSERVICE_ON_CLIENT_CONNECTION_REQUEST;
    strncpy(payload.src_dev_mac, client_mac.c_str(),sizeof(payload.src_dev_mac));
    payload.src_dev_mac[sizeof(payload.src_dev_mac) - 1] = '\0';
    strncpy(payload.src_dev_name, client_name.c_str(),sizeof(payload.src_dev_name));
    payload.src_dev_name[sizeof(payload.src_dev_name) - 1] = '\0';

    MIRACASTLOG_INFO("client_mac:[%s] client_name:[%s]", client_mac.c_str(), client_name.c_str());
    send_msgto_event_hdler_thread(payload);
}

void MiracastPluginEventListener::onMiracastServiceClientConnectionError(const std::string &client_mac, const std::string &client_name, const std::string &error_code, const std::string &reason )
{
    MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT payload;

    payload.eventType = MIRACASTSERVICE_ON_CLIENT_CONNECTION_ERROR;
    strncpy(payload.src_dev_mac, client_mac.c_str(),sizeof(payload.src_dev_mac));
    payload.src_dev_mac[sizeof(payload.src_dev_mac) - 1] = '\0';
    strncpy(payload.src_dev_name, client_name.c_str(),sizeof(payload.src_dev_name));
    payload.src_dev_name[sizeof(payload.src_dev_name) - 1] = '\0';
    strncpy(payload.error_code, error_code.c_str(),sizeof(payload.error_code));
    payload.error_code[sizeof(payload.error_code) - 1] = '\0';
    strncpy(payload.reason, reason.c_str(),sizeof(payload.reason));
    payload.reason[sizeof(payload.reason) - 1] = '\0';

    MIRACASTLOG_INFO("client_mac:[%s] client_name:[%s] error_code:[%s] reason:[%s]", client_mac.c_str(), client_name.c_str(), error_code.c_str(), reason.c_str());
    send_msgto_event_hdler_thread(payload);
}

void MiracastPluginEventListener::onMiracastServiceLaunchRequest(const string &src_dev_ip, const string &src_dev_mac, const string &src_dev_name, const string & sink_dev_ip)
{
    MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT payload;

    payload.eventType = MIRACASTSERVICE_ON_LAUNCH_REQUEST;
    strncpy(payload.src_dev_ip, src_dev_ip.c_str(),sizeof(payload.src_dev_ip));
    payload.src_dev_ip[sizeof(payload.src_dev_ip) - 1] = '\0';
    strncpy(payload.sink_dev_ip, sink_dev_ip.c_str(),sizeof(payload.sink_dev_ip));
    payload.sink_dev_ip[sizeof(payload.sink_dev_ip) - 1] = '\0';

    strncpy(payload.src_dev_mac, src_dev_mac.c_str(),sizeof(payload.src_dev_mac));
    payload.src_dev_mac[sizeof(payload.src_dev_mac) - 1] = '\0';
    strncpy(payload.src_dev_name, src_dev_name.c_str(),sizeof(payload.src_dev_name));
    payload.src_dev_name[sizeof(payload.src_dev_name) - 1] = '\0';

    MIRACASTLOG_INFO("src_dev_ip:[%s] src_dev_mac:[%s] src_dev_name:[%s] sink_dev_ip:[%s]", src_dev_ip.c_str(), src_dev_mac.c_str(), src_dev_name.c_str(), sink_dev_ip.c_str());
    send_msgto_event_hdler_thread(payload);
}

void MiracastPluginEventListener::onMiracastPlayerStateChange(const std::string &client_mac, const std::string &client_name, const std::string &state, const std::string &reason, const std::string &reason_code)
{
    MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT payload;

    payload.eventType = MIRACASTPLAYER_ON_STATE_CHANGE;
    strncpy(payload.src_dev_mac, client_mac.c_str(),sizeof(payload.src_dev_mac));
    payload.src_dev_mac[sizeof(payload.src_dev_mac) - 1] = '\0';
    strncpy(payload.src_dev_name, client_name.c_str(),sizeof(payload.src_dev_name));
    payload.src_dev_name[sizeof(payload.src_dev_name) - 1] = '\0';
    strncpy(payload.state, state.c_str(),sizeof(payload.state));
    payload.state[sizeof(payload.state) - 1] = '\0';
    strncpy(payload.reason, reason.c_str(),sizeof(payload.reason));
    payload.reason[sizeof(payload.reason) - 1] = '\0';
    strncpy(payload.error_code, reason_code.c_str(),sizeof(payload.error_code));
    payload.error_code[sizeof(payload.error_code) - 1] = '\0';

    MIRACASTLOG_INFO("client_mac:[%s] client_name:[%s] state:[%s] reason:[%s] reason_code:[%s]", client_mac.c_str(), client_name.c_str(), state.c_str(), reason.c_str(), reason_code.c_str());
    send_msgto_event_hdler_thread(payload);
}

void MiracastPluginEventListener::MiracastPluginEventHandler_Thread(void *args)
{
    MIRACASTPLUGIN_REQ_HANDLER_MSGQ_STRUCT payload = {};
    bool m_running_state = true;
    while (m_miracastplugin_event_handler_thread && m_running_state)
    {
        memset(&payload, 0x00, sizeof(payload));
        m_miracastplugin_event_handler_thread->receive_message(&payload, sizeof(payload), THREAD_RECV_MSG_INDEFINITE_WAIT);
        MIRACASTLOG_INFO("Request : Event:0x%x",payload.eventType);
        switch(payload.eventType)
        {
            case MIRACASTSERVICE_ON_CLIENT_CONNECTION_REQUEST:
            {
                std::string client_mac = payload.src_dev_mac,
                            client_name = payload.src_dev_name;
                MiracastApp::Application::Engine::getAppEngineInstance()->setCurrentAppScreenState(APPSCREEN_STATE_CONNECTING, client_name, "");
                MIRACASTLOG_INFO("AutoConnecting [%s - %s]",client_name.c_str(),client_mac.c_str());
                MiracastApp::Application::Engine::getAppEngineInstance()->getRDKMiracastPluginInstance()->acceptClientConnection("Accept");
            }
            break;
            case MIRACASTSERVICE_ON_CLIENT_CONNECTION_ERROR:
            {
                std::string client_mac = payload.src_dev_mac,
                            client_name = payload.src_dev_name,
                            error_code = payload.error_code,
                            reason = payload.reason,
                            actual_error_code = "ENT-32" + error_code;
                MIRACASTLOG_INFO("ON_CLIENT_CONNECTION_ERROR : client_mac:[%s] client_name:[%s] error_code:[%s] reason:[%s]",client_mac.c_str(),client_name.c_str(),error_code.c_str(),reason.c_str());
                MiracastApp::Application::Engine::getAppEngineInstance()->setCurrentAppScreenState(APPSCREEN_STATE_ERROR, client_name, actual_error_code);
            }
            break;
            case MIRACASTSERVICE_ON_LAUNCH_REQUEST:
            {
                VideoRectangleInfo video_rect;
                _source_dev_ip = payload.src_dev_ip;
                _source_dev_mac = payload.src_dev_mac;
                _source_dev_name = payload.src_dev_name;
                _sink_dev_ip = payload.sink_dev_ip;
                MiracastApp::Application::Engine::getAppEngineInstance()->getVideoResolution(video_rect);

                MiracastApp::Application::Engine::getAppEngineInstance()->setCurrentAppScreenState(APPSCREEN_STATE_MIRRORING, _source_dev_name, "");
                MIRACASTLOG_INFO("ON_LAUNCH_REQUEST : source_dev_ip:[%s] source_dev_mac:[%s] source_dev_name:[%s] sink_dev_ip:[%s]",_source_dev_ip.c_str(),_source_dev_mac.c_str(),_source_dev_name.c_str(),_sink_dev_ip.c_str());
                MiracastApp::Application::Engine::getAppEngineInstance()->getRDKMiracastPluginInstance()->setWesterosEnvToMiracastPlayer();
                MiracastApp::Application::Engine::getAppEngineInstance()->getRDKMiracastPluginInstance()->playRequestToMiracastPlayer(_source_dev_ip, _source_dev_mac, _source_dev_name, _sink_dev_ip, video_rect);
            }
            break;
            case MIRACASTPLAYER_ON_STATE_CHANGE:
            {
                std::string client_mac = payload.src_dev_mac,
                            client_name = payload.src_dev_name,
                            state = payload.state,
                            reason = payload.reason,
                            reason_code = payload.error_code;
                MIRACASTLOG_INFO(">>> client_mac:[%s] client_name:[%s] state:[%s] reason:[%s] reason_code:[%s]",client_mac.c_str(),client_name.c_str(),state.c_str(),reason.c_str());

                if ("PLAYING" == state)
                {
                    MiracastApp::Application::Engine::getAppEngineInstance()->setCurrentAppScreenState(APPSCREEN_STATE_CONNECTED, client_name, "");
                }
                else if ( "STOPPED" == state )
                {
                    if (("200" == reason_code)||("201" == reason_code)||("202" == reason_code))
                    {
                        MiracastApp::Application::Engine::getAppEngineInstance()->setCurrentAppScreenState(APPSCREEN_STATE_STOPPED, client_name, "");
                    }
                    else
                    {
                        std::string actual_error_code = "ENT-32" + reason_code;
                        MiracastApp::Application::Engine::getAppEngineInstance()->setCurrentAppScreenState(APPSCREEN_STATE_ERROR, client_name, actual_error_code);
                    }
                }
                MiracastApp::Application::Engine::getAppEngineInstance()->getRDKMiracastPluginInstance()->updatePlayerState(client_mac, state, reason_code);
            }
            break;
            case SELF_ABORT:
            {
                MIRACASTLOG_INFO("SELF_ABORT ACTION Received");
                m_running_state = false;
                MiracastApp::Application::Engine::getAppEngineInstance()->getRDKMiracastPluginInstance()->stopMiracastPlayer();
            }
            default:
            {
                MIRACASTLOG_ERROR("Invalid event type [%d]",payload.eventType);
            }
            break;
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
}

}
}