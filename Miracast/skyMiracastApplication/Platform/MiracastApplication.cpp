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
MiracastRTSPMsg *Engine::_mMiracastRTSPInstance {nullptr};
MiracastGstPlayer *Engine::_mMiracastGstPlayer {nullptr};

static void MiracastServiceEventHandlerCallback(void *args)
{
    MiracastServiceEventListener *miracastServiceListenerInstance = (MiracastServiceEventListener *)args;
    MIRACASTLOG_TRACE("Entering...");
    if ( nullptr != miracastServiceListenerInstance)
    {
        miracastServiceListenerInstance->MiracastServiceEventHandler_Thread(nullptr);
    }
    MIRACASTLOG_TRACE("Exiting...");
}

Engine::Engine()
 : _argc(0)
 , _argv(nullptr)
 , _graphicsDelegate(nullptr)
 , _appStopRequested(false)
 , _state(STOPPED)
{
    MiracastError ret_code = MIRACAST_OK;

    MIRACASTLOG_INFO("Constructor() MiracastApp Application instance");

    _mMiracastRTSPInstance = MiracastRTSPMsg::getInstance(ret_code, this);
    if (!_mMiracastRTSPInstance )
    {
        MIRACASTLOG_ERROR("Failed to get MiracastRTSPMsginstance");
    }
    else
    {
        _mMiracastGstPlayer = MiracastGstPlayer::getInstance();

        if (!_mMiracastGstPlayer)
        {
            MIRACASTLOG_ERROR("Failed to get MiracastGstPlayer instance");
        }
    }

    _mRDKTextToSpeech = RDKTextToSpeech::getInstance();
    MIRACASTLOG_INFO("TRACE: RDKTextToSpeech instance created");
    _mRDKMiracastService = RDKMiracastService::getInstance();
    MIRACASTLOG_INFO("TRACE: RDKMiracastService instance created");
    if (_mRDKMiracastService)
    {
        MIRACASTLOG_INFO("TRACE: RDKMiracastService instance created");
        _mRDKMiracastServiceListener = new MiracastServiceEventListener();
        if (nullptr == _mRDKMiracastServiceListener)
        {
            MIRACASTLOG_ERROR("Failed to create MiracastServiceEventListener instance");
        }
        else
        {
            MIRACASTLOG_INFO("TRACE: RDKMiracastServiceEventListener instance created");
            _mRDKMiracastServiceListener->startRequestHandlerThread();
            MIRACASTLOG_INFO("TRACE: RDKMiracastServiceEventListener request handler thread started");
            _mRDKMiracastService->registerListener(_mRDKMiracastServiceListener);
        }
    }
     //Redirect stdout and stderr logs to /dev/null
    redirect_std_out_err_logs();
}

