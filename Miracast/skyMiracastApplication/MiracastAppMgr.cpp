#include <cassert>
#include "MiracastAppInterface.hpp"
#include "MiracastAppLogging.hpp"
#include "MiracastAppMgr.h"
#include "AppLaunchDetails.h"

MiracastAppMgr *MiracastAppMgr::_mMiracastAppMgrInstance = nullptr;

MiracastRTSPMsg *MiracastAppMgr::_mMiracastRTSPInstance = nullptr;
MiracastGstPlayer *MiracastAppMgr::_mMiracastGstPlayer = nullptr;

MiracastAppMgr *MiracastAppMgr::getInstance(int argc, char *argv[])
{
    MIRACASTLOG_TRACE("Entering...");
    if (_mMiracastAppMgrInstance == nullptr)
    {
        _mMiracastAppMgrInstance = new MiracastAppMgr(argc,argv);

        if ( nullptr != _mMiracastAppMgrInstance )
        {
            bool status = _mMiracastAppMgrInstance->Initialize();

            if (false == status)
            {
                delete _mMiracastAppMgrInstance;
                _mMiracastAppMgrInstance = nullptr;
                MIRACASTLOG_ERROR("Failed to Initialize MiracastAppMgr");
            }
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return _mMiracastAppMgrInstance;
}

void MiracastAppMgr::destroyInstance()
{
    MIRACASTLOG_TRACE("Entering...");
    if (_mMiracastAppMgrInstance != nullptr)
    {
        delete _mMiracastAppMgrInstance;
        _mMiracastAppMgrInstance = nullptr;
    }
    MIRACASTLOG_TRACE("Exiting...");
}

bool MiracastAppMgr::Initialize(void)
{
    bool returnValue = false;
    MIRACASTLOG_TRACE("Entering...");
    _mStartupLaunchDetails = LaunchDetails::parseLaunchDetailsFromEnv();

    if (nullptr == _mStartupLaunchDetails )
    {
        MIRACASTLOG_ERROR("Failed to LaunchDetails::parseLaunchDetailsFromEnv()");
    }
    else
    {
        MiracastError ret_code = MIRACAST_OK;
        _mIsLaunchAppOnStartup = islaunchAppOnStartup();
        _mMiracastRTSPInstance = MiracastRTSPMsg::getInstance(ret_code, this);
        if (nullptr != _mMiracastRTSPInstance)
        {
            _mMiracastGstPlayer = MiracastGstPlayer::getInstance();
            if (nullptr == _mMiracastGstPlayer)
            {
                MiracastRTSPMsg::destroyInstance();
                _mMiracastRTSPInstance = nullptr;
                LaunchDetails::AppLaunchDetails::destroyInstance();
                _mStartupLaunchDetails = nullptr;
                MIRACASTLOG_ERROR("Failed to MiracastGstPlayer::getInstance()");
            }
            else
            {
                MIRACASTLOG_INFO("Initialize() success");
                returnValue = true;
            }
        }
        else
        {
            MIRACASTLOG_ERROR("Failed to MiracastRTSPMsg::getInstance()");
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return returnValue;
}

std::vector<std::string> MiracastAppMgr::constructArguments(int argc, char *argv[]){
    std::vector<std::string> arguments;
    int _arg = 0;
    while(_arg< argc){
        arguments.push_back(argv[_arg]);
        _arg++;
    }
    return arguments;
}
bool MiracastAppMgr::islaunchAppOnStartup()
{
    MIRACASTLOG_TRACE("Entering ...");
    bool launchAppOnStartup = false;
    if (true == _mStartupLaunchDetails->mlaunchAppInForeground ){
        launchAppOnStartup = true;
    }
    MIRACASTLOG_TRACE("Exiting ...");
    return launchAppOnStartup;
}

MiracastAppMgr::MiracastAppMgr(int argc, char *argv[])

{
    assert(!_mMiracastAppMgrInstance);
    _mMiracastAppMgrInstance = this;
}

MiracastAppMgr::~MiracastAppMgr(){
    assert(_mMiracastAppMgrInstance == this);
    _mMiracastAppMgrInstance = nullptr;
}

void MiracastAppMgr::initializeMonitors(bool isLaunchAppOnStart)
{
    MIRACASTLOG_TRACE("Entering ...");
    mMiracastAppStateMonitor = std::make_shared<MiracastAppStateMonitor>(isLaunchAppOnStart);
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastAppMgr::initIARM()
{
    MIRACASTLOG_INFO("initializing IARM bus...");

    if (IARM_Bus_Init("MiracastApp") != IARM_RESULT_SUCCESS)
    {
        MIRACASTLOG_ERROR("Connection_Error:error initializing IARM Bus!");
    }

    if (IARM_Bus_Connect() != IARM_RESULT_SUCCESS)
    {
        MIRACASTLOG_ERROR("Connection_Error:error connecting to IARM Bus!");
    }
}

void MiracastAppMgr::deinitIARM()
{
    MIRACASTLOG_INFO("disconnecting from IARM Bus...");
    IARM_Bus_Disconnect();
}

int MiracastAppMgr::startAppAndWaitForFinish()
{
    int status=0;
    std::vector<const char*> miracastappArguments;
    MIRACASTLOG_TRACE("Entering ...");
    initializeMonitors(_mIsLaunchAppOnStartup);
    mMiracastAppStateMonitor->start();
#if 0
    miracastappArguments.reserve(_mStartupLaunchDetails.mArguments.size());
    for(unsigned int i=0;i<_mStartupLaunchDetails.mArguments.size(); i++){
        MIRACASTLOG_INFO("argv%d:%s\n",i, _mStartupLaunchDetails.mArguments[i].c_str());
        miracastappArguments.push_back(_mStartupLaunchDetails.mArguments[i].c_str());
    }
#endif
    MIRACASTLOG_VERBOSE("Copied argc:%d number of params\n",miracastappArguments.size());
    status = appInterface_startMiracastApp(miracastappArguments.size(),const_cast<char **>(miracastappArguments.data()),this);
    mMiracastAppStateMonitor->stop();
    MIRACASTLOG_TRACE("Exiting ...");
    return status;
}

void MiracastAppMgr::onStateChange(const std::string& client_mac, const std::string& client_name, eMIRA_PLAYER_STATES player_state, eM_PLAYER_REASON_CODE reason_code)
{
    MIRACASTLOG_TRACE("Entering ...");
    //MIRACASTLOG_INFO("client_name[%s]client_mac[%s]player_state[%x]reason_code[%x]",client_name.c_str(),client_mac.c_str(),player_state,reason_code);
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastAppMgr::onMiracastAppEngineStarted(void)
{
    RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};
    std::string srcDevIP,
                srcDevMAC,
                srcDevName,
                sinkDevIP;
    LaunchDetails::VideoRectangleInfo rect = {0};

    MIRACASTLOG_TRACE("Entering ...");

    if (_mStartupLaunchDetails && _mMiracastRTSPInstance )
    {
        _mStartupLaunchDetails->getPlayRequestDetails(srcDevIP,srcDevMAC,srcDevName,sinkDevIP,&rect);

        strncpy( rtsp_hldr_msgq_data.source_dev_ip, srcDevIP.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_ip));
        rtsp_hldr_msgq_data.source_dev_ip[sizeof(rtsp_hldr_msgq_data.source_dev_ip) - 1] = '\0';
        strncpy( rtsp_hldr_msgq_data.source_dev_mac, srcDevMAC.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_mac));
        rtsp_hldr_msgq_data.source_dev_mac[sizeof(rtsp_hldr_msgq_data.source_dev_mac) - 1] = '\0';
        strncpy( rtsp_hldr_msgq_data.source_dev_name, srcDevName.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_name));
        rtsp_hldr_msgq_data.source_dev_name[sizeof(rtsp_hldr_msgq_data.source_dev_name) - 1] = '\0';
        strncpy( rtsp_hldr_msgq_data.sink_dev_ip, sinkDevIP.c_str() , sizeof(rtsp_hldr_msgq_data.sink_dev_ip));
        rtsp_hldr_msgq_data.sink_dev_ip[sizeof(rtsp_hldr_msgq_data.sink_dev_ip) - 1] = '\0';

        rtsp_hldr_msgq_data.videorect.startX = rect.startX;
        rtsp_hldr_msgq_data.videorect.startY = rect.startY;
        rtsp_hldr_msgq_data.videorect.width = rect.width;
        rtsp_hldr_msgq_data.videorect.height = rect.height;

        rtsp_hldr_msgq_data.state = RTSP_START_RECEIVE_MSGS;

        MIRACASTLOG_VERBOSE("RTSP_START_RECEIVE_MSGS initiated");
        _mMiracastRTSPInstance->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
    }
    else
    {
        MIRACASTLOG_ERROR("Null Instance [%x] [%x]",_mStartupLaunchDetails,_mMiracastRTSPInstance);
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastAppMgr::onMiracastAppEngineStopped(void)
{
    RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};

    if ( _mMiracastRTSPInstance )
    {
        rtsp_hldr_msgq_data.stop_reason_code = MIRACAST_PLAYER_APP_REQ_TO_STOP_ON_EXIT;
        rtsp_hldr_msgq_data.state = RTSP_TEARDOWN_FROM_SINK2SRC;
        _mMiracastRTSPInstance->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
    }
}