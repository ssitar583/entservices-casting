#include "MiracastAppLogging.hpp"
#include "MiracastAppStateMonitor.h"
#include "MiracastAppInterface.hpp"

#include "SkyAsProxy.h"

#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <cassert>

static void state_changed(void* data, struct skyq_shell*/*shell*/, uint32_t state)
{
    MiracastAppStateMonitor* eventLoop = static_cast<MiracastAppStateMonitor*>(data);
    eventLoop->handleStateChangeEvent(state);
}

static void close(void* data, struct skyq_shell*/*shell*/)
{
    MiracastAppStateMonitor* eventLoop = static_cast<MiracastAppStateMonitor*>(data);
    eventLoop->handleCloseEvent();
}

static void registryHandleGlobal(void* data,
    struct wl_registry* registry, uint32_t id,
    const char* interface, uint32_t /*version*/)
{
    MiracastAppStateMonitor* eventLoop = static_cast<MiracastAppStateMonitor*>(data);
    eventLoop->handleInterfaceRegistry(registry, id, interface);
}

static void registryHandleGlobalRemove(void*/*data*/,
    struct wl_registry*/*registry*/,
    uint32_t /*name*/)
{
}

static const struct skyq_shell_listener skyListener =
{
      state_changed,
      close
};

static const struct wl_registry_listener registryListener =
{
    registryHandleGlobal,
    registryHandleGlobalRemove
};

