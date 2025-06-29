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
#include <interfaces/IMiracastPlayer.h>
#include<interfaces/IConfiguration.h>

#include <com/com.h>
#include <core/core.h>
#include <mutex>
#include <vector>

#include "MiracastRTSPMsg.h"
#include "MiracastGstPlayer.h"

#include "libIBus.h"

using namespace WPEFramework;

using MiracastPlayerState = WPEFramework::Exchange::IMiracastPlayer::State;
using MiracastPlayerReasonCode = WPEFramework::Exchange::IMiracastPlayer::ReasonCode;
using ParamsType = boost::variant<std::tuple<std::string, std::string, MiracastPlayerState, MiracastPlayerReasonCode>>;

namespace WPEFramework
{
    namespace Plugin
    {
        class MiracastPlayerImplementation : public Exchange::IMiracastPlayer, public Exchange::IConfiguration, public MiracastPlayerNotifier
        {
        public:
            // We do not allow this plugin to be copied !!
            MiracastPlayerImplementation();
            ~MiracastPlayerImplementation() override;

            static MiracastPlayerImplementation *instance(MiracastPlayerImplementation *MiracastPlayerImpl = nullptr);

            // We do not allow this plugin to be copied !!
            MiracastPlayerImplementation(const MiracastPlayerImplementation &) = delete;
            MiracastPlayerImplementation &operator=(const MiracastPlayerImplementation &) = delete;

            virtual void onStateChange(string client_mac, string client_name, MiracastPlayerState player_state, MiracastPlayerReasonCode reason_code ) override;

            BEGIN_INTERFACE_MAP(MiracastPlayerImplementation)
            INTERFACE_ENTRY(Exchange::IMiracastPlayer)
            INTERFACE_ENTRY(Exchange::IConfiguration)
            END_INTERFACE_MAP

        public:
            enum Event
            {
                MIRACASTPLAYER_EVENT_ON_STATE_CHANGE
            };
            class EXTERNAL Job : public Core::IDispatch
            {
            protected:
                Job(MiracastPlayerImplementation *MiracastPlayerImplementation, Event event, ParamsType &params)
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
                static Core::ProxyType<Core::IDispatch> Create(MiracastPlayerImplementation *miracastPlayerImplementation, Event event, ParamsType params)
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
                ParamsType _params;
            };


        public:
            uint32_t Configure(PluginHost::IShell* service) override;

            Core::hresult Register(Exchange::IMiracastPlayer::INotification *notification) override;
            Core::hresult Unregister(Exchange::IMiracastPlayer::INotification *notification) override;

            Core::hresult PlayRequest(const DeviceParameters &deviceParam , const VideoRectangle videoRect , Result &result ) override;
            Core::hresult StopRequest(const string &clientMac , const string &clientName , const int reasonCode , Result &result ) override;
            Core::hresult SetVideoRectangle(const int startX , const int startY , const int width , const int height , Result &result ) override;
            Core::hresult SetWesterosEnvironment( IEnvArgumentsIterator * const westerosArgs , Result &result ) override;
            Core::hresult UnsetWesterosEnvironment(Result &result ) override;
            Core::hresult SetEnvArguments( IEnvArgumentsIterator * const envArgs , Result &result ) override;
            Core::hresult UnsetEnvArguments(Result &result ) override;

        private:
            mutable Core::CriticalSection _adminLock;
            PluginHost::IShell *mService;
            std::list<Exchange::IMiracastPlayer::INotification *> _miracastPlayerNotification; // List of registered notifications
            PluginHost::IShell* _service;
            MiracastGstPlayer *m_GstPlayer;
            VIDEO_RECT_STRUCT m_video_sink_rect;
            bool m_isPluginInitialized;

            void dispatchEvent(Event, const ParamsType &params);
            void Dispatch(Event event, const ParamsType &params);
            void unsetEnvArgumentsInternal(void);
            std::string stateDescription(MiracastPlayerState e);

        public:
            static MiracastPlayerImplementation *_instance;
            static MiracastRTSPMsg *m_miracast_rtsp_obj;

            friend class Job;
        };
    } // namespace Plugin
} // namespace WPEFramework