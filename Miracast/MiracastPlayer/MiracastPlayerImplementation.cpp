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

#include "MiracastPlayerImplementation.h"

#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"

#include "UtilsSynchroIarm.hpp"

static std::vector<std::string> m_westerosEnvArgs;

namespace WPEFramework
{
	namespace Plugin
	{
		SERVICE_REGISTRATION(MiracastPlayerImplementation, MIRACAST_PLAYER_API_VERSION_NUMBER_MAJOR, MIRACAST_PLAYER_API_VERSION_NUMBER_MINOR);
		
		MiracastPlayerImplementation *MiracastPlayerImplementation::_instance = nullptr;
		MiracastRTSPMsg *MiracastPlayerImplementation::m_miracast_rtsp_obj = nullptr;
	
		MiracastPlayerImplementation::MiracastPlayerImplementation()
		: _adminLock()
		{
			LOGINFO("Create MiracastPlayerImplementation Instance");
			MiracastPlayerImplementation::_instance = this;
			m_isServiceInitialized = false;
			MIRACAST::logger_init("MiracastPlayer");
		}

		MiracastPlayerImplementation::~MiracastPlayerImplementation()
		{
			if(m_CurrentService != nullptr)
			{
				m_CurrentService->Release();
			}
			MIRACAST::logger_deinit();
			MiracastPlayerImplementation::_instance = nullptr;
		}

