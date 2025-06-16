/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
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
#include <interfaces/json/JsonData_MiracastService.h>
#include <interfaces/json/JMiracastService.h>
#include <interfaces/IMiracastService.h>
#include "UtilsLogging.h"
#include "tracing/Logging.h"
#include <mutex>

namespace WPEFramework
{
	namespace Plugin
	{
		class MiracastService : public PluginHost::IPlugin, public PluginHost::JSONRPC
		{
			private:
				class Notification : public RPC::IRemoteConnection::INotification,
									public Exchange::IMiracastService::INotification
				{
					private:
						Notification() = delete;
						Notification(const Notification&) = delete;
						Notification& operator=(const Notification&) = delete;

					public:
					explicit Notification(MiracastService* parent)
						: _parent(*parent)
						{
							ASSERT(parent != nullptr);
						}

						virtual ~Notification()
						{
						}

						BEGIN_INTERFACE_MAP(Notification)
						INTERFACE_ENTRY(Exchange::IMiracastService::INotification)
						INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
						END_INTERFACE_MAP

						void Activated(RPC::IRemoteConnection*) override
						{
						}

						void Deactivated(RPC::IRemoteConnection *connection) override
						{
						_parent.Deactivated(connection);
						}

						void OnClientConnectionRequest(const string &clientMac , const string &clientName ) override
						{
							LOGINFO("OnClientConnectionRequest -> clientMac:[%s], clientName:[%s]", clientMac.c_str(), clientName.c_str());
							Exchange::JMiracastService::Event::OnClientConnectionRequest(_parent, clientMac, clientName);
						}

						void OnClientConnectionError(const string &clientMac , const string &clientName , const Exchange::IMiracastService::ReasonCode &reasonCode , const string &reasonCodeStr ) override
						{
							LOGINFO("OnClientConnectionError -> clientMac:[%s], clientName:[%s], errorCode:[%s], reason:[%s]", clientMac.c_str(), clientName.c_str(), reasonCodeStr.c_str(), std::to_string(reasonCode).c_str());
							Exchange::JMiracastService::Event::OnClientConnectionError(_parent, clientMac, clientName, reasonCode, reasonCodeStr);
						}

						void OnLaunchRequest(const Exchange::IMiracastService::DeviceParameters deviceParameters) override
						{
							LOGINFO("OnLaunchRequest -> SrcDevIP[%s] SrcDevMac[%s] SrcDevName[%s] SinkDevIP[%s]",
									deviceParameters.sourceDeviceIP.c_str(),
									deviceParameters.sourceDeviceMac.c_str(),
									deviceParameters.sourceDeviceName.c_str(),
									deviceParameters.sinkDeviceIP.c_str());
							Exchange::JMiracastService::Event::OnLaunchRequest(_parent, deviceParameters);
						}

					private:
						MiracastService& _parent;
				}; // class Notification

			public:
				MiracastService(const MiracastService&) = delete;
				MiracastService& operator=(const MiracastService&) = delete;

				MiracastService();
				virtual ~MiracastService();

				BEGIN_INTERFACE_MAP(MiracastService)
				INTERFACE_ENTRY(PluginHost::IPlugin)
				INTERFACE_ENTRY(PluginHost::IDispatcher)
				INTERFACE_AGGREGATE(Exchange::IMiracastService, mMiracastServiceImpl)
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
				Exchange::IMiracastService* mMiracastServiceImpl{};
				Core::Sink<Notification> mMiracastServiceNotification;

			public /* constants */:
				static const string SERVICE_NAME;

			public /* members */:
				static MiracastService* _instance;
		}; // class MiracastService
	} // namespace Plugin
} // namespace WPEFramework