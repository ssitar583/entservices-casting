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

#include <gmock/gmock.h>

#include "host.hpp"

class HostImplMock : public device::HostImpl {
public:
    virtual ~HostImplMock() = default;

    MOCK_METHOD(device::SleepMode, getPreferredSleepMode, (), (override));
    MOCK_METHOD(int, setPreferredSleepMode, (const device::SleepMode), (override));
    MOCK_METHOD(device::List<device::SleepMode>, getAvailableSleepModes, (), (override));
    MOCK_METHOD(device::List<device::VideoOutputPort>, getVideoOutputPorts, (), (override));
    MOCK_METHOD(device::List<device::AudioOutputPort>, getAudioOutputPorts, (), (override));
    MOCK_METHOD(device::List<device::VideoDevice>, getVideoDevices, (), (override));
    MOCK_METHOD(device::VideoOutputPort&, getVideoOutputPort, (const std::string& name), (override));
    MOCK_METHOD(device::AudioOutputPort&, getAudioOutputPort, (const std::string& name), (override));
    MOCK_METHOD(void, getHostEDID, (std::vector<uint8_t> & edid), (const, override));
    MOCK_METHOD(std::string, getDefaultVideoPortName, (), (override));
    //MOCK_METHOD(void, setAudioMixerLevels, (dsAudioInput_t aInput, int volume), (override));
    MOCK_METHOD(std::string, getDefaultAudioPortName, (), (override));
    
    MOCK_METHOD(void, getMS12ConfigDetails, (std::string type), (override));
    MOCK_METHOD(bool, isHDMIOutPortPresent, (), (override));
    MOCK_METHOD(void, getSinkDeviceAtmosCapability, (dsATMOSCapability_t atmosCapability), (override));
    MOCK_METHOD(void, getSecondaryLanguage, (std::string secondaryLanguage), (override));
    MOCK_METHOD(void,setSecondaryLanguage, (std::string secondaryLanguage), (override));
    MOCK_METHOD(void, getFaderControl, (int *mixerBalance), (override));
    MOCK_METHOD(void, setFaderControl, (int mixerBalance), (override));
    MOCK_METHOD(void, setPrimaryLanguage, (std::string primaryLanguage), (override));
    MOCK_METHOD(void, getPrimaryLanguage, (std::string primaryLanguage), (override));
    MOCK_METHOD(void, setAudioAtmosOutputMode, (int enable), (override));
    MOCK_METHOD(void, setAssociatedAudioMixing, (bool mixing), (override));
    MOCK_METHOD(void, getCurrentAudioFormat, (dsAudioFormat_t audioFormat), (override));
    MOCK_METHOD(void, getAssociatedAudioMixing, (bool *mixing), (override));

    MOCK_METHOD(void, setAudioMixerLevels, (dsAudioInput_t gain, int volume), (override));

};
