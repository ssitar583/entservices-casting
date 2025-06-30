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

#include "XCastImplementation.h"
#include <sys/prctl.h>
 
#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"
#include "UtilsSynchroIarm.hpp"

#include "rfcapi.h"
#include <string> 
#include <vector>


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


#define DIAL_MAX_ADDITIONALURL (1024)
 
 
namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(XCastImplementation, 1, 0);
        XCastImplementation *XCastImplementation::_instance = nullptr;    
        XCastManager* XCastImplementation::m_xcast_manager = nullptr;
        static std::vector <DynamicAppConfig*> m_appConfigCache;
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
        _pwrMgrNotification(*this),
         _registeredEventHandlers(false), _adminLock()
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
            
            _powerManagerPlugin = PowerManagerInterfaceBuilder(_T("org.rdk.PowerManager"))
                .withIShell(service)
                .withRetryIntervalMS(200)
                .withRetryCount(25)
                .createInterface();
            registerEventHandlers();

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

        void XCastImplementation::onXcastApplicationLaunchRequestWithParam (string appName, string strPayLoad, string strQuery, string strAddDataUrl)
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
            setPowerState(powerState);
        }

        void XCastImplementation::onGDialServiceStopped(void)
        {
            LOGINFO("Timer triggered to monitor the GDial, check after 5sec");
            startTimer(LOCATE_CAST_FIRST_TIMEOUT_IN_MILLIS);
        }
        bool XCastImplementation::connectToGDialService(void)
        {
            LOGINFO("XCastImplementation::connectToGDialService");
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
                appConfigList = m_appConfigCache;
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
            LOGINFO("XCastImplementation::onLocateCastTimer : Timer still active ? %d ",m_locateCastTimer.isActive());
        }

        uint32_t XCastImplementation::enableCastService(string friendlyname,bool enableService) 
        {
            LOGINFO("ARGS = %s : %d", friendlyname.c_str(), enableService);
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
            
            LOGINFO("XCastImplementation::Dispatch: event = %d, callsign = %s", event, callsign.c_str());
            std::list<Exchange::IXCast::INotification*>::iterator index(_xcastNotification.begin());
            while (index != _xcastNotification.end())
            {
                switch(event)
                {
                    case LAUNCH_REQUEST_WITH_PARAMS:
                    {
                        string appName = params["appName"].String();
                        string strPayLoad = params["strPayLoad"].String();
                        string strQuery = params["strQuery"].String();
                        string strAddDataUrl = params["strAddDataUrl"].String();
                        (*index)->OnApplicationLaunchRequestWithParam(appName,strPayLoad,strQuery,strAddDataUrl);
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
                        (*index)->OnApplicationCurrentStateRequest(appName,appId);
                    }
                    break;
                    case RESUME_REQUEST:
                    {
                        string appName = params["appName"].String();
                        string appId = params["appId"].String();
                        (*index)->OnApplicationResumeRequest(appName,appId);
                    }
                    break;
                    default: break;
                }
                ++index;
            }

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

        Core::hresult XCastImplementation::UpdateApplicationState(const string& applicationName, const Exchange::IXCast::State& state, const string& applicationId, const Exchange::IXCast::ErrorCode& error,Exchange::IXCast::XCastSuccess &success){
            LOGINFO("ARGS = %s : %s : %d : %d ", applicationName.c_str(), applicationId.c_str() , state , error);
            success.success = false;
            uint32_t status = Core::ERROR_GENERAL;
            if(!applicationName.empty() && (nullptr != m_xcast_manager))
            {
                LOGINFO("XCastImplementation::UpdateApplicationState  ARGS = %s : %s : %d : %d ", applicationName.c_str(), applicationId.c_str() , state , error);
                string appstate = "";
                if (state == Exchange::IXCast::State::RUNNING)
                {
                    appstate = "running";
                }
                else if (state == Exchange::IXCast::State::STOPPED)
                {
                    appstate = "stopped";
                }
                else if(state == Exchange::IXCast::State::HIDDEN)
                {
                    appstate = "suspended";
                }

                string errorStr = "";
                if (error == Exchange::IXCast::ErrorCode::NONE)
                {
                    errorStr = "none";
                }
                else if (error == Exchange::IXCast::ErrorCode::FORBIDDEN)
                {
                    errorStr = "forbidden";
                }
                else if (error == Exchange::IXCast::ErrorCode::UNAVAILABLE)
                {
                    errorStr = "unavailable";
                }
                else if (error == Exchange::IXCast::ErrorCode::INVALID)
                {
                    errorStr = "invalid";
                }
                else if (error == Exchange::IXCast::ErrorCode::INTERNAL)
                {
                    errorStr = "internal";
                }
                else
                {
                    LOGERR("XCastImplementation::UpdateApplicationState - Invalid Error Code");
                    return Core::ERROR_GENERAL;
                }

                m_xcast_manager->applicationStateChanged(applicationName.c_str(), appstate.c_str(), applicationId.c_str(), errorStr.c_str());
                success.success = true;
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("XCastImplementation::UpdateApplicationState - m_xcast_manager is NULL");
            }
            return status;
        }
		Core::hresult XCastImplementation::GetProtocolVersion(string &protocolVersion , bool &success) {
            LOGINFO("XCastImplementation::getProtocolVersion");
            success = false;
            if (nullptr != m_xcast_manager)
            {
                LOGINFO("m_xcast_manager is not null");
                protocolVersion = m_xcast_manager->getProtocolVersion();
                success = true;
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
        Core::hresult XCastImplementation::SetManufacturerName(const string &manufacturername, Exchange::IXCast::XCastSuccess &success) {
            uint32_t status = Core::ERROR_GENERAL;
            LOGINFO("ManufacturerName : %s", manufacturername.c_str());
            success.success = false;
            if (nullptr != m_xcast_manager)
            {
                m_xcast_manager->setManufacturerName(manufacturername);
                 success.success = true;
                status = Core::ERROR_NONE;
            }
            return status;
        }
		Core::hresult XCastImplementation::GetManufacturerName(string &manufacturername , bool &success){
            LOGINFO("XCastImplementation:getManufacturerName");
            if (nullptr != m_xcast_manager)
            {
                manufacturername = m_xcast_manager->getManufacturerName();
                LOGINFO("Manufacturer[%s]", manufacturername.c_str());
                success = true;
            }
            else
            {
                LOGINFO("XCastImplementation::getManufacturerName m_xcast_manager is NULL");
                success = false;
                return Core::ERROR_GENERAL;
            }
            return Core::ERROR_NONE;
        }
		Core::hresult XCastImplementation::SetModelName(const string &modelname, Exchange::IXCast::XCastSuccess &success) { 
            uint32_t status = Core::ERROR_GENERAL;
            success.success = false;
            LOGINFO("ModelName : %s", modelname.c_str());

            if (nullptr != m_xcast_manager)
            {
                m_xcast_manager->setModelName(modelname);
                 success.success = true;
                status = Core::ERROR_NONE;
            }
            return status;
        }
		Core::hresult XCastImplementation::GetModelName(string &modelname , bool &success) { 
            LOGINFO("XCastImplementation::getModelName");
            if (nullptr != m_xcast_manager)
            {
                modelname = m_xcast_manager->getModelName();
                LOGINFO("Model[%s]", modelname.c_str());
            }
            else
            {
                LOGINFO("XCastImplementation::getModelName m_xcast_manager is NULL");
                return Core::ERROR_GENERAL;
            }
            success = true;
            return Core::ERROR_NONE;
        }

	Core::hresult XCastImplementation::SetEnabled(const bool& enabled, Exchange::IXCast::XCastSuccess &success){
            LOGINFO("XCastImplementation::setEnabled - %d",enabled);
            bool isEnabled = false;
            m_xcastEnable= enabled;
            success.success = false;
            if (m_xcastEnable && ( (m_standbyBehavior == true) || ((m_standbyBehavior == false)&&(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON))))
            {
                isEnabled = true;
            }
            else
            {
                isEnabled = false;
            }
            LOGINFO("XCastImplementation::setEnabled : %d, enabled : %d" , m_xcastEnable, isEnabled);
            enableCastService(m_friendlyName,isEnabled);
	    success.success = true;
            return Core::ERROR_NONE;

        }
	Core::hresult XCastImplementation::GetEnabled(bool &enabled , bool &success ) { 
            LOGINFO("XCastImplementation::getEnabled - %d",m_xcastEnable);
            enabled = m_xcastEnable;
            success = true;
            return Core::ERROR_NONE;
         }
       
	Core::hresult XCastImplementation::SetStandbyBehavior(const Exchange::IXCast::StandbyBehavior &standbybehavior, Exchange::IXCast::XCastSuccess &success) { 
            LOGINFO("XCastImplementation::setStandbyBehavior\n");
             success.success = false;
            bool enabled = false;
            if (standbybehavior == Exchange::IXCast::StandbyBehavior::ACTIVE)
            {
                enabled = true;
            }
            else if (standbybehavior == Exchange::IXCast::StandbyBehavior::INACTIVE)
            {
                enabled = false;
            }
            else
            {
                LOGERR("XCastImplementation::setStandbyBehavior - Invalid standby behavior ");
                return Core::ERROR_GENERAL;
            }
            m_standbyBehavior = enabled;
             success.success = true;
            LOGINFO("XCastImplementation::setStandbyBehavior m_standbyBehavior : %d", m_standbyBehavior);
            return Core::ERROR_NONE;
        }
	Core::hresult XCastImplementation::GetStandbyBehavior(Exchange::IXCast::StandbyBehavior &standbybehavior, bool &success) { 
            LOGINFO("XCastImplementation::getStandbyBehavior m_standbyBehavior :%d",m_standbyBehavior);
            if(m_standbyBehavior)
                standbybehavior = Exchange::IXCast::StandbyBehavior::ACTIVE;
            else
                standbybehavior = Exchange::IXCast::StandbyBehavior::INACTIVE;
            success = true;
            return Core::ERROR_NONE;
        }
	Core::hresult XCastImplementation::SetFriendlyName(const string& friendlyname, Exchange::IXCast::XCastSuccess &success) { 
            LOGINFO("XCastImplementation::setFriendlyName - %s", friendlyname.c_str());
            uint32_t result = Core::ERROR_GENERAL;
             success.success = false;
            bool enabledStatus = false;
            if (!friendlyname.empty())
            {
                m_friendlyName = friendlyname;
                LOGINFO("XCastImplementation::setFriendlyName  :%s",m_friendlyName.c_str());
                if (m_xcastEnable && ( (m_standbyBehavior == true) || ((m_standbyBehavior == false)&&(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON))))
                    {
                        enabledStatus = true;                
                    }
                    else
                    {
                        enabledStatus = false;
                    }
                    enableCastService(m_friendlyName,enabledStatus);
                     success.success = true;
                    result = Core::ERROR_NONE;
            }
            return result;
        }
	Core::hresult XCastImplementation::GetFriendlyName(string &friendlyname , bool &success ) { 
            LOGINFO("XCastImplementation::getFriendlyName :%s ",m_friendlyName.c_str());
            friendlyname = m_friendlyName;
            success = true;
            return Core::ERROR_NONE;
        }

        bool XCastImplementation::deleteFromDynamicAppCache(vector<string>& appsToDelete) {
            LOGINFO("XCastImplementation::deleteFromDynamicAppCache");
            bool ret = true;
            {lock_guard<mutex> lck(m_appConfigMutex);
                /*Check if existing cache need to be updated*/
                std::vector<int> entriesTodelete;
                for (string appNameToDelete : appsToDelete) {
                    bool found = false;
                    int index = 0;
                    for (DynamicAppConfig* pDynamicAppConfigOld : m_appConfigCache) {
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
                    LOGINFO("Going to delete the entry: %d from m_appConfigCache  size: %d", indexToDelete, (int)m_appConfigCache.size());
                    //Delete the old unwanted item here.
                    DynamicAppConfig* pDynamicAppConfigOld = m_appConfigCache[indexToDelete];
                    m_appConfigCache.erase (m_appConfigCache.begin()+indexToDelete);
                    free (pDynamicAppConfigOld); pDynamicAppConfigOld = NULL;
                }
                entriesTodelete.clear();

            }
            //Even if requested app names not there return true.
            return ret;
        }
        void XCastImplementation::updateDynamicAppCache(Exchange::IXCast::IApplicationInfoIterator* const appInfoList)
        {
            LOGINFO("XcastService::UpdateDynamicAppCache");

            std::vector <DynamicAppConfig*> appConfigList;
            if (appInfoList != nullptr)
            {
                LOGINFO("Applications:");
                Exchange::IXCast::ApplicationInfo appInfo;
                while (appInfoList->Next(appInfo))
                {
                    LOGINFO("Application: %s", appInfo.appName.c_str());
                    DynamicAppConfig* pDynamicAppConfig = (DynamicAppConfig*) malloc (sizeof(DynamicAppConfig));
                    if(pDynamicAppConfig)
                    {
                        memset ((void*)pDynamicAppConfig, '\0', sizeof(DynamicAppConfig));
                    
                        memset (pDynamicAppConfig->appName, '\0', sizeof(pDynamicAppConfig->appName));
                        strncpy (pDynamicAppConfig->appName, appInfo.appName.c_str(), sizeof(pDynamicAppConfig->appName) - 1);
                        pDynamicAppConfig->appName[sizeof(pDynamicAppConfig->appName) - 1] = '\0';

                        memset (pDynamicAppConfig->prefixes, '\0', sizeof(pDynamicAppConfig->prefixes));
                        strncpy (pDynamicAppConfig->prefixes, appInfo.prefixes.c_str(), sizeof(pDynamicAppConfig->prefixes) - 1);
                        pDynamicAppConfig->prefixes[sizeof(pDynamicAppConfig->prefixes) - 1] = '\0';

                        memset (pDynamicAppConfig->cors, '\0', sizeof(pDynamicAppConfig->cors));
                        strncpy (pDynamicAppConfig->cors, appInfo.cors.c_str(), sizeof(pDynamicAppConfig->cors) - 1);
                        pDynamicAppConfig->cors[sizeof(pDynamicAppConfig->cors) - 1] = '\0';

                        memset (pDynamicAppConfig->query, '\0', sizeof(pDynamicAppConfig->query));
                        strncpy (pDynamicAppConfig->query, appInfo.query.c_str(), sizeof(pDynamicAppConfig->query) - 1);
                        pDynamicAppConfig->query[sizeof(pDynamicAppConfig->query) - 1] = '\0';

                        memset (pDynamicAppConfig->payload, '\0', sizeof(pDynamicAppConfig->payload));
                        strncpy (pDynamicAppConfig->payload, appInfo.payload.c_str(), sizeof(pDynamicAppConfig->payload) - 1);
                        pDynamicAppConfig->payload[sizeof(pDynamicAppConfig->payload) - 1] = '\0';

                        pDynamicAppConfig->allowStop = appInfo.allowStop ? true : false;

                        LOGINFO("appName:%s, prefixes:%s, cors:%s, allowStop:%d, query:%s, payload:%s",
                                pDynamicAppConfig->appName,
                                pDynamicAppConfig->prefixes,
                                pDynamicAppConfig->cors,
                                pDynamicAppConfig->allowStop,
                                pDynamicAppConfig->query,
                                pDynamicAppConfig->payload);
                        appConfigList.push_back (pDynamicAppConfig);
                    }
                    else
                    {
                        LOGINFO("Memory allocation failed for DynamicAppConfig");
                        return;
                    }
                }
            }
          
            dumpDynamicAppCacheList(string("appConfigList"), appConfigList);
            vector<string> appsToDelete;
            for (DynamicAppConfig* pDynamicAppConfig : appConfigList) {
                    appsToDelete.push_back(string(pDynamicAppConfig->appName));
            }
            deleteFromDynamicAppCache (appsToDelete);

            LOGINFO("appConfigList count: %d", (int)appConfigList.size());
            //Update the new entries here.
            {
                lock_guard<mutex> lck(m_appConfigMutex);
                for (DynamicAppConfig* pDynamicAppConfig : appConfigList) {
                    m_appConfigCache.push_back(pDynamicAppConfig);
                }
                LOGINFO("m_appConfigCache count: %d", (int)m_appConfigCache.size());
            }
            //Clear the tempopary list here
            appsToDelete.clear();
            appConfigList.clear();
            
            dumpDynamicAppCacheList(string("m_appConfigCache"), m_appConfigCache);
            return;
        }

        Core::hresult XCastImplementation::RegisterApplications(Exchange::IXCast::IApplicationInfoIterator* const appInfoList, Exchange::IXCast::XCastSuccess &success) { 
            LOGINFO("XCastImplementation::registerApplications \n");
            enableCastService(m_friendlyName,false);
            m_isDynamicRegistrationsRequired = true;
            updateDynamicAppCache(appInfoList);
            std::vector<DynamicAppConfig*> appConfigList;
            {
                lock_guard<mutex> lck(m_appConfigMutex);
                appConfigList = m_appConfigCache;
            }
            dumpDynamicAppCacheList(string("m_appConfigCache"), appConfigList);
            //Pass the dynamic cache to xdial process
            m_xcast_manager->registerApplications(m_appConfigCache);

            /*Reenabling cast service after registering Applications*/
            if (m_xcastEnable && ( (m_standbyBehavior == true) || ((m_standbyBehavior == false)&&(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON)) ) ) {
                LOGINFO("Enable CastService  m_xcastEnable: %d m_standbyBehavior: %d m_powerState:%d", m_xcastEnable, m_standbyBehavior, m_powerState);
                enableCastService(m_friendlyName,true);
            }
            else {
                LOGINFO("CastService not enabled m_xcastEnable: %d m_standbyBehavior: %d m_powerState:%d", m_xcastEnable, m_standbyBehavior, m_powerState);
            }
            success.success = true;
            return Core::ERROR_NONE;
        }
	Core::hresult XCastImplementation::UnregisterApplications(Exchange::IXCast::IStringIterator* const apps, Exchange::IXCast::XCastSuccess &success) 
        {
            LOGINFO("XcastService::unregisterApplications \n ");
            auto returnStatus = false;
            /*Disable cast service before registering Applications*/
            enableCastService(m_friendlyName,false);
            m_isDynamicRegistrationsRequired = true;

            std::vector<string> appsToDelete;
            string appName;
            while (apps->Next(appName))
            {
                LOGINFO("Going to delete the app: %s from dynamic cache", appName.c_str());
                appsToDelete.push_back(appName);
            }
            
            returnStatus = deleteFromDynamicAppCache(appsToDelete);

            std::vector<DynamicAppConfig*> appConfigList;
            {
                lock_guard<mutex> lck(m_appConfigMutex);
                appConfigList = m_appConfigCache;
            }
            dumpDynamicAppCacheList(string("m_appConfigCache"), appConfigList);
            m_xcast_manager->registerApplications(appConfigList);

            /*Reenabling cast service after registering Applications*/
            if (m_xcastEnable && ( (m_standbyBehavior == true) || ((m_standbyBehavior == false)&&(m_powerState == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON)) ) ) {
                LOGINFO("Enable CastService  m_xcastEnable: %d m_standbyBehavior: %d m_powerState:%d", m_xcastEnable, m_standbyBehavior, m_powerState);
                enableCastService(m_friendlyName,true);
            }
            else {
                LOGINFO("CastService not enabled m_xcastEnable: %d m_standbyBehavior: %d m_powerState:%d", m_xcastEnable, m_standbyBehavior, m_powerState);
            }
            success.success = (returnStatus)? true : false;
            return (returnStatus)? Core::ERROR_NONE : Core::ERROR_GENERAL;
        }

        bool XCastImplementation::setPowerState(std::string powerState)
        {
            PowerState cur_powerState = m_powerState,
            new_powerState = WPEFramework::Exchange::IPowerManager::POWER_STATE_OFF;
            Core::hresult status = Core::ERROR_GENERAL;
            bool ret = true;
            if ("ON" == powerState)
            {
                new_powerState = WPEFramework::Exchange::IPowerManager::POWER_STATE_ON;
            }
            else if ("STANDBY" == powerState)
            {
                new_powerState = WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY;
            }
            else if ("TOGGLE" == powerState)
            {
                new_powerState = ( WPEFramework::Exchange::IPowerManager::POWER_STATE_ON == cur_powerState ) ? WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY : WPEFramework::Exchange::IPowerManager::POWER_STATE_ON;
            }

            if ((WPEFramework::Exchange::IPowerManager::POWER_STATE_OFF != new_powerState) && (cur_powerState != new_powerState))
            {
                ASSERT (_powerManagerPlugin);

                if (_powerManagerPlugin)
                {
                    status = _powerManagerPlugin->SetPowerState(0, new_powerState, "random");
                }

                if (status == Core::ERROR_GENERAL)
                {
                    ret = false;
                    LOGINFO("Failed to change power state [%d] -> [%d] ret[%x]",cur_powerState,new_powerState,ret);
                }
                else
                {
                    LOGINFO("changing power state [%d] -> [%d] success",cur_powerState,new_powerState);
                    sleep(m_sleeptime);
                }
            }
            return ret;
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
        
    } // namespace Plugin
} // namespace WPEFramework
