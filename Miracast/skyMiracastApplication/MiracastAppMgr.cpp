#include <cassert>
#include "MiracastAppInterface.hpp"
#include "MiracastAppLogging.hpp"
#include "MiracastAppMgr.h"
#include "AppLaunchDetails.h"

MiracastAppMgr *MiracastAppMgr::_mMiracastAppMgrInstance = nullptr;

MiracastAppMgr *MiracastAppMgr::getInstance(void)
{
    MIRACASTLOG_TRACE("Entering...");
    if (_mMiracastAppMgrInstance == nullptr)
    {
        _mMiracastAppMgrInstance = new MiracastAppMgr();

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

    MIRACASTLOG_TRACE("Entering ...");
    _mStartupLaunchDetails = LaunchDetails::parseLaunchDetailsFromEnv();

    if (nullptr == _mStartupLaunchDetails )
    {
        MIRACASTLOG_ERROR("Failed to LaunchDetails::parseLaunchDetailsFromEnv()");
    }
    else
    {
        _mIsLaunchAppOnStartup = islaunchAppOnStartup();
        m_video_rect = _mStartupLaunchDetails->mVideoRect;
        returnValue = true;
        MIRACASTLOG_INFO("Initialize() success");
    }

    if (false == returnValue)
    {
        DeInitialize();
    }
    MIRACASTLOG_TRACE("Exiting...");
    return returnValue;
}

void MiracastAppMgr::DeInitialize(void)
{
    MIRACASTLOG_TRACE("Entering ...");
    if (_mStartupLaunchDetails)
    {
        LaunchDetails::AppLaunchDetails::destroyInstance();
        _mStartupLaunchDetails = nullptr;
    }
    MIRACASTLOG_TRACE("Exiting ...");
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
    if (_mStartupLaunchDetails && _mStartupLaunchDetails->mlaunchAppInForeground )
    {
        launchAppOnStartup = true;
    }
    MIRACASTLOG_TRACE("Exiting ...");
    return launchAppOnStartup;
}

MiracastAppMgr::MiracastAppMgr(void)

{
    MIRACASTLOG_TRACE("Entering ...");

    MIRACASTLOG_VERBOSE("MiracastAppMgr %p ",_mMiracastAppMgrInstance);
    assert(!_mMiracastAppMgrInstance);
    _mMiracastAppMgrInstance = this;
    MIRACASTLOG_TRACE("Exiting ...");
}

MiracastAppMgr::~MiracastAppMgr()
{
    MIRACASTLOG_TRACE("Entering ...");
    MIRACASTLOG_VERBOSE("MiracastAppMgr %p - %p",_mMiracastAppMgrInstance, this);
    assert(_mMiracastAppMgrInstance == this);
    _mMiracastAppMgrInstance = nullptr;
    MIRACASTLOG_TRACE("Exiting ...");
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
    status = appInterface_startMiracastApp(miracastappArguments.size(),const_cast<char **>(miracastappArguments.data()),this);
    mMiracastAppStateMonitor->stop();
    MIRACASTLOG_TRACE("Exiting ...");
    return status;
}

void MiracastAppMgr::getVideoResolution(VideoRectangleInfo &rect)
{
    MIRACASTLOG_TRACE("Entering ...");
    rect.startX = m_video_rect.startX;
    rect.startY = m_video_rect.startY;
    rect.width = m_video_rect.width;
    rect.height = m_video_rect.height;
    MIRACASTLOG_TRACE("Exiting ...");
}