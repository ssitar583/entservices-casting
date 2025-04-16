#ifndef _RDK_PLUGIN_HELPER_H_
#define _RDK_PLUGIN_HELPER_H_

#include <string>
#include <vector>
#include "helpers.hpp"

namespace TextToSpeech
{
//forward declaration
class ITextToSpeechListener;

   /*^*
     * @enum EventType
     * @value EVENT_TYPE_CANCEL=0                       TTS engine has finished after canceling an utterance.  The engine is ready for  a new utterance.
     * @value EVENT_TYPE_UTTERANCE_STARTED=1            TTS engine has started an utterance (but is not finished).  The engine is NOT ready for a new utterance.
     * @value EVENT_TYPE_UTTERANCE_COMPLETED=2          TTS engine has completed and utterance, but was not cancelled.  The engine is ready for a new utterance.
     * @value EVENT_TYPE_UTTERANCE_ERROR=3              A non-fatal error occurred.  The engine is ready for a new Utterance.
     * 
     * @description 
     *   Enum informing MiracastApp the state of the Native TTS engine, see ITextToSpeechListener. Send by the commands "speak(..)" and "cancel(..)"
     *^*/
enum EventType 
{
    EVENT_TYPE_CANCEL,
    EVENT_TYPE_UTTERANCE_STARTED,
    EVENT_TYPE_UTTERANCE_COMPLETED,
    EVENT_TYPE_UTTERANCE_ERROR
};

/*^*
    * @class TextToSpeech::Manager
    * @struct Properties
    * @member std::vector<std::string> speakableLanguages         Languages the Voice Engine can change to programatically without delay. This must include current spoken language.
    *                                                             Currently spoken Language is included in the list. The list is typically 1 to N items long.
    *                                                             Specify as IETF language tag. Format example: en-US, fr-CA, see: https://www.w3.org/International/articles/language-tags/  
    *
    * @member std::vector<std::string> availableLanguages         optional: Fill with all available languages it is possible to select by the user.
    *                                                             Typically these are selectable by the user via the System menu settings. This will help the app to
    *                                                             instruct the user to change language.
    *                                                             Will contain all the languages from the vector "speakableLanguages". If left out the assumption is the same languages as
    *                                                             specified in "speakableLanguages".
    *                                                             Specify as IETF language tag. Format example: en-US, fr-CA, see: https://www.w3.org/International/articles/language-tags/
    * 
    * @member bool isTextToSpeechEnabled                          True if text to speech is enabled on the device. False otherwise.
    * @member bool isSpeakRateSupported                           True if the platform supports changing speak rate. False otherwise.
    * @member bool isPitchSupported                               True if the platform support changing pitch. False otherwise.
    * @member bool canInstallAdditionalLanguages                  True if the platform supports downloading new languages. False otherwise. These installable languages are not
    *                                                             included in "availableLanguages".
    * @member uint32_t maxTextSize                                Some engines can only handle an utterance of under a certain size.  //Todo we should think about removing findSentenceBreak() and maxTextSize as this is platform specific. If not we need a number to indicate that this is not used... -1?
    * 
    * @description
    *  Arguments Device properties.                  
    * ^*/
struct Properties
{
    std::vector<std::string>    speakableLanguages;
    std::vector<std::string>    availableLanguages;
        
    bool                        isTextToSpeechEnabled;
    bool                        isSpeakRateSupported;
    bool                        isPitchSupported;
    bool                        canInstallAdditionalLanguages;
    uint32_t                    maxTextSize;
};

/*^*
 * @class TextToSpeech::Manager
 * @mapto TextToSpeech
 * @description 
 * to enable Text To Speech (TTS) functionality.
 * Register in MiracastApp by calling "setNativeTtsImplementation(..)", MiracastApp will in turn call "Manager->registerListener(..)"" to enable callback.
 * 
 * Phase           Action                           Comment
 * =======================================================================================================================
 * Setup:
 *                 setNativeTtsImplementation(..)      Register TTS, MiracastApp will register callback
 *                 TTS->registerListener(..)           
 * 
 * Running:
 *                 TTS->speak()                     Speak an utterance 
 *                 TTS->cancel()                    Cancel an utterance
 * 
 *                 onPropertiesChanged(..)          when Properties has changed.
 *                 onStateChanged(..)               When the speech engine updates its state after speak() or cancel(). See EventType.
 * 
 *                 TTS->setRate()                   change rate for next utterance (Optional to support)
 *                 TTS->setPitch()                  change pitch for next utterance (Optional to support)
 *                 TTS->setLanguage()               change language for nect utterance (Optional to support)
 * 
 * Cleanup:
 *                 unsetNativeTtsImplementation();   let MiracastApp know to clean up TTS and unregister callback.
 * 
 * =======================================================================================================================
 *  Note on threading:  There are 2 threading models.
 *           Multi-threaded (default).  In this model speak(), cancel(), setRate, setPitch, setLanguage() will be called 
 *           from a worker thread.  The advantage is the above functions are called immediately.
 * 
 *           Single-threaded model.  This is for systems that require speak() and cancel() to be called from a specific thread.
 *           For this model to work one must set the parameter useSingleThreaded in the function setNativeTtsImplementation() to true.
 *           Then one must call the listener function singleThreadUpdate().  From your thread at a regular interval.
 *^*/
class Manager
{
public:
    /*^*
     * @class TextToSpeech::Manager
     * @function void registerListener(TextToSpeech::ITextToSpeechListener *listener)
     * @param listener  pointer to a ITextToSpeechListener object. This should be stored in a list which can be notified when EVENT_TYPE or Properties change.
     * 
     * @description  Registers a listener for informing MiracastApp of changes in State or properties.
     *^*/
    virtual void registerListener( ITextToSpeechListener * const listener) = 0;
    
