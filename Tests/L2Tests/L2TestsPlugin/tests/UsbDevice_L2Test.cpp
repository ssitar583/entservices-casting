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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <interfaces/IUSBDevice.h>

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

#define JSON_TIMEOUT   (1000)
#define USBDEVICE_CALLSIGN  _T("org.rdk.USBDevice")
#define USBDEVICEL2TEST_CALLSIGN _T("L2tests.1")

#define MOCK_USB_DEVICE_BUS_NUMBER_1    100
#define MOCK_USB_DEVICE_ADDRESS_1       001
#define MOCK_USB_DEVICE_PORT_1          123

#define MOCK_USB_DEVICE_BUS_NUMBER_2    101
#define MOCK_USB_DEVICE_ADDRESS_2       002
#define MOCK_USB_DEVICE_PORT_2          124

#define LIBUSB_CONFIG_ATT_SELF_POWERED      0x40
#define LIBUSB_CONFIG_ATT_BUS_POWERED       0x80
#define MOCK_USB_DEVICE_SERIAL_NO "0401805e4532973503374df52a239c898397d348"
#define MOCK_USB_DEVICE_MANUFACTURER "USB"
#define MOCK_USB_DEVICE_PRODUCT "SanDisk 3.2Gen1"
using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::IUSBDevice;

typedef enum : uint32_t {
    USBDevice_onDevicePluggedIn = 0x00000001,
    USBDevice_onDevicePluggedOut = 0x00000002,
    USBDevice_StateInvalid = 0x00000000
}USBDeviceL2test_async_events_t;

class AsyncHandlerMock_USBDevice
{
    public:
        AsyncHandlerMock_USBDevice()
        {
        }
        MOCK_METHOD(void, OnDevicePluggedIn, (const Core::JSON::Container& eventPayload));
        MOCK_METHOD(void, OnDevicePluggedOut,(const Core::JSON::Container& eventPayload));
};

class USBDeviceNotificationHandler : public Exchange::IUSBDevice::INotification {
    private:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled;

        WPEFramework::Exchange::IUSBDevice::USBDevice m_usb_device;

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::IUSBDevice::INotification)
        END_INTERFACE_MAP

    public:
        USBDeviceNotificationHandler(){}
        ~USBDeviceNotificationHandler(){}

        void OnDevicePluggedIn(const Exchange::IUSBDevice::USBDevice &device)
        {
            TEST_LOG("OnDevicePluggedIn event triggered ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("OnDevicePluggedIn received device.deviceClass: %d device.deviceSubclass: %d device.deviceName: %s device.devicePath: %s",
				device.deviceClass, 
				device.deviceSubclass, 
				device.deviceName.c_str(),
				device.devicePath.c_str());

            /* Notify the requester thread. */
            m_event_signalled |= USBDevice_onDevicePluggedIn;

            m_usb_device.deviceClass = device.deviceClass;
            m_usb_device.deviceSubclass = device.deviceSubclass;
            m_usb_device.deviceName = device.deviceName;
            m_usb_device.devicePath = device.devicePath;

            m_condition_variable.notify_one();
        }

        void OnDevicePluggedOut(const Exchange::IUSBDevice::USBDevice &device)
        {
            TEST_LOG("OnDevicePluggedOut event triggered ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("OnDevicePluggedOut received");
        
            /* Notify the requester thread. */
            m_event_signalled |= USBDevice_onDevicePluggedOut;

            m_usb_device.deviceClass = device.deviceClass;
            m_usb_device.deviceSubclass = device.deviceSubclass;
            m_usb_device.deviceName = device.deviceName;
            m_usb_device.devicePath = device.devicePath;

            m_condition_variable.notify_one();
        }

        uint32_t WaitForRequestStatus(uint32_t timeout_ms, USBDeviceL2test_async_events_t expected_status, Exchange::IUSBDevice::USBDevice &actual_usbDevice)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto now = std::chrono::system_clock::now();
            std::chrono::milliseconds timeout(timeout_ms);
            uint32_t signalled = USBDevice_StateInvalid;
            while (!(expected_status & m_event_signalled))
            {
               if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
               {
                  TEST_LOG("Timeout waiting for request status event");
                  break;
               }
           }
 
           signalled = m_event_signalled;
           actual_usbDevice.deviceClass = m_usb_device.deviceClass;
           actual_usbDevice.deviceSubclass = m_usb_device.deviceSubclass;
           actual_usbDevice.deviceName = m_usb_device.deviceName;
           actual_usbDevice.devicePath = m_usb_device.devicePath;

           m_event_signalled = USBDevice_StateInvalid;
           return signalled;
        }
    };

class USBDeviceTest : public L2TestMocks {
protected:
    virtual ~USBDeviceTest() override;

protected:
    libusb_hotplug_callback_fn libUSBHotPlugCbDeviceAttached = nullptr;
    libusb_hotplug_callback_fn libUSBHotPlugCbDeviceDetached = nullptr;

    public:
    USBDeviceTest();

      uint32_t WaitForRequestStatus(uint32_t timeout_ms, USBDeviceL2test_async_events_t expected_status);
      uint32_t CreateUSBDeviceInterfaceObjectUsingComRPCConnection();
      void Mock_SetDeviceDesc(uint8_t bus_number, uint8_t device_address);
      void Mock_SetSerialNumberInUSBDevicePath();

    private:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled;

    protected:
        /** @brief Pointer to the IShell interface */
        PluginHost::IShell *m_controller_usbdevice;

