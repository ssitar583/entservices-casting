#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <interfaces/IUSBDevice.h>
#include <interfaces/IUSBMassStorage.h>

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

#include <fstream>
#include <iostream>
#include <csignal>
#include <atomic>
#include <signal.h>
#include <elf.h>
#include <threads.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/vfs.h> 
#include <linux/fs.h>

//extern "C" int __real_open(const char* pathname, int flags,int mode=0);

typedef struct sigaction SigAction;
class AsyncHandlerMock_UsbMassStorage
{
    public:
        AsyncHandlerMock_UsbMassStorage()
        {
        }
        MOCK_METHOD(void, OnDeviceMounted, (const Core::JSON::Container& eventPayload));
        MOCK_METHOD(void, OnDeviceUnmounted,(const Core::JSON::Container& eventPayload));
};
void l1_DumpBacktrace()
{
    void* callstack[32];
    uint32_t entries = 0;
    entries = backtrace(callstack, (sizeof(callstack) / sizeof(callstack[0])));
    char** symbols = backtrace_symbols(callstack, entries);
    for (uint32_t i = 0; i < entries; i++)
    {
        char  buffer[1024];
        Dl_info info;
        if (dladdr(callstack[i], &info) && info.dli_sname)
        {
            char* demangled = NULL;
            int status = -1;
            if (info.dli_sname[0] == '_')
            {
                demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
            }
            snprintf(buffer, sizeof(buffer), "%-3d %*p %s + %zd\n", i, int(2 + sizeof(void*) * 2), callstack[i],
                status == 0 ? demangled : info.dli_sname == 0 ? symbols[i] : info.dli_sname,
                (char*)callstack[i] - (char*)info.dli_saddr);
            free(demangled);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "%-3d %*p %s\n", i, int(2 + sizeof(void*) * 2), callstack[i], symbols[i]);
        }
        TEST_LOG("%s", buffer);
    }
    free(symbols);
}
void l1_signal_segv(int signum, siginfo_t* info, void*ptr)
{
    TEST_LOG("Signal received %d", signum);
    l1_DumpBacktrace();
    exit(EXIT_FAILURE);
}
void register_l1_signalHandler()
{
    SigAction sa;
    memset(&sa, 0, sizeof(SigAction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = l1_signal_segv;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV,&sa,nullptr);
}

#define JSON_TIMEOUT   (45000)
#define MOUNT_PATH           "/tmp/media/usb"

#define USBDEVICE_CALLSIGN  _T("org.rdk.USBDevice")
#define USBDEVICEL2TEST_CALLSIGN _T("L2tests.1")

#define USBMASSSTORAGE_CALLSIGN  _T("org.rdk.UsbMassStorage")
#define USBMASSSTORAGEL2TEST_CALLSIGN _T("L2tests.1")


#define MOCK_USB_DEVICE_BUS_NUMBER_1    100
#define MOCK_USB_DEVICE_ADDRESS_1       001
#define MOCK_USB_DEVICE_PORT_1          123

#define MOCK_USB_DEVICE_BUS_NUMBER_2    101
#define MOCK_USB_DEVICE_ADDRESS_2       002
#define MOCK_USB_DEVICE_PORT_2          124

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::IUSBMassStorage;

typedef enum : uint32_t {
    USBMassStorage_OnDeviceMounted= 0x00000001,
    USBMassStorage_OnDeviceUnmounted= 0x00000002,
    USBMassStorage_StateInvalid = 0x00000000

}USBMassStorageL2test_async_events_t;

class USBMassStorageNotificationHandler : public Exchange::IUSBMassStorage::INotification {
    private:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled;

        std::vector<WPEFramework::Exchange::IUSBMassStorage::USBStorageMountInfo> m_usb_mass_storage;

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::IUSBMassStorage::INotification)
        END_INTERFACE_MAP

    public:
        USBMassStorageNotificationHandler(){}
        ~USBMassStorageNotificationHandler(){}

        void OnDeviceMounted(const Exchange::IUSBMassStorage::USBStorageDeviceInfo &deviceInfo ,Exchange::IUSBMassStorage::IUSBStorageMountInfoIterator* const mountPoints) override{
            TEST_LOG("onDeviceMounted event triggered ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("onDeviceMounted received deviceInfo.deviceName: %s deviceInfo.devicePath: %s", 
				deviceInfo.deviceName.c_str(),
				deviceInfo.devicePath.c_str());

            m_event_signalled |= USBMassStorage_OnDeviceMounted;

            if (mountPoints == nullptr) {
                TEST_LOG("MountPoints_1 is null");
                return;
            }

            TEST_LOG("Iterating over mount points %d", mountPoints->Count());

            Exchange::IUSBMassStorage::USBStorageMountInfo resultItem{};
            int count = 0;
            mountPoints->Reset(0);
            while (mountPoints->Next(resultItem)) {
                TEST_LOG("Next called."); 
                TEST_LOG("Partition Name: %s", resultItem.partitionName.c_str());
                TEST_LOG("Mount Flags: %d", resultItem.mountFlags);  
                TEST_LOG("Mount Path: %s", resultItem.mountPath.c_str());
                m_usb_mass_storage.push_back(resultItem);
                count ++;
                 
            }

            if (count == 0) {
                TEST_LOG("No mount points found.");
            }
        
            m_condition_variable.notify_one();
        }

        void OnDeviceUnmounted(const Exchange::IUSBMassStorage::USBStorageDeviceInfo &deviceInfo ,Exchange::IUSBMassStorage::IUSBStorageMountInfoIterator* const mountPoints) override{
            TEST_LOG("onDeviceUnmounted event triggered ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("onDeviceUnmounted received");
        
            /* Notify the requester thread. */
            m_event_signalled |= USBMassStorage_OnDeviceUnmounted;

            Exchange::IUSBMassStorage::USBStorageMountInfo resultItem{};
            mountPoints->Reset(0);
            while (mountPoints->Next(resultItem)) {
            }
            m_usb_mass_storage.push_back(resultItem);
            m_condition_variable.notify_one();
        }

        uint32_t WaitForRequestStatus(uint32_t timeout_ms, USBMassStorageL2test_async_events_t expected_status, Exchange::IUSBMassStorage::USBStorageMountInfo* actual_mountPoints)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto now = std::chrono::system_clock::now();
            std::chrono::milliseconds timeout(timeout_ms);
            uint32_t signalled = USBMassStorage_StateInvalid;
            while (!(expected_status & m_event_signalled))
            {
               if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
               {
                  TEST_LOG("Timeout waiting for request status event");
                  break;
               }
           }
 
           signalled = m_event_signalled;
           if (actual_mountPoints) {
            size_t count = m_usb_mass_storage.size();
            for (size_t i = 0; i < count; ++i) {
                actual_mountPoints[i] = m_usb_mass_storage[i];
            }
           }

           m_event_signalled = USBMassStorage_StateInvalid;
           return signalled;
        }
};

