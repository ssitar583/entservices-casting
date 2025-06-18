/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
 **/

#include <string> 

 #include "XCastImplementation.h"
  #include <sys/prctl.h>
 #include "UtilsJsonRpc.h"
 #include "UtilsIarm.h"
 
 #include "UtilsSynchroIarm.hpp"
 #include "rfcapi.h"
 #include "UtilsgetRFCConfig.h"


#if defined(SECURITY_TOKEN_ENABLED) && ((SECURITY_TOKEN_ENABLED == 0) || (SECURITY_TOKEN_ENABLED == false))
#define GetSecurityToken(a, b) 0
#define GetToken(a, b, c) 0
#else
#include <securityagent/securityagent.h>
#include <securityagent/SecurityTokenUtil.h>
#endif


#define SERVER_DETAILS "127.0.0.1:9998"
#define NETWORK_CALLSIGN_VER "org.rdk.Network.1"
#define THUNDER_RPC_TIMEOUT 5000
#define MAX_SECURITY_TOKEN_SIZE 1024

 #define API_VERSION_NUMBER_MAJOR 1
 #define API_VERSION_NUMBER_MINOR 0
 #define API_VERSION_NUMBER_PATCH 2

 #define DIAL_MAX_ADDITIONALURL (1024)

 #define LOCATE_CAST_FIRST_TIMEOUT_IN_MILLIS  5000  //5 seconds
#define LOCATE_CAST_SECOND_TIMEOUT_IN_MILLIS 10000  //10 seconds
 
