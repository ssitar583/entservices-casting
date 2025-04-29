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
#include "ThunderUtils.h"
#include "RDKPluginCore.h"
#include "MiracastCommon.h"
#include "MiracastAppLogging.hpp"
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace WPEFramework;

#define SECURITY_TOKEN_LEN_MAX 1024
#define THUNDER_RPC_TIMEOUT 5000
#define MIRACASTAPP_CALLSIGN "MiracastApp"
std::string client_id_;
std::mutex thunderUtilsMutex_;
ThunderUtils *ThunderUtils::_instance(nullptr);

static std::map<std::string, std::string> westerosEnvArgs = 
{
    {"WAYLAND_DISPLAY", ""},
    {"XDG_RUNTIME_DIR", "/tmp"},
    {"LD_PRELOAD", "libwesteros_gl.so.0.0.0"},
    {"WESTEROS_GL_GRAPHICS_MAX_SIZE", "1920x1080"},
    {"WESTEROS_GL_MODE", "3840x2160x60"},
    {"WESTEROS_GL_USE_REFRESH_LOCK", "1"},
    {"WESTEROS_GL_USE_AMLOGIC_AVSYNC", "1"},
    {"WESTEROS_SINK_AMLOGIC_USE_DMABUF", "1"},
    {"WESTEROS_SINK_USE_FREERUN", "1"},
    {"WESTEROS_SINK_USE_ESSRMGR", "1"}
};

ThunderUtils::ThunderUtils()
{
#ifndef SKY_BUILD // (SKY_BUILD_ENV != 1)
    for (const auto& arg : envArgs) {
        Core::SystemInfo::SetEnvironment(_T(arg.first.c_str()), _T(arg.second.c_str()));
    }
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:9998")));
    MIRACASTLOG_INFO(" Thunder plugins setEnvironment\n");
#else 
    MIRACASTLOG_INFO(" Thunder plugins not setEnvironment\n");
#endif
    /* Create Thunder Security token */
    unsigned char buffer[SECURITY_TOKEN_LEN_MAX] = {0};
    int ret = GetSecurityToken(SECURITY_TOKEN_LEN_MAX, buffer);
    string sToken;
    string query;
    if (ret > 0)
    {
        sToken = (char *)buffer;
        query = "token=" + sToken;
    }
    controller = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>("", "", false, query);
    registerPlugins();
    for (Plugins::iterator it=plugins.begin(); it!=plugins.end(); it++)
    {
        (it->second).obj = new WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>(it->first.c_str(), "", false, query);
        if ( (it->second).obj != nullptr)
        {
            //Activate plugin
            JsonObject result, params;
            params["callsign"] = it->first.c_str();
            int rpcRet = controller->Invoke("activate", params, result);
            if (rpcRet == Core::ERROR_NONE)
                MIRACASTLOG_VERBOSE("Activated %s plugin", it->first.c_str());
            else
               MIRACASTLOG_ERROR("Could not activate %s plugin.  Failed with %d", it->first.c_str(), rpcRet);
            //register for events
            Event events = (it->second).events;
            for (Event::iterator evtItr=events.begin(); evtItr!=events.end(); evtItr++)
            {
                int evRet = (it->second).obj->Subscribe<JsonObject>(THUNDER_RPC_TIMEOUT, evtItr->first.c_str(), evtItr->second);
                if (evRet == Core::ERROR_NONE)
                    MIRACASTLOG_VERBOSE("%s - Updated event %s", it->first.c_str(), evtItr->first.c_str());
                else
                   MIRACASTLOG_ERROR("%s - failed to subscribe %s", it->first.c_str(), evtItr->first.c_str());
            }
        }
    }
}

ThunderUtils::~ThunderUtils()
{
    MIRACASTLOG_VERBOSE("ThunderUtils::~ThunderUtils", MIRACASTAPP_APP_LOG);
    for (Plugins::iterator it = plugins.begin(); it != plugins.end(); it++)
    {
        Event events = (it->second).events;
        for (Event::iterator evtItr = events.begin(); evtItr != events.end(); evtItr++)
            (it->second).obj->Unsubscribe(1000, evtItr->first.c_str());
        delete (it->second).obj;
        (it->second).obj = 0;
    }
}