void MiracastAppStateMonitor::handleInterfaceRegistry(wl_registry* registry, uint32_t id, const char* interface)
{
    size_t len = strlen(interface);
    MIRACASTLOG_TRACE("Entering ...");
    if (!strncmp(interface, "skyq_shell", len))
    {
        mWaylandComponents.mShell = (struct skyq_shell*)wl_registry_bind(registry, id, &skyq_shell_interface, 3);
        skyq_shell_add_listener(mWaylandComponents.mShell, &skyListener, this);
        MIRACASTLOG_VERBOSE("invoked skyq_shell_add_listener()");
    }

    if (!strncmp(interface, "wl_compositor", len))
    {
        mWaylandComponents.mCompositor = (struct wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
        MIRACASTLOG_VERBOSE("Connected to wayland compositor: %p", mWaylandComponents.mCompositor);
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

MiracastAppStateMonitor::MiracastAppStateMonitor(bool isLaunchAppOnStart /*, const LaunchDetails::AppLaunchDetails:: &launchDetails*/)
{
    mMode = isLaunchAppOnStart ? Foreground : Background;
    _mIsAppLaunchOnStartup = isLaunchAppOnStart;
    mIsLoopRunning = false;
    mSkyAsProxy = std::make_shared<SkyAsProxy>();
}

pthread_t   setAppVisibilityAtLaunch_thread_;

void* setAppVisibilityAtLaunchThread(void * ctx)
{
	MiracastAppStateMonitor *_miracastAppStateMonitor = (MiracastAppStateMonitor *)ctx;
    bool isAppLaunchOnStart = _miracastAppStateMonitor->getIsAppLaunchOnStartup();
    MIRACASTLOG_VERBOSE("isAppLaunchOnStart:%s\n", isAppLaunchOnStart?"Foreground":"Background");
    if(isAppLaunchOnStart){
        _miracastAppStateMonitor->resumeApp();
    }
    else {
        _miracastAppStateMonitor->suspendApp();
    }
    pthread_exit(NULL);
}

MiracastAppStateMonitor::~MiracastAppStateMonitor()
{
    stop();
}

bool MiracastAppStateMonitor::start()
{
    MIRACASTLOG_TRACE("Entering ...");
    if(!mIsLoopRunning){
        mIsLoopRunning = true;
        wl_display* display = wl_display_connect(nullptr);
        if(!display){
            MIRACASTLOG_ERROR("Unable to connect to Wayland Display.\n");
            MIRACASTLOG_TRACE("Exiting ...");
            return false;
        }
        wl_registry* registry = wl_display_get_registry(display);
        if(!registry){
            MIRACASTLOG_ERROR("Unable to get Wayland Display Registry.\n");
            MIRACASTLOG_TRACE("Exiting ...");
            return false;           
        }
        mWaylandComponents.mDisplay = display;
        mWaylandComponents.mRegistry = registry;
        wl_registry_add_listener(mWaylandComponents.mRegistry, &registryListener,this);
        
        MIRACASTLOG_VERBOSE("Waiting for registry roundtrip for display [%p] ...", mWaylandComponents.mDisplay);
        assert(wl_display_roundtrip(mWaylandComponents.mDisplay) != -1);
		if(setAppVisibilityAtLaunch_thread_)
            pthread_cancel(setAppVisibilityAtLaunch_thread_);
        pthread_create(&setAppVisibilityAtLaunch_thread_, NULL, &setAppVisibilityAtLaunchThread, (void *)this);
        startWorkerThread();
    }
    MIRACASTLOG_TRACE("Exiting ...");
    return true;
}

void MiracastAppStateMonitor::stop()
{
    MIRACASTLOG_TRACE("Entering ...");

    if(mIsLoopRunning){
        MIRACASTLOG_TRACE("Stopping ...");
        mIsLoopRunning = false;
        waitForWorkerThread();
        if (mWaylandComponents.mRegistry)
        {
            wl_registry_destroy(mWaylandComponents.mRegistry);
            mWaylandComponents.mRegistry = nullptr;
        }

        if (mWaylandComponents.mShell)
        {
            skyq_shell_destroy(mWaylandComponents.mShell);
            mWaylandComponents.mShell = nullptr;
        }

        if (mWaylandComponents.mCompositor)
        {
            wl_compositor_destroy(mWaylandComponents.mCompositor);
            mWaylandComponents.mCompositor = nullptr;
        }

        if (mWaylandComponents.mDisplay)
        {
            wl_display_disconnect(mWaylandComponents.mDisplay);
            mWaylandComponents.mDisplay = nullptr;
        }
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastAppStateMonitor::run()
{
    const int pollTimeout = -1;
    MIRACASTLOG_TRACE("Entering ...");
    while (mIsLoopRunning){
        if (wl_display_flush(mWaylandComponents.mDisplay) == -1)
        {
            MIRACASTLOG_ERROR("Failed to flush display, errno %s", strerror(errno));
            break;
        }
        const unsigned FDS_SIZE = 1;
        pollfd fds[FDS_SIZE];
        fds[0].fd = wl_display_get_fd(mWaylandComponents.mDisplay);
        fds[0].events = POLLIN;
        fds[0].revents = 0;
        int ret = TEMP_FAILURE_RETRY(poll(fds, FDS_SIZE, pollTimeout));
        if (ret < 0)
        {
            MIRACASTLOG_ERROR("Unexpected poll failure, quiting event loop (%d - %s)", errno, strerror(errno));
            break;
        }
        if (fds[0].revents & POLLIN) //wayland fd
        {
            if (wl_display_dispatch(mWaylandComponents.mDisplay) < 0)
            {
                MIRACASTLOG_ERROR("Failed to read and dispatch wayland events (%d)", errno);
                break;
            }
        }
        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            MIRACASTLOG_ERROR("Wayland fd poll exits with error %u", fds[0].revents);
            break;
        }
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastAppStateMonitor::handleCloseEvent()
{
    MIRACASTLOG_TRACE("Entering ...");
    appInterface_stopMiracastApp();
    mMode = MiracastAppStateMonitor::Exiting;
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastAppStateMonitor::handleStateChangeEvent(uint32_t state)
{
    skyq_shell_state newState = static_cast<skyq_shell_state>(state); 
    MIRACASTLOG_INFO("Entered. NewState:%u, Current State:%u\n", newState, mMode);
    if (mMode == MiracastAppStateMonitor::Foreground && newState == SKYQ_SHELL_STATE_SUSPENDED){
         MIRACASTLOG_INFO("NewState:Suspend, Current State:Foreground\n");
         suspendApp();
    }
    else if (mMode == MiracastAppStateMonitor::Background && newState == SKYQ_SHELL_STATE_ACTIVE) {
        MIRACASTLOG_INFO("NewState:Active, Current State:Background\n");
        resumeApp();
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

bool MiracastAppStateMonitor::resumeApp()
{
    MIRACASTLOG_TRACE("Entering ...");
    if (!mSkyAsProxy->stopAllPlayers())
    {
        MIRACASTLOG_ERROR("Failed to stop players upon MiracastApp App foreground");
    }
    appInterface_setAppVisibility(true);
    mMode = MiracastAppStateMonitor::Foreground;
    MIRACASTLOG_TRACE("Exiting ...");
    return true;
}

bool MiracastAppStateMonitor::suspendApp()
{
    MIRACASTLOG_TRACE("Entering ...");
    appInterface_setAppVisibility(false);
    mMode = MiracastAppStateMonitor::Background;
    MIRACASTLOG_TRACE("Exiting ...");
    return true;
}