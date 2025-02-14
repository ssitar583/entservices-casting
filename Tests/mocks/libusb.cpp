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

#include "libusb.h"
#include <gmock/gmock.h>

libusbApiImpl* libusbApi::impl = nullptr;

libusbApi::libusbApi() {}

void libusbApi::setImpl(libusbApiImpl* newImpl)
{
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

int libusbApi::libusb_init_context(libusb_context **ctx, const struct libusb_init_option options[], int num_options)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_init_context(ctx, options, num_options);
}

int libusbApi::libusb_has_capability(uint32_t capability) 
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_has_capability(capability);
}

int libusbApi::libusb_hotplug_register_callback(libusb_context *ctx,
                                                                                  int events, int flags,
                                                                                  int vendor_id, int product_id, int dev_class,
                                                                                  libusb_hotplug_callback_fn cb_fn, void *user_data,
                                                                                  libusb_hotplug_callback_handle *callback_handle) 
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_hotplug_register_callback(ctx, events, flags, vendor_id, product_id, dev_class, cb_fn, user_data, callback_handle);
}

void libusbApi::libusb_exit(libusb_context *ctx) 
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_exit(ctx);
}

void libusbApi::libusb_hotplug_deregister_callback(libusb_context *ctx,  libusb_hotplug_callback_handle callback_handle)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_hotplug_deregister_callback(ctx,  callback_handle);
}

int libusbApi::libusb_handle_events(libusb_context *ctx)
{
        return LIBUSB_SUCCESS;
}

int libusbApi::libusb_open(libusb_device *dev, libusb_device_handle **dev_handle)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_open(dev, dev_handle);
}

void libusbApi::libusb_close(libusb_device_handle *dev_handle)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_close(dev_handle);
}

int libusbApi::libusb_get_string_descriptor(libusb_device_handle *dev_handle,
	uint8_t desc_index, uint16_t langid, unsigned char *data, int length)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_string_descriptor(dev_handle, desc_index, langid, data, length);
}
int libusbApi::libusb_get_string_descriptor_ascii(libusb_device_handle *dev_handle,
	uint8_t desc_index, unsigned char *data, int length)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_string_descriptor_ascii(dev_handle, desc_index, data, length);
}
int libusbApi::libusb_get_active_config_descriptor(libusb_device *dev,
	struct libusb_config_descriptor **config)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_active_config_descriptor(dev, config);
}

int libusbApi::libusb_get_device_speed(libusb_device *dev)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_device_speed(dev);
}

int libusbApi::libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_device_descriptor(dev, desc);
}

void libusbApi::libusb_free_config_descriptor(struct libusb_config_descriptor *config)
{
    EXPECT_NE(impl, nullptr);
    impl->libusb_free_config_descriptor(config);
}

int libusbApi::libusb_attach_kernel_driver(libusb_device_handle *dev_handle, int interface_number)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_attach_kernel_driver(dev_handle, interface_number);
}

int libusbApi::libusb_detach_kernel_driver(libusb_device_handle *dev_handle, int interface_number)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_detach_kernel_driver(dev_handle, interface_number);
}

int libusbApi::libusb_kernel_driver_active(libusb_device_handle *dev_handle, int interface_number)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_kernel_driver_active(dev_handle, interface_number);
}

int libusbApi::libusb_get_config_descriptor(libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_config_descriptor(dev, config_index, config);
}

uint8_t libusbApi::libusb_get_port_number(libusb_device *dev)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_port_number(dev);
}

uint8_t libusbApi::libusb_get_device_address(libusb_device *dev)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_device_address(dev);
}

uint8_t libusbApi::libusb_get_bus_number(libusb_device *dev)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_bus_number(dev);
}

int libusbApi::libusb_get_port_numbers(libusb_device *dev, uint8_t *port_numbers, int port_numbers_len)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_port_numbers(dev, port_numbers, port_numbers_len);
}

const char* libusbApi::libusb_strerror(int errcode)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_strerror(errcode);
}