class USBMassStorageTest : public L2TestMocks {
protected:
    virtual ~USBMassStorageTest() override;

protected:
    libusb_hotplug_callback_fn libUSBHotPlugCbDeviceAttached = nullptr;
    libusb_hotplug_callback_fn libUSBHotPlugCbDeviceDetached = nullptr;

    public:
    USBMassStorageTest();

      uint32_t WaitForRequestStatus(uint32_t timeout_ms, USBMassStorageL2test_async_events_t expected_status);
      uint32_t CreateUSBMassStorageInterfaceObjectUsingComRPCConnection();
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
        PluginHost::IShell *m_controller_usbmassstorage;

        /** @brief Pointer to the IUSBMassStorage interface */
        Exchange::IUSBMassStorage *m_usbmassstorageplugin;
};

USBMassStorageTest:: USBMassStorageTest():L2TestMocks()
{
        Core::JSONRPC::Message message;
        string response;
        uint32_t status = Core::ERROR_GENERAL;

        TEST_LOG("Test Constructor");

        register_l1_signalHandler();

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

        TEST_LOG("Activate plugins");

         /* Activate plugin in constructor */
         status = ActivateService("org.rdk.USBDevice");
         EXPECT_EQ(Core::ERROR_NONE, status);

         status = ActivateService("org.rdk.UsbMassStorage");
         EXPECT_EQ(Core::ERROR_NONE, status);
}
         
USBMassStorageTest::~USBMassStorageTest()
{
    uint32_t status = Core::ERROR_GENERAL;

    TEST_LOG("Test Destructor");

    ON_CALL(*p_libUSBApiImplMock, libusb_hotplug_deregister_callback(::testing::_, ::testing::_))
    .WillByDefault(::testing::Invoke(
        [&](libusb_context *ctx, libusb_hotplug_callback_handle callback_handle) {
            // Check the callback handle to determine which callback to deregister
            if (callback_handle == 1) {
                libUSBHotPlugCbDeviceAttached = nullptr;  // Deregister the "device arrived" callback
            } else if (callback_handle == 2) {
                libUSBHotPlugCbDeviceDetached = nullptr;  // Deregister the "device left" callback
            }
            // No return value needed as function returns void
        }));

    TEST_LOG("DeActivate USBDevice plugin");

    status = DeactivateService("org.rdk.UsbMassStorage");
    EXPECT_EQ(Core::ERROR_NONE, status);

    status = DeactivateService("org.rdk.USBDevice");
    EXPECT_EQ(Core::ERROR_NONE, status);
    sleep(5);

}

uint32_t USBMassStorageTest::WaitForRequestStatus(uint32_t timeout_ms, USBMassStorageL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(timeout_ms);
    uint32_t signalled = USBMassStorage_StateInvalid;

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

void USBMassStorageTest::Mock_SetDeviceDesc(uint8_t bus_number, uint8_t device_address)
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

void USBMassStorageTest::Mock_SetSerialNumberInUSBDevicePath()
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

    std::string serialNumFileSda = "/dev/sda";
    std::ofstream serialNumOutFileSda(serialNumFileSda);

    if (!serialNumOutFileSda) {
        TEST_LOG("Error opening file for writing!");
    }
    serialNumOutFileSda << "B32FD507 100-123" << std::endl;
    serialNumOutFileSda.close();


    std::string serialNumFileSdb = "/dev/sdb";
    std::ofstream serialNumOutFileSdb(serialNumFileSdb);

    if (!serialNumOutFileSdb) {
        TEST_LOG("Error opening file for writing!");
    }
    serialNumOutFileSdb << "UEUIRCXT 101-124" << std::endl;
    serialNumOutFileSdb.close();
}

