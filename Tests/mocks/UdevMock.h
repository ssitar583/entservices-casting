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

#include "Udev.h"

class UdevImplMock : public UdevImpl {
public:
    virtual ~UdevImplMock() = default;

    MOCK_METHOD(struct udev*, udev_new, (), (override));
    MOCK_METHOD(struct udev_enumerate*, udev_enumerate_new, (struct udev * udev), (override));
    MOCK_METHOD(void, udev_enumerate_add_match_subsystem, (struct udev_enumerate * enumerate, const char* subsystem), (override));
    MOCK_METHOD(void, udev_enumerate_scan_devices, (struct udev_enumerate * enumerate), (override));
    MOCK_METHOD(struct udev_list_entry*, udev_enumerate_get_list_entry, (struct udev_enumerate * enumerate), (override));
    MOCK_METHOD(void, udev_enumerate_unref, (struct udev_enumerate * enumerate), (override));
    MOCK_METHOD(void, udev_unref, (struct udev * udev), (override));
    MOCK_METHOD(const char*, udev_list_entry_get_name, (struct udev_list_entry * entry), (override));
    MOCK_METHOD(struct udev_device*, udev_device_new_from_syspath, (struct udev * udev, const char* path), (override));
    MOCK_METHOD(void, udev_device_unref, (struct udev_device * dev), (override));
    MOCK_METHOD(struct udev_device*, udev_device_get_parent_with_subsystem_devtype, (struct udev_device * dev, const char* subsystem, const char* devtype), (override));
    MOCK_METHOD(const char*, udev_device_get_devtype, (struct udev_device * dev), (override));
    MOCK_METHOD(const char*, udev_device_get_devnode, (struct udev_device * dev), (override));
};
