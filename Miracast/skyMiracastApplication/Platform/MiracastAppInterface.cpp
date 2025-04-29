#include "MiracastAppCommon.hpp"
#include "MiracastGraphicsDelegate.hpp"
#include "MiracastAppInterface.hpp"
#include "MiracastAppMainStub.hpp"
#include "MiracastAppLogging.hpp"

int app_exit_err_code = 0;
int appInterface_startMiracastApp(int argc, const char **argv,void* userdata)
{
    int err = EXIT_SUCCESS;
	app_exit_err_code = EXIT_SUCCESS;
    MIRACASTLOG_INFO("Received call to MiracastApp Application");
    err = MiracastAppMain::getMiracastAppMainInstance()->launchMiracastAppMain(argc, argv, userdata);
	if(err == EXIT_SUCCESS){
		if(app_exit_err_code == EXIT_APP_REQUESTED){
			err = EXIT_APP_REQUESTED;
		}
	}
	MIRACASTLOG_VERBOSE("MiracastApp Application Exited with code:%d\n",err);
    return err;
}
void appInterface_stopMiracastApp()
{
	app_exit_err_code = EXIT_APP_REQUESTED;
        MIRACASTLOG_INFO("Received call Stop to MiracastApp Application\n");
	if(MiracastAppMain::isMiracastAppMainObjValid()){
		if(MiracastAppMain::getMiracastAppMainInstance()->getMiracastAppstate() != MIRACASTAPP_STOPPED){
			MiracastAppMain::getMiracastAppMainInstance()->handleStopMiracastApp();
		} else {
			MIRACASTLOG_INFO("Received stopMiracastApp call when Application in stopped state\n");
		}
	}
	else{
		MIRACASTLOG_INFO("Received stopMiracastApp call when Application in destroyed state\n");
	}
}
int appInterface_getAppState()
{
	return MiracastAppMain::getMiracastAppMainInstance()->getMiracastAppstate();
}
void appInterface_destroyAppInstance()
{
    MIRACASTLOG_VERBOSE("MiracastApp App stopped and freed resources. Ok to call destructor.\n");
	MiracastAppMain::getMiracastAppMainInstance()->destroyMiracastAppMainInstance();
}

pthread_t   playback_thread_;
static bool hasConnection = false;

void* playbackThread(void * ctx)
{
	bool visible = (void *)ctx;
	
	int miracastapp_state;
	if (!hasConnection && !visible)
	{
		miracastapp_state = appInterface_getAppState();
		if (miracastapp_state == MIRACASTAPP_STARTED || miracastapp_state == MIRACASTAPP_STOPPED)
		{
			MIRACASTLOG_INFO(" MiracastApp plugin waiting for engine state RUNNING\n");
			while (miracastapp_state != MIRACASTAPP_RUNNING){
				usleep(1000);
				miracastapp_state = appInterface_getAppState();
			}
		}
		sleep(10);
		hasConnection = true;
	}
	if (playback_thread_ == pthread_self())
		MiracastAppMain::getMiracastAppMainInstance()->setMiracastAppVisibility(visible);
	pthread_exit(NULL);
	return 0;
}
void appInterface_setAppVisibility(bool visible)
{
    MIRACASTLOG_INFO("MiracastApp visibility request = %d\n",visible);
    if(visible) {
         MIRACASTLOG_INFO("MiracastApp visibility request = %d & cancel running connection thread \n",visible);
         hasConnection = true; //we assume app is in background now  
		 if(playback_thread_)
         	pthread_cancel(playback_thread_);
         MiracastAppMain::getMiracastAppMainInstance()->setMiracastAppVisibility(visible);
     } 
     else
         pthread_create(&playback_thread_, NULL, &playbackThread, (void *)visible);
}

bool isAppExitRequested(){
	return (app_exit_err_code == EXIT_APP_REQUESTED)?true:false;
}