uint32_t USBMassStorageTest::CreateUSBMassStorageInterfaceObjectUsingComRPCConnection()
{
    uint32_t return_value =  Core::ERROR_GENERAL;
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> Engine_USBMassStorage;
    Core::ProxyType<RPC::CommunicatorClient> Client_USBMassStorage;

    TEST_LOG("Creating Engine_USBMassStorage");
    Engine_USBMassStorage = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    Client_USBMassStorage = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(Engine_USBMassStorage));

    TEST_LOG("Creating Engine_USBMassStorage Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    Engine_USBMassStorage->Announcements(mClient_USBMassStorage->Announcement());
#endif
    if (!Client_USBMassStorage.IsValid())
    {
        TEST_LOG("Invalid Client_USBMassStorage");
    }
    else
    {
        m_controller_usbmassstorage = Client_USBMassStorage->Open<PluginHost::IShell>(USBMASSSTORAGE_CALLSIGN, ~0, 3000);
        if (m_controller_usbmassstorage)
        {
        m_usbmassstorageplugin = m_controller_usbmassstorage->QueryInterface<Exchange::IUSBMassStorage>();
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

TEST_F(USBMassStorageTest, comrpc_getpartitionInfo_success)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBMassStorageNotificationHandler> notification;
    uint32_t signalled = USBMassStorage_StateInvalid;
    Exchange::IUSBMassStorage::IUSBStorageMountInfoIterator* mountPoints = {};
    WPEFramework::Exchange::IUSBMassStorage::USBStoragePartitionInfo partitionInfo = {};
    
    TEST_LOG("** Test Case Started **");

    Mock_SetSerialNumberInUSBDevicePath();

    if (CreateUSBMassStorageInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBMassStorage");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbmassstorage!= nullptr);
        if (m_controller_usbmassstorage)
        {
            ASSERT_TRUE(m_usbmassstorageplugin!= nullptr);

            if (m_usbmassstorageplugin)
            {
                std::string message;

                m_usbmassstorageplugin->AddRef();
                m_usbmassstorageplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                static int index = 1;

                EXPECT_CALL(*p_wrapsImplMock, stat(::testing::_, ::testing::_))
                    .WillRepeatedly([](const std::string& path, struct stat* info) {
                    std::string expectedPath = MOUNT_PATH + std::to_string(index);
                    TEST_LOG("expectedPath =%s, path =%s",expectedPath.c_str(), path.c_str());

                    if (path == expectedPath) {
                        index++;
                        return -1;
                    }
                    info->st_mode = S_IFDIR;
                    return 0;
                });
            

                EXPECT_CALL(*p_wrapsImplMock, mkdir(::testing::_, ::testing::_))
                .WillRepeatedly([](const char* path, mode_t mode) {
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, mount(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data) {
                return 0;
                });

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                const size_t maxMountPointsSize = 10;
                Exchange::IUSBMassStorage::USBStorageMountInfo* mountPointsArray = new Exchange::IUSBMassStorage::USBStorageMountInfo[maxMountPointsSize];


                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceMounted,mountPointsArray);

                EXPECT_TRUE(signalled & USBMassStorage_OnDeviceMounted);


                EXPECT_CALL(*p_wrapsImplMock, statfs(::testing::_, ::testing::_))
                    .WillRepeatedly([](const char* path, struct statfs* buf) -> int {
                        // if (buf != nullptr) {
                        //     buf->f_type = 0x565a;
                        //     return 0;
                        // }
                        return 0; 
                    });

                    EXPECT_CALL(*p_wrapsImplMock, statvfs(::testing::_, ::testing::_))
                    .WillRepeatedly([](const char* path, struct statvfs* stat) -> int {
                        if (stat != nullptr) {
                            stat->f_blocks = 1024000;
                            stat->f_frsize = 4096;
                            stat->f_bfree = 256000;
                            return 0;
                        }
                        return -1;
                    });

                    EXPECT_CALL(*p_wrapsImplMock, open(::testing::_, ::testing::_, ::testing::_))
                    .WillRepeatedly([](const char* pathname, int flags, mode_t mode) {
                    TEST_LOG("Inside open mock, calling real open.");
                    return 0;
                    });


                EXPECT_CALL(*p_wrapsImplMock, ioctl(::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](int fd, unsigned long request, void* argp) -> int {
                    if (request == BLKGETSIZE64) {
                        *(reinterpret_cast<uint64_t*>(argp)) = 50ULL * 1024 * 1024 *1024; 
                    }
                    else if (request == BLKGETSIZE) {
                        *(reinterpret_cast<unsigned long*>(argp)) = 1024;
                    }
                    else if (request == BLKSSZGET) {
                        *(reinterpret_cast<uint32_t*>(argp)) = 512; 
                    }
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, umount(::testing::_))
                .WillRepeatedly([](const char* path) {
                return 0;
                });

                   size_t i = 0;
                   while (!mountPointsArray[i].mountPath.empty()) {

                        status = m_usbmassstorageplugin->GetPartitionInfo(mountPointsArray[i].mountPath, partitionInfo);
                        EXPECT_EQ(status, Core::ERROR_NONE);

                        EXPECT_EQ(partitionInfo.fileSystem, WPEFramework::Exchange::IUSBMassStorage::USBStorageFileSystem::VFAT);
                        EXPECT_EQ(partitionInfo.size, 51200); 
                        EXPECT_EQ(partitionInfo.numSectors, 1024);
                        EXPECT_EQ(partitionInfo.sectorSize, 512);
                        EXPECT_EQ(partitionInfo.totalSpace, 4000);
                        EXPECT_EQ(partitionInfo.usedSpace, 3000);
                        EXPECT_EQ(partitionInfo.availableSpace, partitionInfo.totalSpace - partitionInfo.usedSpace);
                        ++i;
                }

                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceUnmounted, mountPointsArray);

                EXPECT_TRUE(signalled & USBMassStorage_OnDeviceUnmounted);
                delete[] mountPointsArray;
                m_usbmassstorageplugin->Unregister(&notification);
                m_usbmassstorageplugin->Release();
            }
            else
            {
                TEST_LOG("m_usbmassstorageplugin is NULL");
            }
            m_controller_usbmassstorage->Release();
        }
    }
    TEST_LOG("** Test Case:  Ended **");

}

TEST_F(USBMassStorageTest, comrpc_getDeviceListAndMount_success)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBMassStorageNotificationHandler> notification;
    uint32_t signalled = USBMassStorage_StateInvalid;
    Exchange::IUSBMassStorage::IUSBStorageMountInfoIterator* mountPoints = {};
    Exchange::IUSBMassStorage::IUSBStorageDeviceInfoIterator* deviceInfo = nullptr;
    std::string selectedDeviceName;

    TEST_LOG("** Test Case Started **");

    Mock_SetSerialNumberInUSBDevicePath();
    if (CreateUSBMassStorageInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBMassStorage");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbmassstorage!= nullptr);
        if (m_controller_usbmassstorage)
        {
            ASSERT_TRUE(m_usbmassstorageplugin!= nullptr);

            if (m_usbmassstorageplugin)
            {
                std::string message;

                m_usbmassstorageplugin->AddRef();
                m_usbmassstorageplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                static int index = 1;

                EXPECT_CALL(*p_wrapsImplMock, stat(::testing::_, ::testing::_))
                    .WillRepeatedly([](const std::string& path, struct stat* info) {
                    std::string expectedPath = MOUNT_PATH + std::to_string(index);
                    TEST_LOG("expectedPath =%s, path =%s",expectedPath.c_str(), path.c_str());

                    if (path == expectedPath) {
                        index++;
                        return -1;
                    }
                    info->st_mode = S_IFDIR;
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, mkdir(::testing::_, ::testing::_))
                .WillRepeatedly([](const char* path, mode_t mode) {
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, mount(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data) {
                return 0;
                });

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                const size_t maxMountPointsSize = 10;
                Exchange::IUSBMassStorage::USBStorageMountInfo* mountPointsArray = new Exchange::IUSBMassStorage::USBStorageMountInfo[maxMountPointsSize];


                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceMounted,mountPointsArray);

                EXPECT_TRUE(signalled & USBMassStorage_OnDeviceMounted);

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

                EXPECT_CALL(*p_wrapsImplMock, umount(::testing::_))
                .WillRepeatedly([](const char* path) {
                return 0;
                });

                const std::string expectedDevicePath = "/dev/sda";
                const std::string expectedDeviceName = "100/001";

                status = m_usbmassstorageplugin->GetDeviceList(deviceInfo);
                EXPECT_EQ(status, Core::ERROR_NONE);

                Exchange::IUSBMassStorage::USBStorageDeviceInfo currentDevice;
                while (deviceInfo->Next(currentDevice)) {
                    EXPECT_FALSE(currentDevice.devicePath.empty());
                    EXPECT_FALSE(currentDevice.deviceName.empty());

                    TEST_LOG("Device path: %s, Device name: %s", currentDevice.devicePath.c_str(),currentDevice.deviceName.c_str());
                    EXPECT_EQ(currentDevice.devicePath, expectedDevicePath);
                    EXPECT_EQ(currentDevice.deviceName, expectedDeviceName);
                    selectedDeviceName = currentDevice.deviceName;
                }
                status = m_usbmassstorageplugin->GetMountPoints(selectedDeviceName, mountPoints);
                EXPECT_EQ(status, Core::ERROR_NONE);

                IUSBMassStorage::USBStorageMountInfo mountInfo;
                while (mountPoints->Next(mountInfo)) {
                    TEST_LOG("Partition Name: %s", mountInfo.partitionName.c_str());
                    TEST_LOG("Mount Flags: %d", mountInfo.mountFlags);  
                    TEST_LOG("Mount Path: %s", mountInfo.mountPath.c_str());
                }

                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceUnmounted, mountPointsArray);

                EXPECT_TRUE(signalled & USBMassStorage_OnDeviceUnmounted);
                delete[] mountPointsArray;
                m_usbmassstorageplugin->Unregister(&notification);
                m_usbmassstorageplugin->Release();
            
            }
            else
            {
                TEST_LOG("m_usbmassstorageplugin is NULL");
            }
            m_controller_usbmassstorage->Release();
        
        }

    }


}



TEST_F(USBMassStorageTest, comrpc_mount_failure)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBMassStorageNotificationHandler> notification;
    uint32_t signalled = USBMassStorage_StateInvalid;
    Exchange::IUSBMassStorage::IUSBStorageDeviceInfoIterator* deviceInfo = nullptr;

    TEST_LOG("** Test Case Started **");

    Mock_SetSerialNumberInUSBDevicePath();

    if (CreateUSBMassStorageInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBMassStorage");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbmassstorage!= nullptr);
        if (m_controller_usbmassstorage)
        {
            ASSERT_TRUE(m_usbmassstorageplugin!= nullptr);

            if (m_usbmassstorageplugin)
            {
                std::string message;

                m_usbmassstorageplugin->AddRef();
                m_usbmassstorageplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                static int index = 1;

                EXPECT_CALL(*p_wrapsImplMock, stat(::testing::_, ::testing::_))
                    .WillRepeatedly([](const std::string& path, struct stat* info) {
                    std::string expectedPath = MOUNT_PATH + std::to_string(index);
                    TEST_LOG("expectedPath =%s, path =%s",expectedPath.c_str(), path.c_str());

                    if (path == expectedPath) {
                        index++;
                        return -1;
                    }
                    info->st_mode = S_IFDIR;
                    return 0;
                });
            

                EXPECT_CALL(*p_wrapsImplMock, mkdir(::testing::_, ::testing::_))
                .WillRepeatedly([](const char* path, mode_t mode) {
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, mount(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data) {
                return -1;
                });

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                const size_t maxMountPointsSize = 10;
                Exchange::IUSBMassStorage::USBStorageMountInfo* mountPointsArray = new Exchange::IUSBMassStorage::USBStorageMountInfo[maxMountPointsSize];


                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceMounted,mountPointsArray);

                EXPECT_FALSE(signalled & USBMassStorage_OnDeviceMounted);

                delete[] mountPointsArray;
                m_usbmassstorageplugin->Unregister(&notification);
                m_usbmassstorageplugin->Release();
            }
            else
            {
                TEST_LOG("m_usbmassstorageplugin is NULL");
            }
            m_controller_usbmassstorage->Release();
        }
    }
}

