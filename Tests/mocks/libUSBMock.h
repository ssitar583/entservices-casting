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

#include "libusb.h"

class libUSBImplMock : public libusbApiImpl {
public:
    libUSBImplMock()
        : libusbApiImpl()
    {
        // Defaults:
        ON_CALL(*this, libusb_init_context(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(LIBUSB_SUCCESS));
        ON_CALL(*this, libusb_has_capability(::testing::_))
            .WillByDefault(::testing::Return(1));
        ON_CALL(*this, libusb_hotplug_register_callback(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(LIBUSB_SUCCESS));
        ON_CALL(*this, libusb_get_device_descriptor(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(LIBUSB_SUCCESS));
        ON_CALL(*this, libusb_strerror(::testing::_))
            .WillByDefault(::testing::Return("Other error"));
        ON_CALL(*this, libusb_open(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(LIBUSB_SUCCESS));
        ON_CALL(*this, libusb_get_device_list(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(0));
        ON_CALL(*this, libusb_kernel_driver_active(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(LIBUSB_SUCCESS));
        ON_CALL(*this, libusb_attach_kernel_driver(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(LIBUSB_SUCCESS));
        ON_CALL(*this, libusb_detach_kernel_driver(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(LIBUSB_SUCCESS));
        ON_CALL(*this, libusb_get_config_descriptor(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(LIBUSB_SUCCESS));
        ON_CALL(*this, libusb_get_port_number(::testing::_))
            .WillByDefault(::testing::Return(1));
        ON_CALL(*this, libusb_get_device_address(::testing::_))
            .WillByDefault(::testing::Return(001));
        ON_CALL(*this, libusb_get_bus_number(::testing::_))
            .WillByDefault(::testing::Return(12));
        ON_CALL(*this, libusb_get_device_speed(::testing::_))
            .WillByDefault(::testing::Return(3)); /* DEVICE_SPEED_HIGH */
        ON_CALL(*this, libusb_get_port_numbers(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(0));
    }
    virtual ~libUSBImplMock() = default;

    MOCK_METHOD(int, libusb_init_context, (libusb_context **ctx, const struct libusb_init_option options[], int num_options), (override));
    MOCK_METHOD(int, libusb_has_capability, (uint32_t capability), (override));
    MOCK_METHOD(int, libusb_hotplug_register_callback, (libusb_context *ctx,
                                                                                              int events, int flags,
                                                                                              int vendor_id, int product_id, int dev_class,
                                                                                              libusb_hotplug_callback_fn cb_fn, void *user_data,
                                                                                              libusb_hotplug_callback_handle *callback_handle), (override));
    MOCK_METHOD(void, libusb_exit, (libusb_context *ctx), (override));
    MOCK_METHOD(void, libusb_hotplug_deregister_callback, (libusb_context *ctx,	libusb_hotplug_callback_handle callback_handle), (override));
    MOCK_METHOD(int, libusb_get_device_descriptor, (libusb_device *dev, struct libusb_device_descriptor *desc), (override));
    MOCK_METHOD(int, libusb_open, (libusb_device *dev, libusb_device_handle **dev_handle), (override));
    MOCK_METHOD(void, libusb_close, (libusb_device_handle *dev_handle), (override));
    MOCK_METHOD(int, libusb_get_string_descriptor, (libusb_device_handle *dev_handle,
	                                                                                uint8_t desc_index, uint16_t langid, unsigned char *data, int length), (override));
    MOCK_METHOD(int, libusb_get_string_descriptor_ascii, (libusb_device_handle *dev_handle,
	                                                                                uint8_t desc_index, unsigned char *data, int length), (override));
    MOCK_METHOD(int, libusb_get_active_config_descriptor, (libusb_device *dev,
	                                                                                struct libusb_config_descriptor **config), (override));
    MOCK_METHOD(int, libusb_get_device_speed, (libusb_device *dev), (override));
    MOCK_METHOD(int, libusb_kernel_driver_active, (libusb_device_handle *dev_handle, int interface_number), (override));
    MOCK_METHOD(int, libusb_attach_kernel_driver, (libusb_device_handle *dev_handle, int interface_number), (override));
    MOCK_METHOD(int, libusb_detach_kernel_driver, (libusb_device_handle *dev_handle, int interface_number), (override));
    MOCK_METHOD(void, libusb_free_config_descriptor, (struct libusb_config_descriptor *config), (override));
    MOCK_METHOD(int, libusb_get_config_descriptor, (libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config), (override));
    MOCK_METHOD(uint8_t, libusb_get_port_number, (libusb_device *dev), (override));
    MOCK_METHOD(uint8_t, libusb_get_device_address, (libusb_device *dev), (override));
    MOCK_METHOD(uint8_t, libusb_get_bus_number, (libusb_device *dev), (override));
    MOCK_METHOD(int, libusb_get_port_numbers, (libusb_device *dev, uint8_t *port_numbers, int port_numbers_len), (override));
    MOCK_METHOD(const char *, libusb_strerror, (int errcode), (override));
    MOCK_METHOD(ssize_t, libusb_get_device_list, (libusb_context *ctx, libusb_device ***list), (override));
    MOCK_METHOD(void, libusb_free_device_list, (libusb_device **list, int unref_devices), (override));
};