Engine::~Engine() {
    this->_freeSavedArgs();

    if (_mMiracastGstPlayer)
    {
        MiracastGstPlayer::destroyInstance();
        _mMiracastGstPlayer = nullptr;
    }
    if (_mMiracastRTSPInstance)
    {
        MiracastRTSPMsg::destroyInstance();
        _mMiracastRTSPInstance = nullptr;
    }

    if ( _mRDKTextToSpeech )
    {
        RDKTextToSpeech::destroyInstance();
        _mRDKTextToSpeech = nullptr;
    }

    if( _mRDKMiracastService )
    {
        if ( nullptr != _mRDKMiracastServiceListener ){
            delete _mRDKMiracastServiceListener;
            _mRDKMiracastServiceListener = nullptr;
        }
        RDKMiracastService::destroyInstance();
        _mRDKMiracastService = nullptr;
    }
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

void Engine::setVideoResolution(VideoRectangleInfo &rect)
{
    MIRACASTLOG_TRACE("Entering ...");
    _video_rect.startX = rect.startX;
    _video_rect.startY = rect.startY;
    _video_rect.width = rect.width;
    _video_rect.height = rect.height;
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
        updateTTSVoiceCommand(READY_TO_CAST_TTS_VOICE_COMMAND, {});
        if (_mRDKMiracastService)
        {
            _mRDKMiracastService->setEnable(true);
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

void Engine::onStateChange(const std::string& client_mac, const std::string& client_name, eMIRA_PLAYER_STATES player_state, eM_PLAYER_REASON_CODE reason_code)
{
    vector<std::string> strArgs;
    std::string stateStr = stateDescription(player_state),
                reason_codeStr = std::to_string(reason_code),
                reasonStr = reasonDescription(reason_code);
    MIRACASTLOG_INFO("client_name[%s]client_mac[%s]player_state[%s]reason_code[%s]",client_name.c_str(),client_mac.c_str(),stateStr.c_str(),reason_codeStr.c_str());
    if ((MIRACAST_PLAYER_STATE_STOPPED == player_state) && (MIRACAST_PLAYER_REASON_CODE_SUCCESS != reason_code ))
    {
        strArgs.push_back(client_name);
        strArgs.push_back(reason_codeStr);
        updateTTSVoiceCommand(CONNECTION_ERROR_TTS_VOICE_COMMAND, strArgs);
    }
    if (_mRDKMiracastService)
    {
        _mRDKMiracastService->updatePlayerState(client_mac, stateStr, reason_codeStr);
    }
}

void Engine::updateTTSVoiceCommand(MiracastTTSVoiceCommandTypes type, std::vector<std::string> args)
{
    std::string command;

    switch (type)
    {
        case READY_TO_CAST_TTS_VOICE_COMMAND:
        {
            command = "Miracast Ready to cast";
        }
        break;
        case CONNECT_REQUEST_TTS_VOICE_COMMAND:
        {
            if ( 1 == args.size()) {
                command = args[0] + "  wants to connect to this TV and Connecting to it";
            }
        }
        break;
        case LAUNCH_REQUEST_TTS_VOICE_COMMAND:
        {
            if ( 1 == args.size()) {
                command = "Starting to cast from " + args[0];
            }
        }
        break;
        case CONNECTION_ERROR_TTS_VOICE_COMMAND:
        {
            if ( 2 == args.size()) {
                command = args[0] + " couldn't connect to this TV\nUse your device to try again.\nError code: ENT-32" + args[1];
            }
        }
        break;
        default:
            break;
    }
    if (_mRDKTextToSpeech)
    {
        MIRACASTLOG_INFO("TTS command: %s", command.c_str());
        _mRDKTextToSpeech->speak(command);
    }
}

void Engine::playRequest(std::string source_dev_ip, std::string source_dev_mac , std::string source_dev_name, std::string sink_dev_ip)
{
    RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};
    MIRACASTLOG_INFO("Entering..!!!");

    strncpy( rtsp_hldr_msgq_data.source_dev_ip, source_dev_ip.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_ip));
    rtsp_hldr_msgq_data.source_dev_ip[sizeof(rtsp_hldr_msgq_data.source_dev_ip) - 1] = '\0';
    strncpy( rtsp_hldr_msgq_data.source_dev_mac, source_dev_mac.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_mac));
    rtsp_hldr_msgq_data.source_dev_mac[sizeof(rtsp_hldr_msgq_data.source_dev_mac) - 1] = '\0';
    strncpy( rtsp_hldr_msgq_data.source_dev_name, source_dev_name.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_name));
    rtsp_hldr_msgq_data.source_dev_name[sizeof(rtsp_hldr_msgq_data.source_dev_name) - 1] = '\0';
    strncpy( rtsp_hldr_msgq_data.sink_dev_ip, sink_dev_ip.c_str() , sizeof(rtsp_hldr_msgq_data.sink_dev_ip));
    rtsp_hldr_msgq_data.sink_dev_ip[sizeof(rtsp_hldr_msgq_data.sink_dev_ip) - 1] = '\0';

    rtsp_hldr_msgq_data.state = RTSP_START_RECEIVE_MSGS;

    rtsp_hldr_msgq_data.videorect.startX = _video_rect.startX;
    rtsp_hldr_msgq_data.videorect.startY = _video_rect.startY;
    rtsp_hldr_msgq_data.videorect.width = _video_rect.width;
    rtsp_hldr_msgq_data.videorect.height = _video_rect.height;

    if (_mMiracastRTSPInstance)
    {
        _mMiracastRTSPInstance->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
    }
    MIRACASTLOG_INFO("Exiting..!!!");
}

void Engine::stopRequest(void)
{
    MIRACASTLOG_INFO("Entering ..!!!");
    RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};

    rtsp_hldr_msgq_data.state = RTSP_TEARDOWN_FROM_SINK2SRC;
    rtsp_hldr_msgq_data.stop_reason_code = MIRACAST_PLAYER_APP_REQ_TO_STOP_ON_EXIT;
    if (_mMiracastRTSPInstance)
    {
        _mMiracastRTSPInstance->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
    }

    MIRACASTLOG_INFO("Exiting..!!!");
}