TEST_F(USBMassStorageTest, comrpc_open_failure)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBMassStorageNotificationHandler> notification;
    uint32_t signalled = USBMassStorage_StateInvalid;
    Exchange::IUSBMassStorage::IUSBStorageMountInfoIterator* mountPoints = {};
    WPEFramework::Exchange::IUSBMassStorage::USBStoragePartitionInfo partitionInfo = {};
    
    TEST_LOG("** Test Case Started **");

    Mock_SetSerialNumberInUSBDevicePath();

    if (CreateUSBMassStorageInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBMassStorage");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbmassstorage!= nullptr);
        if (m_controller_usbmassstorage)
        {
            ASSERT_TRUE(m_usbmassstorageplugin!= nullptr);

            if (m_usbmassstorageplugin)
            {
                std::string message;

                m_usbmassstorageplugin->AddRef();
                m_usbmassstorageplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                static int index = 1;

                EXPECT_CALL(*p_wrapsImplMock, stat(::testing::_, ::testing::_))
                    .WillRepeatedly([](const std::string& path, struct stat* info) {
                    std::string expectedPath = MOUNT_PATH + std::to_string(index);
                    TEST_LOG("expectedPath =%s, path =%s",expectedPath.c_str(), path.c_str());

                    if (path == expectedPath) {
                        index++;
                        return -1;
                    }
                    info->st_mode = S_IFDIR;
                    return 0;
                });
            

                EXPECT_CALL(*p_wrapsImplMock, mkdir(::testing::_, ::testing::_))
                .WillRepeatedly([](const char* path, mode_t mode) {
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, mount(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data) {
                return 0;
                });

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                const size_t maxMountPointsSize = 10;
                Exchange::IUSBMassStorage::USBStorageMountInfo* mountPointsArray = new Exchange::IUSBMassStorage::USBStorageMountInfo[maxMountPointsSize];

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceMounted,mountPointsArray);

                EXPECT_TRUE(signalled & USBMassStorage_OnDeviceMounted);

                    EXPECT_CALL(*p_wrapsImplMock, statfs(::testing::_, ::testing::_))
                    .WillRepeatedly([](const char* path, struct statfs* buf) -> int {
                        // if (buf != nullptr) {
                        //     buf->f_type = 0x565a;
                        //     return 0;
                        // }
                        return 0; 
                    });

                    EXPECT_CALL(*p_wrapsImplMock, statvfs(::testing::_, ::testing::_))
                    .WillRepeatedly([](const char* path, struct statvfs* stat) -> int {
                        if (stat != nullptr) {
                            stat->f_blocks = 1024000;
                            stat->f_frsize = 4096;
                            stat->f_bfree = 256000;
                            return 0;
                        }
                        return -1;
                    });

                    EXPECT_CALL(*p_wrapsImplMock, open(::testing::_, ::testing::_, ::testing::_))
                    .WillRepeatedly([](const char* pathname, int flags, mode_t mode) {
                    return -1;
                    });

                   size_t i =0;
                    status = m_usbmassstorageplugin->GetPartitionInfo(mountPointsArray[i].mountPath, partitionInfo);
                    EXPECT_NE(status, Core::ERROR_NONE);
                    delete[] mountPointsArray;
                    m_usbmassstorageplugin->Unregister(&notification);
                    m_usbmassstorageplugin->Release();
            
            }
            else
            {
                TEST_LOG("m_usbmassstorageplugin is NULL");
            }
            m_controller_usbmassstorage->Release();
        }
    }
    TEST_LOG("** Test Case:  Ended **");
}

