/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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

#include "MiracastServiceImplementation.h"

#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"

#include "UtilsSynchroIarm.hpp"

using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

#define MIRACAST_DEVICE_PROPERTIES_FILE "/etc/device.properties"
#define SERVER_DETAILS "127.0.0.1:9998"
#define SYSTEM_CALLSIGN "org.rdk.System"
#define SYSTEM_CALLSIGN_VER SYSTEM_CALLSIGN ".1"
#define WIFI_CALLSIGN "org.rdk.Wifi"
#define WIFI_CALLSIGN_VER WIFI_CALLSIGN ".1"
#define SECURITY_TOKEN_LEN_MAX 1024
#define THUNDER_RPC_TIMEOUT 2000

static PowerState m_powerState = WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY;
static bool m_IsTransitionFromDeepSleep = false;
static bool m_IsWiFiConnectingState = false;

namespace WPEFramework
{
	namespace Plugin
	{
		SERVICE_REGISTRATION(MiracastServiceImplementation, MIRACAST_SERVICE_API_VERSION_NUMBER_MAJOR, MIRACAST_SERVICE_API_VERSION_NUMBER_MINOR);
		
		MiracastServiceImplementation *MiracastServiceImplementation::_instance = nullptr;
		PowerManagerInterfaceRef MiracastServiceImplementation::_powerManagerPlugin;
		MiracastController *MiracastServiceImplementation::m_miracast_ctrler_obj = nullptr;
	
		MiracastServiceImplementation::MiracastServiceImplementation()
		: _adminLock(), mShell(nullptr), _pwrMgrNotification(*this)
		{
			LOGINFO("Create MiracastServiceImplementation Instance");
			MiracastServiceImplementation::_instance = this;
			MIRACAST::logger_init("MiracastService");
		}

		MiracastServiceImplementation::~MiracastServiceImplementation()
		{
			LOGINFO("Call MiracastServiceImplementation destructor\n");
			if (_powerManagerPlugin) {
				_powerManagerPlugin.Reset();
			}
			if(m_CurrentService != nullptr)
			{
				m_CurrentService->Release();
			}
			if (nullptr != m_SystemPluginObj)
			{
				delete m_SystemPluginObj;
				m_SystemPluginObj = nullptr;
			}
			if (nullptr != m_WiFiPluginObj)
			{
				delete m_WiFiPluginObj;
				m_WiFiPluginObj = nullptr;
			}
			if (m_FriendlyNameMonitorTimerID)
			{
				g_source_remove(m_FriendlyNameMonitorTimerID);
				m_FriendlyNameMonitorTimerID = 0;
			}
			remove_wifi_connection_state_timer();
			remove_miracast_connection_timer();
			MIRACAST::logger_deinit();
			MiracastServiceImplementation::_instance = nullptr;
			mShell = nullptr;
		}

