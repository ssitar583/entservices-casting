
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <fstream>
#include "devicesettings.h"
#include "FrontPanelIndicatorMock.h"

#include <mutex>
#include <condition_variable>
 
 
#define JSON_TIMEOUT   (1000)
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);
#define DISPLAYSETTINGS_CALLSIGN  _T("org.rdk.DisplaySettings.1")
#define DISPLAYSETTINGSL2TEST_CALLSIGN _T("L2tests.1")
using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;


class DisplaySettings_L2test : public L2TestMocks {
protected:
    Core::JSONRPC::Message message;
    string response;
    // testing::NiceMock<FrontPanelIndicatorMock> frontPanelIndicatorMock;
    // IARM_EventHandler_t checkAtmosCapsEventHandler = nullptr;
    // IARM_EventHandler_t DisplResolutionHandler = nullptr;
    // IARM_EventHandler_t dsHdmiEventHandler = nullptr;
    // IARM_EventHandler_t formatUpdateEventHandler = nullptr;
    // IARM_EventHandler_t powerEventHandler = nullptr;
    // IARM_EventHandler_t audioPortStateEventHandler = nullptr;
    // IARM_EventHandler_t dsSettingsChangeEventHandler = nullptr;
    // IARM_EventHandler_t ResolutionPostChange = nullptr;

    virtual ~DisplaySettings_L2test() override;
        
    public:
        DisplaySettings_L2test();

};
 
/**
* @brief Constructor for DisplaySettings L2 test class
*/
DisplaySettings_L2test::DisplaySettings_L2test()
        : L2TestMocks()
{
    
        printf("DISPLAYSETTINGS Constructor\n");
        uint32_t status = Core::ERROR_GENERAL;

         /* Activate plugin in constructor */
         status = ActivateService("org.rdk.DisplaySettings");
         EXPECT_EQ(Core::ERROR_NONE, status);
 
}
 
