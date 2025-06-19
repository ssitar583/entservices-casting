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

#include "MiracastPlayer.h"

const string WPEFramework::Plugin::MiracastPlayer::SERVICE_NAME = "org.rdk.MiracastPlayer";

namespace WPEFramework
{
	namespace
	{
		static Plugin::Metadata<Plugin::MiracastPlayer> metadata(
			/* Version (Major, Minor, Patch) */
			MIRACAST_PLAYER_API_VERSION_NUMBER_MAJOR, MIRACAST_PLAYER_API_VERSION_NUMBER_MINOR, MIRACAST_PLAYER_API_VERSION_NUMBER_PATCH,
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
		SERVICE_REGISTRATION(MiracastPlayer, MIRACAST_PLAYER_API_VERSION_NUMBER_MAJOR, MIRACAST_PLAYER_API_VERSION_NUMBER_MINOR, MIRACAST_PLAYER_API_VERSION_NUMBER_PATCH);

		MiracastPlayer* MiracastPlayer::_instance = nullptr;

		MiracastPlayer::MiracastPlayer()
				: PluginHost::JSONRPC()
				, mCurrentService(nullptr)
				, mConnectionId(0)
				, mMiracastPlayerImpl(nullptr)
				, mMiracastPlayerNotification(this)
		{
			SYSLOG(Logging::Startup, (_T("MiracastPlayer Constructor")));
			MiracastPlayer::_instance = this;
		}

		MiracastPlayer::~MiracastPlayer()
		{
			SYSLOG(Logging::Startup, (_T("MiracastPlayer Destructor")));
			MiracastPlayer::_instance = nullptr;
		}

		const string MiracastPlayer::Initialize(PluginHost::IShell* service )
		{
			uint32_t result = Core::ERROR_GENERAL;
			string retStatus = "";

			ASSERT(nullptr != service);
			ASSERT(nullptr == mCurrentService);
			ASSERT(nullptr == mMiracastPlayerImpl);
			ASSERT(0 == mConnectionId);

			SYSLOG(Logging::Startup, (_T("MiracastPlayer::Initialize: PID=%u"), getpid()));

			mCurrentService = service;

			if (nullptr != mCurrentService)
			{
				mCurrentService->AddRef();
				mCurrentService->Register(&mMiracastPlayerNotification);

				mMiracastPlayerImpl = mCurrentService->Root<Exchange::IMiracastPlayer>(mConnectionId, 5000, _T(PLUGIN_MIRACAST_PLAYER_IMPLEMENTATION_NAME));

				if (nullptr != mMiracastPlayerImpl)
				{
					if (Core::ERROR_NONE != (result = mMiracastPlayerImpl->Initialize(mCurrentService)))
					{
						SYSLOG(Logging::Startup, (_T("MiracastPlayer::Initialize: Failed to initialise %s"), PLUGIN_MIRACAST_PLAYER_IMPLEMENTATION_NAME));
						retStatus = _T("MiracastPlayer plugin could not be initialised");
					}
					else
					{
						/* Register for notifications */
						mMiracastPlayerImpl->Register(&mMiracastPlayerNotification);
						/* Invoking Plugin API register to wpeframework */
						Exchange::JMiracastPlayer::Register(*this, mMiracastPlayerImpl);
					}
				}
				else
				{
					SYSLOG(Logging::Startup, (_T("MiracastPlayer::Initialize: MiracastPlayerImpl[%s] object creation failed"), PLUGIN_MIRACAST_PLAYER_IMPLEMENTATION_NAME));
					retStatus = _T("MiracastPlayer plugin could not be initialised");
				}
			}
			else
			{
				SYSLOG(Logging::Startup, (_T("MiracastPlayer::Initialize: service is not valid")));
				retStatus = _T("MiracastPlayer plugin could not be initialised");
			}

			if (0 != retStatus.length())
			{
				Deinitialize(service);
			}

			return retStatus;
		}

		void MiracastPlayer::Deinitialize(PluginHost::IShell* service)
		{
			SYSLOG(Logging::Startup, (_T("MiracastPlayer::Deinitialize: PID=%u"), getpid()));

			ASSERT(mCurrentService == service);
			ASSERT(0 == mConnectionId);

			if (nullptr != mMiracastPlayerImpl)
			{
				mMiracastPlayerImpl->Unregister(&mMiracastPlayerNotification);
				Exchange::JMiracastPlayer::Unregister(*this);

				if (nullptr != mCurrentService)
				{
					mMiracastPlayerImpl->Deinitialize(mCurrentService);
				}

				/* Stop processing: */
				RPC::IRemoteConnection* connection = service->RemoteConnection(mConnectionId);
				VARIABLE_IS_NOT_USED uint32_t result = mMiracastPlayerImpl->Release();

				mMiracastPlayerImpl = nullptr;

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
					 * not stopped friendly :-)
					 */
					connection->Terminate();
					connection->Release();
				}
			}

			if (nullptr != mCurrentService)
			{
				/* Make sure the Activated and Deactivated are no longer called before we start cleaning up.. */
				mCurrentService->Unregister(&mMiracastPlayerNotification);
				mCurrentService->Release();
				mCurrentService = nullptr;
			}

			mConnectionId = 0;
			SYSLOG(Logging::Shutdown, (string(_T("MiracastPlayer de-initialised"))));
		}

		string MiracastPlayer::Information() const
		{
			return(string("{\"service\": \"") + SERVICE_NAME + string("\"}"));
		}

		void MiracastPlayer::Deactivated(RPC::IRemoteConnection* connection)
		{
			if (connection->Id() == mConnectionId) {
				ASSERT(nullptr != mCurrentService);
				LOGINFO("MiracastPlayer::Deactivated");
				Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mCurrentService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
			}
		}
	} // namespace Plugin
} // namespace WPEFramework