		/**
		 * Register a notification callback
		 */
		Core::hresult MiracastServiceImplementation::Register(Exchange::IMiracastService::INotification *notification)
		{
			ASSERT(nullptr != notification);

			_adminLock.Lock();
			printf("MiracastServiceImplementation::Register: notification = %p", notification);
			LOGINFO("Register notification");

			// Make sure we can't register the same notification callback multiple times
			if (std::find(_miracastServiceNotification.begin(), _miracastServiceNotification.end(), notification) == _miracastServiceNotification.end())
			{
				_miracastServiceNotification.push_back(notification);
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
		Core::hresult MiracastServiceImplementation::Unregister(Exchange::IMiracastService::INotification *notification)
		{
			Core::hresult status = Core::ERROR_GENERAL;

			ASSERT(nullptr != notification);

			_adminLock.Lock();

			// we just unregister one notification once
			auto itr = std::find(_miracastServiceNotification.begin(), _miracastServiceNotification.end(), notification);
			if (itr != _miracastServiceNotification.end())
			{
				(*itr)->Release();
				LOGINFO("Unregister notification");
				_miracastServiceNotification.erase(itr);
				status = Core::ERROR_NONE;
			}
			else
			{
				LOGERR("notification not found");
			}

			_adminLock.Unlock();

			return status;
		}

		/*  Helper methods Start */
		/* ------------------------------------------------------------------------------------------------------- */
		eMIRA_SERVICE_STATES MiracastServiceImplementation::getCurrentServiceState(void)
		{
			MIRACASTLOG_INFO("current state [%#08X]",m_eService_state);
			return m_eService_state;
		}

		void MiracastServiceImplementation::changeServiceState(eMIRA_SERVICE_STATES eService_state)
		{
			eMIRA_SERVICE_STATES old_state = m_eService_state,
								new_state = eService_state;
			m_eService_state = eService_state;
			MIRACASTLOG_INFO("changing state [%#08X] -> [%#08X]",old_state,new_state);
		}

		std::string MiracastServiceImplementation::reasonDescription(eMIRACAST_SERVICE_ERR_CODE e)
		{
			switch (e)
			{
				case MIRACAST_SERVICE_ERR_CODE_SUCCESS:
					return "SUCCESS";
				case MIRACAST_SERVICE_ERR_CODE_P2P_GROUP_NEGO_ERROR:
					return "P2P GROUP NEGOTIATION FAILURE.";
				case MIRACAST_SERVICE_ERR_CODE_P2P_GROUP_FORMATION_ERROR:
					return "P2P GROUP FORMATION FAILURE.";
				case MIRACAST_SERVICE_ERR_CODE_GENERIC_FAILURE:
					return "P2P GENERIC FAILURE.";
				case MIRACAST_SERVICE_ERR_CODE_P2P_CONNECT_ERROR:
					return "P2P CONNECT FAILURE.";
				default:
					return "Unimplemented item";
			}
		}

		bool MiracastServiceImplementation::envGetValue(const char *key, std::string &value)
		{
			std::ifstream fs(MIRACAST_DEVICE_PROPERTIES_FILE, std::ifstream::in);
			std::string::size_type delimpos;
			std::string line;
			if (!fs.fail())
			{
				while (std::getline(fs, line))
				{
					if (!line.empty() && ((delimpos = line.find('=')) > 0))
					{
						std::string itemKey = line.substr(0, delimpos);
						if (itemKey == key)
						{
							value = line.substr(delimpos + 1, std::string::npos);
							return true;
						}
					}
				}
			}
			return false;
		}

		void MiracastServiceImplementation::getThunderPlugins(void)
		{
			MIRACASTLOG_INFO("Entering..!!!");

			if (nullptr == m_SystemPluginObj)
			{
				string token;
				// TODO: use interfaces and remove token
				auto security = m_CurrentService->QueryInterfaceByCallsign<PluginHost::IAuthenticate>("SecurityAgent");
				if (nullptr != security)
				{
					string payload = "http://localhost";
					if (security->CreateToken(static_cast<uint16_t>(payload.length()),
											reinterpret_cast<const uint8_t *>(payload.c_str()),
											token) == Core::ERROR_NONE)
					{
						MIRACASTLOG_INFO("got security token\n");
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
				m_SystemPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(SYSTEM_CALLSIGN_VER), (_T("MiracastService")), false, query);
				if (nullptr == m_SystemPluginObj)
				{
					LOGERR("JSONRPC: %s: initialization failed", SYSTEM_CALLSIGN_VER);
				}
				else
				{
					MIRACASTLOG_INFO("JSONRPC: %s: initialization ok", SYSTEM_CALLSIGN_VER);
				}

				m_WiFiPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(WIFI_CALLSIGN_VER), (_T("MiracastService")), false, query);
				if (nullptr == m_WiFiPluginObj)
				{
					LOGERR("JSONRPC: %s: initialization failed", WIFI_CALLSIGN_VER);
				}
				else
				{
					MIRACASTLOG_INFO("JSONRPC: %s: initialization ok", WIFI_CALLSIGN_VER);
				}
			}
			MIRACASTLOG_INFO("Exiting..!!!");
		}

		bool MiracastServiceImplementation::updateSystemFriendlyName()
		{
			JsonObject params, Result;
			bool return_value = false;
			MIRACASTLOG_INFO("Entering..!!!");

			getThunderPlugins();

			if (nullptr == m_SystemPluginObj)
			{
				LOGERR("m_SystemPluginObj not yet instantiated");
				return false;
			}

			uint32_t ret = m_SystemPluginObj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("getFriendlyName"), params, Result);

			if (Core::ERROR_NONE == ret)
			{
				if (Result["success"].Boolean())
				{
					std::string friendlyName = "";
					friendlyName = Result["friendlyName"].String();
					m_miracast_ctrler_obj->set_FriendlyName(friendlyName,m_isServiceEnabled);
					MIRACASTLOG_INFO("Miracast FriendlyName=%s", friendlyName.c_str());
					return_value = true;
				}
				else
				{
					ret = Core::ERROR_GENERAL;
					LOGERR("updateSystemFriendlyName call failed");
				}
			}
			else
			{
				LOGERR("updateSystemFriendlyName call failed E[%u]", ret);
			}
			return return_value;
		}

		void MiracastServiceImplementation::setEnableInternal(bool isEnabled)
		{
			MIRACASTLOG_INFO("Entering..!!!");
			if ( nullptr != m_miracast_ctrler_obj )
			{
				m_miracast_ctrler_obj->set_enable(isEnabled);
			}
			MIRACASTLOG_INFO("Exiting..!!!");
		}
		/*  Helper and Internal methods End */
		/* ------------------------------------------------------------------------------------------------------- */

		void MiracastServiceImplementation::dispatchEvent(Event event, const JsonObject &params)
		{
			Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
		}

		void MiracastServiceImplementation::Dispatch(Event event,const JsonObject& params)
		{
			_adminLock.Lock();

			std::list<Exchange::IMiracastService::INotification *>::const_iterator index(_miracastServiceNotification.begin());

			switch (event)
			{
				case MIRACASTSERVICE_EVENT_CLIENT_CONNECTION_REQUEST:
				{
					while (index != _miracastServiceNotification.end())
					{
						string clientMac = params["mac"].String();
						string clientName = params["name"].String();
						(*index)->OnClientConnectionRequest(clientMac , clientName );
						++index;
					}
				}
				break;
				case MIRACASTSERVICE_EVENT_CLIENT_CONNECTION_ERROR:
				{
					while (index != _miracastServiceNotification.end())
					{
						string clientMac = params["mac"].String();
						string clientName = params["name"].String();
						string errorCode = params["error_code"].String();
						string reason = params["reason"].String();
						(*index)->OnClientConnectionError(clientMac , clientName , errorCode , reason);
						++index;
					}
				}
				break;
				case MIRACASTSERVICE_EVENT_PLAYER_LAUNCH_REQUEST:
				{
					while (index != _miracastServiceNotification.end())
					{
						DeviceParameters deviceParameters;
						deviceParameters.sourceDeviceIP = params["source_dev_ip"].String();
						deviceParameters.sourceDeviceMac = params["source_dev_mac"].String();
						deviceParameters.sourceDeviceName = params["source_dev_name"].String();
						deviceParameters.sinkDeviceIP = params["sink_dev_ip"].String();
						(*index)->OnLaunchRequest(deviceParameters);
						++index;
					}
				}
				break;
				default:
					LOGWARN("Event[%u] not handled", event);
					break;
			}
			_adminLock.Unlock();
		}

		/*  COMRPC Methods Start */
		/* ------------------------------------------------------------------------------------------------------- */
		Core::hresult MiracastServiceImplementation::Initialize(PluginHost::IShell *service)
		{
			Core::hresult result = Core::ERROR_GENERAL;

			ASSERT(nullptr != service);

			m_CurrentService = service;

			if (nullptr != m_CurrentService)
			{
				m_CurrentService->AddRef();
				string	msg,
						p2p_ctrl_iface = "";
				MIRACASTLOG_INFO("Entering..!!!");

				if (!(envGetValue("WIFI_P2P_CTRL_INTERFACE", p2p_ctrl_iface)))
				{
					MIRACASTLOG_ERROR("WIFI_P2P_CTRL_INTERFACE not configured in device properties file");
					return Core::ERROR_GENERAL;
				}

				if (!m_isServiceInitialized)
				{
					MiracastError ret_code = MIRACAST_OK;

					InitializePowerManager(service);
					InitializePowerState();
			
					m_miracast_ctrler_obj = MiracastController::getInstance(ret_code, this,p2p_ctrl_iface);
					if (nullptr != m_miracast_ctrler_obj)
					{
						m_CurrentService = service;
						getThunderPlugins();
						// subscribe for event
						if (nullptr != m_SystemPluginObj)
						{
							m_SystemPluginObj->Subscribe<JsonObject>(1000, "onFriendlyNameChanged", &MiracastServiceImplementation::onFriendlyNameUpdateHandler, this);
						}
						if (nullptr != m_WiFiPluginObj)
						{
							m_WiFiPluginObj->Subscribe<JsonObject>(1000, "onWIFIStateChanged", &MiracastServiceImplementation::onWIFIStateChangedHandler, this);
						}

						if ( false == updateSystemFriendlyName())
						{
							m_FriendlyNameMonitorTimerID = g_timeout_add(2000, MiracastServiceImplementation::monitor_friendly_name_timercallback, this);
							MIRACASTLOG_WARNING("Unable to get friendlyName, requires polling [%u]...",m_FriendlyNameMonitorTimerID);
						}
						else
						{
							MIRACASTLOG_INFO("friendlyName updated properly...");
						}
						m_isServiceInitialized = true;
						m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_IDLE;
						result = Core::ERROR_NONE;
					}
					else
					{
						switch (ret_code)
						{
							case MIRACAST_INVALID_P2P_CTRL_IFACE:
							{
								MIRACASTLOG_ERROR("Invalid P2P Ctrl iface configured");
							}
							break;
							case MIRACAST_CONTROLLER_INIT_FAILED:
							{
								MIRACASTLOG_ERROR("Controller Init Failed");
							}
							break;
							case MIRACAST_P2P_INIT_FAILED:
							{
								MIRACASTLOG_ERROR("P2P Init Failed");
							}
							break;
							default:
							{
								MIRACASTLOG_ERROR("Unknown Error:Failed to obtain MiracastController Object");
							}
							break;
						}
					}
				}
			}
			return result;
		}

		Core::hresult MiracastServiceImplementation::Deinitialize(PluginHost::IShell* service)
		{
			MIRACASTLOG_INFO("Entering..!!!");

			ASSERT(nullptr != service);

			if (_powerManagerPlugin)
			{
				_powerManagerPlugin->Unregister(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
				_powerManagerPlugin.Reset();
			}
			_registeredEventHandlers = false;

			if (m_WiFiPluginObj)
			{
				m_WiFiPluginObj->Unsubscribe(1000, _T("onWIFIStateChanged"));
				delete m_WiFiPluginObj;
				m_WiFiPluginObj = nullptr;
			}

			if (m_SystemPluginObj)
			{
				m_SystemPluginObj->Unsubscribe(1000, _T("onFriendlyNameChanged"));
				delete m_SystemPluginObj;
				m_SystemPluginObj = nullptr;
			}

			LOGINFO("Disconnect from the COM-RPC socket\n");
			_registeredEventHandlers = false;

			if (m_isServiceInitialized)
			{
				MiracastController::destroyInstance();
				m_CurrentService = nullptr;
				m_miracast_ctrler_obj = nullptr;
				m_isServiceInitialized = false;
				m_isServiceEnabled = false;
				MIRACASTLOG_INFO("Done..!!!");
			}

			if (nullptr != m_CurrentService)
			{
				m_CurrentService->Release();
				m_CurrentService = nullptr;
			}
			MIRACASTLOG_INFO("Exiting..!!!");
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastServiceImplementation::SetEnabled(const bool &enabled , Result &returnPayload )
		{
			bool isSuccessOrFailure = false;
			lock_guard<mutex> lck(m_DiscoveryStateMutex);
			eMIRA_SERVICE_STATES current_state = getCurrentServiceState();
			if (enabled)
			{
				if (!m_isServiceEnabled)
				{
					lock_guard<recursive_mutex> lock(m_EventMutex);
					m_isServiceEnabled = true;
					if (m_IsTransitionFromDeepSleep)
					{
						LOGINFO("#### MCAST-TRIAGE-OK Enable Miracast discovery Async");
						m_miracast_ctrler_obj->restart_discoveryAsync();
						m_IsTransitionFromDeepSleep = false;
					}
					else
					{
						setEnableInternal(true);
					}
					returnPayload.message = "Successfully enabled the WFD Discovery";
					isSuccessOrFailure = true;
				}
				else
				{
					returnPayload.message = "WFD Discovery already enabled.";
				}
			}
			else
			{
				if ( MIRACAST_SERVICE_STATE_PLAYER_LAUNCHED == current_state )
				{
					returnPayload.message = "Failed as MiracastPlayer already Launched";
				}
				else if (m_isServiceEnabled)
				{
					lock_guard<recursive_mutex> lock(m_EventMutex);
					m_isServiceEnabled = false;
					if (!m_IsTransitionFromDeepSleep)
					{
						if ( MIRACAST_SERVICE_STATE_RESTARTING_SESSION == current_state )
						{
							m_miracast_ctrler_obj->stop_discoveryAsync();
						}
						else
						{
							setEnableInternal(false);
						}
					}
					else
					{
						LOGINFO("#### MCAST-TRIAGE-OK Skipping Disable discovery as done by PwrMgr");
					}
					remove_wifi_connection_state_timer();
					remove_miracast_connection_timer();
					isSuccessOrFailure = true;
					returnPayload.message = "Successfully disabled the WFD Discovery";
				}
				else
				{
					returnPayload.message = "WFD Discovery already disabled.";
				}
			}
			returnPayload.success = isSuccessOrFailure;
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastServiceImplementation::GetEnabled(bool &enabled , bool &success )
		{
			MIRACASTLOG_INFO("Entering..!!!");
			enabled = m_isServiceEnabled;
			success = true;
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastServiceImplementation::AcceptClientConnection(const string &requestStatus , Result &returnPayload )
		{
			bool isSuccessOrFailure = false;
			if (("Accept" == requestStatus) || ("Reject" == requestStatus))
			{
				lock_guard<recursive_mutex> lock(m_EventMutex);
				eMIRA_SERVICE_STATES current_state = getCurrentServiceState();
				isSuccessOrFailure = true;

				remove_miracast_connection_timer();

				if ( MIRACAST_SERVICE_STATE_DIRECT_LAUCH_WITH_CONNECTING == current_state)
				{
					if ("Accept" == requestStatus)
					{
						MIRACASTLOG_INFO("#### Notifying Launch Request ####");
						m_miracast_ctrler_obj->switch_launch_request_context(m_src_dev_ip, m_src_dev_mac, m_src_dev_name, m_sink_dev_ip );
						changeServiceState(MIRACAST_SERVICE_STATE_CONNECTION_ACCEPTED);
					}
					else
					{
						changeServiceState(MIRACAST_SERVICE_STATE_CONNECTION_REJECTED);
						m_miracast_ctrler_obj->restart_session_discovery(m_src_dev_mac);
						m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_IDLE;
						changeServiceState(MIRACAST_SERVICE_STATE_RESTARTING_SESSION);
						MIRACASTLOG_INFO("#### Refreshing the Session ####");
					}
					m_src_dev_ip.clear();
					m_src_dev_mac.clear();
					m_src_dev_name.clear();
					m_sink_dev_ip.clear();
				}
				else
				{
					if ( MIRACAST_SERVICE_STATE_CONNECTING == current_state )
					{
						m_miracast_ctrler_obj->accept_client_connection(requestStatus);
						if ("Accept" == requestStatus)
						{
							changeServiceState(MIRACAST_SERVICE_STATE_CONNECTION_ACCEPTED);
						}
						else
						{
							changeServiceState(MIRACAST_SERVICE_STATE_CONNECTION_REJECTED);
						}
					}
					else
					{
						MIRACASTLOG_INFO("Ignoring '%s' as Session already Refreshed and Current State[%#08X]",requestStatus.c_str(),current_state);
					}
				}
			}
			else
			{
				returnPayload.message = "Supported 'requestStatus' parameter values are Accept or Reject";
				LOGERR("Unsupported param passed [%s]", requestStatus.c_str());
			}
			returnPayload.success = isSuccessOrFailure;
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastServiceImplementation::StopClientConnection(const string &clienMac , const string &clienName, Result &returnPayload )
		{
			bool isSuccessOrFailure = false;
			lock_guard<recursive_mutex> lock(m_EventMutex);
			eMIRA_SERVICE_STATES current_state = getCurrentServiceState();

			if ( MIRACAST_SERVICE_STATE_CONNECTION_ACCEPTED != current_state )
			{
				MIRACASTLOG_WARNING("stopClientConnection Already Received..!!!");
				returnPayload.message = "stopClientConnection Already Received";
			}
			else
			{
				if ((( 0 == clienName.compare(m_miracast_ctrler_obj->get_WFDSourceName())) &&
					( 0 == clienMac.compare(m_miracast_ctrler_obj->get_WFDSourceMACAddress())))||
					(( 0 == clienName.compare(m_miracast_ctrler_obj->get_NewSourceName())) &&
					( 0 == clienMac.compare(m_miracast_ctrler_obj->get_NewSourceMACAddress()))))
				{
					std::string cached_mac_address = "";
					if ( 0 == clienMac.compare(m_miracast_ctrler_obj->get_NewSourceMACAddress()))
					{
						cached_mac_address = clienMac;
					}

					if ( MIRACAST_SERVICE_STATE_PLAYER_LAUNCHED != current_state )
					{
						changeServiceState(MIRACAST_SERVICE_STATE_APP_REQ_TO_ABORT_CONNECTION);
						m_miracast_ctrler_obj->restart_session_discovery(cached_mac_address);
						changeServiceState(MIRACAST_SERVICE_STATE_RESTARTING_SESSION);
						isSuccessOrFailure = true;
					}
					else
					{
						returnPayload.message = "stopClientConnection received after Launch";
						LOGERR("stopClientConnection received after Launch..!!!");
					}
				}
				else
				{
					returnPayload.message = "Invalid MAC and Name";
					LOGERR("Invalid MAC and Name[%s][%s]..!!!",clienMac.c_str(),clienName.c_str());
				}
			}
			MIRACASTLOG_INFO("Exiting..!!!");
			returnPayload.success = isSuccessOrFailure;
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastServiceImplementation::UpdatePlayerState(const string &clientMac , const string &playerState , const PlayerReasonCode &reasonCode , Result &returnPayload )
		{
			bool restart_discovery_needed = false;

			if ("PLAYING" == playerState || "playing" == playerState)
			{
				m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_PLAYING;
			}
			else if ("STOPPED" == playerState || "stopped" == playerState)
			{
				eM_PLAYER_REASON_CODE stop_reason_code = static_cast<eM_PLAYER_REASON_CODE>(reasonCode);

				if ( MIRACAST_PLAYER_REASON_CODE_NEW_SRC_DEV_CONNECT_REQ == stop_reason_code )
				{
					MIRACASTLOG_INFO("!!! STOPPED RECEIVED FOR NEW CONECTION !!!");
					m_miracast_ctrler_obj->flush_current_session();
				}
				else
				{
					restart_discovery_needed = true;
					if ( MIRACAST_PLAYER_REASON_CODE_APP_REQ_TO_STOP == stop_reason_code )
					{
						MIRACASTLOG_INFO("!!! STOPPED RECEIVED FOR ON EXIT !!!");
					}
					else if ( MIRACAST_PLAYER_REASON_CODE_SRC_DEV_REQ_TO_STOP == stop_reason_code )
					{
						MIRACASTLOG_INFO("!!! SRC DEV TEARDOWN THE CONNECTION !!!");
					}
					else
					{
						MIRACASTLOG_ERROR("!!! STOPPED RECEIVED FOR REASON[%#04X] !!!",stop_reason_code);
					}
				}
				m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_STOPPED;
			}
			else if ("INITIATED" == playerState || "initiated" == playerState)
			{
				m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_INITIATED;
			}
			else if ("INPROGRESS" == playerState || "inprogress" == playerState)
			{
				m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_INPROGRESS;
			}
			else
			{
				m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_IDLE;
			}

			if ( m_isServiceEnabled && restart_discovery_needed )
			{
				// It will restart the discovering
				m_miracast_ctrler_obj->restart_session_discovery(clientMac);
				changeServiceState(MIRACAST_SERVICE_STATE_RESTARTING_SESSION);
			}
			MIRACASTLOG_INFO("Player State set to [%s (%d)] for Source device [%s].", playerState.c_str(), (int)m_miracast_ctrler_obj->m_ePlayer_state, clientMac.c_str());

			MIRACASTLOG_INFO("Exiting..!!!");
			returnPayload.success = true;
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastServiceImplementation::SetLogging(const string &logLevel , const SeparateLogger &separateLogger , Result &returnPayload)
		{
			bool isSuccessOrFailure = false;

			if (!separateLogger.logStatus.empty())
			{
				std::string status = separateLogger.logStatus;

				if ( "ENABLE" == separateLogger.logStatus || "enable" == separateLogger.logStatus )
				{
					if (!separateLogger.logfileName.empty())
					{
						MIRACAST::enable_separate_logger(separateLogger.logfileName);
						isSuccessOrFailure = true;
					}
					else
					{
						returnPayload.message = "'separate_logger.logfilename' parameter is required";
					}
				}
				else if ( "DISABLE" == separateLogger.logStatus || "disable" == separateLogger.logStatus )
				{
					MIRACAST::disable_separate_logger();
					isSuccessOrFailure = true;
				}
				else
				{
					returnPayload.message = "Supported 'separate_logger.status' parameter values are ENABLE or DISABLE";
				}
			}

			if (!logLevel.empty())
			{
				LogLevel level = FATAL_LEVEL;
				isSuccessOrFailure = true;
				if ("FATAL" == logLevel || "fatal" == logLevel)
				{
					level = FATAL_LEVEL;
				}
				else if ("ERROR" == logLevel || "error" == logLevel)
				{
					level = ERROR_LEVEL;
				}
				else if ("WARNING" == logLevel || "warning" == logLevel)
				{
					level = WARNING_LEVEL;
				}
				else if ("INFO" == logLevel || "info" == logLevel)
				{
					level = INFO_LEVEL;
				}
				else if ("VERBOSE" == logLevel || "verbose" == logLevel)
				{
					level = VERBOSE_LEVEL;
				}
				else if ("TRACE" == logLevel || "trace" == logLevel)
				{
					level = TRACE_LEVEL;
				}
				else
				{
					returnPayload.message = "Supported 'level' parameter values are FATAL, ERROR, WARNING, INFO, VERBOSE or TRACE";
					isSuccessOrFailure = false;
				}

				if (isSuccessOrFailure)
				{
					set_loglevel(level);
				}
			}
			returnPayload.success = isSuccessOrFailure;
			MIRACASTLOG_INFO("Exiting..!!!");
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastServiceImplementation::SetP2PBackendDiscovery(const bool &enabled , Result &returnPayload )
		{
			MIRACASTLOG_INFO("Entering..!!!");
			if (m_miracast_ctrler_obj)
			{
				m_miracast_ctrler_obj->setP2PBackendDiscovery(enabled);
			}
			MIRACASTLOG_INFO("Exiting..!!!");
			returnPayload.success = true;
			return Core::ERROR_NONE;
		}
		/*  COMRPC Methods End */
		/* ------------------------------------------------------------------------------------------------------- */

		/*  Events Start */
		/* ------------------------------------------------------------------------------------------------------- */
		void MiracastServiceImplementation::onMiracastServiceClientConnectionRequest(string client_mac, string client_name)
		{
			std::string requestStatus = "Accept";
			bool is_another_connect_request = false;
			MIRACASTLOG_INFO("Entering..!!!");

			lock_guard<recursive_mutex> lock(m_EventMutex);
			eMIRA_SERVICE_STATES current_state = getCurrentServiceState();

			if ( MIRACAST_SERVICE_STATE_PLAYER_LAUNCHED == current_state )
			{
				is_another_connect_request = true;
				MIRACASTLOG_WARNING("Another Connect Request received while casting");
			}

			if (0 == access("/opt/miracast_autoconnect", F_OK))
			{
				char commandBuffer[768] = {0};

				if ( is_another_connect_request )
				{
					MIRACASTLOG_INFO("!!! NEED TO STOP ONGOING SESSION !!!");
					strncpy(commandBuffer,"curl -H \"Authorization: Bearer `WPEFrameworkSecurityUtility | cut -d '\"' -f 4`\" --header \"Content-Type: application/json\" --request POST --data '{\"jsonrpc\":\"2.0\", \"id\":3,\"method\":\"org.rdk.MiracastPlayer.1.stopRequest\", \"params\":{\"reason\": \"NEW_CONNECTION\"}}' http://127.0.0.1:9998/jsonrpc &",sizeof(commandBuffer));
					commandBuffer[sizeof(commandBuffer) - 1] = '\0';
					MIRACASTLOG_INFO("Stopping old Session by [%s]",commandBuffer);
					MiracastCommon::execute_SystemCommand(commandBuffer);
					sleep(1);
				}
				if (MIRACAST_SERVICE_STATE_DIRECT_LAUCH_REQUESTED == current_state)
				{
					changeServiceState(MIRACAST_SERVICE_STATE_DIRECT_LAUCH_WITH_CONNECTING);
				}
				else
				{
					changeServiceState(MIRACAST_SERVICE_STATE_CONNECTING);
				}
				memset(commandBuffer,0x00,sizeof(commandBuffer));
				snprintf( commandBuffer,
						sizeof(commandBuffer),
						"curl -H \"Authorization: Bearer `WPEFrameworkSecurityUtility | cut -d '\"' -f 4`\" --header \"Content-Type: application/json\" --request POST --data '{\"jsonrpc\":\"2.0\", \"id\":3,\"method\":\"org.rdk.MiracastService.1.acceptClientConnection\", \"params\":{\"requestStatus\": \"%s\"}}' http://127.0.0.1:9998/jsonrpc &",
						requestStatus.c_str());
				MIRACASTLOG_INFO("AutoConnecting [%s - %s] by [%s]",client_name.c_str(),client_mac.c_str(),commandBuffer);
				MiracastCommon::execute_SystemCommand(commandBuffer);
			}
			else
			{
				JsonObject params;
				params["mac"] = client_mac;
				params["name"] = client_name;
				dispatchEvent(MIRACASTSERVICE_EVENT_CLIENT_CONNECTION_REQUEST, params);

				m_src_dev_mac = client_mac;

				if (MIRACAST_SERVICE_STATE_DIRECT_LAUCH_REQUESTED == current_state)
				{
					changeServiceState(MIRACAST_SERVICE_STATE_DIRECT_LAUCH_WITH_CONNECTING);
				}
				else
				{
					changeServiceState(MIRACAST_SERVICE_STATE_CONNECTING);
				}
				m_MiracastConnectionMonitorTimerID = g_timeout_add(40000, MiracastServiceImplementation::monitor_miracast_connection_timercallback, this);
				MIRACASTLOG_INFO("Timer created to Monitor Miracast Connection Status [%u]",m_MiracastConnectionMonitorTimerID);
			}
		}

		void MiracastServiceImplementation::onMiracastServiceClientConnectionError(string client_mac, string client_name , eMIRACAST_SERVICE_ERR_CODE error_code )
		{
			MIRACASTLOG_INFO("Entering..!!!");

			lock_guard<recursive_mutex> lock(m_EventMutex);
			eMIRA_SERVICE_STATES current_state = getCurrentServiceState();

			if ( MIRACAST_SERVICE_STATE_CONNECTION_ACCEPTED != current_state )
			{
				MIRACASTLOG_INFO("Session already refreshed, So no need to report Error. Current state [%#08X]",current_state);
			}
			else
			{
				JsonObject params;
				params["mac"] = client_mac;
				params["name"] = client_name;
				params["error_code"] = std::to_string(error_code);
				params["reason"] = reasonDescription(error_code);
				dispatchEvent(MIRACASTSERVICE_EVENT_CLIENT_CONNECTION_ERROR, params);
			}
			MIRACASTLOG_INFO("Exiting..!!!");
		}

		void MiracastServiceImplementation::onMiracastServiceLaunchRequest(string src_dev_ip, string src_dev_mac, string src_dev_name, string sink_dev_ip, bool is_connect_req_reported )
		{
			lock_guard<recursive_mutex> lock(m_EventMutex);
			eMIRA_SERVICE_STATES current_state = getCurrentServiceState();
			MIRACASTLOG_INFO("Entering[%u]..!!!",is_connect_req_reported);

			if ( !is_connect_req_reported )
			{
				changeServiceState(MIRACAST_SERVICE_STATE_DIRECT_LAUCH_REQUESTED);
				m_src_dev_ip = src_dev_ip;
				m_src_dev_mac = src_dev_mac;
				m_src_dev_name = src_dev_name;
				m_sink_dev_ip = sink_dev_ip;
				MIRACASTLOG_INFO("Direct Launch request has received. So need to notify connect Request");
				onMiracastServiceClientConnectionRequest( src_dev_mac, src_dev_name );
			}
			else if ( MIRACAST_SERVICE_STATE_CONNECTION_ACCEPTED != current_state )
			{
				MIRACASTLOG_INFO("Session already refreshed, So no need to notify Launch Request. Current state [%#08X]",current_state);
				//m_miracast_ctrler_obj->restart_session_discovery();
			}
			else
			{
				JsonObject params;
				JsonObject device_params;
				device_params["source_dev_ip"] = src_dev_ip;
				device_params["source_dev_mac"] = src_dev_mac;
				device_params["source_dev_name"] = src_dev_name;
				device_params["sink_dev_ip"] = sink_dev_ip;
				params["device_parameters"] = device_params;

				if (0 == access("/opt/miracast_autoconnect", F_OK))
				{
					char commandBuffer[768] = {0};
					snprintf( commandBuffer,
							sizeof(commandBuffer),
							"curl -H \"Authorization: Bearer `WPEFrameworkSecurityUtility | cut -d '\"' -f 4`\" --header \"Content-Type: application/json\" --request POST --data '{\"jsonrpc\":\"2.0\", \"id\":3,\"method\":\"org.rdk.MiracastPlayer.1.playRequest\", \"params\":{\"device_parameters\": {\"source_dev_ip\": \"%s\",\"source_dev_mac\": \"%s\",\"source_dev_name\": \"%s\",\"sink_dev_ip\": \"%s\"},\"video_rectangle\": {\"X\": 0,\"Y\": 0,\"W\": 1280,\"H\": 720}}}' http://127.0.0.1:9998/jsonrpc &",
							src_dev_ip.c_str(),
							src_dev_mac.c_str(),
							src_dev_name.c_str(),
							sink_dev_ip.c_str());
					MIRACASTLOG_INFO("System Command [%s]",commandBuffer);
					MiracastCommon::execute_SystemCommand(commandBuffer);
				}
				else
				{
					dispatchEvent(MIRACASTSERVICE_EVENT_PLAYER_LAUNCH_REQUEST, params);
				}
				changeServiceState(MIRACAST_SERVICE_STATE_PLAYER_LAUNCHED);
			}
		}

		void MiracastServiceImplementation::onStateChange(eMIRA_SERVICE_STATES state)
		{
			MIRACASTLOG_INFO("Entering state [%#08X]",state);
			lock_guard<recursive_mutex> lock(m_EventMutex);
			switch (state)
			{
				case MIRACAST_SERVICE_STATE_IDLE:
				case MIRACAST_SERVICE_STATE_DISCOVERABLE:
				{
					if ((!m_isServiceEnabled) && (MIRACAST_SERVICE_STATE_DISCOVERABLE == state))
					{
						/*User already disabled the discovery, so should not enable again.*/
						m_miracast_ctrler_obj->stop_discoveryAsync();
					}
					else
					{
						changeServiceState(state);
					}
				}
				break;
				default:
				{

				}
				break;
			}
			MIRACASTLOG_INFO("Exiting...");
		}
		/*  Events End */
		/* ------------------------------------------------------------------------------------------------------- */

		/*  PowerManager related methods Start */
		/* ------------------------------------------------------------------------------------------------------- */
		void MiracastServiceImplementation::InitializePowerManager(PluginHost::IShell *service)
		{
			_powerManagerPlugin = PowerManagerInterfaceBuilder(_T("org.rdk.PowerManager"))
										.withIShell(service)
										.withRetryIntervalMS(200)
										.withRetryCount(25)
										.createInterface();
			registerEventHandlers();
		}

		void MiracastServiceImplementation::registerEventHandlers()
		{
			ASSERT (_powerManagerPlugin);
			if(!_registeredEventHandlers && _powerManagerPlugin)
			{
				_registeredEventHandlers = true;
				_powerManagerPlugin->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
			}
		}

		void MiracastServiceImplementation::onPowerModeChanged(const PowerState currentState, const PowerState newState)
		{
			if (nullptr == _instance)
			{
				LOGERR("#### MCAST-TRIAGE-NOK-PWR Miracast Service not enabled yet ####");
				return;
			}
			lock_guard<mutex> lck(_instance->m_DiscoveryStateMutex);
			_instance->setPowerStateInternal(newState);
		}

		const void MiracastServiceImplementation::InitializePowerState()
		{
			Core::hresult res = Core::ERROR_GENERAL;
			PowerState pwrStateCur = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
			PowerState pwrStatePrev = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;

			ASSERT (_powerManagerPlugin);
			if (_powerManagerPlugin){
				res = _powerManagerPlugin->GetPowerState(pwrStateCur, pwrStatePrev);
				if (Core::ERROR_NONE == res)
				{
					setPowerStateInternal(pwrStateCur);
					LOGINFO("Current state is (%d) \n",pwrStateCur);
				}
			}
		}

		std::string MiracastServiceImplementation::getPowerStateString(PowerState pwrState)
		{
			std::string pwrStateStr = "";

			switch (pwrState) 
			{
				case WPEFramework::Exchange::IPowerManager::POWER_STATE_ON: pwrStateStr = "ON"; break;
				case WPEFramework::Exchange::IPowerManager::POWER_STATE_OFF: pwrStateStr = "OFF"; break;
				case WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY: pwrStateStr = "LIGHT_SLEEP"; break;
				case WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_LIGHT_SLEEP: pwrStateStr = "LIGHT_SLEEP"; break;
				case WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_DEEP_SLEEP: pwrStateStr = "DEEP_SLEEP"; break;
				default: pwrStateStr = "UNKNOWN"; break;
			}
			return pwrStateStr;
		}

		PowerState MiracastServiceImplementation::getPowerManagerPluginPowerState(uint32_t powerState)
		{
			PowerState ret_power_state = (PowerState)powerState++;
			return ret_power_state;
		}

		PowerState MiracastServiceImplementation::getCurrentPowerState(void)
		{
			MIRACASTLOG_INFO("current power state [%s]",getPowerStateString(m_powerState).c_str());
			return m_powerState;
		}

		void MiracastServiceImplementation::setPowerStateInternal(PowerState pwrState)
		{
			PowerState old_pwr_state = m_powerState,
										new_pwr_state = pwrState;
			m_powerState = pwrState;
			MIRACASTLOG_INFO("changing power state [%s] -> [%s]",
								getPowerStateString(old_pwr_state).c_str(),
								getPowerStateString(new_pwr_state).c_str());
			if (WPEFramework::Exchange::IPowerManager::POWER_STATE_ON == pwrState)
			{
				lock_guard<recursive_mutex> lock(_instance->m_EventMutex);
				if ((m_IsTransitionFromDeepSleep) && (_instance->m_isServiceEnabled))
				{
					LOGINFO("#### MCAST-TRIAGE-OK-PWR Enable Miracast discovery from PwrMgr [%d]",_instance->m_isServiceEnabled);
					_instance->m_miracast_ctrler_obj->restart_discoveryAsync();
					m_IsTransitionFromDeepSleep = false;
				}
				else if (!_instance->m_isServiceEnabled)
				{
					LOGINFO("#### MCAST-TRIAGE-OK-PWR Miracast discovery already Disabled [%d]. No need to enable it",_instance->m_isServiceEnabled);
				}
			}
			else if (WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_DEEP_SLEEP == pwrState)
			{
				lock_guard<recursive_mutex> lock(_instance->m_EventMutex);
				if ( _instance->m_isServiceEnabled )
				{
					LOGINFO("#### MCAST-TRIAGE-OK-PWR Miracast Discovery Disabled ####");
					_instance->setEnableInternal(false);
				}
				else
				{
					LOGINFO("#### MCAST-TRIAGE-OK-PWR Miracast discovery already Disabled [%d]. No need to disable it",_instance->m_isServiceEnabled);
				}
				_instance->remove_wifi_connection_state_timer();
				_instance->remove_miracast_connection_timer();
				m_IsTransitionFromDeepSleep = true;
			}
		}
		/*  PowerManager related methods End */
		/* ------------------------------------------------------------------------------------------------------- */

		/*  WiFi related methods Start */
		/* ------------------------------------------------------------------------------------------------------- */
		void MiracastServiceImplementation::setWiFiStateInternal(DEVICE_WIFI_STATES wifiState)
		{
			MIRACASTLOG_INFO("Miracast WiFi State=%#08X", wifiState);
			lock_guard<mutex> lck(m_DiscoveryStateMutex);
			if (m_isServiceEnabled)
			{
				switch(wifiState)
				{
					case DEVICE_WIFI_STATE_CONNECTING:
					{
						MIRACASTLOG_INFO("#### MCAST-TRIAGE-OK-WIFI DEVICE_WIFI_STATE [CONNECTING] ####");
						{lock_guard<recursive_mutex> lock(m_EventMutex);
							setEnableInternal(false);
						}
						remove_wifi_connection_state_timer();
						m_IsWiFiConnectingState = true;
						m_WiFiConnectedStateMonitorTimerID = g_timeout_add(30000, MiracastServiceImplementation::monitor_wifi_connection_state_timercallback, this);
						MIRACASTLOG_INFO("Timer created to Monitor WiFi Connection Status [%u]",m_WiFiConnectedStateMonitorTimerID);
					}
					break;
					case DEVICE_WIFI_STATE_CONNECTED:
					case DEVICE_WIFI_STATE_FAILED:
					{
						MIRACASTLOG_INFO("#### MCAST-TRIAGE-OK-WIFI DEVICE_WIFI_STATE [%s] ####",( DEVICE_WIFI_STATE_CONNECTED == wifiState ) ? "CONNECTED" : "FAILED");
						if (m_IsWiFiConnectingState)
						{
							{lock_guard<recursive_mutex> lock(m_EventMutex);
								setEnableInternal(true);
							}
							m_IsWiFiConnectingState = false;
						}
						remove_wifi_connection_state_timer();
					}
					break;
					default:
					{
						/* NOP */
					}
					break;
				}
			}
		}
		/*  WiFi related methods End */
		/* ------------------------------------------------------------------------------------------------------- */

		/*  JsonRPC Event Subscribed handler methods Start */
		/* ------------------------------------------------------------------------------------------------------- */
		void MiracastServiceImplementation::onFriendlyNameUpdateHandler(const JsonObject &parameters)
		{
			string message;
			string value;
			parameters.ToString(message);
			MIRACASTLOG_INFO("[Friendly Name Event], [%s]", message.c_str());

			if (parameters.HasLabel("friendlyName"))
			{
				value = parameters["friendlyName"].String();
				m_miracast_ctrler_obj->set_FriendlyName(value, m_isServiceEnabled);
				MIRACASTLOG_INFO("Miracast FriendlyName=%s", value.c_str());
			}
		}

		void MiracastServiceImplementation::onWIFIStateChangedHandler(const JsonObject &parameters)
		{
			string message;
			uint32_t wifiState;
			parameters.ToString(message);
			MIRACASTLOG_INFO("[WiFi State Changed Event], [%s]", message.c_str());

			if (parameters.HasLabel("state"))
			{
				wifiState = parameters["state"].Number();
				setWiFiStateInternal(static_cast<DEVICE_WIFI_STATES>(wifiState));
			}
		}
		/*  JsonRPC Event Subscribed handler methods End */
		/* ------------------------------------------------------------------------------------------------------- */

		/*  Internal Timer Callback and methods Start */
		/* ------------------------------------------------------------------------------------------------------- */
		gboolean MiracastServiceImplementation::monitor_friendly_name_timercallback(gpointer userdata)
		{
			gboolean timer_retry_state = G_SOURCE_CONTINUE;
			MIRACASTLOG_TRACE("Entering..!!!");
			MiracastServiceImplementation *self = (MiracastServiceImplementation *)userdata;
			MIRACASTLOG_INFO("TimerCallback Triggered for updating friendlyName...");
			if ( true == self->updateSystemFriendlyName() )
			{
				MIRACASTLOG_INFO("friendlyName updated properly, No polling required...");
				timer_retry_state = G_SOURCE_REMOVE;
			}
			else
			{
				MIRACASTLOG_WARNING("Unable to get friendlyName, still requires polling...");
			}
			MIRACASTLOG_TRACE("Exiting..!!!");
			return timer_retry_state;
		}

		gboolean MiracastServiceImplementation::monitor_wifi_connection_state_timercallback(gpointer userdata)
		{
			MIRACASTLOG_TRACE("Entering..!!!");
			MiracastServiceImplementation *self = (MiracastServiceImplementation *)userdata;
			MIRACASTLOG_INFO("TimerCallback Triggered for Monitor WiFi Connection Status...");
			{lock_guard<mutex> lck(self->m_DiscoveryStateMutex);
				MIRACASTLOG_INFO("#### MCAST-TRIAGE-OK-WIFI Discovery[%u] WiFiConnectingState[%u] ####",
									self->m_isServiceEnabled,m_IsWiFiConnectingState);
				if (self->m_isServiceEnabled && m_IsWiFiConnectingState)
				{
					{lock_guard<recursive_mutex> lock(self->m_EventMutex);
						self->setEnableInternal(true);
					}
				}
				m_IsWiFiConnectingState = false;
			}
			MIRACASTLOG_TRACE("Exiting..!!!");
			return G_SOURCE_REMOVE;
		}

		gboolean MiracastServiceImplementation::monitor_miracast_connection_timercallback(gpointer userdata)
		{
			MiracastServiceImplementation *self = (MiracastServiceImplementation *)userdata;
			MIRACASTLOG_TRACE("Entering..!!!");
			lock_guard<recursive_mutex> lock(self->m_EventMutex);
			MIRACASTLOG_INFO("TimerCallback Triggered for Monitor Miracast Connection Expired and Restarting Session...");
			if (self->m_isServiceEnabled)
			{
				self->m_miracast_ctrler_obj->restart_session_discovery(self->m_src_dev_mac);
				self->m_src_dev_mac.clear();
				self->changeServiceState(MIRACAST_SERVICE_STATE_RESTARTING_SESSION);
			}
			MIRACASTLOG_TRACE("Exiting..!!!");
			return G_SOURCE_REMOVE;
		}

		void MiracastServiceImplementation::remove_wifi_connection_state_timer(void)
		{
			MIRACASTLOG_TRACE("Entering..!!!");
			if (m_WiFiConnectedStateMonitorTimerID)
			{
				MIRACASTLOG_INFO("Removing WiFi Connection Status Monitor Timer");
				g_source_remove(m_WiFiConnectedStateMonitorTimerID);
				m_WiFiConnectedStateMonitorTimerID = 0;
			}
			m_IsWiFiConnectingState = false;
			MIRACASTLOG_TRACE("Exiting..!!!");
		}

		void MiracastServiceImplementation::remove_miracast_connection_timer(void)
		{
			MIRACASTLOG_TRACE("Entering..!!!");
			if (m_MiracastConnectionMonitorTimerID)
			{
				MIRACASTLOG_INFO("Removing Miracast Connection Status Monitor Timer");
				g_source_remove(m_MiracastConnectionMonitorTimerID);
				m_MiracastConnectionMonitorTimerID = 0;
			}
			MIRACASTLOG_TRACE("Exiting..!!!");
		}
		/*  Internal Timer Callback and methods End */
		/* ------------------------------------------------------------------------------------------------------- */
	} // namespace Plugin
} // namespace WPEFramework