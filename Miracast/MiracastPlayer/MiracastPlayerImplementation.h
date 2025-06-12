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
#include <interfaces/IMiracastPlayer.h>

#include "MiracastRTSPMsg.h"
#include "MiracastGstPlayer.h"

#include <com/com.h>
#include <core/core.h>
#include <mutex>
#include <vector>

using namespace WPEFramework;

namespace WPEFramework
{
	namespace Plugin
	{		
		class MiracastPlayerImplementation : public Exchange::IMiracastPlayer, public MiracastPlayerNotifier
		{
			public:
				// We do not allow this plugin to be copied !!
				MiracastPlayerImplementation();
				~MiracastPlayerImplementation() override;

				static MiracastPlayerImplementation *instance(MiracastPlayerImplementation *MiracastPlayerImpl = nullptr);
				static MiracastRTSPMsg *m_miracast_rtsp_obj;

				// We do not allow this plugin to be copied !!
				MiracastPlayerImplementation(const MiracastPlayerImplementation &) = delete;
				MiracastPlayerImplementation &operator=(const MiracastPlayerImplementation &) = delete;

				virtual void onStateChange(const std::string& client_mac, const std::string& client_name, eMIRA_PLAYER_STATES player_state, eM_PLAYER_REASON_CODE reason_code) override;
				
				BEGIN_INTERFACE_MAP(MiracastPlayerImplementation)
				INTERFACE_ENTRY(Exchange::IMiracastPlayer)
				END_INTERFACE_MAP

			public:
				enum Event
				{
					MIRACASTPLAYER_EVENT_ON_STATE_CHANGE
				};

				class EXTERNAL Job : public Core::IDispatch
				{
					protected:
						Job(MiracastPlayerImplementation *MiracastPlayerImplementation, Event event, JsonObject &params)
							: _miracastPlayerImplementation(MiracastPlayerImplementation), _event(event), _params(params)
						{
							if (_miracastPlayerImplementation != nullptr)
							{
								_miracastPlayerImplementation->AddRef();
							}
						}

					public:
						Job() = delete;
						Job(const Job &) = delete;
						Job &operator=(const Job &) = delete;
						~Job()
						{
							if (_miracastPlayerImplementation != nullptr)
							{
								_miracastPlayerImplementation->Release();
							}
						}

					public:
						static Core::ProxyType<Core::IDispatch> Create(MiracastPlayerImplementation *miracastPlayerImplementation, Event event, JsonObject params)
						{
		#ifndef USE_THUNDER_R4
							return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(miracastPlayerImplementation, event, params)));
		#else
							return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(miracastPlayerImplementation, event, params)));
		#endif
						}

						virtual void Dispatch()
						{
							_miracastPlayerImplementation->Dispatch(_event, _params);
						}

					private:
						MiracastPlayerImplementation *_miracastPlayerImplementation;
						const Event _event;
						JsonObject _params;
				}; // class Job

			public:
				Core::hresult Initialize(PluginHost::IShell* service) override;
				Core::hresult Deinitialize(PluginHost::IShell* service) override;
				Core::hresult Register(Exchange::IMiracastPlayer::INotification *notification) override;
				Core::hresult Unregister(Exchange::IMiracastPlayer::INotification *notification) override;
				Core::hresult PlayRequest(const DeviceParameters &deviceParam , const VideoRectangle &videoRect , Result &returnPayload ) override;
				Core::hresult StopRequest(const string &clientMac , const string &clientName , const PlayerStopReasonCode &reasonCode , Result &returnPayload ) override;
				Core::hresult SetVideoRectangle(const int32_t &startX , const int32_t &startY , const int32_t &width , const int32_t &height , Result &returnPayload ) override;
				Core::hresult SetWesterosEnvironment( IWesterosEnvArgumentsIterator * const westerosArgs , Result &returnPayload ) override;
				Core::hresult UnsetWesterosEnvironment(Result &returnPayload ) override;
				Core::hresult SetLogging(const std::string &logLevel, const SeparateLogger &separateLogger, Result &returnPayload) override;

			private:
				MiracastGstPlayer *m_GstPlayer;
				VIDEO_RECT_STRUCT m_video_sink_rect;
            	bool m_isServiceInitialized;

				mutable Core::CriticalSection _adminLock;
				std::list<Exchange::IMiracastPlayer::INotification *> _miracastPlayerNotification; // List of registered notifications
				PluginHost::IShell *m_CurrentService;
				void dispatchEvent(Event, const JsonObject &params);
				void Dispatch(Event event, const JsonObject &params);

				void unsetWesterosEnvironmentInternal(void);
				std::string stateDescription(eMIRA_PLAYER_STATES e);
				std::string reasonDescription(eM_PLAYER_REASON_CODE e);

			public:
				static MiracastPlayerImplementation *_instance;
		}; // class MiracastPlayerImplementation
	} // namespace Plugin
} // namespace WPEFramework