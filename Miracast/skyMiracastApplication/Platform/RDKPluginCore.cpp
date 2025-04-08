#include "RDKPluginCore.h"
#include "MiracastAppLogging.hpp"

ThunderUtils *RDKPluginCore::_mThunderUtils{nullptr};
RDKPluginCore *RDKPluginCore::_mRDKPluginCore{nullptr};
RDKTextToSpeech *RDKTextToSpeech::_mRDKtts{nullptr};
RDKMiracastService *RDKMiracastService::_mRDKMiracastService{nullptr};

#define WORDS_PER_MINUTE 140
#define UINT32_MAX_ 4294967295

RDKPluginCore *RDKPluginCore::getInstance()
{
    if (_mThunderUtils == nullptr)
    {
        _mThunderUtils = ThunderUtils::getinstance();

        if (_mThunderUtils == nullptr)
        {
            MIRACASTLOG_ERROR("Failed to get ThunderUtils instance");
            return nullptr;
        }
    }

    if (_mRDKPluginCore == nullptr)
    {
        _mRDKPluginCore = new RDKPluginCore();
    }
    return _mRDKPluginCore;
}

void RDKPluginCore::destroyInstance()
{
    if (_mThunderUtils != nullptr)
    {
        delete _mThunderUtils;
        _mThunderUtils = nullptr;
    }

    if (_mRDKPluginCore != nullptr)
    {
        delete _mRDKPluginCore;
        _mRDKPluginCore = nullptr;
    }
}

RDKTextToSpeech *RDKTextToSpeech::getInstance()
{
    if (_mRDKtts == nullptr)
    {
        _mRDKtts = new RDKTextToSpeech();
    }
    return _mRDKtts;
}

RDKTextToSpeech::RDKTextToSpeech()
    : _mTtsSpeakId(0)
{
    mEventListener = nullptr;
}

void RDKTextToSpeech::destroyInstance()
{
    if (_mRDKtts != nullptr)
    {
        delete _mRDKtts;
        _mRDKtts = nullptr;
    }
}

void RDKTextToSpeech::registerListener(TextToSpeech::ITextToSpeechListener *const listener)
{
    MIRACASTLOG_INFO("RDK_TEXT_TO_SPEECH");
    mEventListener = listener;
    if (mEventListener != nullptr)
        MIRACASTLOG_VERBOSE("TTS registration completed");
}
void RDKTextToSpeech::cancel()
{
    MIRACASTLOG_VERBOSE("Cancelling the current speech");
    RDKPluginCore::getInstance()->getThunderUtilsInstance()->cancelTts(_mTtsSpeakId);
    RDKPluginCore::getInstance()->getThunderUtilsInstance()->cancelTts(_mTtsSpeakId);
}
void RDKTextToSpeech::speak(const std::string &text)
{
    std::string speech;
    speech = text;
    MIRACASTLOG_VERBOSE("speak requested [%s]", speech.c_str());
    _mTtsSpeakId = RDKPluginCore::getInstance()->getThunderUtilsInstance()->SpeakTts(speech, _mTtsSpeakId);
}
void RDKTextToSpeech::setRate(const float rate)
{
    double wpm = 0.0;
    wpm = rate * WORDS_PER_MINUTE;
    MIRACASTLOG_VERBOSE("Setting speech rate to [%f]", wpm);
    RDKPluginCore::getInstance()->getThunderUtilsInstance()->setttsRate(wpm);
}
void RDKTextToSpeech::setPitch(const float pitch)
{
    MIRACASTLOG_VERBOSE("Setting speech pitch to [%f]", pitch);
}
void RDKTextToSpeech::setLanguage(const std::string &language)
{
    MIRACASTLOG_VERBOSE("Setting speech language to [%s]", language.c_str());
}

void RDKTextToSpeech::getProperties(TextToSpeech::Properties &property)
{
    MIRACASTLOG_TRACE("Entering ...");
    property.speakableLanguages = getSupportedLanguages();
    property.availableLanguages = getSupportedLanguages();
    property.isTextToSpeechEnabled = isttsenabled();
    property.isSpeakRateSupported = true;
    property.isPitchSupported = false;
    property.canInstallAdditionalLanguages = false;
    property.maxTextSize = UINT32_MAX_;
    MIRACASTLOG_TRACE("Exiting ...");
}

bool RDKTextToSpeech::isttsenabled()
{
    bool ttsEnabled = false;
    ttsEnabled = RDKPluginCore::getInstance()->getThunderUtilsInstance()->isttsenabled();
    MIRACASTLOG_VERBOSE("TTS Status Called [%d]", ttsEnabled);
    return ttsEnabled;
}
void RDKTextToSpeech::pendingUpdate()
{
    MIRACASTLOG_VERBOSE("Pending update");
}

std::vector<std::string> RDKTextToSpeech::getSupportedLanguages()
{
    std::vector<std::string> lang;

    lang.push_back("en-US");
    lang.push_back("es-MX");

    return lang;
}

void RDKTextToSpeech::setttsRate(double &wpm)
{
    ThunderUtils *_setrate = ThunderUtils::getinstance();
    _setrate->setttsRate(wpm);
}