ThunderUtils *ThunderUtils::getinstance()
{
    const std::lock_guard<std::mutex> lock(thunderUtilsMutex_);
    if (_instance == nullptr)
    {
        _instance = new ThunderUtils;
    }
    return _instance;
}

void ThunderUtils::thunderInvoke(const std::string &callsign, const std::string &method, const JsonObject &parameters, JsonObject &result)
{
    LOGINFOMETHOD();
    JsonObject response;
    if (plugins[callsign].obj != nullptr)
    {
        uint32_t ret = plugins[callsign].obj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, method.c_str(), parameters, response);
        if (ret == Core::ERROR_NONE)
        {
            MIRACASTLOG_VERBOSE("%s -%s call success", callsign.c_str(), method.c_str());
            result = response;
        }
        else
        {
            MIRACASTLOG_ERROR("%s -%s call Failed", callsign.c_str(), method.c_str());
        }
    }
    LOGTRACEMETHODFIN();
}

void ThunderUtils::thunderInvoke(const std::string &callsign, const std::string &method, JsonObject &result)
{
    JsonObject response;
    if (plugins[callsign].obj != nullptr)
    {
        uint32_t ret = plugins[callsign].obj->Invoke<void, JsonObject>(THUNDER_RPC_TIMEOUT, method.c_str(), response);
        if (ret == Core::ERROR_NONE)
        {
            MIRACASTLOG_VERBOSE("%s -%s call success", callsign.c_str(), method.c_str());
            result = response;
        }
        else
        {
            MIRACASTLOG_ERROR("%s -%s call failed", callsign.c_str(), method.c_str());
        }
    }
}

void ThunderUtils::thunderSet(const std::string &callsign, const std::string &method, const JsonObject &param)
{
    string result = "";
    if (plugins[callsign].obj != nullptr)
    {
        uint32_t ret = plugins[callsign].obj->Set(THUNDER_RPC_TIMEOUT, method.c_str(), param);
        if (ret == Core::ERROR_NONE)
        {
            MIRACASTLOG_VERBOSE("%s -%s call success", callsign.c_str(), method.c_str());
        }
        else
        {
           MIRACASTLOG_ERROR("%s -%s call failed", callsign.c_str(), method.c_str());
        }
    }
}
void ThunderUtils::thunderGet(const std::string &callsign, const std::string &method, auto &result)
{
   if (plugins[callsign].obj != nullptr)
    {
        uint32_t ret = plugins[callsign].obj->Get(THUNDER_RPC_TIMEOUT, method.c_str(), result);
        if (ret == Core::ERROR_NONE)
        {
            MIRACASTLOG_INFO("%s -%s call success", callsign.c_str(), method.c_str());
        }
        else
        {
            MIRACASTLOG_INFO("%s -%s call failed", callsign.c_str(), method.c_str());
        }
    }
}

string ThunderUtils::getAudioFormat(std::string AudioFormats)
{
    JsonObject params, result;
    thunderInvoke(DISPLAYSETTINGS_CALLSIGN, "getAudioFormat", params, result);
    if(result["success"].Boolean())
    {
        string sName = result["supportedAudioFormat"].String().c_str();
        AudioFormats = sName.c_str();
        MIRACASTLOG_VERBOSE("getAudioFormat call success", MIRACASTAPP_APP_LOG);
        return AudioFormats;
    }
    else
    {
        MIRACASTLOG_VERBOSE("getAudioFormat call failed", MIRACASTAPP_APP_LOG);
        return AudioFormats;
    }
}
string ThunderUtils::getColorFormat(std::string ColorFormats)
{
    JsonObject params, result;
    thunderInvoke(DISPLAYSETTINGS_CALLSIGN, "getVideoFormat", params, result);
    if(result["success"].Boolean())
    {
        string sName = result["supportedVideoFormat"].String().c_str();
        ColorFormats = sName.c_str();
        MIRACASTLOG_VERBOSE("getColorFormats call success", MIRACASTAPP_APP_LOG);
        return ColorFormats;
    }
    else
    {
        MIRACASTLOG_VERBOSE("getColorFormats call failed", MIRACASTAPP_APP_LOG);
	return ColorFormats;
    }
}

