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

#include "XCast.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 9

namespace WPEFramework
{

    namespace {

        static Plugin::Metadata<Plugin::XCast> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    namespace Plugin
    {
        SERVICE_REGISTRATION(XCast, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

		XCast::XCast()
            : _service(nullptr)
            , _connectionId(0)
            , _xcast(nullptr)
            , _xcastNotification(this)
        {
            SYSLOG(Logging::Startup, (_T("XCast Constructor")));
        }

        XCast::~XCast()
        {
            SYSLOG(Logging::Shutdown, (string(_T("XCast Destructor"))));
        }
    
	    const string XCast::Initialize(PluginHost::IShell *service)
        {
            string message = "";

            ASSERT(nullptr != service);
            ASSERT(nullptr == _service);
            ASSERT(nullptr == _xcast);
            ASSERT(0 == _connectionId);

            SYSLOG(Logging::Startup, (_T("XCast::Initialize: PID=%u"), getpid()));

            _service = service;
            _service->AddRef();
            _service->Register(&_xcastNotification); 

            _xcast = _service->Root<Exchange::IXCast>(_connectionId, 5000, _T("XCastImplementation"));
            
            if (nullptr != _xcast)
            {
                auto configure = _xcast->QueryInterface<Exchange::IConfiguration>();
                if (configure != nullptr)
                {
                    uint32_t result = configure->Configure(_service);
                    if(result != Core::ERROR_NONE)
                    {
                        message = _T("XCast could not be configured");
                    }
			        configure->Release();
                }
                else
                {
                    message = _T("XCast implementation did not provide a configuration interface");
                }
                // Register for notifications
                _xcast->Register(&_xcastNotification);
                
                // Invoking Plugin API register to wpeframework
                Exchange::JXCast::Register(*this, _xcast);
            }
            else
            {
                SYSLOG(Logging::Startup, (_T("XCast::Initialize: Failed to initialise XCast plugin")));
                message = _T("XCast plugin could not be initialised");
            }

            if (0 != message.length())
            {
                printf("XCast::Initialize: Failed to initialise XCast plugin");
                Deinitialize(service);
            }

            return message;
        }

        void XCast::Deinitialize(PluginHost::IShell *service)
        {
            ASSERT(_service == service);
            printf("XCast::Deinitialize: service = %p", service);
            SYSLOG(Logging::Shutdown, (string(_T("XCast::Deinitialize"))));

            // Make sure the Activated and Deactivated are no longer called before we start cleaning up..
            if (_service != nullptr)
            {
                _service->Unregister(&_xcastNotification);
            }
            if (nullptr != _xcast)
            {
                
                _xcast->Unregister(&_xcastNotification);
                Exchange::JXCast::Unregister(*this);
                // Stop processing:
                RPC::IRemoteConnection *connection = service->RemoteConnection(_connectionId);
                VARIABLE_IS_NOT_USED uint32_t result = _xcast->Release();

                _xcast = nullptr;

                // It should have been the last reference we are releasing,
                // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                // are leaking...
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // If this was running in a (container) process...
                if (nullptr != connection)
                {
                    // Lets trigger the cleanup sequence for
                    // out-of-process code. Which will guard
                    // that unwilling processes, get shot if
                    // not stopped friendly :-)
                    connection->Terminate();
                    connection->Release();
                }
            }
			_connectionId = 0;

            if (_service != nullptr)
            {
                _service->Release();
                _service = nullptr;
            }
            SYSLOG(Logging::Shutdown, (string(_T("XCast de-initialised"))));
        }
	    string XCast::Information() const
        {
            return ("This XCast Plugin facilitates to persist event data for monitoring applications");
        }

        void XCast::Deactivated(RPC::IRemoteConnection *connection)
        {
            if (connection->Id() == _connectionId)
            {
                ASSERT(nullptr != _service);
                Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            }
        }
    } // namespace Plugin
} // namespace WPEFramework