void RDKTextToSpeech::ttsEventHandler(ThunderUtils::ttsNotificationType event, const JsonObject &parameters)
{
    MIRACASTLOG_VERBOSE("event:[%d]", event);
    if (RDKTextToSpeech::getInstance()->mEventListener != nullptr)
    {
        switch (event)
        {
        case ThunderUtils::ttsNotificationType::EVENT_ONSPEECHINTERRUPTED:
            RDKTextToSpeech::getInstance()->mEventListener->onStateChanged(TextToSpeech::EventType::EVENT_TYPE_CANCEL);
            break;
        case ThunderUtils::ttsNotificationType::EVENT_ONSPEECHCOMPLETE:
            RDKTextToSpeech::getInstance()->mEventListener->onStateChanged(TextToSpeech::EventType::EVENT_TYPE_UTTERANCE_COMPLETED);
            break;
        case ThunderUtils::ttsNotificationType::EVENT_ONSPEECHSTART:
            RDKTextToSpeech::getInstance()->mEventListener->onStateChanged(TextToSpeech::EventType::EVENT_TYPE_UTTERANCE_STARTED);
            break;
        case ThunderUtils::ttsNotificationType::EVENT_ONSPEECHERROR:
            RDKTextToSpeech::getInstance()->mEventListener->onStateChanged(TextToSpeech::EventType::EVENT_TYPE_UTTERANCE_ERROR);
            break;
        case ThunderUtils::ttsNotificationType::EVENT_ONTTSSTATECHANGE:
        {
            TextToSpeech::Properties property;
            RDKTextToSpeech::getInstance()->getProperties(property);
            RDKTextToSpeech::getInstance()->mEventListener->onPropertiesChanged(property);
            break;
        }
        default:
            MIRACASTLOG_WARNING("TTS: unknown event received:[%d]",event);
        }
    }
    else
    {
        MIRACASTLOG_WARNING("No Registered listener");
    }
}

RDKMiracastService *RDKMiracastService::getInstance()
{
    if (_mRDKMiracastService == nullptr)
    {
        _mRDKMiracastService = new RDKMiracastService();
    }
    return _mRDKMiracastService;
}

RDKMiracastService::RDKMiracastService()
{
    mEventListener = nullptr;
}

void RDKMiracastService::destroyInstance()
{
    if (_mRDKMiracastService != nullptr)
    {
        delete _mRDKMiracastService;
        _mRDKMiracastService = nullptr;
    }
}
void RDKMiracastService::registerListener(MiracastService::IMiracastServiceListener *const listener)
{
    MIRACASTLOG_INFO("RDK_TEXT_TO_SPEECH");
    mEventListener = listener;
    if (mEventListener != nullptr)
        MIRACASTLOG_VERBOSE("TTS registration completed");
}

void RDKMiracastService::setEnable(bool enabledStatus)
{
    MIRACASTLOG_TRACE("Entering ...");
    RDKPluginCore::getInstance()->getThunderUtilsInstance()->setMiracastDiscovery(enabledStatus);
    MIRACASTLOG_TRACE("Exiting ...");
}

void RDKMiracastService::acceptClientConnection(const std::string &requestStatus)
{
    MIRACASTLOG_TRACE("Entering ...");
    RDKPluginCore::getInstance()->getThunderUtilsInstance()->acceptMiracastClientConnection(requestStatus);
    MIRACASTLOG_TRACE("Exiting ...");
}

void RDKMiracastService::updatePlayerState(const std::string &clientMac, const std::string &state, const std::string &reason_code)
{
    MIRACASTLOG_TRACE("Entering ...");
    RDKPluginCore::getInstance()->getThunderUtilsInstance()->updateMiracastPlayerState(clientMac, state, reason_code);
    MIRACASTLOG_TRACE("Exiting ...");
}

void RDKMiracastService::miracastServiceClientConnectionRequestHandler(const std::string &client_mac, const std::string &client_name)
{
    MIRACASTLOG_VERBOSE("client_mac:[%s] client_name:[%s]", client_mac.c_str(), client_name.c_str());
    if (RDKMiracastService::getInstance()->mEventListener != nullptr)
    {
        RDKMiracastService::getInstance()->mEventListener->onClientConnectionRequest(client_mac, client_name);
    }
    else
    {
        MIRACASTLOG_WARNING("No Registered listener");
    }
}

void RDKMiracastService::miracastServiceClientConnectionErrorHandler(const std::string &client_mac, const std::string &client_name, const std::string &error_code, const std::string &reason)
{
    MIRACASTLOG_VERBOSE("client_mac:[%s] client_name:[%s] error_code:[%s] reason:[%s]", client_mac.c_str(), client_name.c_str(), error_code.c_str(), reason.c_str());
    if (RDKMiracastService::getInstance()->mEventListener != nullptr)
    {
        RDKMiracastService::getInstance()->mEventListener->onClientConnectionError(client_mac, client_name, error_code, reason);
    }
    else
    {
        MIRACASTLOG_WARNING("No Registered listener");
    }
}

void RDKMiracastService::miracastServiceLaunchRequestHandler(const std::string &source_dev_ip, const std::string &source_dev_mac, const std::string &source_dev_name, const std::string &sink_dev_ip)
{
    MIRACASTLOG_VERBOSE("source_dev_ip:[%s] source_dev_mac:[%s] source_dev_name:[%s] sink_dev_ip:[%s]", source_dev_ip.c_str(), source_dev_mac.c_str(), source_dev_name.c_str(), sink_dev_ip.c_str());
    if (RDKMiracastService::getInstance()->mEventListener != nullptr)
    {
        RDKMiracastService::getInstance()->mEventListener->onLaunchRequest(source_dev_ip, source_dev_mac, source_dev_name, sink_dev_ip);
    }
    else
    {
        MIRACASTLOG_WARNING("No Registered listener");
    }
}