        /** @brief Pointer to the IUSBDevice interface */
        Exchange::IUSBDevice *m_usbdeviceplugin;
};

USBDeviceTest:: USBDeviceTest():L2TestMocks()
{
        Core::JSONRPC::Message message;
        string response;
        uint32_t status = Core::ERROR_GENERAL;

        TEST_LOG("Test Constructor");

        /* Set all the asynchronouse event handler with libusb to handle various events*/
        ON_CALL(*p_libUSBApiImplMock, libusb_hotplug_register_callback(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](libusb_context *ctx, int events, int flags, int vendor_id, int product_id, int dev_class,
                 libusb_hotplug_callback_fn cb_fn, void *user_data, libusb_hotplug_callback_handle *callback_handle) {
                if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == events) {
                    libUSBHotPlugCbDeviceAttached = cb_fn;
                    *callback_handle = 1;
                }
                if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == events) {
                    libUSBHotPlugCbDeviceDetached = cb_fn;
                    *callback_handle = 2;
                }
                return LIBUSB_SUCCESS;
            }));

        TEST_LOG("Activate USBDevice plugin");

         /* Activate plugin in constructor */
         status = ActivateService(USBDEVICE_CALLSIGN);
         EXPECT_EQ(Core::ERROR_NONE, status);
}

/**
 * @brief Destructor for SystemServices L2 test class
 */
USBDeviceTest::~USBDeviceTest()
{
    uint32_t status = Core::ERROR_GENERAL;

    TEST_LOG("Test Destructor");

    ON_CALL(*p_libUSBApiImplMock, libusb_hotplug_deregister_callback(::testing::_, ::testing::_))
    .WillByDefault(::testing::Invoke(
        [&](libusb_context *ctx, libusb_hotplug_callback_handle callback_handle) {
            if (callback_handle == 1) {
                libUSBHotPlugCbDeviceAttached = nullptr;  
            } else if (callback_handle == 2) {
                libUSBHotPlugCbDeviceDetached = nullptr;  
            }
        }));
    TEST_LOG("DeActivate USBDevice plugin");
    status = DeactivateService(USBDEVICE_CALLSIGN);
    EXPECT_EQ(Core::ERROR_NONE, status);
}

uint32_t USBDeviceTest::WaitForRequestStatus(uint32_t timeout_ms, USBDeviceL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(timeout_ms);
    uint32_t signalled = USBDevice_StateInvalid;

   while (!(expected_status & m_event_signalled))
   {
      if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
      {
         TEST_LOG("Timeout waiting for request status event");
         break;
      }
   }

    signalled = m_event_signalled;
    return signalled;
}