    /*^*
     * @class TextToSpeech::Manager
     * @function  void cancel()
     * 
     * @description  Function that cancels any utterance in progress.  One must notify the listeners that the state has changed to EVENT_TYPE_CANCEL once the utterance is finshed being canceled.
     *^*/
    virtual void cancel() = 0;

    /*^*
     * @class TextToSpeech::Manager
     * @function  void speak(const std::string &text)
     * @param text Text to be spoken.  text is in UTF-8 format
     * 
     * @description  Function that speaks an utterance.  One must notify the listeners that the state has changed to EVENT_TYPE_UTTERANCE_STARTED once the utterance has started.
     * Only one utterance at a time, an utterance must be completed or cancelled before a new one is made.
     *^*/
    virtual void speak(const std::string &text) = 0;
   
    /*^*
     * @class TextToSpeech::Manager
     * @function void setRate(float rate) 
     * @param rate  rate at which words are spoken  Examples values:  1 = normal speed around 140 words per minute; 0.5 = half speed; 2 = double speed
     * 
     * @description  Optional: Function used to change the rate of words spoken. Affects the next call to "speak".
     *                         Will only be call when engine is ready for a new utterance, see EventType
     *^*/
    virtual void setRate(const float rate) {};

     /*^*
     * @class TextToSpeech::Manager
     * @function void setPitch(float pitch) 
     * @param pitch  1.0 = normal, set between 0.0 and 2.0
     * 
     * @description  Optional: Function used to change the pitch that the utterance are read. Affects the next call to "speak".
     *                         Will only be call when engine is ready for a new utterance, see EventType
     *^*/
    virtual void setPitch(const float pitch) {};

    /*^*
     * @class TextToSpeech::Manager
     * @function  void setLanguage(std::string &language)
     * @param language  language to be spoken in in IETF language tag. Format example: en-US, fr-CA, see: https://www.w3.org/International/articles/language-tags/   
     * 
     * @description  Optional: Function used to change what language the voice engine is to use. Affects the next call to "speak".
     *                         Will only be call when engine is ready for a new utterance, see EventType
     *^*/
    virtual void setLanguage(const std::string &language) {};

    /*^*
     * @class TextToSpeech::Manager
     * @function  Properties getProperties()
     * 
     * @description  Returns the properties for the specific implementation of Text To Speech
     *^*/
    virtual void getProperties(TextToSpeech::Properties &property) = 0;

    /*^*
     * @class TextToSpeech::Manager
     * @function  void pendingUpdate
     * 
     * @description  Optional: Called during single thread operation when there is a pending update.  If one is calling update() at regular intervals,
     *                          then one does not need to use this function.  This will be called from a different thread, than the thread used 
     *                          for speak() and cancel().
     *^*/
    virtual void pendingUpdate() {};

protected:
    /*^*
     * @class TextToSpeech::Manager
     * @function  ~Manager()
     * 
     * @description  Destructor
     *^*/
    virtual ~Manager() {};

private:
    const Manager &operator = (const Manager &);    // DON'T COPY ME!

};  //class Manager

class ITextToSpeechListener
{
public:
    /*^*
     * @class TextToSpeech::ITextToSpeechListener
     * @function  void onStateChanged(const EventType event)
     * @param EventType the new state the engine has changed to.
     * 
     * @description  This function is implemented in the MiracastApp.  This is to be called from the registered listeners to inform MiracastApp that the state has changed.
     *^*/
    virtual void onStateChanged(EventType event) {};
    /*^*
     * @class TextToSpeech::ITextToSpeechListener
     * @function   void onPropertiesChanged(const TextToSpeech::Manager::Properties)
     * @param properties A new properties structure.
     * 
     * @description  This function is implemented in the MiracastApp.  This is to be called from the registered listeners to inform MiracastApp that the properties have changed.
     *^*/
    virtual void onPropertiesChanged(const TextToSpeech::Properties &properties) {};