std::string ThunderUtils::getDeviceLocation()
{
    JsonObject result;
    std::string deviceLocation;
    thunderGet(LOCATION_CALLSIGN, "location", result);
    if(result.HasLabel("country"))
    {
        deviceLocation = result["country"].String();
        MIRACASTLOG_VERBOSE(" getDeviceLocation call success, location:%s", deviceLocation.c_str());
    }
    else {
        MIRACASTLOG_VERBOSE(" getDeviceLocation call failed");
    }
    return deviceLocation;
}

std::string ThunderUtils::getSystemLocale()
{
    JsonObject result;
    std::string systemLocale;
    thunderInvoke(USER_PREFERENCES_CALLSIGN,"getUILanguage", result);
    if(result["success"].Boolean() && result.HasLabel("ui_language")){
        systemLocale = result["ui_language"].String();
        MIRACASTLOG_VERBOSE(" getSystemLocale call success, Locale:%s", systemLocale.c_str());
    }
    else {
        MIRACASTLOG_VERBOSE(" getSystemLocale call failed");
    }
    return systemLocale;
}

std::string ThunderUtils::getModelName()
{
    JsonObject result;
    std::string modelName;
    thunderGet(DEVICE_INFO_CALLSIGN, "modelname", result);
    if (result.HasLabel("model"))
    {
        modelName = result["model"].String();
        MIRACASTLOG_INFO("getModelName call success: %s", modelName.c_str());
    }
    else {
        MIRACASTLOG_INFO("getModelName call failed");
    }
    return modelName;
}
void ThunderUtils::getResolution(char* resolution, int sLen){
    Core::JSON::String response;
    thunderGet(PLAYER_INFO_CALLSIGN, "resolution",response);
    string result = response.Value();
    int resultlength = result.size();
    int bytesToCopy = std::min(sLen, resultlength);
    strncpy(resolution, result.c_str(), bytesToCopy);
    resolution[bytesToCopy] = '\0';
}
void ThunderUtils::getActiveAudioPorts(string &activeAudioPort)
{
  JsonObject params, result;
  std::string audioPort = "";
  thunderInvoke(DISPLAYSETTINGS_CALLSIGN, "getConnectedAudioPorts", params, result);
  if (result["connectedAudioPorts"].Content() == JsonValue::type::ARRAY)
    {
    JsonArray audioPortsArray = result["connectedAudioPorts"].Array();
    for(int i= 0 ; i < audioPortsArray.Length() ; i++)
    {
        JsonObject audioPort_Obj = audioPortsArray[i].Object();
        audioPort = audioPortsArray[i].String().c_str();
        bool active_AudioPort = getEnableAudioPort(audioPort);
        if(active_AudioPort)
        {
                if((audioPort.compare("HDMI_ARC0")==0))
                {
                    activeAudioPort=audioPort;
                }
        }
    }
    MIRACASTLOG_VERBOSE("getActiveAudioPorts call success: %s", activeAudioPort.c_str());
   }
  else
  {
    MIRACASTLOG_ERROR("getActiveAudioPorts call failed");
  }
} 
std::string ThunderUtils::getFirmwareVersion()
{
    JsonObject result;
    std::string fwVersion;
    thunderGet(DEVICE_INFO_CALLSIGN, "firmwareversion", result);
    if (result.HasLabel("imagename"))
    {
        fwVersion = result["imagename"].String();
        MIRACASTLOG_VERBOSE("getFirmwareVersion call success: %s", fwVersion.c_str());
    }
    else {
        MIRACASTLOG_ERROR("getFirmwareVersion call failed");
    }
    return fwVersion;
}