TEST_F(USBMassStorageTest, comrpc_unmount_failure)
{
        uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBMassStorageNotificationHandler> notification;
    uint32_t signalled = USBMassStorage_StateInvalid;
    Exchange::IUSBMassStorage::IUSBStorageMountInfoIterator* mountPoints = {};
    Exchange::IUSBMassStorage::IUSBStorageDeviceInfoIterator* deviceInfo = nullptr;
    std::string selectedDeviceName;

    TEST_LOG("** Test Case Started **");

    Mock_SetSerialNumberInUSBDevicePath();
    if (CreateUSBMassStorageInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBMassStorage");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbmassstorage!= nullptr);
        if (m_controller_usbmassstorage)
        {
            ASSERT_TRUE(m_usbmassstorageplugin!= nullptr);

            if (m_usbmassstorageplugin)
            {
                std::string message;

                m_usbmassstorageplugin->AddRef();
                m_usbmassstorageplugin->Register(&notification);

                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                static int index = 1;

                EXPECT_CALL(*p_wrapsImplMock, stat(::testing::_, ::testing::_))
                    .WillRepeatedly([](const std::string& path, struct stat* info) {
                    std::string expectedPath = MOUNT_PATH + std::to_string(index);
                    TEST_LOG("expectedPath =%s, path =%s",expectedPath.c_str(), path.c_str());

                    if (path == expectedPath) {
                        index++;
                        return -1;
                    }
                    info->st_mode = S_IFDIR;
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, mkdir(::testing::_, ::testing::_))
                .WillRepeatedly([](const char* path, mode_t mode) {
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, mount(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data) {
                TEST_LOG("mount");
                return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, umount(::testing::_))
                .WillRepeatedly([](const char* path) {
                return -1;
                });

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                const size_t maxMountPointsSize = 10;
                Exchange::IUSBMassStorage::USBStorageMountInfo* mountPointsArray = new Exchange::IUSBMassStorage::USBStorageMountInfo[maxMountPointsSize];


                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceMounted,mountPointsArray);

                EXPECT_TRUE(signalled & USBMassStorage_OnDeviceMounted);

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

                const std::string expectedDevicePath = "/dev/sda";
                const std::string expectedDeviceName = "100/001";

                status = m_usbmassstorageplugin->GetDeviceList(deviceInfo);
                EXPECT_EQ(status, Core::ERROR_NONE);

                Exchange::IUSBMassStorage::USBStorageDeviceInfo currentDevice;
                while (deviceInfo->Next(currentDevice)) {
                    EXPECT_FALSE(currentDevice.devicePath.empty());
                    EXPECT_FALSE(currentDevice.deviceName.empty());

                    TEST_LOG("Device path: %s, Device name: %s", currentDevice.devicePath.c_str(),currentDevice.deviceName.c_str());
                    EXPECT_EQ(currentDevice.devicePath, expectedDevicePath);
                    EXPECT_EQ(currentDevice.deviceName, expectedDeviceName);
                    selectedDeviceName = currentDevice.deviceName;
                }

                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceUnmounted, mountPointsArray);

                EXPECT_FALSE(signalled & USBMassStorage_OnDeviceUnmounted);

                delete[] mountPointsArray;
                m_usbmassstorageplugin->Unregister(&notification);
                m_usbmassstorageplugin->Release();
            }
            else
            {
                TEST_LOG("m_usbmassstorageplugin is NULL");
            }
            m_controller_usbmassstorage->Release();
        }
    }

}

TEST_F(USBMassStorageTest, comrpc_Multiple_getDeviceListAndMount_success)
{
    uint32_t status = Core::ERROR_GENERAL;
    Core::Sink<USBMassStorageNotificationHandler> notification;
    uint32_t signalled = USBMassStorage_StateInvalid;
    Exchange::IUSBMassStorage::IUSBStorageMountInfoIterator* mountPoints = {};
    Exchange::IUSBMassStorage::IUSBStorageDeviceInfoIterator* deviceInfo = nullptr;
    std::string selectedDeviceName;

    TEST_LOG("** Test Case: getDeviceListUsingWithMultipleMassStorageUSBComRpcConnectionSuccessCase Started **");

    Mock_SetSerialNumberInUSBDevicePath();
    if (CreateUSBMassStorageInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid Client_USBMassStorage");
    }
    else
    {
        ASSERT_TRUE(m_controller_usbmassstorage!= nullptr);
        if (m_controller_usbmassstorage)
        {
            ASSERT_TRUE(m_usbmassstorageplugin!= nullptr);

            if (m_usbmassstorageplugin)
            {
                std::string message;

                StrictMock<AsyncHandlerMock_UsbMassStorage> async_handler;
                JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(USBMASSSTORAGE_CALLSIGN, USBMASSSTORAGEL2TEST_CALLSIGN);
                JsonObject expected_status;

                status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDeviceMounted"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDeviceMounted(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, status);

                status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                                       _T("onDeviceUnmounted"),
                                                       [this, &async_handler](const Core::JSON::Container &eventPayload) {
                                                       async_handler.OnDeviceUnmounted(eventPayload);
                                       });
                EXPECT_EQ(Core::ERROR_NONE, status);

                m_usbmassstorageplugin->AddRef();
                m_usbmassstorageplugin->Register(&notification);


                /* HotPlug Attach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                libusb_device dev = {0};
                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);

                static int index = 1;

                EXPECT_CALL(*p_wrapsImplMock, stat(::testing::_, ::testing::_))
                    .WillRepeatedly([](const std::string& path, struct stat* info) {
                    std::string expectedPath = MOUNT_PATH + std::to_string(index);
                    TEST_LOG("expectedPath =%s, path =%s",expectedPath.c_str(), path.c_str());

                    if (path == expectedPath) {
                        index++; 
                        return -1; 
                    }
                    info->st_mode = S_IFDIR;
                    return 0; 
                });
            

                EXPECT_CALL(*p_wrapsImplMock, mkdir(::testing::_, ::testing::_))
                .WillRepeatedly([](const char* path, mode_t mode) {
                    return 0;
                });

                 EXPECT_CALL(*p_wrapsImplMock, mount(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data) {
                TEST_LOG("mount");
                return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, statfs(::testing::_, ::testing::_))
                .WillRepeatedly([](const char* path, struct statfs* buf) -> int {
                        // if (buf != nullptr) {
                        //     buf->f_type = 0x565a;
                        //     return 0;
                        // }
                        return 0; 
                    });

                    EXPECT_CALL(*p_wrapsImplMock, statvfs(::testing::_, ::testing::_))
                    .WillRepeatedly([](const char* path, struct statvfs* stat) -> int {
                        if (stat != nullptr) {
                            stat->f_blocks = 1024000;
                            stat->f_frsize = 4096;
                            stat->f_bfree = 256000;
                            return 0;
                        }
                        return -1;
                    });

                    EXPECT_CALL(*p_wrapsImplMock, open(::testing::_, ::testing::_, ::testing::_))
                    .WillRepeatedly([](const char* pathname, int flags, mode_t mode) {
                    TEST_LOG("Inside open mock, calling real open.");
                    return 0;
                    });


                EXPECT_CALL(*p_wrapsImplMock, ioctl(::testing::_, ::testing::_, ::testing::_))
                .WillRepeatedly([](int fd, unsigned long request, void* argp) -> int {
                    if (request == BLKGETSIZE64) {
                        *(reinterpret_cast<uint64_t*>(argp)) = 50ULL * 1024 * 1024 *1024; 
                    }
                    else if (request == BLKGETSIZE) {
                        *(reinterpret_cast<unsigned long*>(argp)) = 1024;
                    }
                    else if (request == BLKSSZGET) {
                        *(reinterpret_cast<uint32_t*>(argp)) = 512; 
                    }
                    return 0;
                });

                EXPECT_CALL(*p_wrapsImplMock, umount(::testing::_))
                .WillRepeatedly([](const char* path) {
                return 0;
                });

                const size_t maxMountPointsSize = 10;
                Exchange::IUSBMassStorage::USBStorageMountInfo* mountPointsArray = new Exchange::IUSBMassStorage::USBStorageMountInfo[maxMountPointsSize];

                message = "{\"mountPoints\":[{\"partitionName\":\"\\/dev\\/sda1\",\"mountFlags\":\"READ_WRITE\",\"mountPath\":\"\\/tmp\\/media\\/usb1\",\"fileSystem\":\"VFAT\"},{\"partitionName\":\"\\/dev\\/sda2\",\"mountFlags\":\"READ_WRITE\",\"mountPath\":\"\\/tmp\\/media\\/usb2\",\"fileSystem\":\"VFAT\"}],\"deviceinfo\":{\"devicePath\":\"\\/dev\\/sda\",\"deviceName\":\"100\\/001\"}}";

                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDeviceMounted(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceMounted,mountPointsArray);
                EXPECT_TRUE(signalled & USBMassStorage_OnDeviceMounted);


                /* HotPlug Attach Device 2 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_2, MOCK_USB_DEVICE_ADDRESS_2);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_2;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_2;
                dev.port_number = MOCK_USB_DEVICE_PORT_2;

                libUSBHotPlugCbDeviceAttached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0);


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

                status = m_usbmassstorageplugin->GetDeviceList(deviceInfo);
                EXPECT_EQ(status, Core::ERROR_NONE);

                std::vector<std::string> deviceNames;
                Exchange::IUSBMassStorage::USBStorageDeviceInfo currentDevice;
                while (deviceInfo->Next(currentDevice)) {
                    EXPECT_FALSE(currentDevice.devicePath.empty());
                    EXPECT_FALSE(currentDevice.deviceName.empty());
                    TEST_LOG("Device path: %s, Device name: %s", currentDevice.devicePath.c_str(), currentDevice.deviceName.c_str());
                    deviceNames.push_back(currentDevice.deviceName);
                }

                for (const auto& deviceName : deviceNames) {
                    status = m_usbmassstorageplugin->GetMountPoints(deviceName, mountPoints);
                    // if there are no partitions then no need to check getMountPoints.
                    if (mountPoints == nullptr) {
                        EXPECT_EQ(status, Core::ERROR_GENERAL);
                    }
                    else
                    {
                        EXPECT_EQ(status, Core::ERROR_NONE);
                        IUSBMassStorage::USBStorageMountInfo mountInfo;
                        while (mountPoints->Next(mountInfo)) {
                            TEST_LOG("Partition Name: %s", mountInfo.partitionName.c_str());
                            TEST_LOG("Mount Flags: %d", mountInfo.mountFlags);
                            TEST_LOG("Mount Path: %s", mountInfo.mountPath.c_str());

                            Exchange::IUSBMassStorage::USBStoragePartitionInfo partitionInfo;
                            status = m_usbmassstorageplugin->GetPartitionInfo(mountInfo.mountPath, partitionInfo);
                            EXPECT_EQ(status, Core::ERROR_NONE);

                            TEST_LOG("Partition Info: FileSystem=%d, Size=%lu, NumSectors=%lu, SectorSize=%lu, TotalSpace=%lu, UsedSpace=%lu, AvailableSpace=%lu",
                            partitionInfo.fileSystem, partitionInfo.size, partitionInfo.numSectors,
                            partitionInfo.sectorSize, partitionInfo.totalSpace,
                            partitionInfo.usedSpace, partitionInfo.availableSpace);
                        }
                    }
                }

                 /* HotPlug Dettach Device 1 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_1, MOCK_USB_DEVICE_ADDRESS_1);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_1;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_1;
                dev.port_number = MOCK_USB_DEVICE_PORT_1;

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                message = "{\"mountPoints\":[{\"partitionName\":\"\\/dev\\/sda1\",\"mountFlags\":\"READ_WRITE\",\"mountPath\":\"\\/tmp\\/media\\/usb1\",\"fileSystem\":\"VFAT\"},{\"partitionName\":\"\\/dev\\/sda2\",\"mountFlags\":\"READ_WRITE\",\"mountPath\":\"\\/tmp\\/media\\/usb2\",\"fileSystem\":\"VFAT\"}],\"deviceinfo\":{\"devicePath\":\"\\/dev\\/sda\",\"deviceName\":\"100\\/001\"}}";

                expected_status.FromString(message);
                EXPECT_CALL(async_handler, OnDeviceUnmounted(MatchRequestStatus(expected_status)));

                signalled = notification.WaitForRequestStatus(JSON_TIMEOUT, USBMassStorage_OnDeviceUnmounted, mountPointsArray);

                EXPECT_TRUE(signalled & USBMassStorage_OnDeviceUnmounted);

                /* HotPlug Deattach Device 2 Verification */
                Mock_SetDeviceDesc(MOCK_USB_DEVICE_BUS_NUMBER_2, MOCK_USB_DEVICE_ADDRESS_2);

                dev.bus_number = MOCK_USB_DEVICE_BUS_NUMBER_2;
                dev.device_address = MOCK_USB_DEVICE_ADDRESS_2;
                dev.port_number = MOCK_USB_DEVICE_PORT_2;

                libUSBHotPlugCbDeviceDetached(nullptr, &dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0);

                delete[] mountPointsArray;
                m_usbmassstorageplugin->Unregister(&notification);
                m_usbmassstorageplugin->Release();
            
                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDeviceMounted"));
                jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onDeviceUnmounted"));
            }
            else
            {
                TEST_LOG("m_usbmassstorageplugin is NULL");
            }
            m_controller_usbmassstorage->Release();
        
        }

    }

}

