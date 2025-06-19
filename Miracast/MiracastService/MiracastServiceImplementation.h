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

#include <boost/variant.hpp>

#include <interfaces/Ids.h>
#include <interfaces/IMiracastService.h>
#include <interfaces/IPowerManager.h>

#include <MiracastController.h>
#include "libIARM.h"

#include <com/com.h>
#include <core/core.h>
#include <mutex>
#include <vector>

#include "libIBus.h"

#include "PowerManagerInterface.h"

using namespace WPEFramework;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
using MiracastLogLevel = WPEFramework::Exchange::IMiracastService::LogLevel;
using MiracastPlayerState = WPEFramework::Exchange::IMiracastService::PlayerState;
using MiracastPlayerReasonCode = WPEFramework::Exchange::IMiracastService::PlayerReasonCode;
using MiracastServiceReasonCode = WPEFramework::Exchange::IMiracastService::ReasonCode;
using ParamsType = boost::variant<std::tuple<std::string, std::string, WPEFramework::Exchange::IMiracastService::ReasonCode>,
	std::tuple<std::string, std::string, std::string, std::string>,
	std::pair<std::string, std::string>>;

typedef enum DeviceWiFiStates
{
	DEVICE_WIFI_STATE_UNINSTALLED = 0,
	DEVICE_WIFI_STATE_DISABLED = 1,
	DEVICE_WIFI_STATE_DISCONNECTED = 2,
	DEVICE_WIFI_STATE_PAIRING = 3,
	DEVICE_WIFI_STATE_CONNECTING = 4,
	DEVICE_WIFI_STATE_CONNECTED = 5,
	DEVICE_WIFI_STATE_FAILED = 6
}DEVICE_WIFI_STATES;

namespace WPEFramework
{
	namespace Plugin
	{		
		class MiracastServiceImplementation : public Exchange::IMiracastService, public MiracastServiceNotifier
		{
			public:
				// We do not allow this plugin to be copied !!
				MiracastServiceImplementation();
				~MiracastServiceImplementation() override;

				static MiracastServiceImplementation *instance(MiracastServiceImplementation *MiracastServiceImpl = nullptr);
				static MiracastController *m_miracast_ctrler_obj;

				// We do not allow this plugin to be copied !!
				MiracastServiceImplementation(const MiracastServiceImplementation &) = delete;
				MiracastServiceImplementation &operator=(const MiracastServiceImplementation &) = delete;

				virtual void onMiracastServiceClientConnectionRequest(string client_mac, string client_name) override;
				virtual void onMiracastServiceClientConnectionError(string client_mac, string client_name , MiracastServiceReasonCode reason_code ) override;
				virtual void onMiracastServiceLaunchRequest(string src_dev_ip, string src_dev_mac, string src_dev_name, string sink_dev_ip, bool is_connect_req_reported ) override;
				virtual void onStateChange(eMIRA_SERVICE_STATES state ) override;
				
				BEGIN_INTERFACE_MAP(MiracastServiceImplementation)
				INTERFACE_ENTRY(Exchange::IMiracastService)
				END_INTERFACE_MAP

			public:
				enum Event
				{
					MIRACASTSERVICE_EVENT_CLIENT_CONNECTION_REQUEST,
					MIRACASTSERVICE_EVENT_CLIENT_CONNECTION_ERROR,
					MIRACASTSERVICE_EVENT_PLAYER_LAUNCH_REQUEST
				};

				class EXTERNAL Job : public Core::IDispatch
				{
					protected:
						Job(MiracastServiceImplementation *MiracastServiceImplementation, Event event, ParamsType &params)
							: _miracastServiceImplementation(MiracastServiceImplementation), _event(event), _params(params)
						{
							if (_miracastServiceImplementation != nullptr)
							{
								_miracastServiceImplementation->AddRef();
							}
						}

					public:
						Job() = delete;
						Job(const Job &) = delete;
						Job &operator=(const Job &) = delete;
						~Job()
						{
							if (_miracastServiceImplementation != nullptr)
							{
								_miracastServiceImplementation->Release();
							}
						}

