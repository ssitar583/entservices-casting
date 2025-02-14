#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <interfaces/IFirmwareUpdate.h>
#include "HdmiCec.h"

#define TEST_LOG(x, ...) fprintf( stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

#define JSON_TIMEOUT   (1000)
#define FIRMWAREUPDATE_CALLSIGN  _T("org.rdk.FirmwareUpdate")
#define FIRMWAREUPDATEL2TEST_CALLSIGN _T("L2tests.1")

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::IFirmwareUpdate;


class AsyncHandlerMock
{
    public:
        AsyncHandlerMock()
        {
        }
};

class FirmwareUpdateTest : public L2TestMocks {
protected:
    virtual ~FirmwareUpdateTest() override;

    public:
    FirmwareUpdateTest();

};

FirmwareUpdateTest:: FirmwareUpdateTest():L2TestMocks()
{
        Core::JSONRPC::Message message;
        string response;
        uint32_t status = Core::ERROR_GENERAL;

         /* Activate plugin in constructor */
         status = ActivateService("org.rdk.FirmwareUpdate");
         EXPECT_EQ(Core::ERROR_NONE, status);
}

/**
 * @brief Destructor for SystemServices L2 test class
 */
FirmwareUpdateTest::~FirmwareUpdateTest()
{
    uint32_t status = Core::ERROR_GENERAL;

    status = DeactivateService("org.rdk.FirmwareUpdate");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

TEST_F(FirmwareUpdateTest,EmptyFirmwareFilepath)
{
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;

    //Case1 Empty firmwareFilepath
    params["firmwareFilepath"] = "";
    params["firmwareType"]     = "PCI";
    status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "updateFirmware", params, result);

    JsonObject params1;
    JsonObject result1;
    status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "getUpdateState", params1, result1);
    EXPECT_EQ(Core::ERROR_NONE, status);

    if (result1.HasLabel("state")) {
        EXPECT_STREQ("VALIDATION_FAILED", result1["state"].String().c_str());
    }
    if (result1.HasLabel("substate")) {
        EXPECT_STREQ("FIRMWARE_NOT_FOUND", result1["substate"].String().c_str());
    }
}

TEST_F(FirmwareUpdateTest,EmptyFirmwareType)
{
    uint32_t status = Core::ERROR_GENERAL;
    
    const char* filePath = "/tmp/ELTE11MWR_MIDDLEWARE_DEV_default_20241122145614.bin";
    std::ofstream file(filePath, std::ios::binary); 

    if (file.is_open()) {
        JsonObject params;
        JsonObject result;
        params["firmwareFilepath"] = "/tmp/ELTE11MWR_MIDDLEWARE_DEV_default_20241122145614.bin";
        params["firmwareType"]     = "";
        status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "updateFirmware", params, result);
    
        JsonObject params1;
        JsonObject result1;
        status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "getUpdateState", params1, result1);
        EXPECT_EQ(Core::ERROR_NONE, status);
    
        if (result1.HasLabel("state")) {
            EXPECT_STREQ("VALIDATION_FAILED", result1["state"].String().c_str());
        }
        if (result1.HasLabel("substate")) {
            EXPECT_STREQ("NOT_APPLICABLE", result1["substate"].String().c_str());
        }
    }
}

TEST_F(FirmwareUpdateTest,InvalidFirmwareType)
{
    uint32_t status = Core::ERROR_GENERAL;
    const char* filePath = "/tmp/ELTE11MWR_MIDDLEWARE_DEV_default_20241122145614.bin";
    std::ofstream file(filePath, std::ios::binary); 

    if (file.is_open()) {
        JsonObject params;
        JsonObject result;
        params["firmwareFilepath"] = "/tmp/ELTE11MWR_MIDDLEWARE_DEV_default_20241122145614.bin";
        params["firmwareType"]     = "ABC";
        status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "updateFirmware", params, result);

    
        JsonObject params1;
        JsonObject result1;
        status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "getUpdateState", params1, result1);
        EXPECT_EQ(Core::ERROR_NONE, status);
    
        if (result1.HasLabel("state")) {
            EXPECT_STREQ("VALIDATION_FAILED", result1["state"].String().c_str());
        }
        if (result1.HasLabel("substate")) {
            EXPECT_STREQ("NOT_APPLICABLE", result1["substate"].String().c_str());
        }
    }
}

TEST_F(FirmwareUpdateTest,FirmwareFilepath_not_exist)
{
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;

   //Case4 given firmwareFilepath not exist
    params["firmwareFilepath"] = "/tmp/ELTE11MWR_MIDDLEWARE_DEV_default_20241122145614.bin";
    params["firmwareType"]     = "PCI";
    const char* filePath = "/tmp/ELTE11MWR_MIDDLEWARE_DEV_default_20241122145614.bin";

    if (std::remove(filePath) == 0) {
        std::cout << "File removed successfully.\n";
        status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "updateFirmware", params, result);
 
        JsonObject params1;
        JsonObject result1;
        status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "getUpdateState", params1, result1);
        EXPECT_EQ(Core::ERROR_NONE, status);

        if (result1.HasLabel("state")) {
            EXPECT_STREQ("VALIDATION_FAILED", result1["state"].String().c_str());
        }
        if (result1.HasLabel("substate")) {
            EXPECT_STREQ("FIRMWARE_NOT_FOUND", result1["substate"].String().c_str());
        }
    }    
}

TEST_F(FirmwareUpdateTest,FirmwareFilepath_exist)
{
    const char* filePath = "/tmp/ELTE11MWR_MIDDLEWARE_DEV_default_20241122145614.bin";
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;

    // Create and open the file
    std::ofstream file(filePath, std::ios::binary); // Open in binary mode (if needed)

    if (file.is_open()) {
        //Case5 given firmwareFilepath is exist but not valid .bin.
        params["firmwareFilepath"] = "/tmp/ELTE11MWR_MIDDLEWARE_DEV_default_20241122145614.bin";
        params["firmwareType"]     = "PCI";
        status = InvokeServiceMethod("org.rdk.FirmwareUpdate", "updateFirmware", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

}