std::string ThunderUtils::getConnectedNWInterface(){
    JsonObject result;
    std::string nwInterface;
    thunderInvoke(NETWORK_CALLSIGN,"getInterfaces", result);
    if (result["success"].Boolean()){
        if (result.HasLabel("interfaces") && (result["interfaces"].Content() == JsonValue::type::ARRAY)){
            const JsonArray& interfaces = result["interfaces"].Array();
            MIRACASTLOG_VERBOSE("No. interfaces:%d", interfaces.Length());
            for(int i=0; i < interfaces.Length();i++)
            {
                const JsonObject& interfaceObj = interfaces[i].Object();
                if(interfaceObj["connected"].Boolean()==true){
                     nwInterface = interfaceObj["interface"].String();
                     break;
                }
            }
            MIRACASTLOG_VERBOSE("getInterfaces call success: Connected interface: %s",nwInterface.c_str());
        }
        else {
            MIRACASTLOG_ERROR("No interfaces found or interfaces type is not array");
        }
    }
    else {
        MIRACASTLOG_ERROR("getInterfaces call failed");
    }
    return nwInterface;
}

bool ThunderUtils::isttsenabled()
{
    JsonObject params, result;
    bool isTTSEnabled = false;
    thunderInvoke(TEXTTOSPEECH_CALLSIGN, "isttsenabled", params, result);
    if (result["success"].Boolean())
    {
        isTTSEnabled = result["isenabled"].Boolean();
        MIRACASTLOG_VERBOSE("TTS Enabled call success");
    }
    else
    {
        MIRACASTLOG_ERROR("TTS Enabled call failed");
    }
    return isTTSEnabled;
}

void ThunderUtils::setttsRate(double& wpm)
{
        JsonObject params, result;
        int rate;
         params.Set(_T("rate"),wpm);
        thunderInvoke(TEXTTOSPEECH_CALLSIGN, "setttsconfiguration",params,result);
          if (result["success"].Boolean())
            {
                     rate = result["rate"].Number();
                MIRACASTLOG_VERBOSE("TTS SetRate call success");
            }
            else
            {
                MIRACASTLOG_ERROR("TTS SetRate check call failed");
            }
}

void ThunderUtils::cancelTts(int& speech_id){
        JsonObject params, result;
         params.Set(_T("speechid"),speech_id);
        thunderInvoke(TEXTTOSPEECH_CALLSIGN, "cancel",params,result);
          if (result["success"].Boolean())
            {

                MIRACASTLOG_VERBOSE("TTS Cancel check call success");
            }
            else
            {
                MIRACASTLOG_ERROR("TTS Cancel check call failed");
            }
}
int ThunderUtils::SpeakTts(std::string& text, int speak_id)
{
    JsonObject params, result;
    params.Set(_T("text"), text);
    params.Set(_T("callsign"), MIRACASTAPP_CALLSIGN);
    int speechId=speak_id;
    thunderInvoke(TEXTTOSPEECH_CALLSIGN, "speak", params, result);
      if (result["success"].Boolean())
        {
            if (speak_id != result["speechid"].Number())
            {
                speechId = result["speechid"].Number();
            }
            MIRACASTLOG_VERBOSE("SpeakTts: Speak call success, Speak ID: %d", speechId);
        }
        else
        {
            MIRACASTLOG_ERROR("SpeakTts: Speak call  failed");
        }

    return speechId;
}
bool ThunderUtils::isspeaking(int& speech_id)
{
    JsonObject params, result;
    bool isspeaking = false;
    thunderInvoke(TEXTTOSPEECH_CALLSIGN, "isspeaking", params, result);
    if (result["success"].Boolean())
    {
        isspeaking = result["isenabled"].Boolean();
        MIRACASTLOG_VERBOSE("TTS Enabled check call success");
    }
    else
    {
        MIRACASTLOG_ERROR("TTS Enabled check call failed");
    }
    return isspeaking;
}

void ThunderUtils::eventHandler_onspeechinterrupted(const JsonObject& parameters)
{
    MIRACASTLOG_VERBOSE("%s plugin \n", parameters["callsign"].String().c_str());
    RDKTextToSpeech::ttsEventHandler(ttsNotificationType::EVENT_ONSPEECHINTERRUPTED, parameters);
}

void ThunderUtils::eventHandler_onspeechcomplete(const JsonObject& parameters)
{
    MIRACASTLOG_VERBOSE("%s plugin \n", parameters["callsign"].String().c_str());
    RDKTextToSpeech::ttsEventHandler(ttsNotificationType::EVENT_ONSPEECHCOMPLETE, parameters);
}

