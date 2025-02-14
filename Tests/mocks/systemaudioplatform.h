#ifndef SYSTEMAUDIO_PLATFORM
#define SYSTEMAUDIO_PLATFORM
#include <stdint.h>
#include <string>
#include <gst/gst.h>
#include <gst/audio/audio.h>

   enum AudioType
   {
       AudioType_None,
       PCM,
       MP3,
       WAV
   };

   enum SourceType
   {
       SourceType_None,
       DATA,
       HTTPSRC,
       FILESRC,
       WEBSOCKET
   };

   enum PlayMode
   {
       PlayMode_None,
       SYSTEM,
       APP
   };
 
   enum MixGain {
        MIXGAIN_PRIM,
        MIXGAIN_SYS, //direct-mode=false, aml calls it sys mode
        MIXGAIN_TTS //tts=mode=true, AML calls it app mode
   };
	
    void systemAudioInitialize();
    void systemAudioDeinitialize();
    
    //change primary audio volume
    void systemAudioChangePrimaryVol(MixGain gain,int volume);
   
    //SAP specific
    void systemAudioSetDetectTime( int detectTimeMs); 
    void systemAudioSetHoldTime( int holdTimeMs);
    void systemAudioSetThreshold(double thresHold);
    void systemAudioSetVolume(GstElement *audioVolume,AudioType audioType,PlayMode playMode,int thisVol);

    //Pipeline creation
    bool systemAudioGeneratePipeline(GstElement  *pipeline, GstElement  *source,GstElement *capsfilter,GstElement **audioSink,GstElement **audioVolume,AudioType type,PlayMode mode,SourceType sourceType,bool smartVolumeEnable);
#endif