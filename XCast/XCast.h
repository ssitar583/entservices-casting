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
#include <interfaces/IXCast.h>
#include <interfaces/json/JsonData_XCast.h>
#include <interfaces/json/JXCast.h>
#include <interfaces/IConfiguration.h>
#include "UtilsLogging.h"
#include "tracing/Logging.h"


namespace WPEFramework {

    namespace Plugin {
			
		class XCast : public PluginHost::IPlugin, public PluginHost::JSONRPC
		{
			private:
            	class Notification : public RPC::IRemoteConnection::INotification, public Exchange::IXCast::INotification
                {
					private:
			        	Notification() = delete;
			            Notification(const Notification&) = delete;
			            Notification& operator=(const Notification&) = delete;
						
					public:
						explicit Notification(XCast *parent)
							: _parent(*parent)
						{
							ASSERT(parent != nullptr);
						}
		
						virtual ~Notification()
						{
						}
					
						BEGIN_INTERFACE_MAP(Notification)
						INTERFACE_ENTRY(Exchange::IXCast::INotification)
						INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
						END_INTERFACE_MAP

						 virtual void OnApplicationLaunchRequestWithLaunchParam(const string& appName, const string& strPayLoad, const string& strQuery, const string& strAddDataUrl) override
						{
							LOGINFO("OnApplicationLaunchRequestWithLaunchParam: appName=%s, strPayLoad=%s, strQuery=%s, strAddDataUrl=%s",
								appName.c_str(), strPayLoad.c_str(), strQuery.c_str(), strAddDataUrl.c_str());
							Exchange::JXCast::Event::OnApplicationLaunchRequestWithLaunchParam(_parent, appName, strPayLoad, strQuery, strAddDataUrl);
						}
						virtual void OnApplicationLaunchRequest(const string& appName, const string& parameter) override
						{
							LOGINFO("OnApplicationLaunchRequest: appName=%s, parameter=%s", appName.c_str(), parameter.c_str());
							Exchange::JXCast::Event::OnApplicationLaunchRequest(_parent, appName, parameter);
						}
						virtual void OnApplicationStopRequest(const string& appName, const string& appID) override
						{
							LOGINFO("OnApplicationStopRequest: appName=%s, appID=%s", appName.c_str(), appID.c_str());
							Exchange::JXCast::Event::OnApplicationStopRequest(_parent, appName, appID);
						}
						virtual void OnApplicationHideRequest(const string& appName, const string& appID) override
						{
							LOGINFO("OnApplicationHideRequest: appName=%s, appID=%s", appName.c_str(), appID.c_str());
							Exchange::JXCast::Event::OnApplicationHideRequest(_parent, appName, appID);
						}
						virtual void OnApplicationStateRequest(const string& appName, const string& appID) override
						{
							LOGINFO("OnApplicationStateRequest: appName=%s, appID=%s", appName.c_str(), appID.c_str());
							Exchange::JXCast::Event::OnApplicationStateRequest(_parent, appName, appID);
						}
						virtual void OnApplicationResumeRequest(const string& appName, const string& appID) override
						{
							LOGINFO("OnApplicationResumeRequest: appName=%s, appID=%s", appName.c_str(), appID.c_str());
							Exchange::JXCast::Event::OnApplicationResumeRequest(_parent, appName, appID);
						}
							
						void Activated(RPC::IRemoteConnection *connection) final
						{
							LOGINFO("XCast Notification Activated");
							// if (connection->Id() == _parent._connectionId)
            				// {
							// 	LOGINFO("XCast Notification Activated");
            				//     ASSERT(nullptr != _parent._service);
            				//     Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_parent._service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            				// }
						}
		
						void Deactivated(RPC::IRemoteConnection *connection) final
						{
							//  if (connection->Id() == _parent._connectionId)
            				// {
							// 	LOGINFO("XCast Notification Deactivated");
            				//     ASSERT(nullptr != _parent._service);
            				//     Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_parent._service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            				// }
							LOGINFO("XCast Notification Deactivated");
							_parent.Deactivated(connection);
						}
		
						private:
							XCast &_parent;
				};
			
			public:
				XCast(const XCast &) = delete;
				XCast &operator=(const XCast &) = delete;
				
				XCast();
				virtual ~XCast();
			
				BEGIN_INTERFACE_MAP(XCast)
				INTERFACE_ENTRY(PluginHost::IPlugin)
				INTERFACE_ENTRY(PluginHost::IDispatcher)
				INTERFACE_AGGREGATE(Exchange::IXCast, _xcast)
				END_INTERFACE_MAP
				
				//  IPlugin methods
				// -------------------------------------------------------------------------------------------------------
				const string Initialize(PluginHost::IShell* service) override;
				void Deinitialize(PluginHost::IShell* service) override;
				string Information() const override;
				
				private:
                	void Deactivated(RPC::IRemoteConnection* connection);
			
				private:
					PluginHost::IShell *_service{};
					uint32_t _connectionId{};
					Exchange::IXCast *_xcast{};
					Core::Sink<Notification> _xcastNotification;
					//Exchange::IConfiguration* configure;

					friend class Notification;
        };

    } // namespace Plugin
} // namespace WPEFramework
