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

#pragma once

typedef enum gdialServiceErrorCodes
{
    GDIAL_SERVICE_ERROR_NONE,
    GDIAL_SERVICE_ERROR_FAILED
}
GDIAL_SERVICE_ERROR_CODES;

class RegisterAppEntry
{
public:
    std::string Names;
    std::string prefixes;
    std::string cors;
    int allowStop;
};

class RegisterAppEntryList
{
public:
    void pushBack(RegisterAppEntry* appEntry)
	{
        appEntries.push_back(appEntry);
    }

    const std::vector<RegisterAppEntry*>& getValues() const
	{
        return appEntries;
    }

    ~RegisterAppEntryList()
	{
        for (RegisterAppEntry* appEntry : appEntries)
		{
            delete appEntry;
        }
        appEntries.clear();
    }

private:
    std::vector<RegisterAppEntry*> appEntries;
};

class GDialNotifier
{
public:
    virtual void onApplicationLaunchRequest(std::string appName, std::string parameter)=0;
    virtual void onApplicationLaunchRequestWithLaunchParam (std::string appName,std::string strPayLoad, std::string strQuery, std::string strAddDataUrl)=0;
    virtual void onApplicationStopRequest(std::string appName, std::string appID)=0;
    virtual void onApplicationHideRequest(std::string appName, std::string appID)=0;
    virtual void onApplicationResumeRequest(std::string appName, std::string appID)=0;
    virtual void onApplicationStateRequest(std::string appName, std::string appID)=0;
    virtual void onStopped(void)=0;
    virtual void updatePowerState(std::string powerState)=0;
};

class gdialServiceImpl {
public:
    virtual GDIAL_SERVICE_ERROR_CODES ApplicationStateChanged(std::string applicationName, std::string appState, std::string applicationId, std::string error) const = 0;
    virtual GDIAL_SERVICE_ERROR_CODES ActivationChanged(std::string activation, std::string friendlyname) const = 0;
    virtual GDIAL_SERVICE_ERROR_CODES FriendlyNameChanged(std::string friendlyname) const = 0;
    virtual std::string getProtocolVersion(void) const = 0;
    virtual GDIAL_SERVICE_ERROR_CODES RegisterApplications(RegisterAppEntryList* appConfigList) const = 0;
    virtual void setNetworkStandbyMode(bool nwStandbymode) const = 0;
};

class gdialService {
protected:
    static  gdialServiceImpl* impl;
public:
    static gdialService* getInstance(GDialNotifier* observer, const std::vector<std::string>& gdial_args,const std::string& actualprocessName );
    static void destroyInstance();
    static void setImpl(gdialServiceImpl* newImpl);

    GDIAL_SERVICE_ERROR_CODES ApplicationStateChanged(std::string applicationName, std::string appState, std::string applicationId, std::string error);
    GDIAL_SERVICE_ERROR_CODES ActivationChanged(std::string activation, std::string friendlyname);
    GDIAL_SERVICE_ERROR_CODES FriendlyNameChanged(std::string friendlyname);
    std::string getProtocolVersion(void);
    GDIAL_SERVICE_ERROR_CODES RegisterApplications(RegisterAppEntryList* appConfigList);
    void setNetworkStandbyMode(bool nwStandbymode);
};