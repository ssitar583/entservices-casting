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
 #include <interfaces/IConfiguration.h>
 
 #include <com/com.h>
 #include <core/core.h>
 #include <mutex>
 #include <vector>
 
 #include "libIBus.h"

#include "XCastManager.h"

 #include "UtilsgetRFCConfig.h"



 
#include <interfaces/IPowerManager.h>
#include "PowerManagerInterface.h"
 #include "XCastNotifier.h"
 #include "UtilsJsonRpc.h"
#include "rfcapi.h"


using namespace std;
using namespace WPEFramework;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

using namespace std;
using namespace WPEFramework;
 namespace WPEFramework
 {
     namespace Plugin
     {
         WPEFramework::Exchange::IPowerManager::PowerState m_powerState = WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY;
         class XCastImplementation : public Exchange::IXCast, public Exchange::IConfiguration,public PluginHost::IStateControl, public XCastNotifier 
         {
         public:
            
            enum PluginState
            {
                PLUGIN_DEACTIVATED,
                PLUGIN_ACTIVATED
            };

            enum Event {
                    LAUNCH_REQUEST_WITH_PARAMS,
                    LAUNCH_REQUEST,
                    STOP_REQUEST,
                    HIDE_REQUEST,
                    STATE_REQUEST,
                    RESUME_REQUEST,
                    UPDATE_POWERSTATE
            };
             // We do not allow this plugin to be copied !!
             XCastImplementation();
             ~XCastImplementation() override;
 
             static XCastImplementation *instance(XCastImplementation *XCastImpl = nullptr);
 
             // We do not allow this plugin to be copied !!
             XCastImplementation(const XCastImplementation &) = delete;
             XCastImplementation &operator=(const XCastImplementation &) = delete;
            
             
             BEGIN_INTERFACE_MAP(XCastImplementation)
             INTERFACE_ENTRY(Exchange::IXCast)
             INTERFACE_ENTRY(Exchange::IConfiguration)
             INTERFACE_ENTRY(PluginHost::IStateControl)
             END_INTERFACE_MAP
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
 
         public:
             Core::hresult Register(Exchange::IXCast::INotification *notification) override;
             Core::hresult Unregister(Exchange::IXCast::INotification *notification) override;
      
             uint32_t Configure(PluginHost::IShell* service) override;

            virtual PluginHost::IStateControl::state State() const override { return PluginHost::IStateControl::RESUMED; }
            virtual uint32_t Request(const command state) override { return Core::ERROR_GENERAL; }
            virtual void Register(IStateControl::INotification* notification) override {}
            virtual void Unregister(IStateControl::INotification* notification) override {}

            Core::hresult ApplicationStateChanged(const string& applicationName, const string& state, const string& applicationId, const string& error) override;
            Core::hresult GetProtocolVersion(string &protocolVersion  ) override;
            Core::hresult SetNetworkStandbyMode(bool networkStandbyMode) override;
            Core::hresult SetManufacturerName(string manufacturername) override;
            Core::hresult GetManufacturerName(string &manufacturername  ) override;
            Core::hresult SetModelName(string modelname) override;
            Core::hresult GetModelName(string &modelname  ) override;

            Core::hresult SetEnabled(bool enabled) override;
            Core::hresult GetEnabled(bool &enabled , bool &success ) override;
            Core::hresult SetStandbyBehavior(string standbybehavior) override;
            Core::hresult GetStandbyBehavior(string &standbybehavior , bool &success  ) override;
            Core::hresult SetFriendlyName(string friendlyname) override;
            Core::hresult GetFriendlyName(string &friendlyname , bool &success ) override;
            Core::hresult GetApiVersionNumber(uint32_t &version , bool &success) override;

            Core::hresult RegisterApplications(Exchange::IXCast::IApplicationInfoIterator* const appInfoList) override;


            virtual void onXcastApplicationLaunchRequestWithLaunchParam (string appName, string strPayLoad, string strQuery, string strAddDataUrl) override ;
            virtual void onXcastApplicationLaunchRequest(string appName, string parameter) override ;
            virtual void onXcastApplicationStopRequest(string appName, string appId) override ;
            virtual void onXcastApplicationHideRequest(string appName, string appId) override ;
            virtual void onXcastApplicationResumeRequest(string appName, string appId) override ;
            virtual void onXcastApplicationStateRequest(string appName, string appId) override ;
            virtual void onXcastUpdatePowerStateRequest(string powerState) override;
            virtual void onGDialServiceStopped(void) override;
            


        private:
            class PowerManagerNotification : public Exchange::IPowerManager::INetworkStandbyModeChangedNotification,
                                                     public Exchange::IPowerManager::IModeChangedNotification {
                private:
                    PowerManagerNotification(const PowerManagerNotification&) = delete;
                    PowerManagerNotification& operator=(const PowerManagerNotification&) = delete;

                public:
                    PowerManagerNotification() : _parent(nullptr) {}
                    explicit PowerManagerNotification(XCastImplementation* parent)
                    : _parent(parent)
                    {
                    }
                    ~PowerManagerNotification() override = default;

                public:
                    void OnPowerModeChanged(const PowerState currentState, const PowerState newState) override
                    {
                        // _parent.onPowerModeChanged(currentState, newState);
                        LOGINFO("onPowerModeChanged: State Changed %d -- > %d\r",currentState, newState);
                        m_powerState = newState;
                        LOGWARN("creating worker thread for threadPowerModeChangeEvent m_powerState :%d",m_powerState);
                        std::thread powerModeChangeThread = std::thread(&XCastImplementation::threadPowerModeChangeEvent,_parent);
                        powerModeChangeThread.detach();
                    }

                    void OnNetworkStandbyModeChanged(const bool enabled)
                    {
                        // _parent.onNetworkStandbyModeChanged(enabled);
                        _parent->m_networkStandbyMode = enabled;
                        LOGWARN("creating worker thread for threadNetworkStandbyModeChangeEvent Mode :%u", _parent->m_networkStandbyMode);
                        std::thread networkStandbyModeChangeThread = std::thread(&XCastImplementation::networkStandbyModeChangeEvent,_parent);
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
                    XCastImplementation* _parent;
                };

                public:
                    static XCastImplementation *_instance;

            
         private:
             mutable Core::CriticalSection _adminLock;
             std::list<Exchange::IXCast::INotification *> _notificationClients; // List of registered notifications
             PluginHost::IShell* _service;


            static XCastManager* m_xcast_manager;
            bool m_networkStandbyMode{false};
            PluginState _networkPluginState;

             uint32_t enableCastService(string friendlyname,bool enableService);
             void dumpDynamicAppCacheList(string strListName, std::vector<DynamicAppConfig*> appConfigList);
             bool deleteFromDynamicAppCache(vector<string>& appsToDelete);

            void dispatchEvent(Event,string callsign, const JsonObject &params);
            void Dispatch(Event event,string callsign, const JsonObject params);

            std::vector<DynamicAppConfig*> m_appConfigCache;

            WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_ControllerObj = nullptr;
            WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_NetworkPluginObj = nullptr;
            WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> * m_SystemPluginObj = NULL;

            guint m_FriendlyNameUpdateTimerID{0};
            TpTimer m_locateCastTimer;
            uint32_t Initialize(bool networkStandbyMode);
            void Deinitialize(void);
            bool connectToGDialService(void);
            std::string getSecurityToken();
            void getThunderPlugins();
            int activatePlugin(string callsign);
            int deactivatePlugin(string callsign);
            bool isPluginActivated(string callsign);
            void eventHandler_pluginState(const JsonObject& parameters);

            void updateNWConnectivityStatus(std::string nwInterface, bool nwConnected, std::string ipaddress = "");
            bool getDefaultNameAndIPAddress(std::string& interface, std::string& ipaddress);
            void eventHandler_onDefaultInterfaceChanged(const JsonObject& parameters);
            void eventHandler_ipAddressChanged(const JsonObject& parameters);

            void onLocateCastTimer();
            void startTimer(int interval);
            void stopTimer();
            bool isTimerActive();

            bool _registeredEventHandlers = false;
            void registerEventHandlers();
            void InitializePowerManager(PluginHost::IShell *service);
            void InitializeIARM();
            void DeinitializeIARM();

            PowerManagerInterfaceRef _powerManagerPlugin;
            Core::Sink<PowerManagerNotification> _pwrMgrNotification;
            void threadPowerModeChangeEvent(void);
            void networkStandbyModeChangeEvent(void);
            void getSystemPlugin();
            void onFriendlyNameUpdateHandler(const JsonObject& parameters);
                static gboolean update_friendly_name_timercallback(gpointer userdata);
           // WPEFramework::Exchange::IPowerManager::PowerState::m_powerState = WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY;
        
     };
 
     } // namespace Plugin
 } // namespace WPEFramework
