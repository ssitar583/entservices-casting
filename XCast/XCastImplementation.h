/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#pragma once

#include "Module.h"
#include <interfaces/Ids.h>
#include <interfaces/IXCast.h>
#include <interfaces/IPowerManager.h>
#include <interfaces/IConfiguration.h>
 
#include <com/com.h>
#include <core/core.h>
#include <mutex>
#include <vector>

#include "XCastManager.h"
#include "XCastNotifier.h"

#include "libIBus.h"
#include "PowerManagerInterface.h"


#define SYSTEM_CALLSIGN "org.rdk.System"
#define SYSTEM_CALLSIGN_VER SYSTEM_CALLSIGN".1"
#define SECURITY_TOKEN_LEN_MAX 1024


using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;


namespace WPEFramework
{
    namespace Plugin
    {
        WPEFramework::Exchange::IPowerManager::PowerState m_powerState = WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY;
        class XCastImplementation : public Exchange::IXCast,public Exchange::IConfiguration, public XCastNotifier 
        {
         public:
            enum PluginState
            {
                PLUGIN_DEACTIVATED,
                PLUGIN_ACTIVATED
            };

             // We do not allow this plugin to be copied !!
             XCastImplementation();
             ~XCastImplementation() override;

              enum Event {
                    LAUNCH_REQUEST_WITH_PARAMS,
                    LAUNCH_REQUEST,
                    STOP_REQUEST,
                    HIDE_REQUEST,
                    STATE_REQUEST,
                    RESUME_REQUEST,
                    UPDATE_POWERSTATE
            };
 
             static XCastImplementation *instance(XCastImplementation *XCastImpl = nullptr);
 
             // We do not allow this plugin to be copied !!
             XCastImplementation(const XCastImplementation &) = delete;
             XCastImplementation &operator=(const XCastImplementation &) = delete;
            
 
        public:
             class EXTERNAL Job : public Core::IDispatch {
                protected:
                    Job(XCastImplementation *tts, Event event,string callsign,JsonObject &params)
                        : _xcast(tts)
                        , _event(event)
                        , _callsign(callsign)
                        , _params(params) {
                        if (_xcast != nullptr) {
                            _xcast->AddRef();
                        }
                    }

                public:
                    Job() = delete;
                    Job(const Job&) = delete;
                    Job& operator=(const Job&) = delete;
                    ~Job() {
                        if (_xcast != nullptr) {
                            _xcast->Release();
                        }
                    }

