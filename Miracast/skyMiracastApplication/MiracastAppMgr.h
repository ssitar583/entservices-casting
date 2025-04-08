#ifndef MIRACAST_APP_MGR_H
#define MIRACAST_APP_MGR_H

#include <vector>
#include <libIARM.h>
#include <libIBus.h>
#include <iarmUtil.h>
#include "AppLaunchDetails.h"
#include "MiracastAppStateMonitor.h"
#include "helpers.hpp"

class MiracastAppMgr
{
public:
    static MiracastAppMgr* getInstance();
    static void destroyInstance();
    std::shared_ptr<MiracastAppStateMonitor>  mMiracastAppStateMonitor;
    int startAppAndWaitForFinish();
    void getVideoResolution(VideoRectangleInfo &rect);

private:
    MiracastAppMgr();
    virtual ~MiracastAppMgr();
    MiracastAppMgr &operator=(const MiracastAppMgr &) = delete;
    MiracastAppMgr(const MiracastAppMgr &) = delete;
    static MiracastAppMgr *_mMiracastAppMgrInstance;
    bool _mIsLaunchAppOnStartup;
    LaunchDetails::AppLaunchDetails* _mStartupLaunchDetails;
    VideoRectangleInfo m_video_rect;
    bool islaunchAppOnStartup();
    std::vector<std::string> constructArguments(int argc, char *argv[]);
    void initializeMonitors(bool isSuspendOnStart);
    bool Initialize(void);
    void DeInitialize(void);
    void initIARM();
    void deinitIARM();
};

#endif //MIRACAST_APP_MGR_H