std::string Engine::stateDescription(eMIRA_PLAYER_STATES e)
{
    switch (e)
    {
        case MIRACAST_PLAYER_STATE_IDLE:
            return "IDLE";
        case MIRACAST_PLAYER_STATE_INITIATED:
            return "INITIATED";
        case MIRACAST_PLAYER_STATE_INPROGRESS:
            return "INPROGRESS";
        case MIRACAST_PLAYER_STATE_PLAYING:
            return "PLAYING";
        case MIRACAST_PLAYER_STATE_STOPPED:
        case MIRACAST_PLAYER_STATE_SELF_ABORT:
            return "STOPPED";
        default:
            return "Unimplemented state";
    }
}

std::string Engine::reasonDescription(eM_PLAYER_REASON_CODE e)
{
    switch (e)
    {
        case MIRACAST_PLAYER_REASON_CODE_SUCCESS:
            return "SUCCESS";
        case MIRACAST_PLAYER_REASON_CODE_APP_REQ_TO_STOP:
            return "APP REQUESTED TO STOP.";
        case MIRACAST_PLAYER_REASON_CODE_SRC_DEV_REQ_TO_STOP:
            return "SRC DEVICE REQUESTED TO STOP.";
        case MIRACAST_PLAYER_REASON_CODE_RTSP_ERROR:
            return "RTSP Failure.";
        case MIRACAST_PLAYER_REASON_CODE_RTSP_TIMEOUT:
            return "RTSP Timeout.";
        case MIRACAST_PLAYER_REASON_CODE_RTSP_METHOD_NOT_SUPPORTED:
            return "RTSP Method Not Supported.";
        case MIRACAST_PLAYER_REASON_CODE_GST_ERROR:
            return "GStreamer Failure.";
        case MIRACAST_PLAYER_REASON_CODE_INT_FAILURE:
            return "Internal Failure.";
        case MIRACAST_PLAYER_REASON_CODE_NEW_SRC_DEV_CONNECT_REQ:
            return "APP REQ TO STOP FOR NEW CONNECTION.";
        default:
            return "Unimplemented item.";
    }
}