                public:
                    static Core::ProxyType<Core::IDispatch> Create(XCastImplementation *tts, Event event,string callsign,JsonObject params) {
                        #ifndef USE_THUNDER_R4
                            return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(tts, event, callsign, params)));
                        #else
                            return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(tts, event, callsign, params)));
                        #endif
                    }

                    virtual void Dispatch() {
                        _xcast->Dispatch(_event, _callsign, _params);
                    }

                private:
                    XCastImplementation *_xcast;
                    const Event _event;
                    const string _callsign;
                    const JsonObject _params;
            };

        private:
            class PowerManagerNotification : public Exchange::IPowerManager::INetworkStandbyModeChangedNotification,
                                                     public Exchange::IPowerManager::IModeChangedNotification {
                private:
                    PowerManagerNotification(const PowerManagerNotification&) = delete;
                    PowerManagerNotification& operator=(const PowerManagerNotification&) = delete;

                public:
                    explicit PowerManagerNotification(XCastImplementation& parent)
                    : _parent(parent)
                    {
                    }
                    ~PowerManagerNotification() override = default;

                public:
                    void OnPowerModeChanged(const PowerState currentState, const PowerState newState) override
                    {
                        LOGINFO("onPowerModeChanged: State Changed %d -- > %d\r",currentState, newState);
                        m_powerState = newState;
                        LOGWARN("creating worker thread for threadPowerModeChangeEvent m_powerState :%d",m_powerState);
                        std::thread powerModeChangeThread = std::thread(&XCastImplementation::threadPowerModeChangeEvent,&_parent);
                        powerModeChangeThread.detach();
                    }

                    void OnNetworkStandbyModeChanged(const bool enabled)
                    {
                        _parent.m_networkStandbyMode = enabled;
                        LOGWARN("creating worker thread for threadNetworkStandbyModeChangeEvent Mode :%u", _parent.m_networkStandbyMode);
                        std::thread networkStandbyModeChangeThread = std::thread(&XCastImplementation::networkStandbyModeChangeEvent,&_parent);
                        networkStandbyModeChangeThread.detach();
                    }

                    template <typename T>
                    T* baseInterface()
                    {
                        static_assert(std::is_base_of<T, PowerManagerNotification>(), "base type mismatch");
                        return static_cast<T*>(this);
                    }

                    BEGIN_INTERFACE_MAP(PowerManagerNotification)
                    INTERFACE_ENTRY(Exchange::IPowerManager::INetworkStandbyModeChangedNotification)
                    INTERFACE_ENTRY(Exchange::IPowerManager::IModeChangedNotification)
                    END_INTERFACE_MAP

                private:
                    XCastImplementation& _parent;
            };
 
        public:
            Core::hresult Register(Exchange::IXCast::INotification *notification) override;
            Core::hresult Unregister(Exchange::IXCast::INotification *notification) override; 

            Core::hresult UpdateApplicationState(const string& applicationName, const Exchange::IXCast::State& state, const string& applicationId, const Exchange::IXCast::ErrorCode& error,  Exchange::IXCast::XCastSuccess &success) override;
            Core::hresult GetProtocolVersion(string &protocolVersion, bool &success) override;
            Core::hresult SetManufacturerName(const string &manufacturername,  Exchange::IXCast::XCastSuccess &success) override;
            Core::hresult GetManufacturerName(string &manufacturername, bool &success) override;
            Core::hresult SetModelName(const string &modelname,  Exchange::IXCast::XCastSuccess &success) override;
            Core::hresult GetModelName(string &modelname, bool &success) override;
            Core::hresult SetEnabled(const bool& enabled,  Exchange::IXCast::XCastSuccess &success) override;
            Core::hresult GetEnabled(bool &enabled , bool &success ) override;
            Core::hresult SetStandbyBehavior(const Exchange::IXCast::StandbyBehavior &standbybehavior,  Exchange::IXCast::XCastSuccess &success) override;
            Core::hresult GetStandbyBehavior(Exchange::IXCast::StandbyBehavior &standbybehavior, bool &success) override;
            Core::hresult SetFriendlyName(const string &friendlyname,  Exchange::IXCast::XCastSuccess &success) override;
            Core::hresult GetFriendlyName(string &friendlyname , bool &success ) override;
            Core::hresult RegisterApplications(Exchange::IXCast::IApplicationInfoIterator* const appInfoList,  Exchange::IXCast::XCastSuccess &success) override;
            Core::hresult UnregisterApplications(Exchange::IXCast::IStringIterator* const apps,  Exchange::IXCast::XCastSuccess &success) override;

            virtual void onXcastApplicationLaunchRequestWithParam (string appName, string strPayLoad, string strQuery, string strAddDataUrl) override ;
            virtual void onXcastApplicationLaunchRequest(string appName, string parameter) override ;
            virtual void onXcastApplicationStopRequest(string appName, string appId) override ;
            virtual void onXcastApplicationHideRequest(string appName, string appId) override ;
            virtual void onXcastApplicationResumeRequest(string appName, string appId) override ;
            virtual void onXcastApplicationStateRequest(string appName, string appId) override ;
            virtual void onGDialServiceStopped(void) override;

            BEGIN_INTERFACE_MAP(XCastImplementation)
            INTERFACE_ENTRY(Exchange::IXCast)
            INTERFACE_ENTRY(Exchange::IConfiguration)
            END_INTERFACE_MAP

        private:
            static XCastManager* m_xcast_manager;
            guint m_FriendlyNameUpdateTimerID{0};
            TpTimer m_locateCastTimer;
            PluginState _networkPluginState;
            
            
            WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_ControllerObj = nullptr;
            WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_NetworkPluginObj = nullptr;
            WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_SystemPluginObj = NULL;
            PluginHost::IShell* _service;
            
            PowerManagerInterfaceRef _powerManagerPlugin;
            Core::Sink<PowerManagerNotification> _pwrMgrNotification;
            void threadPowerModeChangeEvent(void);
            void networkStandbyModeChangeEvent(void);
            static bool m_xcastEnable;
            static bool m_standbyBehavior;
            bool m_networkStandbyMode;
            bool _registeredEventHandlers;

        private:
            mutable Core::CriticalSection _adminLock;
             
            std::list<Exchange::IXCast::INotification *> _xcastNotification; // List of registered notifications

            void dumpDynamicAppCacheList(string strListName, std::vector<DynamicAppConfig*> appConfigList);
            bool deleteFromDynamicAppCache(vector<string>& appsToDelete);

            void dispatchEvent(Event,string callsign, const JsonObject &params);
            void Dispatch(Event event,string callsign, const JsonObject params);

            uint32_t Initialize(bool networkStandbyMode);
            void Deinitialize(void);

            void onLocateCastTimer();
            void startTimer(int interval);
            void stopTimer();
            bool isTimerActive();
            
            void registerEventHandlers();
            void InitializePowerManager(PluginHost::IShell *service);

            std::string getSecurityToken();
            void getThunderPlugins();
            int activatePlugin(string callsign);
            int deactivatePlugin(string callsign);
            bool isPluginActivated(string callsign);
            void eventHandler_onDefaultInterfaceChanged(const JsonObject& parameters);
            void eventHandler_ipAddressChanged(const JsonObject& parameters);
            void eventHandler_pluginState(const JsonObject& parameters);
            bool connectToGDialService(void);
            bool getDefaultNameAndIPAddress(std::string& interface, std::string& ipaddress);
            void updateNWConnectivityStatus(std::string nwInterface, bool nwConnected, std::string ipaddress = "");
            uint32_t enableCastService(string friendlyname,bool enableService);
            uint32_t Configure(PluginHost::IShell* shell);
            
            void getSystemPlugin();
            int updateSystemFriendlyName();
            static gboolean update_friendly_name_timercallback(gpointer userdata);
            void onFriendlyNameUpdateHandler(const JsonObject& parameters);
            
            void onXcastUpdatePowerStateRequest(string powerState);
            uint32_t SetNetworkStandbyMode(bool networkStandbyMode);
            bool setPowerState(std::string powerState);
            void getUrlFromAppLaunchParams (const char *app_name, const char *payload, const char *query_string, const char *additional_data_url, char *url);
            void updateDynamicAppCache(Exchange::IXCast::IApplicationInfoIterator* const appInfoList);
            
        public:
            static XCastImplementation* _instance;

            friend class Job;
        };
 
    } // namespace Plugin
} // namespace WPEFramework