void ThunderUtils::eventHandler_onspeechstart(const JsonObject& parameters)
{
    MIRACASTLOG_VERBOSE("%s plugin \n", parameters["callsign"].String().c_str());
    RDKTextToSpeech::ttsEventHandler(ttsNotificationType::EVENT_ONSPEECHSTART, parameters);
}

void ThunderUtils::eventHandler_onerror(const JsonObject& parameters)
{
    MIRACASTLOG_VERBOSE("%s plugin \n", parameters["callsign"].String().c_str());
    RDKTextToSpeech::ttsEventHandler(ttsNotificationType::EVENT_ONSPEECHERROR, parameters);
}

void ThunderUtils::eventHandler_onttsstatechanged(const JsonObject& parameters)
{
    MIRACASTLOG_VERBOSE("MiracastApp.TTS onttsstatechanged : %d \n", parameters["state"].Boolean());
    MIRACASTLOG_VERBOSE(" MiracastApp.TTS onttsstatechanged : %d \n", parameters["state"].Boolean());
    //Call registered listern to handle statechange event
    RDKTextToSpeech::ttsEventHandler(ttsNotificationType::EVENT_ONTTSSTATECHANGE, parameters);
}

void ThunderUtils::eventHandler_onConnectionStatusChanged(const JsonObject& parameters)
{
    std::string connectionStatus, nwInterface;
    connectionStatus = parameters["status"].String();
    nwInterface = parameters["interface"].String();
    MIRACASTLOG_VERBOSE(" MiracastApp onConnectionStatusChanged, interface: %s status: %s",nwInterface.c_str(), connectionStatus.c_str());
    
    if (nwInterface == "WIFI") {
        if (connectionStatus == "CONNECTED") {
            MIRACASTLOG_VERBOSE(" MiracastApp onConnectionStatusChanged, CONNECTED and WIFI Interface");
        } else {
            MIRACASTLOG_VERBOSE(" MiracastApp onConnectionStatusChanged, NOT_CONNECTED and WIFI Interface");
        }
    } else if (nwInterface == "ETHERNET" && connectionStatus != "CONNECTED") {
        MIRACASTLOG_VERBOSE(" MiracastApp onConnectionStatusChanged, NOT_CONNECTED and ETHERNET Interface");
    }
}

void ThunderUtils::eventHandler_onDefaultInterfaceChanged(const JsonObject& parameters)
{
    std::string oldInterfaceName, newInterfaceName;
    oldInterfaceName = parameters["oldInterfaceName"].String();
    newInterfaceName = parameters["newInterfaceName"].String();

    MIRACASTLOG_VERBOSE("MiracastApp onDefaultInterfaceChanged, old interface: %s, new interface: %s", oldInterfaceName.c_str(), newInterfaceName.c_str());
}

void ThunderUtils::eventHandler_onMiracastServiceClientConnectionRequest(const JsonObject& parameters)
{
    std::string client_mac, client_name;

    client_mac = parameters["mac"].String();
    client_name = parameters["name"].String();

    MIRACASTLOG_INFO(">>> client mac: [%s], client name: [%s]", client_mac.c_str(), client_name.c_str());
    RDKMiracastPlugin::miracastServiceClientConnectionRequestHandler(client_mac, client_name);
}

void ThunderUtils::eventHandler_onMiracastServiceClientConnectionError(const JsonObject& parameters)
{
    std::string client_mac, client_name, error_code, reason;

    client_mac = parameters["mac"].String();
    client_name = parameters["name"].String();
    error_code = parameters["error_code"].String();
    reason = parameters["reason"].String();

    MIRACASTLOG_INFO(">>> client mac: [%s], client name: [%s], error code: [%s], reason: [%s]", client_mac.c_str(), client_name.c_str(), error_code.c_str(), reason.c_str());
    RDKMiracastPlugin::miracastServiceClientConnectionErrorHandler(client_mac, client_name, error_code, reason);
}

