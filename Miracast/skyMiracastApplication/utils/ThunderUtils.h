/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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

#ifndef __THUNDERUTILS_H__
#define __THUNDERUTILS_H__

#include "Module.h"
#include "PluginDefines.h"
//#include "RDKTextToSpeech.h"

#include <WPEFramework/core/core.h>
#include <WPEFramework/plugins/Service.h>

#include <map>

typedef void (*callback)(const JsonObject& parameters);
typedef std::map<std::string,callback> Event;
struct PluginDetails
{
    WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *obj = nullptr;
    Event events;
};


class ThunderUtils
{
    public:
    enum ttsNotificationType
    {
        EVENT_ONSPEECHINTERRUPTED,
        EVENT_ONSPEECHCOMPLETE,
        EVENT_ONSPEECHSTART,
        EVENT_ONSPEECHERROR,
        EVENT_ONTTSSTATECHANGE
    };
    ~ThunderUtils();
    static ThunderUtils *getinstance();
    //thunder call
    string getAudioFormat(std::string AudioFormats);
    string getColorFormat(std::string ColorFormats);
	  bool isttsenabled();
	  void setttsRate(double& wpm);
	  void cancelTts(int& speech_id);
	  int SpeakTts(string& text,int speak_id);
	  bool isspeaking(int& speech_id);
      std::string getConnectedNWInterface();
      std::string getModelName();
      std::string getFirmwareVersion();
      std::string getSystemLocale();
      void getResolution(char* resolution, int sLen);
      void getActiveAudioPorts(string &activeAudioPort);
      bool getEnableAudioPort(string &enabledAudioPort); 
      std::string getDeviceLocation();
      std::string getIPAddress();
    void updateAppIPtoDaemon();
    void thunderGet(const std::string &callsign, const std::string &method, auto &result );
    void thunderSet(const std::string &callsign, const std::string &method, const JsonObject &param);
    void thunderInvoke(const std::string &callsign, const std::string &method, const JsonObject &param, JsonObject &result);
    void thunderInvoke(const std::string &callsign, const std::string &method, JsonObject &result);

private:
    ThunderUtils();
    static ThunderUtils *_instance;
    WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *controller = nullptr;
    typedef std::map<std::string, PluginDetails> Plugins;
    Plugins plugins;
    static void eventHandler_onspeechinterrupted(const JsonObject& parameters);
    static void eventHandler_onspeechcomplete(const JsonObject& parameters);
    static void eventHandler_onspeechstart(const JsonObject& parameters);
    static void eventHandler_onttsstatechanged(const JsonObject& parameters);
    static void eventHandler_onerror(const JsonObject& parameters);
    static void eventHandler_onConnectionStatusChanged(const JsonObject& parameters);
    static void eventHandler_onDefaultInterfaceChanged(const JsonObject& parameters);
    void registerPlugins()
    {
        PluginDetails pluginDetails;
        pluginDetails.events = {
            {"onspeechinterrupted", &ThunderUtils::eventHandler_onspeechinterrupted},
            {"onspeechcomplete", &ThunderUtils::eventHandler_onspeechcomplete},
            {"onspeechstart", &ThunderUtils::eventHandler_onspeechstart},
            {"onttsstatechanged", &ThunderUtils::eventHandler_onttsstatechanged},
            {"onnetworkerror", &ThunderUtils::eventHandler_onerror},
            {"onplaybackerror", &ThunderUtils::eventHandler_onerror}
        };
        plugins[TEXTTOSPEECH_CALLSIGN] = pluginDetails;
        pluginDetails.events = {
            {"onConnectionStatusChanged", &ThunderUtils::eventHandler_onConnectionStatusChanged},
            {"onDefaultInterfaceChanged", &ThunderUtils::eventHandler_onDefaultInterfaceChanged}
        };
        plugins[NETWORK_CALLSIGN] = pluginDetails;
        pluginDetails.events = {};
        plugins[DISPLAYSETTINGS_CALLSIGN] = pluginDetails; 
        plugins[DEVICE_INFO_CALLSIGN] = pluginDetails;
        plugins[PLAYER_INFO_CALLSIGN] = pluginDetails;
        plugins[USER_PREFERENCES_CALLSIGN] = pluginDetails;
        plugins[LOCATION_CALLSIGN] = pluginDetails;
        plugins[PERSISTENTSTORE_CALLSIGN] = pluginDetails;
#ifdef SKY_BUILD // (SKY_BUILD_ENV != 1)
        plugins[HOMEKITTV_CALLSIGN] = pluginDetails;
#endif
    }
};
#endif