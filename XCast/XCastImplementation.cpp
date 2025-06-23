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
 #define API_VERSION_NUMBER_PATCH 9

 
 #define LOCATE_CAST_FIRST_TIMEOUT_IN_MILLIS  5000  //5 seconds
#define LOCATE_CAST_SECOND_TIMEOUT_IN_MILLIS 10000  //10 seconds
 
 
 namespace WPEFramework
 {
     namespace Plugin
     {
         SERVICE_REGISTRATION(XCastImplementation, 1, 0);
         XCastImplementation *XCastImplementation::_instance = nullptr;    
         XCastManager* XCastImplementation::m_xcast_manager = nullptr;
         static std::vector <DynamicAppConfig*> appConfigListCache;
        static std::mutex m_appConfigMutex;
        static bool xcastEnableCache = false;

        #ifdef XCAST_ENABLED_BY_DEFAULT
        bool XCastImplementation::m_xcastEnable = true;
        #else
        bool XCastImplementation::m_xcastEnable = false;
        #endif

        #ifdef XCAST_ENABLED_BY_DEFAULT_IN_STANDBY
        bool XCastImplementation::m_standbyBehavior = true;
        #else
        bool XCastImplementation::m_standbyBehavior = false;
        #endif

        bool m_networkStandbyMode = false;
        string m_friendlyName = "";

        bool powerModeChangeActive = false;

        static string friendlyNameCache = "Living Room";
        static string m_activeInterfaceName = "";
        static bool m_isDynamicRegistrationsRequired = false;

         static bool m_is_restart_req = false;
        static int m_sleeptime = 1;

         XCastImplementation::XCastImplementation()
         : _service(nullptr),
         _registeredEventHandlers(false),
         _pwrMgrNotification(*this), _adminLock()
         {
             LOGINFO("Create XCastImplementation Instance");
             m_locateCastTimer.connect( bind( &XCastImplementation::onLocateCastTimer, this ));
             XCastImplementation::_instance = this;
         }
 
         XCastImplementation::~XCastImplementation()
         {
             LOGINFO("Call XCastImplementation destructor\n");
             Deinitialize();
             if(_service != nullptr)
             {
                _service->Release();
             }
             XCastImplementation::_instance = nullptr;
             _service = nullptr;
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
             if (std::find(_xcastNotification.begin(), _xcastNotification.end(), notification) == _xcastNotification.end())
             {
                 _xcastNotification.push_back(notification);
                 notification->AddRef();
             }
             else
             {
                 LOGERR("same notification is registered already");
             }
 
            _adminLock.Unlock();

            LOGINFO("Registered a notification on the xcast inprocess %p", notification);
 
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
             auto itr = std::find(_xcastNotification.begin(), _xcastNotification.end(), notification);
             if (itr != _xcastNotification.end())
             {
                 (*itr)->Release();
                 LOGINFO("Unregister notification");
                 _xcastNotification.erase(itr);
                 status = Core::ERROR_NONE;
             }
             else
             {
                 LOGERR("notification not found");
             }
 
             _adminLock.Unlock();
 
             return status;
         }

            uint32_t XCastImplementation::Initialize(bool networkStandbyMode)
            {
                if(nullptr == m_xcast_manager)
                {
                    m_networkStandbyMode = networkStandbyMode;
                    m_xcast_manager  = XCastManager::getInstance();
                    if(nullptr != m_xcast_manager)
                    {
                        m_xcast_manager->setService(this);
                        if( false == connectToGDialService())
                        {
                            startTimer(LOCATE_CAST_FIRST_TIMEOUT_IN_MILLIS);
                        }
                    }
                }
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
 
         uint32_t XCastImplementation::Configure(PluginHost::IShell* service)
         {
            uint32_t result = Core::ERROR_NONE;
            _service = service;
            _service->AddRef();
            ASSERT(service != nullptr);
            InitializePowerManager(service);
            Initialize(m_networkStandbyMode);
            getSystemPlugin();
            m_SystemPluginObj->Subscribe<JsonObject>(1000, "onFriendlyNameChanged", &XCastImplementation::onFriendlyNameUpdateHandler, this);
            if (Core::ERROR_NONE == updateSystemFriendlyName())
            {
                LOGINFO("XCast::Initialize m_friendlyName:  %s\n ",m_friendlyName.c_str());
            }
            return result;
         }

        void XCastImplementation::InitializePowerManager(PluginHost::IShell* service)
        {

            LOGINFO("Connect the COM-RPC socket\n");

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

            if(!_registeredEventHandlers && _powerManagerPlugin) {
                _registeredEventHandlers = true;
                _powerManagerPlugin->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::INetworkStandbyModeChangedNotification>());
                _powerManagerPlugin->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
            }
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
        bool XCastImplementation::connectToGDialService(void)
        {
            std::string interface,ipaddress;
            bool status = false;

            getDefaultNameAndIPAddress(interface,ipaddress);
            if (!interface.empty())
            {
                status = m_xcast_manager->initialize(interface,m_networkStandbyMode);
                if( true == status)
                {
                    m_activeInterfaceName = interface;
                }
            }
            LOGINFO("GDialService[%u]IF[%s]IP[%s]",status,interface.c_str(),ipaddress.c_str());
            return status;
        }

        bool XCastImplementation::getDefaultNameAndIPAddress(std::string& interface, std::string& ipaddress)
        {
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
            return returnValue;
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

        void XCastImplementation::eventHandler_onDefaultInterfaceChanged(const JsonObject& parameters)
        {
            std::string oldInterfaceName, newInterfaceName;
            oldInterfaceName = parameters["oldInterfaceName"].String();
            newInterfaceName = parameters["newInterfaceName"].String();

            LOGINFO("XCast onDefaultInterfaceChanged, old interface: %s, new interface: %s", oldInterfaceName.c_str(), newInterfaceName.c_str());
            updateNWConnectivityStatus(newInterfaceName.c_str(), true);
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
        void XCastImplementation::onLocateCastTimer()
        {
            if( false == connectToGDialService())
            {
                LOGINFO("Retry after 10 sec...");
                m_locateCastTimer.setInterval(LOCATE_CAST_SECOND_TIMEOUT_IN_MILLIS);
                return ;
            }
            stopTimer();

            if ((NULL != m_xcast_manager) && m_isDynamicRegistrationsRequired )
            {
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
        void XCastImplementation::startTimer(int interval)
        {
            stopTimer();
            m_locateCastTimer.start(interval);
        }

        void XCastImplementation::stopTimer()
        {
            if (m_locateCastTimer.isActive())
            {
                m_locateCastTimer.stop();
            }
        }
        bool XCastImplementation::isTimerActive()
        {
            return (m_locateCastTimer.isActive());
        }
        
        void XCastImplementation::dispatchEvent(Event event, string callsign, const JsonObject &params)
        {
            Core::IWorkerPool::Instance().Submit(Job::Create(this, event, callsign, params));
        }

        void XCastImplementation::Dispatch(Event event, string callsign, const JsonObject params)
        {
            _adminLock.Lock();
            LOGINFO("Event Dispatched");
            _adminLock.Unlock();
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
                LOGINFO("m_xcast_manager is not null");
                protocolVersion = m_xcast_manager->getProtocolVersion();
            }
            return Core::ERROR_NONE;
        }
        Core::hresult XCastImplementation::SetNetworkStandbyMode(bool networkStandbyMode) { 
            LOGINFO("nwStandbymode: %d", networkStandbyMode);
            if (nullptr != m_xcast_manager)
            {
                m_xcast_manager->setNetworkStandbyMode(networkStandbyMode);
                m_networkStandbyMode = networkStandbyMode;
            }
            return 0;
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
            else
            {
                LOGINFO("XcastService::getManufacturerName m_xcast_manager is NULL");
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
            LOGINFO("XcastService::getModelName");
            if (nullptr != m_xcast_manager)
            {
                modelname = m_xcast_manager->getModelName();
                LOGINFO("Model[%s]", modelname.c_str());
            }
            else
            {
                return Core::ERROR_GENERAL;
            }
            return Core::ERROR_NONE;
        }

		Core::hresult XCastImplementation::SetEnabled(bool enabled) { 
            LOGINFO("XcastService::setEnabled - %d",enabled);
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
            LOGINFO("XcastService::setEnabled : %d, enabled : %d" , m_xcastEnable, enabled);
            result = enableCastService(m_friendlyName,enabled);

            
            return result;

         }
		Core::hresult XCastImplementation::GetEnabled(bool &enabled , bool &success ) { 
            LOGINFO("XcastService::getEnabled - %d",m_xcastEnable);
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
            LOGINFO("XcastService::setFriendlyName - %s", friendlyname.c_str());
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

	    Core::hresult XCastImplementation::RegisterApplications(Exchange::IXCast::IApplicationInfoIterator* const appInfoList) { return Core::ERROR_NONE; }

        
     } // namespace Plugin
 } // namespace WPEFramework
