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
 
 #include "UtilsJsonRpc.h"
 #include "UtilsIarm.h"
 
 #include "UtilsSynchroIarm.hpp"
 
 #define API_VERSION_NUMBER_MAJOR 1
 #define API_VERSION_NUMBER_MINOR 0
 #define API_VERSION_NUMBER_PATCH 2
 
 
 namespace WPEFramework
 {
     namespace Plugin
     {
         SERVICE_REGISTRATION(XCastImplementation, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        XCastImplementation *XCastImplementation::_instance = nullptr;
        //XCastManager* XCastImplementation::m_xcast_manager = nullptr;
         
         XCastImplementation::XCastImplementation()
         : _adminLock(), _service(nullptr)
         {
             LOGINFO("Create XCastImplementation Instance");
             LOGINFO("##### API VER[%d : %d : %d] #####", API_VERSION_NUMBER_MAJOR,API_VERSION_NUMBER_MINOR,API_VERSION_NUMBER_PATCH);
            // m_locateCastTimer.connect( bind( &XCastImplementation::onLocateCastTimer, this ));
             XCastImplementation::_instance = this;
         }
 
         XCastImplementation::~XCastImplementation()
         {
            //Deinitialize();
            if (nullptr != _service)
            {
                _service->Release();
                _service = nullptr;
            }
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
             if (std::find(_notificationClients.begin(), _notificationClients.end(), notification) == _notificationClients.end())
             {
                 _notificationClients.push_back(notification);
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
             
             return 0;
         }
 
         uint32_t XCastImplementation::Configure(PluginHost::IShell* service)
         {
            return 0;
         }







        Core::hresult XCastImplementation::ApplicationStateChanged(const string& applicationName, const string& state, const string& applicationId, const string& error) { return 0; }
		Core::hresult XCastImplementation::GetProtocolVersion(string &protocolVersion  ) { return 0; }
		Core::hresult XCastImplementation::SetNetworkStandbyMode(bool networkStandbyMode) { return 0; }
		Core::hresult XCastImplementation::SetManufacturerName(string manufacturername) { return 0; }
		Core::hresult XCastImplementation::GetManufacturerName(string &manufacturername  ) { return 0; }
		Core::hresult XCastImplementation::SetModelName(string modelname) { return 0; }
		Core::hresult XCastImplementation::GetModelName(string &modelname  ) { return 0; }

		Core::hresult XCastImplementation::SetEnabled(bool enabled) { return 0; }
		Core::hresult XCastImplementation::GetEnabled(bool &enabled , bool &success ) { return 0; }
		Core::hresult XCastImplementation::SetStandbyBehavior(string standbybehavior) { return 0; }
		Core::hresult XCastImplementation::GetStandbyBehavior(string &standbybehavior , bool &success  ) { return 0; }
		Core::hresult XCastImplementation::SetFriendlyName(string friendlyname) { return 0; }
		Core::hresult XCastImplementation::GetFriendlyName(string &friendlyname , bool &success ) { return 0; }
		Core::hresult XCastImplementation::GetApiVersionNumber(uint32_t &version , bool &success) { return 0; }

	    Core::hresult XCastImplementation::RegisterApplications(IApplicationInfoIterator* const appInfoList) { return 0; }
     } // namespace Plugin
 } // namespace WPEFramework
