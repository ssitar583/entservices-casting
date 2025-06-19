/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

 #include <string> 

#include "XCastImplementation.h"
#include <sys/prctl.h>
 
 #include "UtilsJsonRpc.h"
 #include "UtilsIarm.h"
 
 #include "UtilsSynchroIarm.hpp"

#include "rfcapi.h"
 
 #define HDMI_HOT_PLUG_EVENT_CONNECTED 0
 #define HDMI_HOT_PLUG_EVENT_DISCONNECTED 1
 
 #define API_VERSION_NUMBER_MAJOR 1
 #define API_VERSION_NUMBER_MINOR 0
 #define API_VERSION_NUMBER_PATCH 9
 
 using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
 
 namespace WPEFramework
 {
     namespace Plugin
     {
         SERVICE_REGISTRATION(XCastImplementation, 1, 0);
         XCastImplementation *XCastImplementation::_instance = nullptr;    
         
         XCastImplementation::XCastImplementation()
         : _adminLock(), _service(nullptr)
         {
             LOGINFO("Create XCastImplementation Instance");
             XCastImplementation::_instance = this;
         }
 
         XCastImplementation::~XCastImplementation()
         {
             LOGINFO("Call XCastImplementation destructor\n");
             if(_service != nullptr)
             {
                _service->Release();
             }
             XCastImplementation::_instance = nullptr;
             _service = nullptr;
         }
         
         /**
          * Register a notification callback
          */
         Core::hresult XCastImplementation::Register(Exchange::IXCast::INotification *notification)
         {
             ASSERT(nullptr != notification);
 
             _adminLock.Lock();
             printf("XCastImplementation::Register: notification = %p", notification);
             LOGINFO("Register notification");
 
             // Make sure we can't register the same notification callback multiple times
             if (std::find(_xcastNotification.begin(), _xcastNotification.end(), notification) == _xcastNotification.end())
             {
                 _xcastNotification.push_back(notification);
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
         Core::hresult XCastImplementation::Unregister(Exchange::IXCast::INotification *notification)
         {
             Core::hresult status = Core::ERROR_GENERAL;
 
             ASSERT(nullptr != notification);
 
             _adminLock.Lock();
 
             // we just unregister one notification once
             auto itr = std::find(_xcastNotification.begin(), _xcastNotification.end(), notification);
             if (itr != _xcastNotification.end())
             {
                 (*itr)->Release();
                 LOGINFO("Unregister notification");
                 _xcastNotification.erase(itr);
                 status = Core::ERROR_NONE;
             }
             else
             {
                 LOGERR("notification not found");
             }
 
             _adminLock.Unlock();
 
             return status;
         }
 
         uint32_t XCastImplementation::Configure(PluginHost::IShell* service)
         {
             uint32_t result = Core::ERROR_NONE;
             _service = service;
             _service->AddRef();
             ASSERT(service != nullptr);
             return result;
         }
        



        
        Core::hresult XCastImplementation::ApplicationStateChanged(const string& applicationName, const string& state, const string& applicationId, const string& error) { 
            return Core::ERROR_NONE;
        }
		Core::hresult XCastImplementation::GetProtocolVersion(string &protocolVersion  ) { 
            return Core::ERROR_NONE;
        }
        Core::hresult XCastImplementation::SetNetworkStandbyMode(bool networkStandbyMode) { return Core::ERROR_NONE;}
        Core::hresult XCastImplementation::SetManufacturerName(string manufacturername) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::GetManufacturerName(string &manufacturername  ) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::SetModelName(string modelname) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::GetModelName(string &modelname  ) { return Core::ERROR_NONE; }

		Core::hresult XCastImplementation::SetEnabled(bool enabled) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::GetEnabled(bool &enabled , bool &success ) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::SetStandbyBehavior(string standbybehavior) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::GetStandbyBehavior(string &standbybehavior , bool &success  ) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::SetFriendlyName(string friendlyname) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::GetFriendlyName(string &friendlyname , bool &success ) { return Core::ERROR_NONE; }
		Core::hresult XCastImplementation::GetApiVersionNumber(uint32_t &version , bool &success) { return Core::ERROR_NONE; }

	    Core::hresult XCastImplementation::RegisterApplications(Exchange::IXCast::IApplicationInfoIterator* const appInfoList) { return Core::ERROR_NONE; }

 
     } // namespace Plugin
 } // namespace WPEFramework