/**
* @brief Destructor for DisplaySettings L2 test class
*/
DisplaySettings_L2test::~DisplaySettings_L2test()
{
    printf("DISPLAYSETTINGS Destructor\n");
    uint32_t status = Core::ERROR_GENERAL;

    status = DeactivateService("org.rdk.DisplaySettings");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

TEST_F(DisplaySettings_L2test, DisplaySettings_L2_MethodTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DISPLAYSETTINGS_CALLSIGN, DISPLAYSETTINGSL2TEST_CALLSIGN);
    JsonObject result, params;

    string videoPort(_T("HDMI0"));
    string audioPort(_T("HDMI0"));

    device::VideoOutputPort videoOutputPort;
    device::VideoDevice videoDevice;
    device::VideoResolution videoResolution;
    device::VideoOutputPortType videoOutputPortType;
    device::VideoDFC actualVideoDFC;
    string videoDFCName(_T("FULL"));
    string videoPortSupportedResolution(_T("1080p"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));

    /*********************DisplaySettings Calls - Start*********************************************/
    device::AudioOutputPort audioFormat;
    ON_CALL(*p_hostImplMock, getCurrentAudioFormat(testing::_))
    .WillByDefault(testing::Invoke(
        [](dsAudioFormat_t audioFormat) {
            audioFormat=dsAUDIO_FORMAT_NONE;
            return 0;
            }));

    // ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
    //     .WillByDefault(::testing::Return(videoPort));

    ON_CALL(*p_videoOutputPortMock, getDefaultResolution())
        .WillByDefault(::testing::ReturnRef(videoResolution));

    ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));

    // ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
    //     .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));

    ON_CALL(*p_videoOutputPortMock, getName())
        .WillByDefault(::testing::ReturnRef(videoPort));

    ON_CALL(*p_audioOutputPortMock, getName())
        .WillByDefault(::testing::ReturnRef(audioPort));

    ON_CALL(*p_videoOutputPortMock, getVideoEOTF())
        .WillByDefault(::testing::Return(1));

    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>({ videoDevice })));

    ON_CALL(*p_videoDeviceMock, getHDRCapabilities(testing::_))
        .WillByDefault([](int* capabilities) {
            if (capabilities) {
                *capabilities = dsHDRSTANDARD_TechnicolorPrime;
            }
        });

    ON_CALL(*p_videoOutputPortMock, getColorDepthCapabilities(testing::_))
        .WillByDefault(testing::Invoke(
            [&](unsigned int* capabilities) {
            *capabilities = dsDISPLAY_COLORDEPTH_8BIT;
        }));

    ON_CALL(*p_videoOutputPortMock, setResolution(testing::_, testing::_, testing::_))
    .WillByDefault(testing::Invoke(
        [&](std::string resolution, bool persist, bool isIgnoreEdid) {
                        printf("Inside setResolution Mock\n");
                        EXPECT_EQ(resolution, "1080p60");
                        EXPECT_EQ(persist, true);

        std::cout << "Resolution: " << resolution 
                << ", Persist: " << persist 
                << ", Ignore EDID: " << isIgnoreEdid << std::endl;
    }));

    ON_CALL(*p_videoOutputPortMock, getCurrentOutputSettings(testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillByDefault([](int& videoEOTF, int& matrixCoefficients, int& colorSpace, 
                        int& colorDepth, int& quantizationRange) {
            videoEOTF = 1;  // example values
            matrixCoefficients = 0;
            colorSpace = 3;
            colorDepth = 10;
            quantizationRange = 4;

            return true;
        });

    ON_CALL(*p_videoOutputPortConfigImplMock, getPortType(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPortType));
    ON_CALL(*p_videoOutputPortMock, getType())
        .WillByDefault(::testing::ReturnRef(videoOutputPortType));
    ON_CALL(*p_videoOutputPortTypeMock, getId())
        .WillByDefault(::testing::Return(0));
    // device::VideoResolution videoResolution;
    ON_CALL(*p_videoOutputPortTypeMock, getSupportedResolutions())
        .WillByDefault(::testing::Return(device::List<device::VideoResolution>({ videoResolution })));
    ON_CALL(*p_videoResolutionMock, getName())
        .WillByDefault(::testing::ReturnRef(videoPortSupportedResolution));

    ON_CALL(*p_videoOutputPortMock, setForceHDRMode(::testing::_))
        .WillByDefault(::testing::Return(true));

    ON_CALL(*p_videoOutputPortMock, getResolution())
    .WillByDefault([]() -> const device::VideoResolution& {
        static device::VideoResolution dynamicResolution;
        return dynamicResolution;
    });

    ON_CALL(*p_videoDeviceMock, getDFC())
        .WillByDefault(::testing::Invoke([&]() -> device::VideoDFC& {
            return actualVideoDFC;
        }));

    ON_CALL(*p_videoDFCMock, getName())
    .WillByDefault(::testing::ReturnRef(videoDFCName));

    ON_CALL(*p_videoOutputPortMock, getTVHDRCapabilities(testing::_))
        .WillByDefault(testing::Invoke(
            [&](int* capabilities) {
            *capabilities = dsHDRSTANDARD_HLG | dsHDRSTANDARD_HDR10;
        }));
    /*********************DisplaySettings Calls - End*********************************************/
    /**************getCurrentResolution********************/

    {
        JsonObject result, params;
        uint32_t status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getCurrentResolution", params, result);
    }
    /************************getAudioFormat***************************/
    uint32_t status = Core::ERROR_NONE;

    {
        JsonObject result, params;
        uint32_t status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getAudioFormat", params, result);
    }
    /************************getVideoFormat***************************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getVideoFormat", params, result);
    }

    /****************getColorDepthCapabilities***************/
    
    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getColorDepthCapabilities", params, result);
    }

    /****************setCurrentResolution***************/

    {
        JsonObject params2, result;
        params2["videoDisplay"] = "HDMI0";
        params2["resolution"] = "1080p60";
        params2["persist"] = true;
        params2["ignoreEdid"] = false;

        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "setCurrentResolution", params2, result);
    }


    /****************getCurrentOutputSettings***************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getCurrentOutputSettings", params, result);
    }

    /**************getSupportedResolutions********************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getSupportedResolutions", params, result);
    }

    /****************setForceHDRMode***************/

    {
        JsonObject result, params;
        params["hdr_mode"] = "DV";
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "setForceHDRMode", params, result);
    }

    /**************getCurrentResolution********************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getCurrentResolution", params, result);
    }

   /************************getZoomSetting***************************/
 
    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getZoomSetting", params, result);
    }

     /************************getTVHDRCapabilities***************************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getTVHDRCapabilities", params, result);
    }

    /************************getSinkAtmosCapability***************************/
    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getSinkAtmosCapability", params, result);
    }

}