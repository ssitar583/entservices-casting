{
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
        ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
            .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
        ON_CALL(*p_videoOutputPortMock, getHDCPStatus())
            .WillByDefault(::testing::Return(dsHDCP_STATUS_AUTHENTICATED));
        ON_CALL(*p_videoOutputPortMock, isContentProtected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getHDCPReceiverProtocol())
            .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
        ON_CALL(*p_videoOutputPortMock, getHDCPCurrentProtocol())
            .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
            .WillByDefault(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                if(string(ownerName) == string(_T(IARM_BUS_NM_SRV_MGR_NAME)))
                {
                    if (string(methodName) == string(_T(IARM_BUS_NETSRVMGR_API_getInterfaceList))) {
                        auto param = static_cast<IARM_BUS_NetSrvMgr_InterfaceList_t *>(arg);
                        param->size = 2;
                        memcpy(&param->interfaces[0].name, "eth0", sizeof("eth0"));
                        memcpy(&param->interfaces[0].mac, "AA:AA:AA:AA:AA:AA", sizeof("AA:AA:AA:AA:AA:AA"));
                        param->interfaces[0].flags = 69699;
                        memcpy(&param->interfaces[1].name, "wlan0", sizeof("wlan0"));
                        memcpy(&param->interfaces[1].mac, "AA:AA:AA:AA:AA:AA", sizeof("AA:AA:AA:AA:AA:AA"));
                        param->interfaces[1].flags = 69699;
                    }
                    else if (string(methodName) == string(_T(IARM_BUS_NETSRVMGR_API_isConnectedToInternet))) {
                        *((bool*) arg) = true;
                        EXPECT_EQ(*((bool*) arg), true);
                    }
                    else if (string(methodName) == string(_T(IARM_BUS_NETSRVMGR_API_getIPSettings))) {
                        auto param = static_cast<IARM_BUS_NetSrvMgr_Iface_Settings_t  *>(arg);
                        memcpy(&param->interface, "ETHERNET", sizeof("ETHERNET"));
                        memcpy(&param->ipversion, "IPV4", sizeof("IPV4"));
                        param->autoconfig  = true;
                        memcpy(&param->ipaddress, "192.168.1.101", sizeof("192.168.1.101"));
                        memcpy(&param->netmask, "255.255.255.0", sizeof("255.255.255.0"));
                        memcpy(&param->gateway, "192.168.1.1", sizeof("192.168.1.1"));
                        memcpy(&param->primarydns, "192.168.1.1", sizeof("192.168.1.1"));
                        memcpy(&param->secondarydns, "192.168.1.2", sizeof("192.168.1.2"));
                        return IARM_RESULT_SUCCESS;
                    }
                }
                return IARM_RESULT_SUCCESS;
            });

        ON_CALL(*p_wrapsImplMock, v_secure_popen(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
            [&](const char* direction, const char* command, va_list args) {
                va_list args2;
                va_copy(args2, args);
                char strFmt[256];
                vsnprintf(strFmt, sizeof(strFmt), command, args2);
                va_end(args2);
                if(string(strFmt) == string(_T("/lib/rdk/getDeviceDetails.sh read estb_mac")))
                {
                    const string deviceInfoScript = _T("/lib/rdk/getDeviceDetails.sh");
                    const uint8_t deviceInfoContent[] = "#\\!/bin/bash\n\necho \"12:AB:CD:34:EF:5C\"";
                    //Create fake device info script
                    Core::File file(deviceInfoScript);
                    if(file.Exists())
                    {
                        std::cout << "File exists1\n";
                        file.Destroy();
                    }
                    file.Create();
                    file.Write(deviceInfoContent, sizeof(deviceInfoContent));
                    file.Close();
                }
                else if(string(strFmt) == string(_T("/lib/rdk/getDeviceDetails.sh read ")))
                {
                    const string deviceInfoScript = _T("/lib/rdk/getDeviceDetails.sh");
                    const uint8_t deviceInfoContent[] = "echo \"bluetooth_mac=D4:52:EE:32:A3:B2\n"
                                                        "boxIP=192.168.1.0\n"
                                                        "build_type=VBN\n"
                                                        "estb_mac=D4:52:EE:32:A3:B0\n"
                                                        "eth_mac=D4:52:EE:32:A3:B0\n"
                                                        "friendly_id=IP061-ec\n"
                                                        "imageVersion=CUSTOM7_VBN_23Q1_sprint_20230129224229sdy_SYNA_CI\n"
                                                        "model_number=CUSTOM7\n"
                                                        "wifi_mac=D4:52:EE:32:A3:B1\"\n";
                    //Create fake device info script
                    Core::File file(deviceInfoScript);
                    if(file.Exists())
                    {
                        std::cout << "File exists1\n";
                        file.Destroy();
                    }
                    file.Create();
                    file.Write(deviceInfoContent, sizeof(deviceInfoContent));
                    file.Close();
                }
                const char* str = static_cast<const char*>(strFmt);
                return popen(str, direction);
            }));

        ON_CALL(*p_btmgrImplMock, BTRMGR_RegisterEventCallback(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](BTRMGR_EventCallback astEventMessage) {
                    return BTRMGR_RESULT_SUCCESS;
                }));
        /*************API_NAME --- "CONNECT"**************/
        ON_CALL(*p_btmgrImplMock, BTRMGR_ConnectToDevice(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                ::testing::Return(BTRMGR_RESULT_SUCCESS));
        /*************API_NAME --- "DISCONNECT"**************/
        ON_CALL(*p_btmgrImplMock, BTRMGR_DisconnectFromDevice(::testing::_, ::testing::_))
            .WillByDefault(
                ::testing::Return(BTRMGR_RESULT_SUCCESS));
        /*************API_NAME --- "getDeviceInfo"***************/
        ON_CALL(*p_btmgrImplMock, BTRMGR_GetDeviceProperties(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DevicesProperty_t *pDeviceProperty) {
                pDeviceProperty->m_deviceType = BTRMGR_DEVICE_TYPE_WEARABLE_HEADSET;
                pDeviceProperty->m_rssi = BTRMGR_RSSI_NONE;
                pDeviceProperty->m_signalLevel = 0;
                pDeviceProperty->m_vendorID = 2075;
                pDeviceProperty->m_batteryLevel = 90;
                pDeviceProperty->m_deviceHandle = 48711265322011;
                strncpy(pDeviceProperty->m_modalias, "v:0009, p:075F, d:0100", sizeof(pDeviceProperty->m_modalias));
                strncpy(pDeviceProperty->m_firmwareRevision, "1.0.0", sizeof(pDeviceProperty->m_firmwareRevision));
                strncpy(pDeviceProperty->m_name, "MID ANC", sizeof(pDeviceProperty->m_name));
                strncpy(pDeviceProperty->m_deviceAddress, "2C:4D:79:AA:84:1B", sizeof(pDeviceProperty->m_deviceAddress));

                const char* profiles[] = {
                    "Headset",
                    "Audio Sink",
                    "AV Remote Target",
                    "AV Remote",
                    "Handsfree"
                };
                int numOfProfiles = sizeof(profiles) / sizeof(profiles[0]);
                pDeviceProperty->m_serviceInfo.m_numOfService = numOfProfiles;

                // Ensure we populate the profileInfo
                for (int i = 0; i < numOfProfiles; ++i)
                {
                    strncpy(pDeviceProperty->m_serviceInfo.m_profileInfo[i].m_profile, profiles[i], sizeof(pDeviceProperty->m_serviceInfo.m_profileInfo[i].m_profile));
                }
                return BTRMGR_RESULT_SUCCESS;
            }));
        ON_CALL(*p_btmgrImplMock, BTRMGR_GetDeviceTypeAsString(::testing::_))
            .WillByDefault(
                ::testing::Return("deviceType"));
        ON_CALL(*p_btmgrImplMock, BTRMGR_GetDiscoveredDevices(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](unsigned char aui8AdapterIdx, BTRMGR_DiscoveredDevicesList_t *pDiscoveredDevices) {
                // Example: Assuming 2 devices found
                pDiscoveredDevices->m_numOfDevices = 2;
                for (int i = 0; i < pDiscoveredDevices->m_numOfDevices; ++i)
                {
                    pDiscoveredDevices->m_deviceProperty[i].m_deviceHandle = 235353293830763 + i;
                    strncpy(pDiscoveredDevices->m_deviceProperty[i].m_name, (i == 0 ? "PMH01A-D60D7579BA6B" : "PMH01A-D60D7579BA6B"), sizeof(pDiscoveredDevices->m_deviceProperty[i].m_name));
                    pDiscoveredDevices->m_deviceProperty[i].m_deviceType = BTRMGR_DEVICE_TYPE_HID;
                    pDiscoveredDevices->m_deviceProperty[i].m_isConnected = false;
                    pDiscoveredDevices->m_deviceProperty[i].m_isPairedDevice = false;
                    pDiscoveredDevices->m_deviceProperty[i].m_ui32DevClassBtSpec = 0x00;
                    pDiscoveredDevices->m_deviceProperty[i].m_ui16DevAppearanceBleSpec = 0x0000;
                }
                return BTRMGR_RESULT_SUCCESS;
            }));
        /*************API_NAME --- "PAIR" ************* */
        ON_CALL(*p_btmgrImplMock, BTRMGR_PairDevice(::testing::_, ::testing::_))
            .WillByDefault(
                ::testing::Return(BTRMGR_RESULT_SUCCESS));
        /*************API_NAME --- "UNPAIR" ************* */
        ON_CALL(*p_btmgrImplMock, BTRMGR_UnpairDevice(::testing::_, ::testing::_))
            .WillByDefault(
                ::testing::Return(BTRMGR_RESULT_SUCCESS));

        /*************API_NAME --- "getInputDevices"***************/
        ON_CALL(*p_hdmiInputImplMock, getNumberOfInputs())
            .WillByDefault(::testing::Return(1));
        ON_CALL(*p_hdmiInputImplMock, isPortConnected(::testing::_))
            .WillByDefault(::testing::Return(true));

        // AVOuput Mocks
        ON_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                // EXPECT_EQ(string(pcCallerID), string("AVOutput"));
                // EXPECT_EQ(string(pcParameterName), string("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AVOutput.DynamicAutoLatency"));
                strncpy(pstParamData->value, "true", sizeof(pstParamData->value));
                return WDMP_SUCCESS;
            }));

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS)) {
                        // EXPECT_TRUE(handler != nullptr);
                        dsHdmiStatusEventHandler = handler;
                }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE)) {
                        // EXPECT_TRUE(handler != nullptr);
                        dsHdmiVideoModeEventHandler = handler;
                    }
                    return IARM_RESULT_SUCCESS;
                }));

        ON_CALL(*p_hdmiInputImplMock, getCurrentVideoModeObj(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](dsVideoPortResolution_t& resolution) {
                    resolution.pixelResolution = dsVIDEO_PIXELRES_1920x1080;
                    return tvERROR_NONE;
                }));

        ON_CALL(*p_tvSettingsImplMock, TvInit())
        .WillByDefault(::testing::Return(tvERROR_NONE));

        ON_CALL(*p_tvSettingsImplMock, RegisterVideoFormatChangeCB(testing::_))
        .WillByDefault(testing::Return(tvERROR_NONE));
        ON_CALL(*p_tvSettingsImplMock, RegisterVideoContentChangeCB(testing::_))
        .WillByDefault(testing::Return(tvERROR_NONE));
        ON_CALL(*p_tvSettingsImplMock, RegisterVideoResolutionChangeCB(testing::_))
        .WillByDefault(testing::Return(tvERROR_NONE));
        ON_CALL(*p_tvSettingsImplMock, RegisterVideoFrameRateChangeCB(testing::_))
        .WillByDefault(testing::Return(tvERROR_NONE));

        ON_CALL(*p_tr181ApiImplMock, getLocalParam(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
        [&](char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData) {
            // EXPECT_EQ(string(pcCallerID), string("AVOutput"));
            strncpy(pstParamData->value, "Normal", sizeof(pstParamData->value));
            return tr181Success;
            }));

        ON_CALL(*p_tvSettingsImplMock, SetAspectRatio(testing::_))
        .WillByDefault(testing::Invoke(
            [&](tvDisplayMode_t dispMode) {
                // EXPECT_EQ(dispMode, tvDisplayMode_NORMAL);
                return tvERROR_NONE;
                }));

        ON_CALL(*p_tvSettingsImplMock, GetCurrentSource(testing::_))
        .WillByDefault(testing::Invoke(
            [&](tvVideoSrcType_t *currentSource) {
                // EXPECT_EQ(*currentSource, VIDEO_SOURCE_IP);
                return tvERROR_NONE;
                }));

        ON_CALL(*p_tvSettingsImplMock, GetCurrentVideoFormat(testing::_))
            .WillByDefault(testing::Invoke(
                [&](tvVideoFormatType_t* videoFormat) {
                // EXPECT_EQ(*videoFormat, VIDEO_FORMAT_NONE);
                return tvERROR_NONE; // Return an appropriate error code
            }));

        ON_CALL(*p_tvSettingsImplMock, SetTVPictureMode(testing::_))
        .WillByDefault(testing::Invoke(
            [&](const char * pictureMode) {
                // EXPECT_EQ(string(pictureMode), string("normal"));
                return tvERROR_NONE;
            }));

        ON_CALL(*p_tvSettingsImplMock, ReadCapablitiesFromConfODM(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
            .WillByDefault([](std::string& rangeInfo, std::string& pqmodeInfo, std::string& formatInfo, std::string& sourceInfo, std::string param, std::string& platformsupport, std::string& index) {
                if(param == "PictureMode")
                {
                    printf("ReadCapablitiesFromConfODM - param - %s\n", param.c_str());
                    rangeInfo = "\"Standard\",\"Vivid\",\"EnergySaving\",\"Custom\",\"Theater\",\"Game\"";
                    pqmodeInfo = "";
                    formatInfo = "\"SDR\"";
                    sourceInfo = "\"HDMI\",\"HDMI2\"";
                    platformsupport = "";
                    index = "0";
                }
                return tvERROR_NONE;
            });
        ON_CALL(*p_tvSettingsImplMock, TvTerm())
        .WillByDefault(::testing::Return(tvERROR_NONE));

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
}