ssize_t libusbApi::libusb_get_device_list(libusb_context *ctx, libusb_device ***list)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_get_device_list(ctx, list);
}

void libusbApi::libusb_free_device_list(libusb_device **list, int unref_devices)
{
    EXPECT_NE(impl, nullptr);
    return impl->libusb_free_device_list(list, unref_devices);
}

int (*libusb_init_context)(libusb_context **ctx, const struct libusb_init_option options[], int num_options) = &libusbApi::libusb_init_context;
int (*libusb_has_capability)(uint32_t capability) = &libusbApi::libusb_has_capability;
int (*libusb_hotplug_register_callback)(libusb_context *ctx,
                                                                             int events, int flags,
                                                                             int vendor_id, int product_id, int dev_class,
                                                                             libusb_hotplug_callback_fn cb_fn, void *user_data,
                                                                             libusb_hotplug_callback_handle *callback_handle) = &libusbApi::libusb_hotplug_register_callback;
void (*libusb_exit)(libusb_context *ctx) = &libusbApi::libusb_exit;
void (*libusb_hotplug_deregister_callback)(libusb_context *ctx,  libusb_hotplug_callback_handle callback_handle) = &libusbApi::libusb_hotplug_deregister_callback;
int (*libusb_handle_events)(libusb_context *ctx) = &libusbApi::libusb_handle_events;
int (*libusb_get_device_descriptor)(libusb_device *dev, struct libusb_device_descriptor *desc) = &libusbApi::libusb_get_device_descriptor;
int (*libusb_open)(libusb_device *dev, libusb_device_handle **dev_handle) = &libusbApi::libusb_open;
void (*libusb_close)(libusb_device_handle *dev_handle) = &libusbApi::libusb_close;
int (*libusb_get_string_descriptor)(libusb_device_handle *dev_handle, uint8_t desc_index, uint16_t langid, unsigned char *data, int length) = &libusbApi::libusb_get_string_descriptor;
int (*libusb_get_string_descriptor_ascii)(libusb_device_handle *dev_handle, uint8_t desc_index, unsigned char *data, int length) = &libusbApi::libusb_get_string_descriptor_ascii;
int (*libusb_get_active_config_descriptor)(libusb_device *dev, struct libusb_config_descriptor **config) = &libusbApi::libusb_get_active_config_descriptor;
int (*libusb_get_device_speed)(libusb_device *dev) = &libusbApi::libusb_get_device_speed;
void (*libusb_free_config_descriptor)(struct libusb_config_descriptor *config) = &libusbApi::libusb_free_config_descriptor;
int (*libusb_kernel_driver_active)(libusb_device_handle *dev_handle, int interface_number) = &libusbApi::libusb_kernel_driver_active;
int (*libusb_detach_kernel_driver)(libusb_device_handle *dev_handle, int interface_number) = &libusbApi::libusb_detach_kernel_driver;
int (*libusb_attach_kernel_driver)(libusb_device_handle *dev_handle, int interface_number) = &libusbApi::libusb_attach_kernel_driver;
int (*libusb_get_config_descriptor)(libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config)  = &libusbApi::libusb_get_config_descriptor;
uint8_t (*libusb_get_port_number)(libusb_device *dev) = &libusbApi::libusb_get_port_number;
uint8_t (*libusb_get_device_address)(libusb_device *dev) = &libusbApi::libusb_get_device_address;
uint8_t (*libusb_get_bus_number)(libusb_device *dev) = &libusbApi::libusb_get_bus_number;
int (*libusb_get_port_numbers)(libusb_device *dev, uint8_t *port_numbers, int port_numbers_len) = &libusbApi::libusb_get_port_numbers;
const char* (*libusb_strerror)(int errcode) = &libusbApi::libusb_strerror;
ssize_t (*libusb_get_device_list)(libusb_context *ctx, libusb_device ***list) = &libusbApi::libusb_get_device_list;
void (*libusb_free_device_list)(libusb_device **list, int unref_devices) = &libusbApi::libusb_free_device_list;