    virtual ~ITextToSpeechListener() {};
};
} // namespace TextToSpeech


namespace MiracastPlugin
{
//forward declaration
class IMiracastPluginListener;

class Manager
{
public:
    /*^*
     * @class MiracastPlugin::Manager
     * @function void registerListener(MiracastPlugin::IMiracastPluginListener *listener)
     * @param listener  pointer to a IMiracastPluginListener object. This should be stored which can be notified during Miracast connection and launch request.
     * 
     * @description  Registers a listener for informing MiracastApp for connect and launch request.
     *^*/
    virtual void registerListener( IMiracastPluginListener * const listener) = 0;
    
    /*^*
     * @class MiracastPlugin::Manager
     * @function  void setEnable()
     * 
     * @description  Function that enables the Miracast discovery.
     *^*/
    virtual void setEnable(bool enabledStatus) = 0;

    /*^*
     * @class MiracastPlugin::Manager
     * @function  void acceptClientConnection()
     * @param requestStatus either Accept or Reject
     * 
     * @description  Function that accept/reject the Miracast client connection.
     *^*/
    virtual void acceptClientConnection(const std::string &requestStatus) = 0;

    /*^*
     * @class MiracastPlugin::Manager
     * @function  void acceptClientConnection()
     * @param clientMac - Mac addres of the Source device
     * @param state - current Miracast Player state
     * @param reason_code - Reason code for the state
     * 
     * @description  Function that update the MiracastPlayer state.
     *^*/
    virtual void updatePlayerState(const std::string &clientMac, const std::string &state, const std::string &reason_code) = 0;

protected:
    /*^*
     * @class MiracastPlugin::Manager
     * @function  ~Manager()
     * 
     * @description  Destructor
     *^*/
    virtual ~Manager() {};

private:
    const Manager &operator = (const Manager &);    // DON'T COPY ME!

};  //class Manager

class IMiracastPluginListener
{
public:
    /*^*
     * @class MiracastPlugin::IMiracastPluginListener
     * @function  void onMiracastServiceClientConnectionRequest(const string &client_mac, const string &client_name)
     * @param client_mac - Source Device Mac Address .
     * @param client_name - Source Device Mac Name .
     * @description  This function is implemented in the MiracastApp.  This is to be called from the registered listeners to inform MiracastApp that the state has changed.
     *^*/
    virtual void onMiracastServiceClientConnectionRequest(const string &client_mac, const string &client_name) {};

    /*^*
     * @class MiracastPlugin::IMiracastPluginListener
     * @function  void onMiracastServiceClientConnectionError(const std::string &client_mac, const std::string &client_name, const std::string &error_code, const std::string &reason )
     * @param client_mac - Source Device Mac Address .
     * @param client_name - Source Device Mac Name .
     * @param error_code - Error code if any failure .
     * @param reason - Error code reason for the failure .
     * @description  This function is implemented in the MiracastApp.  This is to be called from the registered listeners to inform MiracastApp that the state has changed.
     *^*/
    virtual void onMiracastServiceClientConnectionError(const std::string &client_mac, const std::string &client_name, const std::string &error_code, const std::string &reason ) {};

    /*^*
     * @class MiracastPlugin::IMiracastPluginListener
     * @function  void onMiracastServiceClientConnectionError(const std::string &client_mac, const std::string &client_name, const std::string &error_code, const std::string &reason )
     * @param src_dev_ip - Source Device IP Address .
     * @param src_dev_mac - Source Device Mac Name .
     * @param src_dev_name - Error code if any failure .
     * @param sink_dev_ip - Our Device P2P ip address .
     * @description  This function is implemented in the MiracastApp.  This is to be called from the registered listeners to inform MiracastApp that the state has changed.
     *^*/
    virtual void onMiracastServiceLaunchRequest(const string &src_dev_ip, const string &src_dev_mac, const string &src_dev_name, const string & sink_dev_ip) {};

    virtual void onMiracastPlayerStateChange(const std::string &client_mac, const std::string &client_name, const std::string &state, const std::string &reason, const std::string &reason_code) {};

    virtual ~IMiracastPluginListener() {};
};
} // namespace MiracastPlugin

#endif // _RDK_PLUGIN_HELPER_H_