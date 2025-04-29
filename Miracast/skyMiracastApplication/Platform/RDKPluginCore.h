#ifndef _RDK_PLUGIN_CORE_H_
#define _RDK_PLUGIN_CORE_H_
#include "ThunderUtils.h"
#include "RDKPluginhelper.h"
#include "MiracastAppLogging.hpp"

using namespace TextToSpeech;
using namespace MiracastPlugin;

class RDKPluginCore
{
private:
    static RDKPluginCore *_mRDKPluginCore;
    static ThunderUtils *_mThunderUtils;
    RDKPluginCore(){};

protected:
    ~RDKPluginCore(){};

public:
    static RDKPluginCore *getInstance();
    static void destroyInstance();

    ThunderUtils *getThunderUtilsInstance(){return _mThunderUtils;};

    RDKPluginCore(const RDKPluginCore &) = delete;
    const RDKPluginCore &operator=(const RDKPluginCore &) = delete;
};

class RDKTextToSpeech : public TextToSpeech::Manager
{
private:
    static RDKTextToSpeech *_mRDKtts;
    RDKTextToSpeech();
    int _mTtsSpeakId;
    bool isttsenabled();
    std::vector<std::string> getSupportedLanguages();
    void setttsRate(double &rate);
    bool isspeaking(int &_mTtsSpeakId);

protected:
    ~RDKTextToSpeech(){};

public:
    TextToSpeech::ITextToSpeechListener *mEventListener;
    static RDKTextToSpeech *getInstance();
    static void destroyInstance();
    void registerListener(TextToSpeech::ITextToSpeechListener *const listener);
    void cancel();
    void speak(const std::string &text);
    void setRate(const float rate);
    void setPitch(const float pitch);
    void setLanguage(const std::string &language);
    void getProperties(TextToSpeech::Properties &property);
    void pendingUpdate();
    RDKTextToSpeech(const RDKTextToSpeech &) = delete;
    const RDKTextToSpeech &operator=(const RDKTextToSpeech &) = delete;
    static void ttsEventHandler(ThunderUtils::ttsNotificationType NotificationType, const JsonObject& parameters);
};

class RDKMiracastPlugin : public MiracastPlugin::Manager
{
private:
    static RDKMiracastPlugin *_mRDKMiracastPlugin;
    RDKMiracastPlugin();

protected:
    ~RDKMiracastPlugin(){};

public:
    MiracastPlugin::IMiracastPluginListener *mEventListener;
    static RDKMiracastPlugin *getInstance();
    static void destroyInstance();
    void registerListener(MiracastPlugin::IMiracastPluginListener *const listener);

    void setEnable(bool enabledStatus);
    void acceptClientConnection(const std::string &requestStatus);
    void updatePlayerState(const std::string &clientMac, const std::string &state, const std::string &reason_code);
    void playRequestToMiracastPlayer(const std::string &source_dev_ip, const std::string &source_dev_mac, const std::string &source_dev_name, const std::string &sink_dev_ip, VideoRectangleInfo &rect);
    void stopMiracastPlayer(void);
    void setWesterosEnvToMiracastPlayer(void);
    
    RDKMiracastPlugin(const RDKMiracastPlugin &) = delete;
    const RDKMiracastPlugin &operator=(const RDKMiracastPlugin &) = delete;

    static void miracastServiceClientConnectionRequestHandler(const std::string &client_mac, const std::string &client_name);
    static void miracastServiceClientConnectionErrorHandler(const std::string &client_mac, const std::string &client_name, const std::string &error_code, const std::string &reason);
    static void miracastServiceLaunchRequestHandler(const std::string &source_dev_ip, const std::string &source_dev_mac, const std::string &source_dev_name, const std::string &sink_dev_ip);

    static void miracastPlayerStateChangeHandler(const std::string &client_mac, const std::string &client_name, const std::string &state, const std::string &reason, const std::string &reason_code);
};

#endif /*_RDK_PLUGIN_CORE_H_*/