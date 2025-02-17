#ifndef MIRACAST_APP_MGR_H
#define MIRACAST_APP_MGR_H

#include <vector>
#include <libIARM.h>
#include <libIBus.h>
#include <iarmUtil.h>
#include "AppLaunchDetails.h"
#include "MiracastAppStateMonitor.h"
#include "MiracastRTSPMsg.h"
#include "MiracastGstPlayer.h"

/**
 * Abstract class for MiracastAppMainNotifier Notification.
 */
class MiracastAppMainNotifier
{
public:
    virtual void onMiracastAppEngineStarted(void) = 0;
    virtual void onMiracastAppEngineStopped(void) = 0;
};

//#include "AppStateMonitor.h"
class MiracastAppMgr : public MiracastAppMainNotifier, public MiracastPlayerNotifier
{
public:
    static MiracastAppMgr* getInstance(int argc, char *argv[]);
    static void destroyInstance();
    //inline static MiracastAppMgr* getInstance(){return _mMiracastAppMgrInstance;}
    std::shared_ptr<MiracastAppStateMonitor>  mMiracastAppStateMonitor;
    int startAppAndWaitForFinish();

    virtual void onMiracastAppEngineStarted(void) override;
    virtual void onMiracastAppEngineStopped(void) override;

    virtual void onStateChange(const std::string& client_mac, const std::string& client_name, eMIRA_PLAYER_STATES player_state, eM_PLAYER_REASON_CODE reason_code ) override;

private:
    MiracastAppMgr(int argc, char *argv[]);
    virtual ~MiracastAppMgr();
    MiracastAppMgr &operator=(const MiracastAppMgr &) = delete;
    MiracastAppMgr(const MiracastAppMgr &) = delete;
    static MiracastAppMgr *_mMiracastAppMgrInstance;
    static MiracastRTSPMsg *_mMiracastRTSPInstance;
    static MiracastGstPlayer *_mMiracastGstPlayer;
    bool _mIsLaunchAppOnStartup;
    LaunchDetails::AppLaunchDetails* _mStartupLaunchDetails;
    VIDEO_RECT_STRUCT m_video_sink_rect;
    bool islaunchAppOnStartup();
    std::vector<std::string> constructArguments(int argc, char *argv[]);
    void initializeMonitors(bool isSuspendOnStart);
    bool Initialize(void);
    void initIARM();
    void deinitIARM();
};

#endif //MIRACAST_APP_MGR_H