void ThunderUtils::eventHandler_onMiracastServiceLaunchRequest(const JsonObject& parameters)
{
    std::string source_dev_ip, source_dev_mac, source_dev_name, sink_dev_ip;
    JsonObject device_parameters;

    device_parameters = parameters["device_parameters"].Object();
    source_dev_ip = device_parameters["source_dev_ip"].String();
    source_dev_mac = device_parameters["source_dev_mac"].String();
    source_dev_name = device_parameters["source_dev_name"].String();
    sink_dev_ip = device_parameters["sink_dev_ip"].String();

    MIRACASTLOG_INFO(">>> source dev ip: [%s], source dev mac: [%s], source dev name: [%s], sink dev ip: [%s]", source_dev_ip.c_str(), source_dev_mac.c_str(), source_dev_name.c_str(), sink_dev_ip.c_str());
    RDKMiracastPlugin::miracastServiceLaunchRequestHandler(source_dev_ip, source_dev_mac, source_dev_name, sink_dev_ip);
}

void ThunderUtils::eventHandler_onMiracastPlayerStateChange(const JsonObject& parameters)
{
    std::string client_mac, client_name, state, reason, reason_code;

    client_mac = parameters["mac"].String();
    client_name = parameters["name"].String();
    state = parameters["state"].String();
    reason = parameters["reason"].String();
    reason_code = parameters["reason_code"].String();

    MIRACASTLOG_INFO(">>> client mac: [%s], client name: [%s], state: [%s], reason: [%s], reason code: [%s]", client_mac.c_str(), client_name.c_str(), state.c_str(), reason.c_str(), reason_code.c_str());

    RDKMiracastPlugin::miracastPlayerStateChangeHandler(client_mac, client_name, state, reason, reason_code);
}

std::string ThunderUtils::getIPAddress()
{
    struct ifaddrs *ifAddrStruct = nullptr;
    struct ifaddrs *ifa = nullptr;
    std::string ipAddress = "";

    if (getifaddrs(&ifAddrStruct) == -1)
    {
        MIRACASTLOG_ERROR("Error getting network interfaces.", MIRACASTAPP_APP_LOG);
        return "";
    }

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
        {
            continue;
        }

        // Check if it's an IPv4 address
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            char ipAddr[INET_ADDRSTRLEN];
            struct sockaddr_in *inAddr = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
            inet_ntop(AF_INET, &(inAddr->sin_addr), ipAddr, INET_ADDRSTRLEN);
            MIRACASTLOG_INFO("IP address = %s", ipAddr);
            if (strncmp(ipAddr, "100.64.11", 9) == 0)
            {
                ipAddress = ipAddr;
                break;
            }
        }
    }
    freeifaddrs(ifAddrStruct);
    return ipAddress;
}

bool ThunderUtils::getEnableAudioPort(string &enabledAudioPort)
{
    bool enable_port = false;
    MIRACASTLOG_TRACE("Entering ...");
    JsonObject params, result;
    params["audioPort"] = enabledAudioPort.c_str();
    thunderInvoke(DISPLAYSETTINGS_CALLSIGN, "getEnableAudioPort",params, result);
    if (result["success"].Boolean())
    {
        MIRACASTLOG_VERBOSE("getEnabledAudioPorts call success");
        enable_port = result["enable"].Boolean();
        MIRACASTLOG_VERBOSE("getEnabledAudioPorts success, audio port:%s enabled_port = %d\n",enabledAudioPort.c_str(), enable_port);
    }
    else
    {
        MIRACASTLOG_ERROR("getEnabledAudioPorts call failed");
    }
    return enable_port;
    MIRACASTLOG_TRACE("Exiting ...");
}

bool ThunderUtils::setMiracastDiscovery(bool enabledStatus)
{
    JsonObject params, result;
    bool returnValue = false;
    params["enabled"] = enabledStatus;
    thunderInvoke(MIRACASTSERVICE_CALLSIGN, "setEnable", params, result);
    returnValue = result["success"].Boolean();
    if (returnValue)
    {
        MIRACASTLOG_VERBOSE("setMiracastDiscovery call success");
    }
    else
    {
        MIRACASTLOG_ERROR("setMiracastDiscovery call failed");
    }
    return returnValue;
}