void USBDeviceTest::Mock_SetDeviceDesc(uint8_t bus_number, uint8_t device_address)
{
     ON_CALL(*p_libUSBApiImplMock, libusb_get_device_descriptor(::testing::_, ::testing::_))
        .WillByDefault(
            [bus_number, device_address](libusb_device *dev, struct libusb_device_descriptor *desc) {
                 if ((bus_number == dev->bus_number) &&
                     (device_address == dev->device_address))
                 {
                      desc->bDeviceSubClass = LIBUSB_CLASS_MASS_STORAGE;
                      desc->bDeviceClass = LIBUSB_CLASS_MASS_STORAGE;
                 }
                 return LIBUSB_SUCCESS;
     });

    ON_CALL(*p_libUSBApiImplMock, libusb_get_device_address(::testing::_))
        .WillByDefault(::testing::Return(device_address));

    ON_CALL(*p_libUSBApiImplMock, libusb_get_bus_number(::testing::_))
        .WillByDefault(::testing::Return(bus_number));

    ON_CALL(*p_libUSBApiImplMock, libusb_get_port_numbers(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault([](libusb_device *dev, uint8_t *port_numbers, int port_numbers_len) {
            if((nullptr != dev) && (nullptr != port_numbers))
            {
                port_numbers[0] = dev->port_number;
                return 1;
            }
            else
            {
                return 0;
            }
        });

    if (device_address == MOCK_USB_DEVICE_ADDRESS_1)
    {
        std::string vendorFileName = "/tmp/block/sda/device/vendor";
        std::ofstream outVendorStream(vendorFileName);

        if (!outVendorStream) {
            TEST_LOG("Error opening file for writing!");
        }
        outVendorStream << "Generic" << std::endl;
        outVendorStream.close();

        std::string modelFileName = "/tmp/block/sda/device/model";
        std::ofstream outModelStream(modelFileName);

        if (!outModelStream) {
            TEST_LOG("Error opening file for writing!");
        }
        outModelStream << "Flash Disk" << std::endl;
        outModelStream.close();
    }

    if (device_address == MOCK_USB_DEVICE_ADDRESS_2)
    {
        std::string vendorFileName = "/tmp/block/sdb/device/vendor";
        std::ofstream  outVendorStream(vendorFileName);

        if (!outVendorStream) {
            TEST_LOG("Error opening file for writing!");
        }
        outVendorStream << "JetFlash" << std::endl;
        outVendorStream.close();

        std::string modelFileName = "/tmp/block/sdb/device/model";
        std::ofstream outModelStream(modelFileName);

        if (!outModelStream) {
            TEST_LOG("Error opening file for writing!");
        }
        outModelStream << "Transcend_16GB" << std::endl;
        outModelStream.close();
    }
}

void USBDeviceTest::Mock_SetSerialNumberInUSBDevicePath()
{
    std::string serialNumFileName1 = "/tmp/bus/usb/devices/100-123/serial";
    std::ofstream serialNumOutFile1(serialNumFileName1);

    if (!serialNumOutFile1) {
        TEST_LOG("Error opening file for writing!");
    }
    serialNumOutFile1 << "B32FD507" << std::endl;
    serialNumOutFile1.close();

    std::string serialNumFileName2 = "/tmp/bus/usb/devices/101-124/serial";
    std::ofstream serialNumOutFile2(serialNumFileName2);

    if (!serialNumOutFile2) {
        TEST_LOG("Error opening file for writing!");
    }
    serialNumOutFile2<< "UEUIRCXT" << std::endl;
    serialNumOutFile2.close();
}

uint32_t USBDeviceTest::CreateUSBDeviceInterfaceObjectUsingComRPCConnection()
{
    uint32_t return_value =  Core::ERROR_GENERAL;
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> Engine_USBDevice;
    Core::ProxyType<RPC::CommunicatorClient> Client_USBDevice;

    TEST_LOG("Creating Engine_USBDevice");
    Engine_USBDevice = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    Client_USBDevice = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(Engine_USBDevice));

    TEST_LOG("Creating Engine_USBDevice Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    Engine_USBDevice->Announcements(mClient_USBDevice->Announcement());
#endif
    if (!Client_USBDevice.IsValid())
    {
        TEST_LOG("Invalid Client_USBDevice");
    }
    else
    {
        m_controller_usbdevice = Client_USBDevice->Open<PluginHost::IShell>(USBDEVICE_CALLSIGN, ~0, 3000);
        if (m_controller_usbdevice)
        {
        m_usbdeviceplugin = m_controller_usbdevice->QueryInterface<Exchange::IUSBDevice>();
        return_value = Core::ERROR_NONE;
        }
    }
    return return_value;
}

/**
 * @brief Compare two request status objects
 *
 * @param[in] data Expected value
 * @return true if the argument and data match, false otherwise
 */
MATCHER_P(MatchRequestStatus, data, "")
{
    bool match = true;
    std::string expected;
    std::string actual;

    data.ToString(expected);
    arg.ToString(actual);
    TEST_LOG(" rec = %s, arg = %s",expected.c_str(),actual.c_str());
    EXPECT_STREQ(expected.c_str(),actual.c_str());

    return match;
}

/* Test Case for verifying USB Device Plugged In and Plugged Out Notification.
  * Activating USB Device Plugin,
  * Setting Mocks for simulating the plugged in and plugged out events
  * Sending Plugged In event and Verify USBDevice struct with Valid device details
  * Deactivating the USB Device
*/
TEST_F(USBDeviceTest, devicePluggedInAndPluggedOut)
{
    Core::Sink<USBDeviceNotificationHandler> notification;
    uint32_t signalled = USBDevice_StateInvalid;
    Exchange::IUSBDevice::USBDevice actual_usbDevice = {0};

    TEST_LOG("** Test Case: devicePluggedInAndPluggedOut Started **");

    Mock_SetSerialNumberInUSBDevicePath();

    if (CreateUSBDeviceInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBDevice");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbdevice!= nullptr);
        if (m_controller_usbdevice)
        {
            ASSERT_TRUE(m_usbdeviceplugin!= nullptr);

            if (m_usbdeviceplugin)
            {
                std::string message;
                JsonObject expected_status;
                StrictMock<AsyncHandlerMock_USBDevice> async_handler;
                uint32_t return_value =  Core::ERROR_GENERAL;
                JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(USBDEVICE_CALLSIGN, USBDEVICEL2TEST_CALLSIGN);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedIn"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedIn(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedOut"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedOut(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                m_usbdeviceplugin->AddRef();
                m_usbdeviceplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedIn(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedIn, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedIn);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                /* HotPlug Dettach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedOut(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedOut, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedOut);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                m_usbdeviceplugin->Unregister(&notification);
                m_usbdeviceplugin->Release();
                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedIn"));
                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedOut"));
            }
            else
            {
                TEST_LOG("m_usbdeviceplugin is NULL");
            }
            m_controller_usbdevice->Release();
        }
        else
        {
            TEST_LOG("m_controller_usbdevice is NULL");
        }
    }
    TEST_LOG("** Test Case: devicePluggedInAndPluggedOut Ended **");
}

/* Test Case for getDeviceList Method with single mass storage USB
  * Activating USB Device Plugin,
  * Setting Mocks for simulating the plugged in event
  * Setting  Mocks to send the usb details like serial number, device address, port and bus numbers
  * Sending Plugged In event and Verify USBDevice struct with Valid device details
  * Calling the getDeviceList function and verify the Iterator and Release it
  * Deactivating the USB Device
*/
TEST_F(USBDeviceTest, getDeviceListUsingWithSingleMassStorageUSBComRpcConnectionSuccessCase)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBDeviceNotificationHandler> notification;
    uint32_t signalled = USBDevice_StateInvalid;
    Exchange::IUSBDevice::IUSBDeviceIterator* devices = nullptr;
    Exchange::IUSBDevice::USBDevice actual_usbDevice = {0};
    Exchange::IUSBDevice::USBDevice actual_usbDevice_dev_list = {0};

    TEST_LOG("** Test Case: getDeviceListUsingWithSingleMassStorageUSBComRpcConnectionSuccessCase Started **");

    Mock_SetSerialNumberInUSBDevicePath();

    if (CreateUSBDeviceInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBDevice");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbdevice!= nullptr);
        if (m_controller_usbdevice)
        {
            ASSERT_TRUE(m_usbdeviceplugin!= nullptr);

            if (m_usbdeviceplugin)
            {
                std::string message;
                JsonObject expected_status;
                StrictMock<AsyncHandlerMock_USBDevice> async_handler;
                uint32_t return_value =  Core::ERROR_GENERAL;
                JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(USBDEVICE_CALLSIGN, USBDEVICEL2TEST_CALLSIGN);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedIn"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedIn(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedOut"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedOut(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                m_usbdeviceplugin->AddRef();
                m_usbdeviceplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedIn(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedIn, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedIn);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                /* Mock for Device List, 1 Device */
                ON_CALL(*p_libUSBApiImplMock, libusb_get_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_context *ctx, libusb_device ***list) {
                    struct libusb_device **ret = nullptr;
                    ssize_t len = 1;

                    ret = (struct libusb_device **)malloc(len * sizeof(struct libusb_device *));

                    if (nullptr == ret)
                    {
                        std::cout << "malloc failed";
                        len = 0;
                    }
                    else
                    {
                        uint8_t bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                        uint8_t device_address = MOCK_USB_DEVICE_ADDRESS_1;
                        uint8_t port_number = MOCK_USB_DEVICE_PORT_1;

                        for (int index = 0; index < len; ++index)
                        {
                            ret[index] =  (struct libusb_device *)malloc(sizeof(struct libusb_device));

                            if (nullptr == ret[index])
                            {
                                std::cout << "malloc failed";
                                len = 0;
                            }
                            else
                            {
                                ret[index]->bus_number = bus_number;
                                ret[index]->device_address = device_address;
                                ret[index]->port_number = port_number;

                                bus_number += 1;
                                device_address += 1;
                                port_number += 1;
                            }
                        }
                        *list = ret;
                    }

                    return len;
                });

                ON_CALL(*p_libUSBApiImplMock, libusb_free_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_device **list, int unref_devices) {
                    for (int index = 0; index < 2; ++index)
                    {
                        free(list[index]);
                    }
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_device_descriptor(::testing::_, ::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev, struct libusb_device_descriptor *desc) {
                    desc->bDeviceSubClass = LIBUSB_CLASS_MASS_STORAGE;
                    desc->bDeviceClass = LIBUSB_CLASS_MASS_STORAGE;
                    return LIBUSB_SUCCESS;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_device_address(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->device_address;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_bus_number(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->bus_number;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_port_numbers(::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](libusb_device *dev, uint8_t *port_numbers, int port_numbers_len) {
                    if((nullptr != dev) && (nullptr != port_numbers))
                    {
                        port_numbers[0] = dev->port_number;
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                });

                /* Call GetDeviceList method */
                TEST_LOG("Getting DeviceList Values");
                status = m_usbdeviceplugin->GetDeviceList(devices);
                EXPECT_EQ(status,Core::ERROR_NONE);

                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                ASSERT_TRUE(devices != nullptr);
                ASSERT_TRUE(1 == devices->Count()); /* Verify device count */
                ASSERT_TRUE(devices->Next(actual_usbDevice_dev_list));
                /* Verify device details */
                ASSERT_EQ(actual_usbDevice_dev_list.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice_dev_list.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice_dev_list.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice_dev_list.devicePath, std::string("/dev/sda"));

                devices->Release();

                /* HotPlug Dettach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedOut(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedOut, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedOut);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                m_usbdeviceplugin->Unregister(&notification);
                m_usbdeviceplugin->Release();

                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedIn"));
                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedOut"));
            }
            else
            {
                TEST_LOG("m_usbdeviceplugin is NULL");
            }
            m_controller_usbdevice->Release();
        }
        else
        {
            TEST_LOG("m_controller_usbdevice is NULL");
        }
    }

    TEST_LOG("** Test Case: getDeviceListUsingWithSingleMassStorageUSBComRpcConnectionSuccessCase Ended **");
}

/* Test Case for getDeviceList Method with multiple mass storage USB
  * Activating USB Device Plugin,
  * Setting Mocks for simulating the plugged in event
  * Setting  Mocks to send the usb details like serial number, device address, port and bus numbers
  * Sending Plugged In event and Verify USBDevice struct with Valid device details
  * Calling the getDeviceList function and verify the Iterator and Release it
  * Deactivating the USB Device
*/
TEST_F(USBDeviceTest, getDeviceListUsingWithMultipleMassStorageUSBComRpcConnectionSuccessCase)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBDeviceNotificationHandler> notification;
    uint32_t signalled = USBDevice_StateInvalid;
    Exchange::IUSBDevice::IUSBDeviceIterator* devices = nullptr;
    Exchange::IUSBDevice::USBDevice actual_usbDevice = {0};
    Exchange::IUSBDevice::USBDevice actual_usbDevice_dev_list = {0};

    TEST_LOG("** Test Case: getDeviceListUsingWithMultipleMassStorageUSBComRpcConnectionSuccessCase Started **");

    Mock_SetSerialNumberInUSBDevicePath();

    if (CreateUSBDeviceInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBDevice");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbdevice!= nullptr);
        if (m_controller_usbdevice)
        {
            ASSERT_TRUE(m_usbdeviceplugin!= nullptr);

            if (m_usbdeviceplugin)
            {
                std::string message;

                m_usbdeviceplugin->AddRef();
                m_usbdeviceplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedIn, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedIn);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                /* HotPlug Attach Device 2 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_2, MOCK_USB_DEVICE_ADDRESS_2);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_2;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_2;
                dev.port_number = MOCK_USB_DEVICE_PORT_2;

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                Exchange::IUSBDevice::USBDevice actual_usbDevice2 = {0};
                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedIn, actual_usbDevice2);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedIn);

                ASSERT_EQ(actual_usbDevice2.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice2.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice2.deviceName, std::string("101/002"));
                ASSERT_EQ(actual_usbDevice2.devicePath, std::string("/dev/sdb"));

                /* Mock for Device List, 2 Devices */
                ON_CALL(*p_libUSBApiImplMock, libusb_get_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_context *ctx, libusb_device ***list) {
                    struct libusb_device **ret = nullptr;
                    ssize_t len = 2;

                    ret = (struct libusb_device **)malloc(len * sizeof(struct libusb_device *));

                    if (nullptr == ret)
                    {
                        std::cout << "malloc failed";
                        len = 0;
                    }
                    else
                    {
                        uint8_t bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                        uint8_t device_address = MOCK_USB_DEVICE_ADDRESS_1;
                        uint8_t port_number = MOCK_USB_DEVICE_PORT_1;

                        for (int index = 0; index < len; ++index)
                        {
                            ret[index] =  (struct libusb_device *)malloc(sizeof(struct libusb_device));

                            if (nullptr == ret[index])
                            {
                                std::cout << "malloc failed";
                                len = 0;
                            }
                            else
                            {
                                ret[index]->bus_number = bus_number;
                                ret[index]->device_address = device_address;
                                ret[index]->port_number = port_number;

                                bus_number += 1;
                                device_address += 1;
                                port_number += 1;
                            }
                        }
                        *list = ret;
                    }

                    return len;
                });

                ON_CALL(*p_libUSBApiImplMock, libusb_free_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_device **list, int unref_devices) {
                    for (int index = 0; index < 2; ++index)
                    {
                    free(list[index]);
                    }
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_device_descriptor(::testing::_, ::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev, struct libusb_device_descriptor *desc) {
                        desc->bDeviceSubClass = LIBUSB_CLASS_MASS_STORAGE;
                        desc->bDeviceClass = LIBUSB_CLASS_MASS_STORAGE;
                        return LIBUSB_SUCCESS;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_device_address(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->device_address;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_bus_number(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->bus_number;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_port_numbers(::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](libusb_device *dev, uint8_t *port_numbers, int port_numbers_len) {
                if((nullptr != dev) && (nullptr != port_numbers))
                {
                    port_numbers[0] = dev->port_number;
                    return 1;
                }
                else
                {
                    return 0;
                }
                });

                /* Call GetDeviceList method */
                TEST_LOG("Getting DeviceList Values");
                status = m_usbdeviceplugin->GetDeviceList(devices);
                EXPECT_EQ(status,Core::ERROR_NONE);

                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                ASSERT_TRUE(devices != nullptr);
                ASSERT_TRUE(2 == devices->Count()); /* Verify device count */

                /* Verify device 1 details */
                ASSERT_TRUE(devices->Next(actual_usbDevice_dev_list));
                ASSERT_EQ(actual_usbDevice_dev_list.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice_dev_list.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice_dev_list.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice_dev_list.devicePath, std::string("/dev/sda"));

                /* Verify device 2 details */
                ASSERT_TRUE(devices->Next(actual_usbDevice_dev_list));
                ASSERT_EQ(actual_usbDevice_dev_list.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice_dev_list.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice_dev_list.deviceName, std::string("101/002"));
                ASSERT_EQ(actual_usbDevice_dev_list.devicePath, std::string("/dev/sdb"));
                devices->Release();

                /* HotPlug Dettach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedOut, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedOut);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                /* HotPlug Deattach Device 2 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_2, MOCK_USB_DEVICE_ADDRESS_2);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_2;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_2;
                dev.port_number = MOCK_USB_DEVICE_PORT_2;

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedOut, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedOut);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("101/002"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sdb"));
                m_usbdeviceplugin->Unregister(&notification);
                m_usbdeviceplugin->Release();
            }
            else
            {
                TEST_LOG("m_usbdeviceplugin is NULL");
            }
            m_controller_usbdevice->Release();
        }
        else
        {
            TEST_LOG("m_controller_usbdevice is NULL");
        }
    }

    TEST_LOG("** Test Case: getDeviceListUsingWithMultipleMassStorageUSBComRpcConnectionSuccessCase Ended **");
}

/* Test Case for bindDriver Method
  * Activating USB Device Plugin,
  * Setting Mocks for simulating the plugged in event
  * Setting  Mocks to send the usb details like serial number, device address, port and bus numbers
  * Setting  Mocks to send the port and bus numbers, open device, attach kernl libusb functions
  * Sending Plugged In event and Verify USBDevice struct with Valid device details
  * Calling the bindDriver function with deviceName and verify whether it is sucessed
  * Deactivating the USB Device
*/
TEST_F(USBDeviceTest, bindDriverComRpcConnectionSuccessCase)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBDeviceNotificationHandler> notification;
    uint32_t signalled = USBDevice_StateInvalid;
    Exchange::IUSBDevice::USBDevice actual_usbDevice = {0};
    Exchange::IUSBDevice::USBDevice actual_usbDevice_dev_list = {0};

    TEST_LOG("** Test Case: bindDriverComRpcConnectionSuccessCase Started **");

    Mock_SetSerialNumberInUSBDevicePath();

    if (CreateUSBDeviceInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBDevice");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbdevice!= nullptr);
        if (m_controller_usbdevice)
        {
            ASSERT_TRUE(m_usbdeviceplugin!= nullptr);

            if (m_usbdeviceplugin)
            {
                std::string message;
                JsonObject expected_status;
                StrictMock<AsyncHandlerMock_USBDevice> async_handler;
                uint32_t return_value =  Core::ERROR_GENERAL;
                JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(USBDEVICE_CALLSIGN, USBDEVICEL2TEST_CALLSIGN);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedIn"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedIn(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedOut"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedOut(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                m_usbdeviceplugin->AddRef();
                m_usbdeviceplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedIn(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedIn, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedIn);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                /* Mock for Device List, 1 Device */
                ON_CALL(*p_libUSBApiImplMock, libusb_get_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_context *ctx, libusb_device ***list) {
                    struct libusb_device **ret = nullptr;
                    ssize_t len = 1;

                    ret = (struct libusb_device **)malloc(len * sizeof(struct libusb_device *));

                    if (nullptr == ret)
                    {
                        std::cout << "malloc failed";
                        len = 0;
                    }
                    else
                    {
                        uint8_t bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                        uint8_t device_address = MOCK_USB_DEVICE_ADDRESS_1;
                        uint8_t port_number = MOCK_USB_DEVICE_PORT_1;

                        for (int index = 0; index < len; ++index)
                        {
                            ret[index] =  (struct libusb_device *)malloc(sizeof(struct libusb_device));

                            if (nullptr == ret[index])
                            {
                                std::cout << "malloc failed";
                                len = 0;
                            }
                            else
                            {
                                ret[index]->bus_number = bus_number;
                                ret[index]->device_address = device_address;
                                ret[index]->port_number = port_number;

                                bus_number += 1;
                                device_address += 1;
                                port_number += 1;
                            }
                        }
                        *list = ret;
                    }

                    return len;
                });

                ON_CALL(*p_libUSBApiImplMock, libusb_free_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_device **list, int unref_devices) {
                    for (int index = 0; index < 2; ++index)
                    {
                        free(list[index]);
                    }
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_device_address(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->device_address;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_bus_number(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->bus_number;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_port_numbers(::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](libusb_device *dev, uint8_t *port_numbers, int port_numbers_len) {
                    if((nullptr != dev) && (nullptr != port_numbers))
                    {
                        port_numbers[0] = dev->port_number;
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                });

                /* Call bindDriver method */
                TEST_LOG("call BindDriver");
		   std::string deviceName = std::string("100/001");
                status = m_usbdeviceplugin->BindDriver(deviceName);
                EXPECT_EQ(status,Core::ERROR_NONE);

                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                /* HotPlug Dettach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedOut(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedOut, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedOut);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                m_usbdeviceplugin->Unregister(&notification);
                m_usbdeviceplugin->Release();

                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedIn"));
                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedOut"));
            }
            else
            {
                TEST_LOG("m_usbdeviceplugin is NULL");
            }
            m_controller_usbdevice->Release();
        }
        else
        {
            TEST_LOG("m_controller_usbdevice is NULL");
        }
    }

    TEST_LOG("** Test Case: bindDriverComRpcConnectionSuccessCase Ended **");
}

/* Test Case for UnbindDriver Method
  * Activating USB Device Plugin,
  * Setting Mocks for simulating the plugged in event
  * Setting  Mocks to send the usb details like serial number, device address, port and bus numbers
  * Setting  Mocks to send the port and bus numbers, open device, attach kernl libusb functions
  * Sending Plugged In event and Verify USBDevice struct with Valid device details
  * Calling the unbindDriver function with deviceName and verify whether it is sucessed
  * Deactivating the USB Device
*/
TEST_F(USBDeviceTest, unbindDriverComRpcConnectionSuccessCase)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBDeviceNotificationHandler> notification;
    uint32_t signalled = USBDevice_StateInvalid;
    Exchange::IUSBDevice::USBDevice actual_usbDevice = {0};
    Exchange::IUSBDevice::USBDevice actual_usbDevice_dev_list = {0};

    TEST_LOG("** Test Case: unbindDriverComRpcConnectionSuccessCase Started **");

    Mock_SetSerialNumberInUSBDevicePath();

    if (CreateUSBDeviceInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBDevice");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbdevice!= nullptr);
        if (m_controller_usbdevice)
        {
            ASSERT_TRUE(m_usbdeviceplugin!= nullptr);

            if (m_usbdeviceplugin)
            {
                std::string message;
                JsonObject expected_status;
                StrictMock<AsyncHandlerMock_USBDevice> async_handler;
                uint32_t return_value =  Core::ERROR_GENERAL;
                JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(USBDEVICE_CALLSIGN, USBDEVICEL2TEST_CALLSIGN);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedIn"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedIn(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedOut"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedOut(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                m_usbdeviceplugin->AddRef();
                m_usbdeviceplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedIn(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedIn, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedIn);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                /* Mock for Device List, 1 Device */
                ON_CALL(*p_libUSBApiImplMock, libusb_get_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_context *ctx, libusb_device ***list) {
                    struct libusb_device **ret = nullptr;
                    ssize_t len = 1;

                    ret = (struct libusb_device **)malloc(len * sizeof(struct libusb_device *));

                    if (nullptr == ret)
                    {
                        std::cout << "malloc failed";
                        len = 0;
                    }
                    else
                    {
                        uint8_t bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                        uint8_t device_address = MOCK_USB_DEVICE_ADDRESS_1;
                        uint8_t port_number = MOCK_USB_DEVICE_PORT_1;

                        for (int index = 0; index < len; ++index)
                        {
                            ret[index] =  (struct libusb_device *)malloc(sizeof(struct libusb_device));

                            if (nullptr == ret[index])
                            {
                                std::cout << "malloc failed";
                                len = 0;
                            }
                            else
                            {
                                ret[index]->bus_number = bus_number;
                                ret[index]->device_address = device_address;
                                ret[index]->port_number = port_number;

                                bus_number += 1;
                                device_address += 1;
                                port_number += 1;
                            }
                        }
                        *list = ret;
                    }

                    return len;
                });

                ON_CALL(*p_libUSBApiImplMock, libusb_free_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_device **list, int unref_devices) {
                    for (int index = 0; index < 2; ++index)
                    {
                        free(list[index]);
                    }
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_device_address(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->device_address;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_bus_number(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->bus_number;
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_port_numbers(::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](libusb_device *dev, uint8_t *port_numbers, int port_numbers_len) {
                    if((nullptr != dev) && (nullptr != port_numbers))
                    {
                        port_numbers[0] = dev->port_number;
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                });

                EXPECT_CALL(*p_libUSBApiImplMock, libusb_kernel_driver_active(::testing::_, ::testing::_))
                .WillOnce(
                []() {
                    return 1; /* Sending driver is active */
                });

                /* Call UnbindDriver method */
                TEST_LOG("call UnbindDriver");
		   std::string deviceName = std::string("100/001");
                status = m_usbdeviceplugin->UnbindDriver(deviceName);
                EXPECT_EQ(status,Core::ERROR_NONE);

                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                /* HotPlug Dettach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedOut(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedOut, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedOut);

                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));

                m_usbdeviceplugin->Unregister(&notification);
                m_usbdeviceplugin->Release();

                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedIn"));
                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedOut"));
            }
            else
            {
                TEST_LOG("m_usbdeviceplugin is NULL");
            }
            m_controller_usbdevice->Release();
        }
        else
        {
            TEST_LOG("m_controller_usbdevice is NULL");
        }
    }

    TEST_LOG("** Test Case: unbindDriverComRpcConnectionSuccessCase Ended **");
}
TEST_F(USBDeviceTest, getDeviceInfoComRpcConnectionSuccessCase)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBDeviceNotificationHandler> notification;
    uint32_t signalled = USBDevice_StateInvalid;
    Exchange::IUSBDevice::USBDevice actual_usbDevice = {0};
    struct libusb_config_descriptor *temp_config_desc = nullptr;
    TEST_LOG("** Test Case: getDeviceInfoComRpcConnectionSuccessCase Started **");
    Mock_SetSerialNumberInUSBDevicePath();
    if (CreateUSBDeviceInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBDevice");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbdevice!= nullptr);
        if (m_controller_usbdevice)
        {
            ASSERT_TRUE(m_usbdeviceplugin!= nullptr);
            if (m_usbdeviceplugin)
            {
                std::string message;
                JsonObject expected_status;
                StrictMock<AsyncHandlerMock_USBDevice> async_handler;
                uint32_t return_value =  Core::ERROR_GENERAL;
                JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(USBDEVICE_CALLSIGN, USBDEVICEL2TEST_CALLSIGN);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedIn"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedIn(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                return_value = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDevicePluggedOut"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDevicePluggedOut(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, return_value);

                m_usbdeviceplugin->AddRef();
                m_usbdeviceplugin->Register(&notification);
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);
                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;
                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedIn(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedIn, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedIn);
                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));
                ON_CALL(*p_libUSBApiImplMock, libusb_get_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_context *ctx, libusb_device ***list) {
                    struct libusb_device **ret = nullptr;
                    ssize_t len = 1;
                    ret = (struct libusb_device **)malloc(len * sizeof(struct libusb_device *));
                    if (nullptr == ret)
                    {
                        std::cout << "malloc failed";
                        len = 0;
                    }
                    else
                    {
                        uint8_t bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                        uint8_t device_address = MOCK_USB_DEVICE_ADDRESS_1;
                        uint8_t port_number = MOCK_USB_DEVICE_PORT_1;
                        for (int index = 0; index < len; ++index)
                        {
                            ret[index] =  (struct libusb_device *)malloc(sizeof(struct libusb_device));
                            if (nullptr == ret[index])
                            {
                                std::cout << "malloc failed";
                                len = 0;
                            }
                            else
                            {
                                ret[index]->bus_number = bus_number;
                                ret[index]->device_address = device_address;
                                ret[index]->port_number = port_number;
                                bus_number += 1;
                                device_address += 1;
                                port_number += 1;
                            }
                        }
                        *list = ret;
                    }
                    return len;
                });
                ON_CALL(*p_libUSBApiImplMock, libusb_free_device_list(::testing::_, ::testing::_))
                .WillByDefault(
                [](libusb_device **list, int unref_devices) {
                    for (int index = 0; index < 2; ++index)
                    {
                        free(list[index]);
                    }
                });
                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_device_descriptor(::testing::_, ::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev, struct libusb_device_descriptor *desc) {
                        desc->bDeviceSubClass = LIBUSB_CLASS_MASS_STORAGE;
                        desc->bDeviceClass = LIBUSB_CLASS_MASS_STORAGE;
                        desc->idVendor = 0x1234;
                        desc->idProduct = 0x5678;
                        desc->iManufacturer = 1;
                        desc->iProduct = 2;
                        desc->iSerialNumber = 3;
                        return LIBUSB_SUCCESS;
                });
                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_device_address(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->device_address;
                });
                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_bus_number(::testing::_))
                .WillRepeatedly(
                [](libusb_device *dev) {
                    return dev->bus_number;
                });
                EXPECT_CALL(*p_libUSBApiImplMock, libusb_get_port_numbers(::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](libusb_device *dev, uint8_t *port_numbers, int port_numbers_len) {
                    if((nullptr != dev) && (nullptr != port_numbers))
                    {
                        port_numbers[0] = dev->port_number;
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                });
                ON_CALL(*p_libUSBApiImplMock, libusb_get_active_config_descriptor(::testing::_, ::testing::_))
                .WillByDefault([&temp_config_desc](libusb_device* pDev, struct libusb_config_descriptor** config_desc) {
                    *config_desc =  (libusb_config_descriptor *)malloc(sizeof(libusb_config_descriptor));
                    if (nullptr == *config_desc)
                    {
                        std::cout << "malloc failed";
                        return (int)1;
                    }
                    else
                    {
                        temp_config_desc = *config_desc;
                        (*config_desc)->bmAttributes = LIBUSB_CONFIG_ATT_BUS_POWERED;
                    }
                    return (int)LIBUSB_SUCCESS;
                });
                ON_CALL(*p_libUSBApiImplMock, libusb_get_string_descriptor(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillByDefault([](libusb_device_handle *dev_handle,
                    uint8_t desc_index, uint16_t langid, unsigned char *data, int length) {
                    data[1] = LIBUSB_DT_STRING;
                    if ( desc_index == 0 )
                    {
                        data[0] = 4;
                        data[3] = 0x04;
                        data[2] = 0x09;
                    }
                    else if ( desc_index == 1 /* Manufacturer */ )
                    {
                        const char *buf = MOCK_USB_DEVICE_MANUFACTURER;
                        int buffer_len = strlen(buf) * 2,j = 0,index=2;
                        memset(&data[2],0,length-2);
                        while((data[index] = buf[j++]) != '\0')
                        {
                            index+=2;
                        }
                        data[0] = buffer_len+2;
                    }
                    else if ( desc_index == 2 /* ProductID */ )
                    {
                        const char *buf = MOCK_USB_DEVICE_PRODUCT;
                        int buffer_len = strlen(buf) * 2,j = 0,index=2;
                        memset(&data[2],0,length-2);
                        while((data[index] = buf[j++]) != '\0')
                        {
                            index+=2;
                        }
                        data[0] = buffer_len+2;
                    }
                    else if ( desc_index == 3 /* SerialNumber */ )
                    {
                        const char *buf = MOCK_USB_DEVICE_SERIAL_NO;
                        int buffer_len = strlen(buf) * 2,j = 0,index=2;
                        memset(&data[2],0,length-2);
                        while((data[index] = buf[j++]) != '\0')
                        {
                            index+=2;
                        }
                        data[0] = buffer_len+2;
                    }
                    return (int)data[0];
                });
                TEST_LOG("call GetDeviceInfo");
		   std::string deviceName = std::string("100/001");
		   Exchange::IUSBDevice::IUSBDeviceInfoIterator* deviceInfo = nullptr;
		   Exchange::IUSBDevice::USBDeviceInfo resultItem{};
                status = m_usbdeviceplugin->GetDeviceInfo(deviceName,deviceInfo);
                EXPECT_EQ(status,Core::ERROR_NONE);
                while (deviceInfo->Next(resultItem) == true)
                {
                    EXPECT_EQ(resultItem.deviceStatus,Exchange::IUSBDevice::USBDeviceStatus::DEVICE_STATUS_ACTIVE);
                    EXPECT_EQ(resultItem.numLanguageIds, 1);
                    EXPECT_EQ(resultItem.productInfo1.languageId, 0x0409);
                    EXPECT_TRUE(strcmp(resultItem.productInfo1.serialNumber.c_str(), MOCK_USB_DEVICE_SERIAL_NO)== 0);
                    EXPECT_TRUE(strcmp(resultItem.productInfo1.manufacturer.c_str(), MOCK_USB_DEVICE_MANUFACTURER)== 0);
                    EXPECT_TRUE(strcmp(resultItem.productInfo1.product.c_str(), MOCK_USB_DEVICE_PRODUCT)== 0);
                }
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                if (nullptr != temp_config_desc)
                {
                    free(temp_config_desc);
                }
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;
                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                message = "{\"device\":{\"deviceClass\":8,\"deviceSubclass\":8,\"deviceName\":\"100\\/001\",\"devicePath\":\"\\/dev\\/sda\"}}";
                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDevicePluggedOut(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBDevice_onDevicePluggedOut, actual_usbDevice);
                EXPECT_TRUE(signalled & USBDevice_onDevicePluggedOut);
                ASSERT_EQ(actual_usbDevice.deviceClass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceSubclass, LIBUSB_CLASS_MASS_STORAGE);
                ASSERT_EQ(actual_usbDevice.deviceName, std::string("100/001"));
                ASSERT_EQ(actual_usbDevice.devicePath, std::string("/dev/sda"));
                m_usbdeviceplugin->Unregister(&notification);
                m_usbdeviceplugin->Release();

                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedIn"));
                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDevicePluggedOut"));
            }
            else
            {
                TEST_LOG("m_usbdeviceplugin is NULL");
            }
            m_controller_usbdevice->Release();
        }
        else
        {
            TEST_LOG("m_controller_usbdevice is NULL");
        }
    }
    TEST_LOG("** Test Case: getDeviceInfoComRpcConnectionSuccessCase Ended **");
}
