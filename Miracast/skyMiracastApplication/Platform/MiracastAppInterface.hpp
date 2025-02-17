#ifndef MiracastAppInterface_hpp
#define MiracastAppInterface_hpp

#ifndef EXIT_APP_REQUESTED
#define EXIT_APP_REQUESTED 79
#endif

extern int app_exit_err_code;
enum miracastapp_engine_state {
    MIRACASTAPP_STOPPED,
    MIRACASTAPP_STARTED,
    MIRACASTAPP_RUNNING,
    MIRACASTAPP_BACKGROUNDING,
    MIRACASTAPP_BACKGROUNDED,
    MIRACASTAPP_RESUMING,
};

int appInterface_startMiracastApp(int argc, const char **argv,void*);
void appInterface_stopMiracastApp();
void appInterface_destroyAppInstance();
int appInterface_getAppState();
void appInterface_setAppVisibility(bool visible);
bool isAppExitRequested();


#endif /*MiracastAppInterface_hpp*/