		/**
		 * Register a notification callback
		 */
		Core::hresult MiracastPlayerImplementation::Register(Exchange::IMiracastPlayer::INotification *notification)
		{
			ASSERT(nullptr != notification);

			_adminLock.Lock();
			printf("MiracastPlayerImplementation::Register: notification = %p", notification);
			LOGINFO("Register notification");

			// Make sure we can't register the same notification callback multiple times
			if (std::find(_miracastPlayerNotification.begin(), _miracastPlayerNotification.end(), notification) == _miracastPlayerNotification.end())
			{
				_miracastPlayerNotification.push_back(notification);
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
		Core::hresult MiracastPlayerImplementation::Unregister(Exchange::IMiracastPlayer::INotification *notification)
		{
			Core::hresult status = Core::ERROR_GENERAL;

			ASSERT(nullptr != notification);

			_adminLock.Lock();

			// we just unregister one notification once
			auto itr = std::find(_miracastPlayerNotification.begin(), _miracastPlayerNotification.end(), notification);
			if (itr != _miracastPlayerNotification.end())
			{
				(*itr)->Release();
				LOGINFO("Unregister notification");
				_miracastPlayerNotification.erase(itr);
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
		void MiracastPlayerImplementation::unsetWesterosEnvironmentInternal(void)
		{
			MIRACASTLOG_TRACE("Entering..!!!");
			for (const auto& argName : m_westerosEnvArgs)
			{
				if (0 == unsetenv(argName.c_str()))
				{
					MIRACASTLOG_INFO("Success, unsetenv for [%s] - strerrorno[%s]", argName.c_str(), strerror(errno));
				}
				else
				{
					MIRACASTLOG_ERROR("Failed, unsetenv for [%s] - strerrorno[%s]", argName.c_str(), strerror(errno));
				}
			}
			m_westerosEnvArgs.clear();
			MIRACASTLOG_TRACE("Exiting..!!!");
		}

		std::string MiracastPlayerImplementation::stateDescription(MiracastPlayerState e)
		{
			switch (e)
			{
				case WPEFramework::Exchange::IMiracastPlayer::STATE_IDLE:
					return "IDLE";
				case WPEFramework::Exchange::IMiracastPlayer::STATE_INITIATED:
					return "INITIATED";
				case WPEFramework::Exchange::IMiracastPlayer::STATE_INPROGRESS:
					return "INPROGRESS";
				case WPEFramework::Exchange::IMiracastPlayer::STATE_PLAYING:
					return "PLAYING";
				case WPEFramework::Exchange::IMiracastPlayer::STATE_STOPPED:
					return "STOPPED";
				default:
					return "Unimplemented state";
			}
		}
		/*  Helper and Internal methods End */
		/* ------------------------------------------------------------------------------------------------------- */

		void MiracastPlayerImplementation::dispatchEvent(Event event, const ParamsType &params)
		{
			Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
		}

		void MiracastPlayerImplementation::Dispatch(Event event,const ParamsType& params)
		{
			_adminLock.Lock();

			std::list<Exchange::IMiracastPlayer::INotification *>::const_iterator index(_miracastPlayerNotification.begin());

			switch (event)
			{
				case MIRACASTPLAYER_EVENT_ON_STATE_CHANGE:
				{
					string clientName;
					string clientMac;
					MiracastPlayerState playerState;
					MiracastPlayerReasonCode reasonCode;
					string reasonCodeStr;

					if (const auto* tupleValue = boost::get<std::tuple<std::string, std::string, MiracastPlayerState, MiracastPlayerReasonCode>>(&params))
					{
						clientMac = std::get<0>(*tupleValue);
						clientName = std::get<1>(*tupleValue);
						playerState = std::get<2>(*tupleValue);
						reasonCode = std::get<3>(*tupleValue);
						reasonCodeStr = std::to_string(reasonCode);
					}

					while (index != _miracastPlayerNotification.end())
					{
						(*index)->OnStateChange(clientName , clientMac , playerState , reasonCode , reasonCodeStr );
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
		Core::hresult MiracastPlayerImplementation::Initialize(PluginHost::IShell *service)
		{
			Core::hresult result = Core::ERROR_GENERAL;

			ASSERT(nullptr != service);

			if (nullptr != m_CurrentService)
			{
				if (!m_isServiceInitialized)
				{
					MiracastError ret_code = MIRACAST_OK;
					m_miracast_rtsp_obj = MiracastRTSPMsg::getInstance(ret_code, this);
					if (nullptr != m_miracast_rtsp_obj)
					{
						m_CurrentService = service;
						m_CurrentService->AddRef();
						m_GstPlayer = MiracastGstPlayer::getInstance();
						m_isServiceInitialized = true;
						result = Core::ERROR_NONE;
					}
					else
					{
						switch (ret_code)
						{
							case MIRACAST_RTSP_INIT_FAILED:
							{
								MIRACASTLOG_ERROR("RTSP handler Init Failed");
							}
							break;
							default:
							{
								MIRACASTLOG_ERROR("Unknown Error:Failed to obtain MiracastRTSPMsg Object");
							}
							break;
						}
					}
				}
			}
			return result;
		}

		Core::hresult MiracastPlayerImplementation::Deinitialize(PluginHost::IShell* service)
		{
			MIRACASTLOG_INFO("Entering..!!!");

			ASSERT(nullptr != service);

			MiracastPlayerImplementation::_instance = nullptr;
			MIRACASTLOG_INFO("Entering..!!!");

			if (m_isServiceInitialized)
			{
				MiracastRTSPMsg::destroyInstance();
				m_CurrentService = nullptr;
				m_miracast_rtsp_obj = nullptr;
				m_isServiceInitialized = false;
				MIRACASTLOG_INFO("Done..!!!");
			}
			MIRACASTLOG_INFO("Exiting..!!!");

			if (nullptr != m_CurrentService)
			{
				m_CurrentService->Release();
				m_CurrentService = nullptr;
			}
			MIRACASTLOG_INFO("Exiting..!!!");
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastPlayerImplementation::PlayRequest(const DeviceParameters &deviceParam , const VideoRectangle &videoRect , Result &returnPayload )
		{
			RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};
			bool isSuccessOrFailure = false;
			MIRACASTLOG_INFO("Entering..!!!");

			strncpy( rtsp_hldr_msgq_data.source_dev_ip, deviceParam.sourceDeviceIP.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_ip));
			rtsp_hldr_msgq_data.source_dev_ip[sizeof(rtsp_hldr_msgq_data.source_dev_ip) - 1] = '\0';
			strncpy( rtsp_hldr_msgq_data.source_dev_mac, deviceParam.sourceDeviceMac.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_mac));
			rtsp_hldr_msgq_data.source_dev_mac[sizeof(rtsp_hldr_msgq_data.source_dev_mac) - 1] = '\0';
			strncpy( rtsp_hldr_msgq_data.source_dev_name, deviceParam.sourceDeviceName.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_name));
			rtsp_hldr_msgq_data.source_dev_name[sizeof(rtsp_hldr_msgq_data.source_dev_name) - 1] = '\0';
			strncpy( rtsp_hldr_msgq_data.sink_dev_ip, deviceParam.sinkDeviceIP.c_str() , sizeof(rtsp_hldr_msgq_data.sink_dev_ip));
			rtsp_hldr_msgq_data.sink_dev_ip[sizeof(rtsp_hldr_msgq_data.sink_dev_ip) - 1] = '\0';
			rtsp_hldr_msgq_data.state = RTSP_START_RECEIVE_MSGS;

			if (( 0 < videoRect.width ) && ( 0 < videoRect.height ))
			{
				m_video_sink_rect.startX = videoRect.startX;
				m_video_sink_rect.startY = videoRect.startY;
				m_video_sink_rect.width = videoRect.width;
				m_video_sink_rect.height = videoRect.height;

				rtsp_hldr_msgq_data.videorect = m_video_sink_rect;

				m_miracast_rtsp_obj->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
				isSuccessOrFailure = true;
			}
			returnPayload.success = isSuccessOrFailure;
			MIRACASTLOG_INFO("Exiting..!!!");
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastPlayerImplementation::StopRequest(const string &clientMac , const string &clientName , const MiracastPlayerStopReasonCode &reasonCode , Result &returnPayload )
		{
			RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};
			bool isSuccessOrFailure = true;

			MIRACASTLOG_INFO("Entering..!!!");

			switch (reasonCode)
			{
				case WPEFramework::Exchange::IMiracastPlayer::STOP_REASON_APP_REQ_FOR_EXIT:
				case WPEFramework::Exchange::IMiracastPlayer::STOP_REASON_APP_REQ_FOR_NEW_CONNECTION:
				{
					rtsp_hldr_msgq_data.stop_reason_code = reasonCode;

					rtsp_hldr_msgq_data.state = RTSP_TEARDOWN_FROM_SINK2SRC;
					m_miracast_rtsp_obj->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
					isSuccessOrFailure = true;
				}
				break;
				default:
				{
					isSuccessOrFailure = false;
					MIRACASTLOG_ERROR("!!! UNKNOWN STOP REASON CODE RECEIVED[%#04X] !!!",reasonCode);
					returnPayload.message = "UNKNOWN STOP REASON CODE RECEIVED";
				}
				break;
			}

			returnPayload.success = isSuccessOrFailure;

			MIRACASTLOG_INFO("Exiting..!!!");
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastPlayerImplementation::SetVideoRectangle(const int32_t &startX , const int32_t &startY , const int32_t &width , const int32_t &height , Result &returnPayload )
		{
			RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};
			bool isSuccessOrFailure = false;
			MIRACASTLOG_INFO("Entering..!!!");

			if (( 0 < width ) && ( 0 < height ) &&
				(( startX != m_video_sink_rect.startX ) ||
				( startY != m_video_sink_rect.startY ) ||
				( width != m_video_sink_rect.width ) ||
				( height != m_video_sink_rect.height )))
			{
				m_video_sink_rect.startX = startX;
				m_video_sink_rect.startY = startY;
				m_video_sink_rect.width = width;
				m_video_sink_rect.height = height;

				rtsp_hldr_msgq_data.videorect = m_video_sink_rect;
				rtsp_hldr_msgq_data.state = RTSP_UPDATE_VIDEO_RECT;
				m_miracast_rtsp_obj->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
				isSuccessOrFailure = true;
			}
			returnPayload.success = isSuccessOrFailure;
			MIRACASTLOG_INFO("Exiting..!!!");
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastPlayerImplementation::SetWesterosEnvironment( IWesterosEnvArgumentsIterator * const westerosArgs , Result &returnPayload )
		{
			std::string waylandDisplayName = "";
			bool isSuccessOrFailure = false;
			if (westerosArgs)
			{
				Exchange::IMiracastPlayer::WesterosEnvArguments entry{};

				while (westerosArgs->Next(entry) == true)
				{
					std::string argName = entry.argName;
					std::string argValue = entry.argName;

					m_westerosEnvArgs.push_back(argName);
					MIRACASTLOG_INFO("Configuring environment variable: %s=%s", argName.c_str(), argValue.c_str());
					if (0 == setenv(argName.c_str(), argValue.c_str(), 1))
					{
						MIRACASTLOG_INFO("Success, setenv: [%s]=[%s] - strerrorno[%s]", argName.c_str(), argValue.c_str(), strerror(errno));
					}
					else
					{
						MIRACASTLOG_ERROR("Failed, setenv for %s: [%s]", argName.c_str(), strerror(errno));
					}

					if ( "WAYLAND_DISPLAY" == argName )
					{
						waylandDisplayName = std::move(argValue);
						MIRACASTLOG_INFO("Wayland Display Name from App: [%s]", waylandDisplayName.c_str());
					}

					char *value = getenv(argName.c_str());
					if (value != NULL)
					{
						MIRACASTLOG_INFO("Success, getenv: [%s]=[%s]", argName.c_str(), value);
					}
					else
					{
						MIRACASTLOG_ERROR("Failed to getenv variable: %s - strerrorno[%s]", argName.c_str(),strerror(errno));
					}
				}

				if (!waylandDisplayName.empty())
				{
					MIRACASTLOG_INFO("Wayland Display Name from App: [%s]", waylandDisplayName.c_str());
				}

				std::string waylandDisplayOverrideName = MiracastCommon::parse_opt_flag("/opt/miracast_custom_westeros_name");
				if (!waylandDisplayOverrideName.empty())
				{
					MIRACASTLOG_INFO("Wayland Display Name from Overrides: [%s]", waylandDisplayOverrideName.c_str());
					waylandDisplayName = std::move(waylandDisplayOverrideName);
				}

				if (!waylandDisplayName.empty())
				{
					if (0 == setenv("WAYLAND_DISPLAY", waylandDisplayName.c_str(), 1))
					{
						MIRACASTLOG_INFO("Success, setenv for WAYLAND_DISPLAY: [%s] - strerrorno[%s]", waylandDisplayName.c_str(), strerror(errno));
						isSuccessOrFailure = true;
					}
					else
					{
						MIRACASTLOG_ERROR("Failed, setenv for WAYLAND_DISPLAY: [%s] - strerrorno[%s]", waylandDisplayName.c_str(), strerror(errno));
					}
				}
				else
				{
					MIRACASTLOG_ERROR("Failed to get Wayland Display Name");
					isSuccessOrFailure = false;
					unsetWesterosEnvironmentInternal();
				}
			}
			returnPayload.success = isSuccessOrFailure;
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastPlayerImplementation::UnsetWesterosEnvironment(Result &returnPayload )
		{
			MIRACASTLOG_INFO("Entering..!!!");
			unsetWesterosEnvironmentInternal();
			returnPayload.success = true;
			MIRACASTLOG_INFO("Exiting..!!!");
			return Core::ERROR_NONE;
		}

		Core::hresult MiracastPlayerImplementation::SetLogging(const MiracastLogLevel &logLevel , const SeparateLogger &separateLogger , Result &returnPayload)
		{
			bool isSuccessOrFailure = false;
			MIRACAST::LogLevel level = INFO_LEVEL;

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

			switch (logLevel)
			{
				case WPEFramework::Exchange::IMiracastPlayer::LOG_LEVEL_FATAL:
				{
					level = FATAL_LEVEL;
				}
				break;
				case WPEFramework::Exchange::IMiracastPlayer::LOG_LEVEL_ERROR:
				{
					level = ERROR_LEVEL;
				}
				break;
				case WPEFramework::Exchange::IMiracastPlayer::LOG_LEVEL_WARNING:
				{
					level = WARNING_LEVEL;
				}
				break;
				case WPEFramework::Exchange::IMiracastPlayer::LOG_LEVEL_INFO:
				{
					level = INFO_LEVEL;
				}
				break;
				case WPEFramework::Exchange::IMiracastPlayer::LOG_LEVEL_VERBOSE:
				{
					level = VERBOSE_LEVEL;
				}
				break;
				case WPEFramework::Exchange::IMiracastPlayer::LOG_LEVEL_TRACE:
				{
					level = TRACE_LEVEL;
				}
				break;
				default:
				{
					returnPayload.message = "Supported 'level' parameter values are FATAL, ERROR, WARNING, INFO, VERBOSE or TRACE";
					isSuccessOrFailure = false;
				}
				break;
			}

			if (isSuccessOrFailure)
			{
				set_loglevel(level);
			}
			returnPayload.success = isSuccessOrFailure;
			MIRACASTLOG_INFO("Exiting..!!!");
			return Core::ERROR_NONE;
		}
		/*  COMRPC Methods End */
		/* ------------------------------------------------------------------------------------------------------- */

		/*  Events Start */
		/* ------------------------------------------------------------------------------------------------------- */
		void MiracastPlayerImplementation::onStateChange(const std::string& client_mac, const std::string& client_name, MiracastPlayerState player_state, MiracastPlayerReasonCode reason_code)
		{
			MIRACASTLOG_INFO("Entering..!!!");

			auto tupleParam = std::make_tuple(client_mac,client_name,player_state,reason_code);

			if (0 == access("/opt/miracast_autoconnect", F_OK))
			{
				char commandBuffer[768] = {0};
				snprintf( commandBuffer,
						sizeof(commandBuffer),
						"curl -H \"Authorization: Bearer `WPEFrameworkSecurityUtility | cut -d '\"' -f 4`\" --header \"Content-Type: application/json\" --request POST --data '{\"jsonrpc\":\"2.0\", \"id\":3,\"method\":\"org.rdk.MiracastService.1.updatePlayerState\", \"params\":{\"mac\": \"%s\",\"state\": \"%s\",\"reason_code\": %s}}' http://127.0.0.1:9998/jsonrpc &",
						client_mac.c_str(),
						stateDescription(player_state).c_str(),
						std::to_string(reason_code).c_str());
				MIRACASTLOG_INFO("System Command [%s]",commandBuffer);
				MiracastCommon::execute_SystemCommand( commandBuffer );
			}
			else
			{
				dispatchEvent(MIRACASTPLAYER_EVENT_ON_STATE_CHANGE, tupleParam);
			}

			if ( WPEFramework::Exchange::IMiracastPlayer::STATE_STOPPED == player_state )
			{
				unsetWesterosEnvironmentInternal();
			}
			MIRACASTLOG_INFO("Exiting..!!!");
		}
		/*  Events End */
		/* ------------------------------------------------------------------------------------------------------- */
	} // namespace Plugin
} // namespace WPEFramework