bool ThunderUtils::acceptMiracastClientConnection(const string &requestStatus)
{
    JsonObject params, result;
    bool returnValue = false;
    params["requestStatus"] = requestStatus.c_str();;
    thunderInvoke(MIRACASTSERVICE_CALLSIGN, "acceptClientConnection", params, result);
    returnValue = result["success"].Boolean();
    if (returnValue)
    {
        MIRACASTLOG_VERBOSE("acceptClientConnection call success");
    }
    else
    {
        MIRACASTLOG_ERROR("acceptClientConnection call failed");
    }
    return returnValue;
}

bool ThunderUtils::updateMiracastPlayerState(const string &clientMac, const string &state, const string &reason_code)
{
    JsonObject params, result;
    bool returnValue = false;
    params["mac"] = clientMac.c_str();
    params["state"] = state.c_str();
    params["reason_code"] = reason_code.c_str();
    thunderInvoke(MIRACASTSERVICE_CALLSIGN, "updatePlayerState", params, result);

    returnValue = result["success"].Boolean();
    if (returnValue)
    {
        MIRACASTLOG_VERBOSE("updateMiracastPlayerState call success");
    }
    else
    {
        MIRACASTLOG_ERROR("updateMiracastPlayerState call failed");
    }
    return returnValue;
}

bool ThunderUtils::setWesterosEnvToMiracastPlayer(void){
    JsonObject params, result;
    JsonArray westerosArgsArray;
    bool returnValue = false;

    for (auto &envArg : westerosEnvArgs)
    {
        JsonObject argObject;

        argObject["argName"] = envArg.first.c_str();
        argObject["argValue"] = envArg.second.c_str();
        westerosArgsArray.Add(argObject);
        MIRACASTLOG_INFO("westerosEnvArgs: [%s] = [%s]", envArg.first.c_str(), envArg.second.c_str());
    }
    params["westerosArgs"] = westerosArgsArray;
    params["appName"] = "MiracastApp";

    thunderInvoke(MIRACASTPLAYER_CALLSIGN, "setWesterosEnvironment", params, result);

    returnValue = result["success"].Boolean();
    if (returnValue)
    {
        MIRACASTLOG_VERBOSE("setWesterosEnvironment call success");
    }
    else
    {
        MIRACASTLOG_ERROR("setWesterosEnvironment call failed");
    }
    return returnValue;
}

bool ThunderUtils::playRequestToMiracastPlayer(const std::string &source_dev_ip, const std::string &source_dev_mac, const std::string &source_dev_name, const std::string &sink_dev_ip, VideoRectangleInfo &rect)
{
    JsonObject params, result, device_parameters, video_rectangle;
    bool returnValue = false;

    device_parameters["source_dev_ip"] = source_dev_ip.c_str();
    device_parameters["source_dev_mac"] = source_dev_mac.c_str();
    device_parameters["source_dev_name"] = source_dev_name.c_str();
    device_parameters["sink_dev_ip"] = sink_dev_ip.c_str();

    video_rectangle["X"] = rect.startX;
    video_rectangle["Y"] = rect.startY;
    video_rectangle["W"] = rect.width;
    video_rectangle["H"] = rect.height;

    params["device_parameters"] = device_parameters;
    params["video_rectangle"] = video_rectangle;

    thunderInvoke(MIRACASTPLAYER_CALLSIGN, "playRequest", params, result);

    returnValue = result["success"].Boolean();
    if (returnValue)
    {
        MIRACASTLOG_VERBOSE("playRequest call success");
    }
    else
    {
        MIRACASTLOG_ERROR("playRequest call failed");
    }
    return returnValue;
}

bool ThunderUtils::stopMiracastPlayer(void)
{
    JsonObject params, result;
    bool returnValue = false;

    params["reason_code"] = MIRACAST_PLAYER_APP_REQ_TO_STOP_ON_EXIT;

    thunderInvoke(MIRACASTPLAYER_CALLSIGN, "playRequest", params, result);

    returnValue = result["success"].Boolean();
    if (returnValue)
    {
        MIRACASTLOG_VERBOSE("updateMiracastPlayerState call success");
    }
    else
    {
        MIRACASTLOG_ERROR("updateMiracastPlayerState call failed");
    }
    return returnValue;
}