void MiracastServiceEventListener::startRequestHandlerThread()
{
    MiracastError error_code = MIRACAST_FAIL;
    MIRACASTLOG_TRACE("Entering...");

    m_miracastservice_event_handler_thread = new MiracastThread( MIRACASTSERVICE_REQ_HANDLER_THREAD_NAME,
                                                    MIRACASTSERVICE_REQ_HANDLER_THREAD_STACK,
                                                    MIRACASTSERVICE_REQ_HANDLER_MSG_COUNT,
                                                    MIRACASTSERVICE_REQ_HANDLER_MSGQ_SIZE,
                                                    reinterpret_cast<void (*)(void *)>(&MiracastServiceEventHandlerCallback),
                                                    this);
    if (nullptr != m_miracastservice_event_handler_thread)
    {
        error_code = m_miracastservice_event_handler_thread->start();

        if ( MIRACAST_OK != error_code )
        {
            delete m_miracastservice_event_handler_thread;
            m_miracastservice_event_handler_thread = nullptr;
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastServiceEventListener::stopRequestHandlerThread()
{
    MIRACASTSERVICE_REQ_HANDLER_MSGQ_STRUCT payload;
    payload.eventType = SELF_ABORT;
    MIRACASTLOG_TRACE("Entering ...");
    if ( nullptr != m_miracastservice_event_handler_thread )
    {
        send_msgto_event_hdler_thread(payload);
        delete m_miracastservice_event_handler_thread;
        m_miracastservice_event_handler_thread = nullptr;
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastServiceEventListener::send_msgto_event_hdler_thread(MIRACASTSERVICE_REQ_HANDLER_MSGQ_STRUCT payload)
{
    MIRACASTLOG_TRACE("Entering...");
    if (nullptr != m_miracastservice_event_handler_thread)
    {
        m_miracastservice_event_handler_thread->send_message(&payload, MIRACASTSERVICE_REQ_HANDLER_MSGQ_SIZE);
    }
    MIRACASTLOG_TRACE("Exiting...");
}

#if 0
void* MiracastServiceEventListener::requestHandlerThread(void *ctx)
{
    MIRACASTLOG_TRACE("Entering ...");
    MiracastServiceEventListener *_instance = (MiracastServiceEventListener *)ctx;
    MiracastServiceRequestHandlerPayload payload;
    while(!_instance->m_RequestHandlerThreadExit)
    {
        payload.eventType = INVALID_REQUEST;
        payload.src_dev_ip = "";
        payload.src_dev_mac = "";
        payload.src_dev_name = "";
        payload.sink_dev_ip = "";
        payload.reason = "";
        payload.error_code = "";

        {
            // Wait for a message to be added to the queue
            std::unique_lock<std::mutex> lk(_instance->m_RequestHandlerEventMutex);
            _instance->m_RequestHandlerCV.wait(lk, [instance = _instance]{return (instance->m_RequestHandlerThreadRun == true);});
        }

        if (_instance->m_RequestHandlerThreadExit == true)
        {
            MIRACASTLOG_INFO(" threadSendRequest Exiting");
            _instance->m_RequestHandlerThreadRun = false;
            break;
        }

        if (_instance->m_RequestHandlerQueue.empty())
        {
            _instance->m_RequestHandlerThreadRun = false;
            continue;
        }

        payload = _instance->m_RequestHandlerQueue.front();
        _instance->m_RequestHandlerQueue.pop();

        MIRACASTLOG_INFO("Request : Event:0x%x",payload.eventType);
        switch(payload.eventType)
        {
            case ON_CLIENT_CONNECTION_REQUEST:
            {
                std::string client_mac = payload.src_dev_mac,
                            client_name = payload.src_dev_name;
                MIRACASTLOG_INFO("AutoConnecting [%s - %s]",client_name.c_str(),client_mac.c_str());
                MiracastApp::Application::Engine::getAppEngineInstance()->getRDKMiracastServiceInstance()->acceptClientConnection("Accept");
            }
            break;
            case ON_CLIENT_CONNECTION_ERROR:
            {
                std::string client_mac = payload.src_dev_mac,
                            client_name = payload.src_dev_name,
                            error_code = payload.error_code,
                            reason = payload.reason;
                MIRACASTLOG_INFO("ON_CLIENT_CONNECTION_ERROR : client_mac:[%s] client_name:[%s] error_code:[%s] reason:[%s]",client_mac.c_str(),client_name.c_str(),error_code.c_str(),reason.c_str());
            }
            break;
            case ON_LAUNCH_REQUEST:
            {
                std::string source_dev_ip = payload.src_dev_ip,
                            source_dev_mac = payload.src_dev_mac,
                            source_dev_name = payload.src_dev_name,
                            sink_dev_ip = payload.sink_dev_ip;
                MIRACASTLOG_INFO("ON_LAUNCH_REQUEST : source_dev_ip:[%s] source_dev_mac:[%s] source_dev_name:[%s] sink_dev_ip:[%s]",source_dev_ip.c_str(),source_dev_mac.c_str(),source_dev_name.c_str(),sink_dev_ip.c_str());
                MiracastApp::Application::Engine::getAppEngineInstance()->playRequest(source_dev_ip, source_dev_mac, source_dev_name, sink_dev_ip);
            }
            break;
            default:
            {
                MIRACASTLOG_ERROR("Invalid event type [%d]",payload.eventType);
            }
            break;
        }
    }
    _instance->m_request_handler_thread = 0;
    MIRACASTLOG_TRACE("Exiting ...");
    pthread_exit(nullptr);
}
#endif

void MiracastServiceEventListener::onClientConnectionRequest(const string &client_mac, const string &client_name)
{
    MIRACASTSERVICE_REQ_HANDLER_MSGQ_STRUCT payload = {0};

    payload.eventType = ON_CLIENT_CONNECTION_REQUEST;
    strncpy(payload.src_dev_mac, client_mac.c_str(),sizeof(payload.src_dev_mac));
    payload.src_dev_mac[sizeof(payload.src_dev_mac) - 1] = '\0';
    strncpy(payload.src_dev_name, client_name.c_str(),sizeof(payload.src_dev_name));
    payload.src_dev_name[sizeof(payload.src_dev_name) - 1] = '\0';

    MIRACASTLOG_INFO("client_mac:[%s] client_name:[%s]", client_mac.c_str(), client_name.c_str());
    send_msgto_event_hdler_thread(payload);
}

void MiracastServiceEventListener::onClientConnectionError(const std::string &client_mac, const std::string &client_name, const std::string &error_code, const std::string &reason )
{
    MIRACASTSERVICE_REQ_HANDLER_MSGQ_STRUCT payload = {0};

    payload.eventType = ON_CLIENT_CONNECTION_ERROR;
    strncpy(payload.src_dev_mac, client_mac.c_str(),sizeof(payload.src_dev_mac));
    payload.src_dev_mac[sizeof(payload.src_dev_mac) - 1] = '\0';
    strncpy(payload.src_dev_name, client_name.c_str(),sizeof(payload.src_dev_name));
    payload.src_dev_name[sizeof(payload.src_dev_name) - 1] = '\0';
    strncpy(payload.error_code, client_mac.c_str(),sizeof(payload.error_code));
    payload.error_code[sizeof(payload.error_code) - 1] = '\0';
    strncpy(payload.reason, client_name.c_str(),sizeof(payload.reason));
    payload.reason[sizeof(payload.reason) - 1] = '\0';

    MIRACASTLOG_INFO("client_mac:[%s] client_name:[%s] error_code:[%s] reason:[%s]", client_mac.c_str(), client_name.c_str(), error_code.c_str(), reason.c_str());
    send_msgto_event_hdler_thread(payload);
}

void MiracastServiceEventListener::onLaunchRequest(const string &src_dev_ip, const string &src_dev_mac, const string &src_dev_name, const string & sink_dev_ip)
{
    MIRACASTSERVICE_REQ_HANDLER_MSGQ_STRUCT payload = {0};

    payload.eventType = ON_LAUNCH_REQUEST;
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

#if 0
void MiracastServiceEventListener::sendRequestHandlerEvent( const MiracastServiceRequestHandlerPayload &payload )
{
    MIRACASTLOG_TRACE("Entering ...");
    {
        std::lock_guard<std::mutex> lk(m_RequestHandlerEventMutex);
        m_RequestHandlerQueue.push(payload);
        m_RequestHandlerThreadRun = true;
    }
    m_RequestHandlerCV.notify_one();
    MIRACASTLOG_TRACE("Exiting ...");
}
#endif

void MiracastServiceEventListener::MiracastServiceEventHandler_Thread(void *args)
{
    MIRACASTSERVICE_REQ_HANDLER_MSGQ_STRUCT payload = {};
    bool m_running_state = true;
    vector<std::string> strArgs;
    while (m_miracastservice_event_handler_thread && m_running_state)
    {
        strArgs.clear();
        memset(&payload, 0x00, sizeof(payload));
        m_miracastservice_event_handler_thread->receive_message(&payload, sizeof(payload), THREAD_RECV_MSG_INDEFINITE_WAIT);
        MIRACASTLOG_INFO("Request : Event:0x%x",payload.eventType);
        switch(payload.eventType)
        {
            case ON_CLIENT_CONNECTION_REQUEST:
            {
                std::string client_mac = payload.src_dev_mac,
                            client_name = payload.src_dev_name;
                strArgs.push_back(client_name);
                MiracastApp::Application::Engine::getAppEngineInstance()->updateTTSVoiceCommand(CONNECT_REQUEST_TTS_VOICE_COMMAND, strArgs);
                MIRACASTLOG_INFO("AutoConnecting [%s - %s]",client_name.c_str(),client_mac.c_str());
                MiracastApp::Application::Engine::getAppEngineInstance()->getRDKMiracastServiceInstance()->acceptClientConnection("Accept");
            }
            break;
            case ON_CLIENT_CONNECTION_ERROR:
            {
                std::string client_mac = payload.src_dev_mac,
                            client_name = payload.src_dev_name,
                            error_code = payload.error_code,
                            reason = payload.reason;
                MIRACASTLOG_INFO("ON_CLIENT_CONNECTION_ERROR : client_mac:[%s] client_name:[%s] error_code:[%s] reason:[%s]",client_mac.c_str(),client_name.c_str(),error_code.c_str(),reason.c_str());
                strArgs.push_back(client_name);
                strArgs.push_back(error_code);
                MiracastApp::Application::Engine::getAppEngineInstance()->updateTTSVoiceCommand(CONNECTION_ERROR_TTS_VOICE_COMMAND, strArgs);
            }
            break;
            case ON_LAUNCH_REQUEST:
            {
                std::string source_dev_ip = payload.src_dev_ip,
                            source_dev_mac = payload.src_dev_mac,
                            source_dev_name = payload.src_dev_name,
                            sink_dev_ip = payload.sink_dev_ip;
                MIRACASTLOG_INFO("ON_LAUNCH_REQUEST : source_dev_ip:[%s] source_dev_mac:[%s] source_dev_name:[%s] sink_dev_ip:[%s]",source_dev_ip.c_str(),source_dev_mac.c_str(),source_dev_name.c_str(),sink_dev_ip.c_str());
                strArgs.push_back(source_dev_name);
                MiracastApp::Application::Engine::getAppEngineInstance()->updateTTSVoiceCommand(LAUNCH_REQUEST_TTS_VOICE_COMMAND, strArgs);
                MiracastApp::Application::Engine::getAppEngineInstance()->playRequest(source_dev_ip, source_dev_mac, source_dev_name, sink_dev_ip);
            }
            break;
            case SELF_ABORT:
            {
                MIRACASTLOG_INFO("SELF_ABORT ACTION Received");
                m_running_state = false;
                MiracastApp::Application::Engine::getAppEngineInstance()->stopRequest();
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