					public:
						static Core::ProxyType<Core::IDispatch> Create(MiracastServiceImplementation *miracastServiceImplementation, Event event, ParamsType params)
						{
		#ifndef USE_THUNDER_R4
							return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(miracastServiceImplementation, event, params)));
		#else
							return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(miracastServiceImplementation, event, params)));
		#endif
						}

						virtual void Dispatch()
						{
							_miracastServiceImplementation->Dispatch(_event, _params);
						}

					private:
						MiracastServiceImplementation *_miracastServiceImplementation;
						const Event _event;
						ParamsType _params;
				}; // class Job

			public:
				Core::hresult Initialize(PluginHost::IShell* service) override;
				Core::hresult Deinitialize(PluginHost::IShell* service) override;
				Core::hresult Register(Exchange::IMiracastService::INotification *notification) override;
				Core::hresult Unregister(Exchange::IMiracastService::INotification *notification) override;

				Core::hresult SetEnabled(const bool &enabled , Result &returnPayload ) override;
				Core::hresult GetEnabled(bool &enabled , bool &success ) override;
				Core::hresult AcceptClientConnection(const string &requestStatus , Result &returnPayload ) override;
				Core::hresult StopClientConnection(const string &clientMac , const string &clientName, Result &returnPayload ) override;
				Core::hresult UpdatePlayerState(const string &clientMac , const MiracastPlayerState &playerState , const MiracastPlayerReasonCode &reasonCode , Result &returnPayload ) override;
				Core::hresult SetLogging(const MiracastLogLevel &logLevel , const SeparateLogger &separateLogger , Result &returnPayload) override;
				Core::hresult SetP2PBackendDiscovery(const bool &enabled , Result &returnPayload ) override;

			private:
				class PowerManagerNotification : public Exchange::IPowerManager::IModeChangedNotification
				{
					private:
						PowerManagerNotification(const PowerManagerNotification&) = delete;
						PowerManagerNotification& operator=(const PowerManagerNotification&) = delete;
					
					public:
						explicit PowerManagerNotification(MiracastServiceImplementation& parent)
							: _parent(parent)
						{
						}
						~PowerManagerNotification() override = default;
					
					public:
						void OnPowerModeChanged(const PowerState currentState, const PowerState newState) override
						{
							_parent.onPowerModeChanged(currentState, newState);
						}

						template <typename T>
						T* baseInterface()
						{
							static_assert(std::is_base_of<T, PowerManagerNotification>(), "base type mismatch");
							return static_cast<T*>(this);
						}

						BEGIN_INTERFACE_MAP(PowerManagerNotification)
						INTERFACE_ENTRY(Exchange::IPowerManager::IModeChangedNotification)
						END_INTERFACE_MAP

					private:
						MiracastServiceImplementation& _parent;
				}; // class PowerManagerNotification

				mutable Core::CriticalSection _adminLock;
				std::mutex m_DiscoveryStateMutex;
				std::recursive_mutex m_EventMutex;
				std::string m_src_dev_ip{""};
				std::string m_src_dev_mac{""};
				std::string m_src_dev_name{""};
				std::string m_sink_dev_ip{""};
				WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_SystemPluginObj = nullptr;
				WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_WiFiPluginObj = nullptr;
				std::list<Exchange::IMiracastService::INotification *> _miracastServiceNotification; // List of registered notifications
				PluginHost::IShell *m_CurrentService;
				guint m_FriendlyNameMonitorTimerID{0};
				guint m_WiFiConnectedStateMonitorTimerID{0};
				guint m_MiracastConnectionMonitorTimerID{0};
				eMIRA_SERVICE_STATES m_eService_state;
				bool m_isServiceInitialized{false};
				bool m_isServiceEnabled{false};

				void dispatchEvent(Event, const ParamsType &params);
				void Dispatch(Event event, const ParamsType &params);

				eMIRA_SERVICE_STATES getCurrentServiceState(void);
				void changeServiceState(eMIRA_SERVICE_STATES eService_state);
				bool envGetValue(const char *key, std::string &value);
				void getThunderPlugins(void);
				bool updateSystemFriendlyName();
				void setEnableInternal(bool isEnabled);

				void InitializePowerManager(PluginHost::IShell *service);
				void registerEventHandlers();
				void onPowerModeChanged(const PowerState currentState, const PowerState newState);
				const void InitializePowerState();
				std::string getPowerStateString(PowerState pwrState);
				PowerState getPowerManagerPluginPowerState(uint32_t powerState);
				PowerState getCurrentPowerState(void);
				void setPowerStateInternal(PowerState pwrState);

				void setWiFiStateInternal(DEVICE_WIFI_STATES wifiState);

				void onFriendlyNameUpdateHandler(const JsonObject &parameters);
				void onWIFIStateChangedHandler(const JsonObject &parameters);

				static gboolean monitor_friendly_name_timercallback(gpointer userdata);
				static gboolean monitor_wifi_connection_state_timercallback(gpointer userdata);
				static gboolean monitor_miracast_connection_timercallback(gpointer userdata);
				void remove_wifi_connection_state_timer(void);
				void remove_miracast_connection_timer(void);

			public:
				static MiracastServiceImplementation *_instance;
				static PowerManagerInterfaceRef _powerManagerPlugin;
				Core::Sink<PowerManagerNotification> _pwrMgrNotification;
				bool _registeredEventHandlers;

				friend class Job;
		}; // class MiracastServiceImplementation
	} // namespace Plugin
} // namespace WPEFramework