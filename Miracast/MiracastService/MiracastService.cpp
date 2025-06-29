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

#include "MiracastService.h"

const string WPEFramework::Plugin::MiracastService::SERVICE_NAME = "org.rdk.MiracastService";

namespace WPEFramework
{
    namespace
    {
        static Plugin::Metadata<Plugin::MiracastService> metadata(
            /* Version (Major, Minor, Patch) */
            MIRACAST_SERVICE_API_VERSION_NUMBER_MAJOR, MIRACAST_SERVICE_API_VERSION_NUMBER_MINOR, MIRACAST_SERVICE_API_VERSION_NUMBER_PATCH,
            /* Preconditions */
            {},
            /* Terminations */
            {},
            /* Controls */
            {}
        );
    }

    namespace Plugin
    {
        SERVICE_REGISTRATION(MiracastService, MIRACAST_SERVICE_API_VERSION_NUMBER_MAJOR, MIRACAST_SERVICE_API_VERSION_NUMBER_MINOR, MIRACAST_SERVICE_API_VERSION_NUMBER_PATCH);

        MiracastService* MiracastService::_instance = nullptr;

        MiracastService::MiracastService()
                : PluginHost::JSONRPC()
                , mCurrentService(nullptr)
                , mConnectionId(0)
                , mMiracastServiceImpl(nullptr)
                , mMiracastServiceNotification(this)
        {
            SYSLOG(Logging::Startup, (_T("MiracastService Constructor")));
            MiracastService::_instance = this;
        }

        MiracastService::~MiracastService()
        {
            SYSLOG(Logging::Startup, (_T("MiracastService Destructor")));
            MiracastService::_instance = nullptr;
        }

        const string MiracastService::Initialize(PluginHost::IShell* service )
        {
            string retStatus = "";

            ASSERT(nullptr != service);
            ASSERT(nullptr == mCurrentService);
            ASSERT(nullptr == mMiracastServiceImpl);
            ASSERT(0 == mConnectionId);

            SYSLOG(Logging::Startup, (_T("MiracastService::Initialize: PID=%u"), getpid()));

            mCurrentService = service;

            if (nullptr != mCurrentService)
            {
                mCurrentService->AddRef();
                mCurrentService->Register(&mMiracastServiceNotification);

                mMiracastServiceImpl = mCurrentService->Root<Exchange::IMiracastService>(mConnectionId, 5000, _T(PLUGIN_MIRACAST_SERVICE_IMPLEMENTATION_NAME));

                if (nullptr != mMiracastServiceImpl)
                {
                    mConfigure = mMiracastServiceImpl->QueryInterface<Exchange::IConfiguration>();
                    if (mConfigure)
                    {
                        uint32_t result = mConfigure->Configure(mCurrentService);
                        if(result != Core::ERROR_NONE)
                        {
                            SYSLOG(Logging::Startup, (_T("MiracastService::Initialize: Failed to Configure %s"), PLUGIN_MIRACAST_SERVICE_IMPLEMENTATION_NAME));
                            retStatus = _T("MiracastService plugin could not be initialised");
                        }
                        else
                        {
                            /* Register for notifications */
                            mMiracastServiceImpl->Register(&mMiracastServiceNotification);
                            /* Invoking Plugin API register to wpeframework */
                            Exchange::JMiracastService::Register(*this, mMiracastServiceImpl);
                        }
                        mConfigure->Release();
                    }
                    else
                    {
                        retStatus = _T("MiracastService implementation did not provide a configuration interface");
                        SYSLOG(Logging::Startup, (_T("MiracastService::Initialize: MiracastServiceImpl[%s] does not provide a configuration interface"), PLUGIN_MIRACAST_SERVICE_IMPLEMENTATION_NAME));
                    }
                }
                else
                {
                    SYSLOG(Logging::Startup, (_T("MiracastService::Initialize: MiracastServiceImpl[%s] object creation failed"), PLUGIN_MIRACAST_SERVICE_IMPLEMENTATION_NAME));
                    retStatus = _T("MiracastService plugin could not be initialised");
                }
            }
            else
            {
                SYSLOG(Logging::Startup, (_T("MiracastService::Initialize: service is not valid")));
                retStatus = _T("MiracastService plugin could not be initialised");
            }

            if (0 != retStatus.length())
            {
                Deinitialize(service);
            }

            return retStatus;
        }

        void MiracastService::Deinitialize(PluginHost::IShell* service)
        {
            SYSLOG(Logging::Startup, (_T("MiracastService::Deinitialize: PID=%u"), getpid()));

            ASSERT(mCurrentService == service);
            ASSERT(0 == mConnectionId);

            if (nullptr != mMiracastServiceImpl)
            {
                mMiracastServiceImpl->Unregister(&mMiracastServiceNotification);
                Exchange::JMiracastService::Unregister(*this);

                /* Stop processing: */
                RPC::IRemoteConnection* connection = service->RemoteConnection(mConnectionId);
                VARIABLE_IS_NOT_USED uint32_t result = mMiracastServiceImpl->Release();
    
                mMiracastServiceImpl = nullptr;
    
                /* It should have been the last reference we are releasing,
                * so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                * are leaking... */
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
    
                /* If this was running in a (container) process... */
                if (nullptr != connection)
                {
                /* Lets trigger the cleanup sequence for
                    * out-of-process code. Which will guard
                    * that unwilling processes, get shot if
                    * not stopped friendly :-) */
                connection->Terminate();
                connection->Release();
                }
            }
    
            if (nullptr != mCurrentService)
            {
                /* Make sure the Activated and Deactivated are no longer called before we start cleaning up.. */
                mCurrentService->Unregister(&mMiracastServiceNotification);
                mCurrentService->Release();
                mCurrentService = nullptr;
            }

            mConnectionId = 0;
            SYSLOG(Logging::Shutdown, (string(_T("MiracastService de-initialised"))));
        }

        string MiracastService::Information() const
        {
            return(string("{\"service\": \"") + SERVICE_NAME + string("\"}"));
        }

        void MiracastService::Deactivated(RPC::IRemoteConnection* connection)
        {
            if (connection->Id() == mConnectionId) {
                ASSERT(nullptr != mCurrentService);
                LOGINFO("MiracastService::Deactivated");
                Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mCurrentService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            }
        }
    } // namespace Plugin
} // namespace WPEFramework