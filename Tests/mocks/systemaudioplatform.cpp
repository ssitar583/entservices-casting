#include "systemaudioplatform.h"


void systemAudioInitialize() {}
    void systemAudioDeinitialize() {}
    
    //change primary audio volume
    void systemAudioChangePrimaryVol(MixGain gain,int volume) {}
   
    //SAP specific
    void systemAudioSetDetectTime( int detectTimeMs) {}
    void systemAudioSetHoldTime( int holdTimeMs) {}
    void systemAudioSetThreshold(double thresHold) {}
    void systemAudioSetVolume(GstElement *audioVolume,AudioType audioType,PlayMode playMode,int thisVol) {}

    //Pipeline creation
    bool systemAudioGeneratePipeline(GstElement  *pipeline, GstElement  *source,GstElement *capsfilter,GstElement **audioSink,GstElement **audioVolume,AudioType type,PlayMode mode,SourceType sourceType,bool smartVolumeEnable) { return false; }