#define SYSTEM_CALLSIGN "org.rdk.System"
#define SYSTEM_CALLSIGN_VER SYSTEM_CALLSIGN".1"
#define SECURITY_TOKEN_LEN_MAX 1024
 
 namespace WPEFramework
 {
     namespace Plugin
     {
        SERVICE_REGISTRATION(XCastImplementation, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        XCastImplementation *XCastImplementation::_instance = nullptr;
        XCastManager* XCastImplementation::m_xcast_manager = nullptr;
        

        static std::vector <DynamicAppConfig*> appConfigListCache;
        static std::mutex m_appConfigMutex;

        #ifdef XCAST_ENABLED_BY_DEFAULT
        bool m_xcastEnable = true;
        #else
        bool m_xcastEnable = false;
        #endif

        #ifdef XCAST_ENABLED_BY_DEFAULT_IN_STANDBY
        bool m_standbyBehavior = true;
        #else
        bool m_standbyBehavior = false;
        #endif

        string m_friendlyName = "";

        static bool m_isDynamicRegistrationsRequired = false;

        static string m_activeInterfaceName = "";

        static bool xcastEnableCache = false;
        static string friendlyNameCache = "Living Room";

        bool powerModeChangeActive = false;

        static bool m_is_restart_req = false;
        static int m_sleeptime = 1;

         XCastImplementation::XCastImplementation()
         : _adminLock(), _service(nullptr),
	 _powerManagerPlugin(this)
         {
             LOGINFO("Create XCastImplementation Instance");
             LOGINFO("##### API VER[%d : %d : %d] #####", API_VERSION_NUMBER_MAJOR,API_VERSION_NUMBER_MINOR,API_VERSION_NUMBER_PATCH);
             m_locateCastTimer.connect( bind( &XCastImplementation::onLocateCastTimer, this ));
             LOGINFO("XCastImplementation::XCastImplementation: m_locateCastTimer = %p", &m_locateCastTimer);
             //XCastImplementation::_instance = this;
         }
 
         XCastImplementation::~XCastImplementation()
         {
            LOGINFO("Destroy XCastImplementation Instance");
            Deinitialize();
            if (nullptr != _service)
            {
                _service->Release();
                _service = nullptr;
            }
         }
 
         /**
          * Register a notification callback
          */
         Core::hresult XCastImplementation::Register(Exchange::IXCast::INotification *notification)
         {
             ASSERT(nullptr != notification);
 
             _adminLock.Lock();
             printf("XCastImplementation::Register: notification = %p", notification);
             LOGINFO("Register notification");
 
             // Make sure we can't register the same notification callback multiple times
             if (std::find(_notificationClients.begin(), _notificationClients.end(), notification) == _notificationClients.end())
             {
                 _notificationClients.push_back(notification);
                 notification->AddRef();
             }
             else
             {
                 LOGERR("same notification is registered already");
             }
 
            _adminLock.Unlock();
 
             return Core::ERROR_NONE;
         }
 
         /**
          * Unregister a notification callback
          */
         Core::hresult XCastImplementation::Unregister(Exchange::IXCast::INotification *notification)
         {
             Core::hresult status = Core::ERROR_GENERAL;
 
             ASSERT(nullptr != notification);
 
             _adminLock.Lock();
 
             // we just unregister one notification once
             auto itr = std::find(_notificationClients.begin(), _notificationClients.end(), notification);
             if (itr != _notificationClients.end())
             {
                 (*itr)->Release();
                 LOGINFO("Unregister notification");
                 _notificationClients.erase(itr);
                 status = Core::ERROR_NONE;
             }
             else
             {
                 LOGERR("notification not found");
             }
 
             _adminLock.Unlock();
 
             return status;
         }
 
         uint32_t XCastImplementation::Configure(PluginHost::IShell* service)
         {
            LOGINFO("Configuring XCast");
            ASSERT(service != nullptr);
            _service = service;
            _service->AddRef();
            InitializePowerManager(service);
            InitializeIARM();
            LOGINFO("XCastImplementation:Configure:Going to Initialize with networkStandbyMode = %d", m_networkStandbyMode);
            Initialize(m_networkStandbyMode);
            getSystemPlugin();
            m_SystemPluginObj->Subscribe<JsonObject>(1000, "onFriendlyNameChanged", &XCastImplementation::onFriendlyNameUpdateHandler, this);
            LOGINFO("XCastImplementation::Configure: Initialization done");
            if (Core::ERROR_NONE == updateSystemFriendlyName())
            {
                LOGINFO("XCast::Initialize m_friendlyName:  %s\n ",m_friendlyName.c_str());
            }
            return Core::ERROR_NONE;
        }

        void XCastImplementation::getUrlFromAppLaunchParams (const char *app_name, const char *payload, const char *query_string, const char *additional_data_url, char *url)
        {
            LOGINFO("getUrlFromAppLaunchParams : Application launch request: appName: %s  query: [%s], payload: [%s], additionalDataUrl [%s]\n",
                app_name, query_string, payload, additional_data_url);

            int url_len = DIAL_MAX_PAYLOAD+DIAL_MAX_ADDITIONALURL+100;
            memset (url, '\0', url_len);
            if(strcmp(app_name,"YouTube") == 0) {
                if ((payload != NULL) && (additional_data_url != NULL)){
                    snprintf( url, url_len, "https://www.youtube.com/tv?%s&additionalDataUrl=%s", payload, additional_data_url);
                }else if (payload != NULL){
                    snprintf( url, url_len, "https://www.youtube.com/tv?%s", payload);
                }else{
                    snprintf( url, url_len, "https://www.youtube.com/tv");
                }
            }
            else if(strcmp(app_name,"YouTubeTV") == 0) {
                if ((payload != NULL) && (additional_data_url != NULL)){
                    snprintf( url, url_len, "https://www.youtube.com/tv/upg?%s&additionalDataUrl=%s", payload, additional_data_url);
                }else if (payload != NULL){
                    snprintf( url, url_len, "https://www.youtube.com/tv/upg?%s", payload);
                }else{
                    snprintf( url, url_len, "https://www.youtube.com/tv/upg?");
                }
            }
            else if(strcmp(app_name,"YouTubeKids") == 0) {
                if ((payload != NULL) && (additional_data_url != NULL)){
                    snprintf( url, url_len, "https://www.youtube.com/tv_kids?%s&additionalDataUrl=%s", payload, additional_data_url);
                }else if (payload != NULL){
                    snprintf( url, url_len, "https://www.youtube.com/tv_kids?%s", payload);
                }else{
                    snprintf( url, url_len, "https://www.youtube.com/tv_kids?");
                }
            }
            else if(strcmp(app_name,"Netflix") == 0) {
                memset( url, 0, url_len );
                strncat( url, "source_type=12", url_len - strlen(url) - 1);
                if(payload != NULL)
                {
                    const char * pUrlEncodedParams;
                    pUrlEncodedParams = payload;
                    if( pUrlEncodedParams ){
                        strncat( url, "&dial=", url_len - strlen(url) - 1);
                        strncat( url, pUrlEncodedParams, url_len - strlen(url) - 1);
                    }
                }

                if(additional_data_url != NULL){
                    strncat(url, "&additionalDataUrl=", url_len - strlen(url) - 1);
                    strncat(url, additional_data_url, url_len - strlen(url) - 1);
                }
            }
            else {
                    memset( url, 0, url_len );
                    url_len -= DIAL_MAX_ADDITIONALURL+1; //save for &additionalDataUrl
                    url_len -= 1; //save for nul byte
                    LOGINFO("query_string=[%s]\r\n", query_string);
                    int has_query = query_string && strlen(query_string);
                    int has_payload = 0;
                    if (has_query) {
                        snprintf(url + strlen(url), url_len, "%s", query_string);
                        url_len -= strlen(query_string);
                    }
                    if(payload && strlen(payload)) {
                        const char payload_key[] = "dialpayload=";
                        if(url_len >= 0){
                            if (has_query) {
                                snprintf(url + strlen(url), url_len, "%s", "&");
                                url_len -= 1;
                            }
                            if(url_len >= 0) {
                                snprintf(url + strlen(url), url_len, "%s%s", payload_key, payload);
                                url_len -= strlen(payload_key) + strlen(payload);
                                has_payload = 1;
                            }
                        }
                        else {
                            LOGINFO("there is not enough room for payload\r\n");
                        }
                    }
                    
                    if(additional_data_url != NULL){
                        if ((has_query || has_payload) && url_len >= 0) {
                            snprintf(url + strlen(url), url_len, "%s", "&");
                            url_len -= 1;
                        }
                        if (url_len >= 0) {
                            snprintf(url + strlen(url), url_len, "additionalDataUrl=%s", additional_data_url);
                            url_len -= strlen(additional_data_url) + 18;
                        }
                    }
                    LOGINFO(" url is [%s]\r\n", url);
            }
        }
        void XCastImplementation::InitializePowerManager(PluginHost::IShell* service)
        {
            LOGINFO("Connect the COM-RPC socket\n");
            _powerManagerPlugin = PowerManagerInterfaceBuilder(_T("org.rdk.PowerManager"))
                .withIShell(service)
                .withRetryIntervalMS(200)
                .withRetryCount(25)
                .createInterface();
            registerEventHandlers();
        }
        void XCastImplementation::registerEventHandlers()
        {
            ASSERT (_powerManagerPlugin);
            LOGINFO("Registering PowerManager event handlers\n");

            if(!_registeredEventHandlers && _powerManagerPlugin) {
                _registeredEventHandlers = true;
                _powerManagerPlugin->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::INetworkStandbyModeChangedNotification>());
                _powerManagerPlugin->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
            }
            LOGINFO("PowerManager event handlers registered\n");
        }
        void XCastImplementation::InitializeIARM()
        {
            Core::hresult retStatus = Core::ERROR_GENERAL;
            PowerState pwrStateCur = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
            PowerState pwrStatePrev = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
            bool nwStandby = false;

            LOGINFO("XCast:: Initialize  plugin called \n");

            ASSERT (_powerManagerPlugin);
            if (_powerManagerPlugin){
                retStatus = _powerManagerPlugin->GetPowerState(pwrStateCur, pwrStatePrev);
                if (Core::ERROR_NONE == retStatus)
                {
                    m_powerState = pwrStateCur;
                    LOGINFO("XCast::m_powerState:%d", m_powerState);
                }

                retStatus = _powerManagerPlugin->GetNetworkStandbyMode(nwStandby);
                if (Core::ERROR_NONE == retStatus)
                {
                    m_networkStandbyMode = nwStandby;
                    LOGINFO("m_networkStandbyMode:%u ",m_networkStandbyMode);
                }
            }
        }

        int XCastImplementation::updateSystemFriendlyName()
        {
            JsonObject params, Result;
            LOGINFO("Entering..!!!");

            if (nullptr == m_SystemPluginObj)
            {
                LOGERR("m_SystemPluginObj not yet instantiated");
                return Core::ERROR_GENERAL;
            }

            uint32_t ret = m_SystemPluginObj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("getFriendlyName"), params, Result);

            if (Core::ERROR_NONE == ret)
            {
                if (Result["success"].Boolean())
                {
                    m_friendlyName = Result["friendlyName"].String();
                }
                else
                {
                    ret = Core::ERROR_GENERAL;
                    LOGERR("getSystemFriendlyName call failed");
                }
            }
            else
            {
                LOGERR("getiSystemFriendlyName call failed E[%u]", ret);
            }
            return ret;
        }
        uint32_t XCastImplementation::Initialize(bool networkStandbyMode)
        {
            LOGINFO("XCastImplementation::Initialize: networkStandbyMode = %d", networkStandbyMode);
            if(nullptr == m_xcast_manager)
            {
                LOGINFO("XCastImplementation::Initialize: m_xcast_manager is null, creating instance");
                m_networkStandbyMode = networkStandbyMode;
                m_xcast_manager  = XCastManager::getInstance();
                LOGINFO("XCastImplementation::Initialize: m_xcast_manager = %p", m_xcast_manager);
                if(nullptr != m_xcast_manager)
                {
                    LOGINFO("XCastImplementation::Initialize: m_xcast_manager is valid, setting service");
                    m_xcast_manager->setService(this);
                    LOGINFO("XCastImplementation::Initialize: going to connect to GDial service");
                    if( false == connectToGDialService())
                    {
                        LOGINFO("XCastImplementation::Initialize: Failed to connect to GDial service");
                        startTimer(LOCATE_CAST_FIRST_TIMEOUT_IN_MILLIS);
                    }
                    LOGINFO("XCastImplementation::Initialize: connectToGDialService done");
                }
            }
            LOGINFO("XCastImplementation::InitializeDone");
            return Core::ERROR_NONE;
        }

        void XCastImplementation::Deinitialize(void)
        {
            if (m_ControllerObj)
            {
                m_ControllerObj->Unsubscribe(THUNDER_RPC_TIMEOUT, _T("statechange"));
                delete m_ControllerObj;
                m_ControllerObj = nullptr;
            }

            if(nullptr != m_xcast_manager)
            {
                stopTimer();
                m_xcast_manager->shutdown();
                m_xcast_manager = nullptr;
            }
        }
        gboolean XCastImplementation::update_friendly_name_timercallback(gpointer userdata)
        {
            XCastImplementation *self = (XCastImplementation *)userdata;
            bool enabledStatus = false;

            if (m_xcastEnable && ( (m_standbyBehavior == true) || ((m_standbyBehavior == false)&&(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON))))
            {
                enabledStatus = true;
            }

            if (self)
            {
                LOGINFO("Updating FriendlyName from Timer [%s] status[%x]",m_friendlyName.c_str(),enabledStatus);
                self->enableCastService(m_friendlyName,enabledStatus);
            }
            else
            {
                LOGERR("instance NULL [%p]",self);
            }
            return G_SOURCE_REMOVE;
        }
        void XCastImplementation::onFriendlyNameUpdateHandler(const JsonObject& parameters)
        {
            string message;
            string value;
            parameters.ToString(message);
            LOGINFO("[Friendly Name Event], %s : %s", __FUNCTION__,message.c_str());

            if (parameters.HasLabel("friendlyName")) {
                value = parameters["friendlyName"].String();

                    m_friendlyName = value;
                    LOGINFO("onFriendlyNameUpdateHandler  :%s",m_friendlyName.c_str());
                    if (m_FriendlyNameUpdateTimerID)
                    {
                        g_source_remove(m_FriendlyNameUpdateTimerID);
                        m_FriendlyNameUpdateTimerID = 0;
                    }
                    m_FriendlyNameUpdateTimerID = g_timeout_add(50, XCastImplementation::update_friendly_name_timercallback, this);
                    if (0 == m_FriendlyNameUpdateTimerID)
                    {
                        bool enabledStatus = false;
                        LOGWARN("Failed to create the timer. Setting friendlyName immediately");
                        if (m_xcastEnable && ( (m_standbyBehavior == true) || ((m_standbyBehavior == false)&&(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON))))
                        {
                            enabledStatus = true;
                        }
                        LOGINFO("Updating FriendlyName [%s] status[%x]",m_friendlyName.c_str(),enabledStatus);
                        enableCastService(m_friendlyName,enabledStatus);
                    }
                    else
                    {
                        LOGINFO("Timer triggered to update friendlyName");
                    }
            }
        }
        void XCastImplementation::getSystemPlugin()
        {
            LOGINFO("Entering..!!!");
            if(nullptr == m_SystemPluginObj)
            {
                string token;
                // TODO: use interfaces and remove token
                auto security = _service->QueryInterfaceByCallsign<PluginHost::IAuthenticate>("SecurityAgent");
                if (nullptr != security)
                {
                    string payload = "http://localhost";
                    if (security->CreateToken( static_cast<uint16_t>(payload.length()),
                                            reinterpret_cast<const uint8_t*>(payload.c_str()),
                                            token) == Core::ERROR_NONE)
                    {
                        LOGINFO("got security token\n");
                    }
                    else
                    {
                        LOGERR("failed to get security token\n");
                    }
                    security->Release();
                }
                else
                {
                    LOGERR("No security agent\n");
                }

                string query = "token=" + token;
                Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T(SERVER_DETAILS)));
                m_SystemPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(SYSTEM_CALLSIGN_VER), (_T(SYSTEM_CALLSIGN_VER)), false, query);
                if (nullptr == m_SystemPluginObj)
                {
                    LOGERR("JSONRPC: %s: initialization failed", SYSTEM_CALLSIGN_VER);
                }
                else
                {
                    LOGINFO("JSONRPC: %s: initialization ok", SYSTEM_CALLSIGN_VER);
                }
            }
            LOGINFO("Exiting..!!!");
        }
        bool XCastImplementation::connectToGDialService(void)
        {
            LOGINFO("XCastImplementation::connectToGDialService: Attempting to connect to GDial service");
            std::string interface,ipaddress;
            bool status = false;
            LOGINFO("XCastImplementation::connectToGDialService: Getting default interface and IP address");
            getDefaultNameAndIPAddress(interface,ipaddress);
            LOGINFO("XCastImplementation::connectToGDialService::Got default interface and IP address");
            if (!interface.empty())
            {
                LOGINFO("XCastImplementation::connectToGDialService: Initializing XCastManager with interface: %s", interface.c_str());
                status = m_xcast_manager->initialize(interface,m_networkStandbyMode);
                LOGINFO("XCastImplementation::connectToGDialService: XCastManager initialized with status: %d", status);
                if( true == status)
                {
                    LOGINFO("XCastImplementation::connectToGDialService: Setting active interface name to: %s", interface.c_str());
                    m_activeInterfaceName = interface;
                }
                LOGINFO("XCastImplementation::connectToGDialService: Set active interface name to: %s", m_activeInterfaceName.c_str());
            }
            LOGINFO("GDialService[%u]IF[%s]IP[%s]",status,interface.c_str(),ipaddress.c_str());
            return status;
        }

        bool XCastImplementation::getDefaultNameAndIPAddress(std::string& interface, std::string& ipaddress)
        {
            LOGINFO("XCastImplementation::getDefaultNameAndIPAddress: Getting default interface and IP address");
            // Read host IP from thunder service and save it into external_network.json
            JsonObject Params, Result, Params0, Result0;
            bool returnValue = false;

            getThunderPlugins();

            if (nullptr == m_NetworkPluginObj)
            {
                LOGINFO("WARN::Unable to get Network plugin handle not yet");
                return false;
            }

            uint32_t ret = m_NetworkPluginObj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("getDefaultInterface"), Params0, Result0);
            if (Core::ERROR_NONE == ret)
            {
                if (Result0["success"].Boolean())
                {
                    interface = Result0["interface"].String();
                }
                else
                {
                    LOGERR("XCastImplementation: failed to load interface");
                }
            }

            Params.Set(_T("interface"), interface);
            Params.Set(_T("ipversion"), string("IPv4"));

            ret = m_NetworkPluginObj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("getIPSettings"), Params, Result);
            if (Core::ERROR_NONE == ret)
            {
                if (Result["success"].Boolean())
                {
                    ipaddress = Result["ipaddr"].String();
                    LOGINFO("ipAddress = %s",ipaddress.c_str());
                    returnValue = true;
                }
                else
                {
                    LOGERR("getIPSettings failed");
                }
            }
            else
            {
                LOGERR("Failed to invoke method \"getIPSettings\". Error: %d",ret);
            }
            LOGINFO("XCastImplementation::getDefaultNameAndIPAddress: Returning interface: %s, ipaddress: %s, returnValue: %d", 
                     interface.c_str(), ipaddress.c_str(), returnValue);
            return returnValue;
        }

        std::string XCastImplementation::getSecurityToken()
        {
            if (nullptr == _service)
            {
                return (std::string(""));
            }

            std::string token;
            auto security = _service->QueryInterfaceByCallsign<PluginHost::IAuthenticate>("SecurityAgent");
            if (nullptr != security)
            {
                std::string payload = "http://localhost";
                if (security->CreateToken(static_cast<uint16_t>(payload.length()),
                                            reinterpret_cast<const uint8_t *>(payload.c_str()),
                                            token) == Core::ERROR_NONE)
                {
                    LOGINFO("got security token - %s", token.empty() ? "" : token.c_str());
                }
                else
                {
                    LOGERR("failed to get security token");
                }
                security->Release();
            }
            else
            {
                LOGERR("No security agent\n");
            }

            std::string query = "token=" + token;
            Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T(SERVER_DETAILS)));
            return query;
        }

        void XCastImplementation::threadPowerModeChangeEvent(void)
        {
            powerModeChangeActive = true;
            LOGINFO(" threadPowerModeChangeEvent m_standbyBehavior:%d , m_powerState:%d ",m_standbyBehavior,m_powerState);
            if(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON)
            {
                m_sleeptime = 1;
                if (m_is_restart_req)
                {
                    Deinitialize();
                    sleep(1);
                    Initialize(m_networkStandbyMode);
                    m_is_restart_req = false;
                }
            }
            else if (m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_DEEP_SLEEP )
            {
                m_sleeptime = 3;
                m_is_restart_req = true; //After DEEPSLEEP, restart xdial again for next transition.
            }

            if(m_standbyBehavior == false)
            {
                if(m_xcastEnable && ( m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON))
                    enableCastService(m_friendlyName,true);
                else
                    enableCastService(m_friendlyName,false);
            }
            powerModeChangeActive = false;
        }

        void XCastImplementation::networkStandbyModeChangeEvent(void)
        {
            LOGINFO("m_networkStandbyMode:%u ",m_networkStandbyMode);
            SetNetworkStandbyMode(m_networkStandbyMode);
        
        }

        // Thunder plugins communication
        void XCastImplementation::getThunderPlugins()
        {
            string token = getSecurityToken();

            if (nullptr == m_ControllerObj)
            {
                if(token.empty())
                {
                    m_ControllerObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>("", "", false);
                }
                else
                {
                    m_ControllerObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>("","", false, token);
                }

                if (nullptr != m_ControllerObj)
                {
                    LOGINFO("JSONRPC: Controller: initialization ok");
                    bool isSubscribed = false;
                    auto ev_ret = m_ControllerObj->Subscribe<JsonObject>(THUNDER_RPC_TIMEOUT, _T("statechange"),&XCastImplementation::eventHandler_pluginState,this);
                    if (ev_ret == Core::ERROR_NONE)
                    {
                        LOGINFO("Controller - statechange event subscribed");
                        isSubscribed = true;
                    }
                    else
                    {
                        LOGERR("Controller - statechange event failed to subscribe : %d",ev_ret);
                    }

                    if (!isPluginActivated(NETWORK_CALLSIGN_VER))
                    {
                        activatePlugin(NETWORK_CALLSIGN_VER);
                        _networkPluginState = PLUGIN_DEACTIVATED;
                    }
                    else
                    {
                        _networkPluginState = PLUGIN_ACTIVATED;
                    }

                    if (false == isSubscribed)
                    {
                        delete m_ControllerObj;
                        m_ControllerObj = nullptr;
                    }
                }
                else
                {
                    LOGERR("Unable to get Controller obj");
                }
            }

            if (nullptr == m_NetworkPluginObj)
            {
                std::string callsign = NETWORK_CALLSIGN_VER;
                if(token.empty())
                {
                    m_NetworkPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(NETWORK_CALLSIGN_VER),"");
                }
                else
                {
                    m_NetworkPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(NETWORK_CALLSIGN_VER),"", false, token);
                }
    
                if (nullptr == m_NetworkPluginObj)
                {
                    LOGERR("JSONRPC: %s: initialization failed", NETWORK_CALLSIGN_VER);
                }
                else
                {
                    LOGINFO("JSONRPC: %s: initialization ok", NETWORK_CALLSIGN_VER);
                    // Network monitor so we can know ip address of host inside container
                    if(m_NetworkPluginObj)
                    {
                        bool isSubscribed = false;
                        auto ev_ret = m_NetworkPluginObj->Subscribe<JsonObject>(THUNDER_RPC_TIMEOUT, _T("onDefaultInterfaceChanged"), &XCastImplementation::eventHandler_onDefaultInterfaceChanged,this);
                        if ( Core::ERROR_NONE == ev_ret )
                        {
                            LOGINFO("Network - Default Interface changed event : subscribed");
                            ev_ret = m_NetworkPluginObj->Subscribe<JsonObject>(THUNDER_RPC_TIMEOUT, _T("onIPAddressStatusChanged"), &XCastImplementation::eventHandler_ipAddressChanged,this);
                            if ( Core::ERROR_NONE == ev_ret )
                            {
                                LOGINFO("Network - IP address status changed event : subscribed");
                                isSubscribed = true;
                            }
                            else
                            {
                                LOGERR("Network - IP address status changed event : failed to subscribe : %d", ev_ret);
                            }
                        }
                        else
                        {
                            LOGERR("Network - Default Interface changed event : failed to subscribe : %d", ev_ret);
                        }
                        if (false == isSubscribed)
                        {
                            LOGERR("Network events subscription failed");
                            delete m_NetworkPluginObj;
                            m_NetworkPluginObj = nullptr;
                        }
                    }
                }
            }
            LOGINFO("Exiting..!!!");
        }

        void XCastImplementation::eventHandler_pluginState(const JsonObject& parameters)
        {
            LOGINFO("Plugin state changed");

            if( 0 == strncmp(parameters["callsign"].String().c_str(), NETWORK_CALLSIGN_VER, parameters["callsign"].String().length()))
            {
                if ( 0 == strncmp( parameters["state"].String().c_str(),"Deactivated", parameters["state"].String().length()))
                {
                    LOGINFO("%s plugin got deactivated with reason : %s",parameters["callsign"].String().c_str(), parameters["reason"].String().c_str());
                    _instance->activatePlugin(parameters["callsign"].String());
                }
            }
            else
            {
                LOGINFO("Plugin %s state changed to %s with reason %s", parameters["callsign"].String().c_str(), parameters["state"].String().c_str(), parameters["reason"].String().c_str());
            }
        }

        int XCastImplementation::activatePlugin(string callsign)
        {
            JsonObject result, params;
            params["callsign"] = callsign;
            int rpcRet = Core::ERROR_GENERAL;
            if (nullptr != m_ControllerObj)
            {
                rpcRet =  m_ControllerObj->Invoke("activate", params, result);
                if(Core::ERROR_NONE == rpcRet)
                    {
                    LOGINFO("Activated %s plugin", callsign.c_str());
                }
                else
                {
                    LOGERR("Could not activate %s plugin.  Failed with %d", callsign.c_str(), rpcRet);
                }
            }
            else
            {
                LOGERR("Controller not active");
            }
            return rpcRet;
        }

        int XCastImplementation::deactivatePlugin(string callsign)
        {
            JsonObject result, params;
            params["callsign"] = callsign;
            int rpcRet = Core::ERROR_GENERAL;

            if (m_NetworkPluginObj && (callsign == NETWORK_CALLSIGN_VER))
            {
                m_NetworkPluginObj->Unsubscribe(THUNDER_RPC_TIMEOUT, _T("onDefaultInterfaceChanged"));
                m_NetworkPluginObj->Unsubscribe(THUNDER_RPC_TIMEOUT, _T("onIPAddressStatusChanged"));
                delete m_NetworkPluginObj;
                m_NetworkPluginObj = nullptr;
            }

            if (nullptr != m_ControllerObj)
            {
                rpcRet =  m_ControllerObj->Invoke("deactivate", params, result);
                if(Core::ERROR_NONE == rpcRet)
                {
                    LOGINFO("Deactivated %s plugin", callsign.c_str());
                }
                else
                {
                    LOGERR("Could not deactivate %s plugin.  Failed with %d", callsign.c_str(), rpcRet);
                }
            }
            else
            {
                LOGERR("Controller not active");
            }
            return rpcRet;
        }

        bool XCastImplementation::isPluginActivated(string callsign)
        {
            std::string method = "status@" + callsign;
            bool isActive = false;
            Core::JSON::ArrayType<PluginHost::MetaData::Service> response;
            if (nullptr != m_ControllerObj)
            {
                int ret  = m_ControllerObj->Get(THUNDER_RPC_TIMEOUT, method, response);
                isActive = (ret == Core::ERROR_NONE && response.Length() > 0 && response[0].JSONState == PluginHost::IShell::ACTIVATED);
                LOGINFO("Plugin \"%s\" is %s, error=%d", callsign.c_str(), isActive ? "active" : "not active", ret);
            }
            else
            {
                LOGERR("Controller not active");
            }
            return isActive;
        }

        void XCastImplementation::startTimer(int interval)
        {
            stopTimer();
            m_locateCastTimer.start(interval);
        }

        void XCastImplementation::stopTimer()
        {
            LOGINFO("XCastImplementation::stopTimer()");
            if (m_locateCastTimer.isActive())
            {
                m_locateCastTimer.stop();
            }
            LOGINFO("XCastImplementation::stopTimer()-stopped");
        }

        bool XCastImplementation::isTimerActive()
        {
            return (m_locateCastTimer.isActive());
        }


        void XCastImplementation::onLocateCastTimer()
        {
            LOGINFO("XCastImplementation::onLocateCastTimer()");
            if( false == connectToGDialService())
            {
                LOGINFO("Retry after 10 sec...");
                m_locateCastTimer.setInterval(LOCATE_CAST_SECOND_TIMEOUT_IN_MILLIS);
                return ;
            }
            stopTimer();    
            LOGINFO(" XCastImplementation::onLocateCastTimer-After stopTimer");
            if ((NULL != m_xcast_manager) && m_isDynamicRegistrationsRequired )
            {
                LOGINFO(" XCastImplementation::onLocateCastTimer-Inside 9f - if ((NULL != m_xcast_manager) && m_isDynamicRegistrationsRequired )");
                std::vector<DynamicAppConfig*> appConfigList;
                lock_guard<mutex> lck(m_appConfigMutex);
                appConfigList = appConfigListCache;
                dumpDynamicAppCacheList(string("CachedAppsFromTimer"), appConfigList);
                LOGINFO("> calling registerApplications");
                m_xcast_manager->registerApplications (appConfigList);
            }
            else {
                LOGINFO("m_xcast_manager: %p: m_isDynamicRegistrationsRequired[%u]",
                        m_xcast_manager,
                        m_isDynamicRegistrationsRequired);
            }
            LOGINFO("XCastImplementation::onLocateCastTimer: m_xcastEnable = %d, m_xcast_manager-%p",m_xcastEnable,m_xcast_manager);
            m_xcast_manager->enableCastService(friendlyNameCache,xcastEnableCache);
            LOGINFO("XCast::onLocateCastTimer : Timer still active ? %d ",m_locateCastTimer.isActive());
        }


        uint32_t XCastImplementation::enableCastService(string friendlyname,bool enableService) 
        {
            LOGINFO("XcastService::enableCastService: ARGS = %s : %d", friendlyname.c_str(), enableService);
            if (nullptr != m_xcast_manager)
            {
                m_xcast_manager->enableCastService(friendlyname,enableService);
            }
            xcastEnableCache = enableService;
            friendlyNameCache = friendlyname;
            return 0;
        }

        void XCastImplementation::dumpDynamicAppCacheList(string strListName, std::vector<DynamicAppConfig*> appConfigList)
        {
            LOGINFO ("=================Current Apps[%s] size[%d] ===========================", strListName.c_str(), (int)appConfigList.size());
            for (DynamicAppConfig* pDynamicAppConfig : appConfigList)
            {
                LOGINFO ("Apps: appName:%s, prefixes:%s, cors:%s, allowStop:%d, query:%s, payload:%s",
                            pDynamicAppConfig->appName,
                            pDynamicAppConfig->prefixes,
                            pDynamicAppConfig->cors,
                            pDynamicAppConfig->allowStop,
                            pDynamicAppConfig->query,
                            pDynamicAppConfig->payload);
            }
            LOGINFO ("=================================================================");
        }  

        Core::hresult XCastImplementation::ApplicationStateChanged(const string& applicationName, const string& state, const string& applicationId, const string& error) { 
            LOGINFO("ApplicationStateChanged  ARGS = %s : %s : %s : %s ", applicationName.c_str(), applicationId.c_str() , state.c_str() , error.c_str());
            uint32_t status = Core::ERROR_GENERAL;
            if(!applicationName.empty() && !state.empty() && (nullptr != m_xcast_manager))
            {
                LOGINFO("XcastService::ApplicationStateChanged  ARGS = %s : %s : %s : %s ", applicationName.c_str(), applicationId.c_str() , state.c_str() , error.c_str());
                m_xcast_manager->applicationStateChanged(applicationName, state, applicationId, error);
                status = Core::ERROR_NONE;
            }
            return status;
        }
		Core::hresult XCastImplementation::GetProtocolVersion(string &protocolVersion  ) { 
            LOGINFO("XcastService::getProtocolVersion");
            if (nullptr != m_xcast_manager)
            {
                protocolVersion = m_xcast_manager->getProtocolVersion();
            }
            return Core::ERROR_NONE;
        }
		uint32_t XCastImplementation::SetNetworkStandbyMode(bool networkStandbyMode) { 
            LOGINFO("nwStandbymode: %d", networkStandbyMode);
            if (nullptr != m_xcast_manager)
            {
                m_xcast_manager->setNetworkStandbyMode(networkStandbyMode);
                m_networkStandbyMode = networkStandbyMode;
            }
            return 0;
        }


         void XCastImplementation::dispatchEvent(Event event, string callsign, const JsonObject &params)
        {
            Core::IWorkerPool::Instance().Submit(Job::Create(this, event, callsign, params));
        }

        void XCastImplementation::Dispatch(Event event, string callsign, const JsonObject params)
        {
            _adminLock.Lock();
            std::list<Exchange::IXCast::INotification*>::iterator index(_notificationClients.begin());
            while (index != _notificationClients.end())
            {
                switch(event)
                {
                    case LAUNCH_REQUEST_WITH_PARAMS:
                    {
                        string appName = params["appName"].String();
                        string strPayLoad = params["strPayLoad"].String();
                        string strQuery = params["strQuery"].String();
                        string strAddDataUrl = params["strAddDataUrl"].String();
                        (*index)->OnApplicationLaunchRequestWithLaunchParam(appName,strPayLoad,strQuery,strAddDataUrl);
                    }
                    break;
                    case LAUNCH_REQUEST:
                    {
                        string appName = params["appName"].String();
                        string parameter = params["parameter"].String();
                        (*index)->OnApplicationLaunchRequest(appName,parameter);
                    }
                    break;
                    case STOP_REQUEST:
                    {
                        string appName = params["appName"].String();
                        string appId = params["appId"].String();
                        (*index)->OnApplicationStopRequest(appName,appId);
                    }
                    break;
                    case HIDE_REQUEST:
                    {
                        string appName = params["appName"].String();
                        string appId = params["appId"].String();
                        (*index)->OnApplicationHideRequest(appName,appId);
                    }
                    break;
                    case STATE_REQUEST:
                    {
                        string appName = params["appName"].String();
                        string appId = params["appId"].String();
                        (*index)->OnApplicationStateRequest(appName,appId);
                    }
                    break;
                    case RESUME_REQUEST:
                    {
                        string appName = params["appName"].String();
                        string appId = params["appId"].String();
                        (*index)->OnApplicationResumeRequest(appName,appId);
                    }
                    break;
                    case UPDATE_POWERSTATE:
                    {
                        string powerState = params["powerstate"].String();
                        (*index)->OnUpdatePowerStateRequest(powerState);
                    }
                    break;
                    default: break;
                }
                ++index;
            }
            _adminLock.Unlock();
        }

        
         void XCastImplementation::onXcastApplicationLaunchRequestWithLaunchParam (string appName, string strPayLoad, string strQuery, string strAddDataUrl)
        {
            LOGINFO("Notify LaunchRequestWithParam, appName: %s, strPayLoad: %s, strQuery: %s, strAddDataUrl: %s",
                    appName.c_str(),strPayLoad.c_str(),strQuery.c_str(),strAddDataUrl.c_str());
            JsonObject params;
            params["appName"]  = appName.c_str();
            params["strPayLoad"]  = strPayLoad.c_str();
            params["strQuery"]  = strQuery.c_str();
            params["strAddDataUrl"]  = strAddDataUrl.c_str();
            dispatchEvent(LAUNCH_REQUEST_WITH_PARAMS, "", params);
        }

        void XCastImplementation::onXcastApplicationLaunchRequest(string appName, string parameter)
        {
            LOGINFO("Notify LaunchRequest, appName: %s, parameter: %s",appName.c_str(),parameter.c_str());
            JsonObject params;
            params["appName"]  = appName.c_str();
            params["parameter"]  = parameter.c_str();
            dispatchEvent(LAUNCH_REQUEST, "", params);
        }

        void XCastImplementation::onXcastApplicationStopRequest(string appName, string appId)
        {
            LOGINFO("Notify StopRequest, appName: %s, appId: %s",appName.c_str(),appId.c_str());
            JsonObject params;
            params["appName"]  = appName.c_str();
            params["appId"]  = appId.c_str();
            dispatchEvent(STOP_REQUEST, "", params);
        }

        void XCastImplementation::onXcastApplicationHideRequest(string appName, string appId)
        {
            LOGINFO("Notify StopRequest, appName: %s, appId: %s",appName.c_str(),appId.c_str());
            JsonObject params;
            params["appName"]  = appName.c_str();
            params["appId"]  = appId.c_str();
            dispatchEvent(HIDE_REQUEST, "", params);
        }

        void XCastImplementation::onXcastApplicationResumeRequest(string appName, string appId)
        {
            LOGINFO("Notify StopRequest, appName: %s, appId: %s",appName.c_str(),appId.c_str());
            JsonObject params;
            params["appName"]  = appName.c_str();
            params["appId"]  = appId.c_str();
            dispatchEvent(RESUME_REQUEST, "", params);
        }

        void XCastImplementation::onXcastApplicationStateRequest(string appName, string appId)
        {
            LOGINFO("Notify StopRequest, appName: %s, appId: %s",appName.c_str(),appId.c_str());
            JsonObject params;
            params["appName"]  = appName.c_str();
            params["appId"]  = appId.c_str();
            dispatchEvent(STATE_REQUEST, "", params);
        }

        void XCastImplementation::onXcastUpdatePowerStateRequest(string powerState)
        {
            LOGINFO("Notify updatePowerState, state: %s",powerState.c_str());
            JsonObject params;
            params["powerstate"]  = powerState.c_str();
            dispatchEvent(UPDATE_POWERSTATE, "", params);
        }

        void XCastImplementation::onGDialServiceStopped(void)
        {
            LOGINFO("Timer triggered to monitor the GDial, check after 5sec");
            startTimer(LOCATE_CAST_FIRST_TIMEOUT_IN_MILLIS);
        }










		Core::hresult XCastImplementation::SetManufacturerName(string manufacturername) { 
            uint32_t status = Core::ERROR_GENERAL;

            LOGINFO("ManufacturerName : %s", manufacturername.c_str());

            if (nullptr != m_xcast_manager)
            {
                m_xcast_manager->setManufacturerName(manufacturername);
                status = Core::ERROR_NONE;
            }
            return status;
        }
		Core::hresult XCastImplementation::GetManufacturerName(string &manufacturername  ) { 
            if (nullptr != m_xcast_manager)
            {
                manufacturername = m_xcast_manager->getManufacturerName();
                LOGINFO("Manufacturer[%s]", manufacturername.c_str());
            }
            return Core::ERROR_NONE;
        }
		Core::hresult XCastImplementation::SetModelName(string modelname) { 
            uint32_t status = Core::ERROR_GENERAL;

            LOGINFO("ModelName : %s", modelname.c_str());

            if (nullptr != m_xcast_manager)
            {
                m_xcast_manager->setModelName(modelname);
                status = Core::ERROR_NONE;
            }
            return status;
        }
		Core::hresult XCastImplementation::GetModelName(string &modelname  ) { 
            if (nullptr != m_xcast_manager)
            {
                modelname = m_xcast_manager->getModelName();
                LOGINFO("Model[%s]", modelname.c_str());
            }
            return Core::ERROR_NONE;
         }

		Core::hresult XCastImplementation::SetEnabled(bool enabled) { 
            LOGINFO("XcastService::setEnabled ");
            uint32_t result = Core::ERROR_NONE;
            m_xcastEnable= enabled;
            if (m_xcastEnable && ( (m_standbyBehavior == true) || ((m_standbyBehavior == false)&&(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON))))
            {
                enabled = true;
            }
            else
            {
                enabled = false;
            }

            result = enableCastService(m_friendlyName,enabled);

        
            return result;
         }
		Core::hresult XCastImplementation::GetEnabled(bool &enabled , bool &success ) { 
            LOGINFO("XcastService::getEnabled ");
            enabled = m_xcastEnable;
            success = true;
            return Core::ERROR_NONE;
        }
		Core::hresult XCastImplementation::SetStandbyBehavior(string standbybehavior) { 
            LOGINFO("XcastService::setStandbyBehavior \n ");
            bool enabled = false;
            if (standbybehavior == "active")
            {
                enabled = true;
            }
            else
            {
                return Core::ERROR_GENERAL;
            }
            m_standbyBehavior = enabled;
            LOGINFO("XcastService::setStandbyBehavior m_standbyBehavior : %d", m_standbyBehavior);
            return Core::ERROR_NONE;
         }
		Core::hresult XCastImplementation::GetStandbyBehavior(string &standbybehavior , bool &success  ) { 
            LOGINFO("XcastService::getStandbyBehavior m_standbyBehavior :%d",m_standbyBehavior);
            if(m_standbyBehavior)
                standbybehavior = "active";
            else
                standbybehavior = "inactive";
            success = true;
            return Core::ERROR_NONE;
        }
		Core::hresult XCastImplementation::SetFriendlyName(string friendlyname) { 
            LOGINFO("XcastService::setFriendlyName \n ");
            uint32_t result = Core::ERROR_GENERAL;
            bool enabledStatus = false;


            if (!friendlyname.empty())
            {
                m_friendlyName = friendlyname;
                LOGINFO("XcastService::setFriendlyName  :%s",m_friendlyName.c_str());
                if (m_xcastEnable && ( (m_standbyBehavior == true) || ((m_standbyBehavior == false)&&(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON))))
                    {
                        enabledStatus = true;                
                    }
                    else
                    {
                        enabledStatus = false;
                    }
                    enableCastService(m_friendlyName,enabledStatus);
                    result = Core::ERROR_NONE;
            }
            return result;
        }
		Core::hresult XCastImplementation::GetFriendlyName(string &friendlyname , bool &success ) { 
            LOGINFO("XcastService::getFriendlyName :%s ",m_friendlyName.c_str());
            friendlyname = m_friendlyName;
            success = true;
            return Core::ERROR_NONE;
        }
		Core::hresult XCastImplementation::GetApiVersionNumber(uint32_t &version , bool &success) { 
            version = API_VERSION_NUMBER_MAJOR;
            success = true;
            return Core::ERROR_NONE;
        }


        bool deleteFromDynamicAppCache(vector<string>& appsToDelete) {
            bool ret = true;
            {lock_guard<mutex> lck(m_appConfigMutex);
                /*Check if existing cache need to be updated*/
                std::vector<int> entriesTodelete;
                for (string appNameToDelete : appsToDelete) {
                    bool found = false;
                    int index = 0;
                    for (DynamicAppConfig* pDynamicAppConfigOld : appConfigListCache) {
                        if (0 == strcmp(pDynamicAppConfigOld->appName, appNameToDelete.c_str())){
                            entriesTodelete.push_back(index);
                            found = true;
                            break;
                        }
                        index ++;
                    }
                    if (!found) {
                        LOGINFO("%s not existing in the dynamic cache", appNameToDelete.c_str());
                    }
                }
                std::sort(entriesTodelete.begin(), entriesTodelete.end(), std::greater<int>());
                for (int indexToDelete : entriesTodelete) {
                    LOGINFO("Going to delete the entry: %d from appConfigListCache  size: %d", indexToDelete, (int)appConfigListCache.size());
                    //Delete the old unwanted item here.
                    DynamicAppConfig* pDynamicAppConfigOld = appConfigListCache[indexToDelete];
                    appConfigListCache.erase (appConfigListCache.begin()+indexToDelete);
                    free (pDynamicAppConfigOld); pDynamicAppConfigOld = NULL;
                }
                entriesTodelete.clear();

            }
            //Even if requested app names not there return true.
            return ret;
        }

        

	    Core::hresult XCastImplementation::RegisterApplications(Exchange::IXCast::IApplicationInfoIterator* const appInfoList) { 

            LOGINFO("XcastService::registerApplications");
            std::vector <DynamicAppConfig*> appConfigListTemp;
            uint32_t status = Core::ERROR_GENERAL;

            if ((nullptr != m_xcast_manager) && (appInfoList))
            {
                enableCastService(m_friendlyName,false);

                m_isDynamicRegistrationsRequired = true;
                Exchange::IXCast::ApplicationInfo entry{};

                while (appInfoList->Next(entry) == true)
                {
                    DynamicAppConfig* pDynamicAppConfig = (DynamicAppConfig*) malloc (sizeof(DynamicAppConfig));
                    if (pDynamicAppConfig)
                    {
                        memset ((void*)pDynamicAppConfig, '\0', sizeof(DynamicAppConfig));
                        memset (pDynamicAppConfig->appName, '\0', sizeof(pDynamicAppConfig->appName));
                        memset (pDynamicAppConfig->prefixes, '\0', sizeof(pDynamicAppConfig->prefixes));
                        memset (pDynamicAppConfig->cors, '\0', sizeof(pDynamicAppConfig->cors));
                        memset (pDynamicAppConfig->query, '\0', sizeof(pDynamicAppConfig->query));
                        memset (pDynamicAppConfig->payload, '\0', sizeof(pDynamicAppConfig->payload));

                        strncpy (pDynamicAppConfig->appName, entry.appName.c_str(), sizeof(pDynamicAppConfig->appName) - 1);
                        strncpy (pDynamicAppConfig->prefixes, entry.prefixes.c_str(), sizeof(pDynamicAppConfig->prefixes) - 1);
                        strncpy (pDynamicAppConfig->cors, entry.cors.c_str(), sizeof(pDynamicAppConfig->cors) - 1);
                        pDynamicAppConfig->allowStop = entry.allowStop;
                        strncpy (pDynamicAppConfig->query, entry.query.c_str(), sizeof(pDynamicAppConfig->query) - 1);
                        strncpy (pDynamicAppConfig->payload, entry.payload.c_str(), sizeof(pDynamicAppConfig->payload) - 1);
                        appConfigListTemp.push_back (pDynamicAppConfig);
                    }
                }
                dumpDynamicAppCacheList(string("appConfigListTemp"), appConfigListTemp);

                vector<string> appsToDelete;
                for (DynamicAppConfig* pDynamicAppConfig : appConfigListTemp) {
                    appsToDelete.push_back(string(pDynamicAppConfig->appName));
                }
                deleteFromDynamicAppCache (appsToDelete);

                LOGINFO("appConfigList count: %d", (int)appConfigListTemp.size());

                m_isDynamicRegistrationsRequired = true;

                m_xcast_manager->registerApplications(appConfigListTemp);
                {
                    lock_guard<mutex> lck(m_appConfigMutex);
                    for (DynamicAppConfig* pDynamicAppConfigOld : appConfigListCache)
                    {
                        free (pDynamicAppConfigOld);
                        pDynamicAppConfigOld = NULL;
                    }
                    appConfigListCache.clear();
                    appConfigListCache = appConfigListTemp;
                    dumpDynamicAppCacheList(string("registeredAppsFromUser"), appConfigListCache);
                }
                status = Core::ERROR_NONE;
            }
            return status;
            return 0;
        }

        void XCastImplementation::updateNWConnectivityStatus(std::string nwInterface, bool nwConnected, std::string ipaddress)
        {
            bool status = false;
            if(nwConnected)
            {
                if(nwInterface.compare("ETHERNET")==0){
                    LOGINFO("Connectivity type Ethernet");
                    status = true;
                }
                else if(nwInterface.compare("WIFI")==0){
                    LOGINFO("Connectivity type WIFI");
                    status = true;
                }
                else{
                    LOGERR("Connectivity type Unknown");
                }
            }
            else
            {
                LOGERR("Connectivity type Unknown");
            }
            if (!m_locateCastTimer.isActive())
            {
                if (status)
                {
                    if ((0 != nwInterface.compare(m_activeInterfaceName)) ||
                        ((0 == nwInterface.compare(m_activeInterfaceName)) && !ipaddress.empty()))
                    {
                        if (m_xcast_manager)
                        {
                            LOGINFO("Stopping GDialService");
                            m_xcast_manager->deinitialize();
                        }
                        LOGINFO("Timer started to monitor active interface");
                        startTimer(LOCATE_CAST_FIRST_TIMEOUT_IN_MILLIS);
                    }
                }
            }
        }

        void XCastImplementation::eventHandler_onDefaultInterfaceChanged(const JsonObject& parameters)
        {
            std::string oldInterfaceName, newInterfaceName;
            oldInterfaceName = parameters["oldInterfaceName"].String();
            newInterfaceName = parameters["newInterfaceName"].String();

            LOGINFO("XCast onDefaultInterfaceChanged, old interface: %s, new interface: %s", oldInterfaceName.c_str(), newInterfaceName.c_str());
            updateNWConnectivityStatus(newInterfaceName.c_str(), true);
        }

        void XCastImplementation::eventHandler_ipAddressChanged(const JsonObject& parameters)
        {
            if(parameters["status"].String() == "ACQUIRED")
            {
                string interface = parameters["interface"].String();
                string ipv4Address = parameters["ip4Address"].String();
                bool isAcquired = false;
                if (!ipv4Address.empty())
                {
                    isAcquired = true;
                }
                updateNWConnectivityStatus(interface.c_str(), isAcquired, ipv4Address.c_str());
            }
        }
     } // namespace Plugin
 } // namespace WPEFramework
