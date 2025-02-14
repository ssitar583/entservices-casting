/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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

#include "gdialservice.h"
#include <gmock/gmock.h>

gdialServiceImpl* gdialService::impl = nullptr;
static gdialService instance;

gdialService* gdialService::getInstance(GDialNotifier* observer, const std::vector<std::string>& gdial_args,const std::string& actualprocessName )
{
    static gdialService instance;
    return &instance;
}

void gdialService::destroyInstance(void) { }

void gdialService::setImpl(gdialServiceImpl* newImpl)
{
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

GDIAL_SERVICE_ERROR_CODES gdialService::ApplicationStateChanged(string applicationName, string appState, string applicationId, string error)
{
    EXPECT_NE(impl, nullptr);
    return (impl->ApplicationStateChanged(applicationName, appState, applicationId, error));
}

GDIAL_SERVICE_ERROR_CODES gdialService::ActivationChanged(string activation, string friendlyname)
{
    EXPECT_NE(impl, nullptr);
    return(impl->ActivationChanged(activation, friendlyname));
}

GDIAL_SERVICE_ERROR_CODES gdialService::FriendlyNameChanged(string friendlyname)
{
    EXPECT_NE(impl, nullptr);
    return(impl->FriendlyNameChanged(friendlyname));
}

string gdialService::getProtocolVersion(void)
{
    EXPECT_NE(impl, nullptr);
    return(impl->getProtocolVersion());
}

GDIAL_SERVICE_ERROR_CODES gdialService::RegisterApplications(RegisterAppEntryList* appConfigList)
{
    EXPECT_NE(impl, nullptr);
    return(impl->RegisterApplications(appConfigList));
}

void gdialService::setNetworkStandbyMode(bool nwStandbymode)
{
    EXPECT_NE(impl, nullptr);
    impl->setNetworkStandbyMode(nwStandbymode);
}

GDIAL_SERVICE_ERROR_CODES (*ApplicationStateChanged)(string applicationName, string appState, string applicationId, string error) = &gdialService::ApplicationStateChanged;
GDIAL_SERVICE_ERROR_CODES (*ActivationChanged)(string activation, string friendlyname) = &gdialService::ActivationChanged;
GDIAL_SERVICE_ERROR_CODES (*FriendlyNameChanged)(string friendlyname) = &gdialService::FriendlyNameChanged;
string (*getProtocolVersion)(void) = &gdialService::getProtocolVersion;
GDIAL_SERVICE_ERROR_CODES (*RegisterApplications)(RegisterAppEntryList* appConfigList) = &gdialService::RegisterApplications;
void (*setNetworkStandbyMode)(bool nwStandbymode) = &gdialService::setNetworkStandbyMode;
