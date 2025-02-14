#pragma once

#include <gmock/gmock.h>
#include <interfaces/IUSBDevice.h>

using ::WPEFramework::Exchange::IUSBDevice;

class USBDeviceMock : public IUSBDevice {
public:
    USBDeviceMock() = default;
    virtual ~USBDeviceMock() = default;

    MOCK_METHOD(uint32_t, Register, (INotification *notification), (override));
    MOCK_METHOD(uint32_t, Unregister, (INotification *notification), (override));
    MOCK_METHOD(uint32_t, GetDeviceList, (IUSBDeviceIterator*& devices), (const, override));
    MOCK_METHOD(uint32_t, GetDeviceInfo, (const string &deviceName, IUSBDeviceInfoIterator*& deviceInfo), (const, override));
    MOCK_METHOD(uint32_t, BindDriver, (const string &deviceName), (const, override));
    MOCK_METHOD(uint32_t, UnbindDriver, (const string &deviceName), (const, override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));
};


class USBDeviceNotificationMock : public IUSBDevice::INotification {
    public:
    USBDeviceNotificationMock() = default;
    virtual ~USBDeviceNotificationMock() = default;

    MOCK_METHOD(void, OnDevicePluggedIn, (const IUSBDevice::USBDevice &device), (override));
    MOCK_METHOD(void, OnDevicePluggedOut, (const IUSBDevice::USBDevice &device), (override));

};