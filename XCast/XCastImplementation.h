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
 #include <interfaces/IXCast.h>
 #include <interfaces/IPowerManager.h>
 #include<interfaces/IConfiguration.h>
 
 #include <com/com.h>
 #include <core/core.h>
 #include <mutex>
 #include <vector>
 
 #include "libIBus.h"
 
 #include "PowerManagerInterface.h"

 #include "XCastManager.h"
#include "XCastNotifier.h"
#include <vector>
 
 namespace WPEFramework
 {
     namespace Plugin
     {
         
        class XCastImplementation : public Exchange::IXCast, public Exchange::IConfiguration
         {
         public:
             // We do not allow this plugin to be copied !!
             XCastImplementation();
             ~XCastImplementation() override;
 
             static XCastImplementation *instance(XCastImplementation *XCastImpl = nullptr);
 
             // We do not allow this plugin to be copied !!
             XCastImplementation(const XCastImplementation &) = delete;
             XCastImplementation &operator=(const XCastImplementation &) = delete;
            
             
             BEGIN_INTERFACE_MAP(XCastImplementation)
             INTERFACE_ENTRY(Exchange::IXCast)
             INTERFACE_ENTRY(Exchange::IConfiguration)
             END_INTERFACE_MAP
 
             
 
         public:
             Core::hresult Register(Exchange::IXCast::INotification *notification) override;
             Core::hresult Unregister(Exchange::IXCast::INotification *notification) override; 
             uint32_t Configure(PluginHost::IShell* service) override;


            Core::hresult ApplicationStateChanged(const string& applicationName, const string& state, const string& applicationId, const string& error) override;
            Core::hresult GetProtocolVersion(string &protocolVersion  ) override;
            Core::hresult SetNetworkStandbyMode(bool networkStandbyMode) override;
            Core::hresult SetManufacturerName(string manufacturername) override;
            Core::hresult GetManufacturerName(string &manufacturername  ) override;
            Core::hresult SetModelName(string modelname) override;
            Core::hresult GetModelName(string &modelname  ) override;
    
            Core::hresult SetEnabled(bool enabled) override;
            Core::hresult GetEnabled(bool &enabled , bool &success ) override;
            Core::hresult SetStandbyBehavior(string standbybehavior) override;
            Core::hresult GetStandbyBehavior(string &standbybehavior , bool &success  ) override;
            Core::hresult SetFriendlyName(string friendlyname) override;
            Core::hresult GetFriendlyName(string &friendlyname , bool &success ) override;
            Core::hresult GetApiVersionNumber(uint32_t &version , bool &success) override;

            Core::hresult RegisterApplications(Exchange::IXCast::IApplicationInfoIterator* const appInfoList) override;
 
         private:
             mutable Core::CriticalSection _adminLock;
             PluginHost::IShell* _service;
             std::list<Exchange::IXCast::INotification *> _xcastNotification; // List of registered notifications
             
 
         public:
             static XCastImplementation *_instance;
         };
 
     } // namespace Plugin
 } // namespace WPEFramework
