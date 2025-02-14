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

struct udev {
};

struct udev_enumerate {
};

struct udev_list_entry {
};

struct udev_device {
};

class UdevImpl {
public:
    virtual ~UdevImpl() = default;

    virtual struct udev* udev_new() = 0;
    virtual struct udev_enumerate* udev_enumerate_new(struct udev* udev) = 0;
    virtual void udev_enumerate_add_match_subsystem(struct udev_enumerate* enumerate, const char* subsystem) = 0;
    virtual void udev_enumerate_scan_devices(struct udev_enumerate* enumerate) = 0;
    virtual struct udev_list_entry* udev_enumerate_get_list_entry(struct udev_enumerate* enumerate) = 0;
    virtual void udev_enumerate_unref(struct udev_enumerate* enumerate) = 0;
    virtual void udev_unref(struct udev* udev) = 0;
    virtual const char* udev_list_entry_get_name(struct udev_list_entry* entry) = 0;
    virtual struct udev_device* udev_device_new_from_syspath(struct udev* udev, const char* path) = 0;
    virtual void udev_device_unref(struct udev_device* dev) = 0;
    virtual struct udev_device* udev_device_get_parent_with_subsystem_devtype(struct udev_device* dev, const char* subsystem, const char* devtype) = 0;
    virtual const char* udev_device_get_devtype(struct udev_device* dev) = 0;
    virtual const char* udev_device_get_devnode(struct udev_device* dev) = 0;
};

class Udev {
protected:
    static UdevImpl* impl;
public:
    Udev();
    Udev(const Udev &obj) = delete;
    static void setImpl(UdevImpl* newImpl);
    static struct udev* udev_new();
    static udev_enumerate* udev_enumerate_new(struct udev* udev);
    static void udev_enumerate_add_match_subsystem(struct udev_enumerate* enumerate, const char* subsystem);
    static void udev_enumerate_scan_devices(struct udev_enumerate* enumerate);
    static struct udev_list_entry* udev_enumerate_get_list_entry(struct udev_enumerate* enumerate);
    static void udev_enumerate_unref(struct udev_enumerate* enumerate);
    static void udev_unref(struct udev* udev);
    static const char* udev_list_entry_get_name(struct udev_list_entry* entry);
    static struct udev_device* udev_device_new_from_syspath(struct udev* udev, const char* path);
    static void udev_device_unref(struct udev_device* dev);
    static struct udev_device* udev_device_get_parent_with_subsystem_devtype(struct udev_device* dev, const char* subsystem, const char* devtype);
    static const char* udev_device_get_devtype(struct udev_device* dev);
    static const char* udev_device_get_devnode(struct udev_device* dev);
};

extern struct udev* (*udev_new)();
extern udev_enumerate* (*udev_enumerate_new)(struct udev*);
extern void (*udev_enumerate_add_match_subsystem)(struct udev_enumerate*, const char*);
extern void (*udev_enumerate_scan_devices)(struct udev_enumerate*);
extern struct udev_list_entry* (*udev_enumerate_get_list_entry)(struct udev_enumerate*);
extern void (*udev_enumerate_unref)(struct udev_enumerate*);
extern void (*udev_unref)(struct udev*);
extern const char* (*udev_list_entry_get_name)(struct udev_list_entry*);
extern struct udev_device* (*udev_device_new_from_syspath)(struct udev*, const char*);
extern void (*udev_device_unref)(struct udev_device*);
extern struct udev_device* (*udev_device_get_parent_with_subsystem_devtype)(struct udev_device*, const char*, const char*);
extern const char* (*udev_device_get_devtype)(struct udev_device*);
extern const char* (*udev_device_get_devnode)(struct udev_device*);

#define udev_list_entry_foreach(list_entry, first_entry) \
    for (list_entry = first_entry;                       \
         list_entry;                                     \
         list_entry = nullptr)
