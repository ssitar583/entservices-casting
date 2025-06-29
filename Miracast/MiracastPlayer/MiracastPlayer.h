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

#pragma once

#include "Module.h"
#include <interfaces/json/JsonData_MiracastPlayer.h>
#include <interfaces/json/JMiracastPlayer.h>
#include <interfaces/IMiracastPlayer.h>
#include <interfaces/IConfiguration.h>
#include "UtilsLogging.h"
#include "tracing/Logging.h"
#include <mutex>

namespace WPEFramework
{
    namespace Plugin
    {
        class MiracastPlayer : public PluginHost::IPlugin, public PluginHost::JSONRPC
        {
            private:
                class Notification : public RPC::IRemoteConnection::INotification,
                                    public Exchange::IMiracastPlayer::INotification
                {
                    private:
                        Notification() = delete;
                        Notification(const Notification&) = delete;
                        Notification& operator=(const Notification&) = delete;

                    public:
                    explicit Notification(MiracastPlayer* parent)
                        : _parent(*parent)
                        {
                            ASSERT(parent != nullptr);
                        }

                        virtual ~Notification()
                        {
                        }

                        BEGIN_INTERFACE_MAP(Notification)
                        INTERFACE_ENTRY(Exchange::IMiracastPlayer::INotification)
                        INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                        END_INTERFACE_MAP

                        void Activated(RPC::IRemoteConnection*) override
                        {
                        }

                        void Deactivated(RPC::IRemoteConnection *connection) override
                        {
                            _parent.Deactivated(connection);
                        }

                        void OnStateChange(const string &clientName , const string &clientMac , const Exchange::IMiracastPlayer::State playerState , const string &reasonCode , const Exchange::IMiracastPlayer::ReasonCode reasonDescription ) override
                        {
                            LOGINFO("=> clientName:[%s], clientMac:[%s], playerState:[%d], Reason:[%s]",clientName.c_str(), clientMac.c_str(), static_cast<int>(playerState), reasonCode.c_str());
                            Exchange::JMiracastPlayer::Event::OnStateChange(_parent, clientName, clientMac, playerState, reasonCode, reasonDescription);
                        }

                    private:
                        MiracastPlayer& _parent;
                }; // class Notification

            public:
                MiracastPlayer(const MiracastPlayer&) = delete;
                MiracastPlayer& operator=(const MiracastPlayer&) = delete;

                MiracastPlayer();
                virtual ~MiracastPlayer();

                BEGIN_INTERFACE_MAP(MiracastPlayer)
                INTERFACE_ENTRY(PluginHost::IPlugin)
                INTERFACE_ENTRY(PluginHost::IDispatcher)
                INTERFACE_AGGREGATE(Exchange::IMiracastPlayer, mMiracastPlayerImpl)
                END_INTERFACE_MAP

                /* IPlugin methods  */
                const string Initialize(PluginHost::IShell* service) override;
                void Deinitialize(PluginHost::IShell* service) override;
                string Information() const override;

            private:
                void Deactivated(RPC::IRemoteConnection* connection);

            private: /* members */
                PluginHost::IShell* mCurrentService{};
                uint32_t mConnectionId{};
                Exchange::IMiracastPlayer* mMiracastPlayerImpl{};
                Exchange::IConfiguration* mConfigure;
                Core::Sink<Notification> mMiracastPlayerNotification;

            public /* constants */:
                static const string SERVICE_NAME;

            public /* members */:
                static MiracastPlayer* _instance;
        }; // class MiracastPlayer
    } // namespace Plugin
} // namespace WPEFramework