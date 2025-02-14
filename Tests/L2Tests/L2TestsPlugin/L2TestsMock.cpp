/* If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/

#include <string>
#include <sstream>

#include "L2TestsMock.h"
#ifdef L2_TEST_OOP_RPC
#include "MockAccessor.h"
#endif /* L2_TEST_OOP_RPC */

#define TEST_CALLSIGN _T("org.rdk.L2Tests.1")  /* Test module callsign. */
#define INVOKE_TIMEOUT 3000                        /* Method invoke timeout in milliseconds. */
#define THUNDER_ADDRESS _T("127.0.0.1:")
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

#ifndef THUNDER_PORT
#define THUNDER_PORT "9998"
#endif

using namespace WPEFramework;

/* L2TestMock consturctor */
L2TestMocks::L2TestMocks()
{
    p_rfcApiImplMock    = new NiceMock <RfcApiImplMock>;
    p_iarmBusImplMock   = new NiceMock <IarmBusImplMock>;
    p_readprocImplMock  = new NiceMock <readprocImplMock>;
    p_wrapsImplMock     = new NiceMock <WrapsImplMock>;
    p_hostImplMock      = new NiceMock <HostImplMock>;
    p_videoOutputPortConfigImplMock = new NiceMock <VideoOutputPortConfigImplMock>;
    p_managerImplMock   = new NiceMock <ManagerImplMock>;
    p_videoOutputPortMock = new NiceMock <VideoOutputPortMock>;
    p_udevImplMock      = new NiceMock <UdevImplMock>;
    p_rBusApiImplMock   = new NiceMock <RBusApiImplMock>;
    p_telemetryApiImplMock  = new NiceMock <TelemetryApiImplMock>;
    p_audioOutputPortMock  = new NiceMock <AudioOutputPortMock>;
    p_audioStereoModeMock  = new NiceMock <AudioStereoModeMock>;
    p_btmgrImplMock     = new NiceMock <BtmgrImplMock>;
    p_hdmiInputImplMock  = new NiceMock <HdmiInputImplMock>;
    p_libUSBApiImplMock  = new NiceMock <libUSBImplMock>;
    p_tvSettingsImplMock  = new NiceMock <TvSettingsImplMock>;
    p_tr181ApiImplMock = new NiceMock <Tr181ApiImplMock>;
    p_videoDeviceMock  = new NiceMock <VideoDeviceMock>;
    p_videoDFCMock = new NiceMock <VideoDFCMock>;
    p_displayMock  = new testing::NiceMock <DisplayMock>;
    p_videoResolutionMock  = new NiceMock <VideoResolutionMock>;
    p_frontPanelConfigImplMock  = new testing::NiceMock <FrontPanelConfigMock>;
   //  p_hostImplMock  = new testing::NiceMock <HostImplMock>;
   //  p_frontPanelIndicatorMock  = new testing::NiceMock <FrontPanelIndicatorMock>;
   //  testing::NiceMock<FrontPanelIndicatorMock> frontPanelIndicatorMock;
   p_audioOutputPortTypeMock  = new NiceMock <AudioOutputPortTypeMock>;
   p_videoOutputPortTypeMock  = new NiceMock <VideoOutputPortTypeMock>;
    TEST_LOG("Inside L2TestMocks constructor");

#ifdef L2_TEST_OOP_RPC
    MockAccessor<IarmBusImpl>::setPtr(p_iarmBusImplMock);
    MockAccessor<readprocImpl>::setPtr(p_readprocImplMock);
    MockAccessor<RBusApiImpl>::setPtr(p_rBusApiImplMock);
    MockAccessor<RfcApiImpl>::setPtr(p_rfcApiImplMock);
#else
    IarmBus::setImpl(p_iarmBusImplMock);
    RfcApi::setImpl(p_rfcApiImplMock);
    ProcImpl::setImpl(p_readprocImplMock);
    Wraps::setImpl(p_wrapsImplMock);
    RBusApi::setImpl(p_rBusApiImplMock);
    TelemetryApi::setImpl(p_telemetryApiImplMock);
    device::Host::setImpl(p_hostImplMock);
    device::VideoOutputPortConfig::setImpl(p_videoOutputPortConfigImplMock);
    device::Manager::setImpl(p_managerImplMock);
    device::VideoOutputPort::setImpl(p_videoOutputPortMock);
    Udev::setImpl(p_udevImplMock);
    Btmgr::setImpl(p_btmgrImplMock);
    device::HdmiInput::setImpl(p_hdmiInputImplMock);
    libusbApi::setImpl(p_libUSBApiImplMock);
    TvSettings::setImpl(p_tvSettingsImplMock);
    Tr181Api::setImpl(p_tr181ApiImplMock);

#endif /* L2_TEST_OOP_RPC */

    device::AudioOutputPort::setImpl(p_audioOutputPortMock);
    device::AudioStereoMode::setImpl(p_audioStereoModeMock);
    device::VideoDevice::setImpl(p_videoDeviceMock);
    device::VideoDFC::setImpl(p_videoDFCMock);
    device::VideoResolution::setImpl(p_videoResolutionMock);

    device::Display::setImpl(p_displayMock);
    device::FrontPanelConfig::setImpl(p_frontPanelConfigImplMock);
    device::AudioOutputPortType::setImpl(p_audioOutputPortTypeMock);
    device::VideoOutputPortType::setImpl(p_videoOutputPortTypeMock);


    thunder_address = THUNDER_ADDRESS + std::string(THUNDER_PORT);
    (void)Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), thunder_address);

#ifdef L2_TEST_OOP_RPC
    ActivateService("org.rdk.MockPlugin");
#endif /* L2_TEST_OOP_RPC */

}

/* L2TestMock Destructor */
L2TestMocks::~L2TestMocks()
{

#ifdef L2_TEST_OOP_RPC
   DeactivateService("org.rdk.MockPlugin");
   MockAccessor<IarmBusImpl>::setPtr(nullptr);
   MockAccessor<readprocImpl>::setPtr(nullptr);
   MockAccessor<RBusApiImpl>::setPtr(nullptr);
   MockAccessor<RfcApiImpl>::setPtr(nullptr);
#else
   IarmBus::setImpl(nullptr);
   RfcApi::setImpl(nullptr);
   ProcImpl::setImpl(nullptr);
   Wraps::setImpl(nullptr);
   RBusApi::setImpl(nullptr);
   TelemetryApi::setImpl(nullptr);
   device::Host::setImpl(nullptr);
   device::VideoOutputPortConfig::setImpl(nullptr);
   device::Manager::setImpl(nullptr);
   device::VideoOutputPort::setImpl(nullptr);
   Udev::setImpl(nullptr);
   Btmgr::setImpl(nullptr);
   device::HdmiInput::setImpl(nullptr);
   libusbApi::setImpl(nullptr);
   TvSettings::setImpl(nullptr);
   Tr181Api::setImpl(nullptr);
#endif /* L2_TEST_OOP_RPC */

   TEST_LOG("Inside L2TestMocks destructor");

   if (p_iarmBusImplMock != nullptr)
   {
        delete p_iarmBusImplMock;
        p_iarmBusImplMock = nullptr;
   }

   if (p_rfcApiImplMock != nullptr)
   {
        delete p_rfcApiImplMock;
        p_rfcApiImplMock = nullptr;
   }

   if (p_readprocImplMock != nullptr)
   {
        delete p_readprocImplMock;
        p_readprocImplMock = nullptr;
   }

   if (p_wrapsImplMock != nullptr)
   {
        delete p_wrapsImplMock;
        p_wrapsImplMock = nullptr;
   }


   if (p_rBusApiImplMock != nullptr)
   {
        delete p_rBusApiImplMock;
        p_rBusApiImplMock = nullptr;
   }


   if (p_telemetryApiImplMock != nullptr)
   {
        delete p_telemetryApiImplMock;
        p_telemetryApiImplMock = nullptr;
   }


   if (p_hostImplMock != nullptr)
   {
      delete p_hostImplMock;
      p_hostImplMock = nullptr;
   }

   if (p_videoOutputPortConfigImplMock != nullptr)
   {
      delete p_videoOutputPortConfigImplMock;
      p_videoOutputPortConfigImplMock = nullptr;
   }

   // device::AudiodeoOutputPortConfig::setImpl(nullptr);
   // if (p_audioOutputPortConfigImplMock != nullptr)
   // {
   //    delete p_audioOutputPortConfigImplMock;
   //    p_audioOutputPortConfigImplMock = nullptr;
   // }

   device::AudioOutputPort::setImpl(nullptr);
   if (p_audioOutputPortMock != nullptr)
   {
      delete p_audioOutputPortMock;
      p_audioOutputPortMock = nullptr;
   }

   device::AudioStereoMode::setImpl(nullptr);
   if (p_audioStereoModeMock != nullptr)
   {
      delete p_audioStereoModeMock;
      p_audioStereoModeMock = nullptr;
   }


   // device::AudiodeoOutputPortConfig::setImpl(nullptr);
   // if (p_audioOutputPortConfigImplMock != nullptr)
   // {
   //    delete p_audioOutputPortConfigImplMock;
   //    p_audioOutputPortConfigImplMock = nullptr;
   // }

   device::AudioOutputPort::setImpl(nullptr);
   if (p_audioOutputPortMock != nullptr)
   {
      delete p_audioOutputPortMock;
      p_audioOutputPortMock = nullptr;
   }

   device::AudioStereoMode::setImpl(nullptr);
   if (p_audioStereoModeMock != nullptr)
   {
      delete p_audioStereoModeMock;
      p_audioStereoModeMock = nullptr;
   }

   device::Manager::setImpl(nullptr);

   if (p_managerImplMock != nullptr)
   {
      delete p_managerImplMock;
      p_managerImplMock = nullptr;
   }

   if (p_videoOutputPortMock != nullptr)
   {
      delete p_videoOutputPortMock;
      p_videoOutputPortMock = nullptr;
   }


   if (p_udevImplMock != nullptr)
   {
        delete p_udevImplMock;
        p_udevImplMock = nullptr;
   }


   if (p_btmgrImplMock != nullptr)
   {
        delete p_btmgrImplMock;
        p_btmgrImplMock = nullptr;
   }

   if (p_hdmiInputImplMock != nullptr)
   {
      delete p_hdmiInputImplMock;
      p_hdmiInputImplMock = nullptr;
   }


   if (p_libUSBApiImplMock != nullptr)
   {
        delete p_libUSBApiImplMock;
        p_libUSBApiImplMock = nullptr;
   }

   if (p_tvSettingsImplMock != nullptr)
   {
      delete p_tvSettingsImplMock;
      p_tvSettingsImplMock = nullptr;
   }

   if (p_tr181ApiImplMock != nullptr)
   {
      delete p_tr181ApiImplMock;
      p_tr181ApiImplMock = nullptr;
   }
   device::VideoDevice::setImpl(nullptr);
   if (p_videoDeviceMock != nullptr)
   {
      delete p_videoDeviceMock;
      p_videoDeviceMock = nullptr;
   }
   device::VideoDFC::setImpl(nullptr);
   if (p_videoDFCMock != nullptr)
   {
      delete p_videoDFCMock;
      p_videoDFCMock = nullptr;
   }

   device::Display::setImpl(nullptr);
   if (p_displayMock != nullptr)
   {
         delete p_displayMock;
         p_displayMock = nullptr;
   }
   
   device::VideoResolution::setImpl(nullptr);
   if (p_videoResolutionMock != nullptr)
   {
      delete p_videoResolutionMock;
      p_videoResolutionMock = nullptr;
   }
    
   device::FrontPanelConfig::setImpl(nullptr);
   if (p_frontPanelConfigImplMock != nullptr)
   {
      delete p_frontPanelConfigImplMock;
      p_frontPanelConfigImplMock = nullptr;
   }

   device::AudioOutputPortType::setImpl(nullptr);
        if (p_audioOutputPortTypeMock != nullptr)
        {
            delete p_audioOutputPortTypeMock;
            p_audioOutputPortTypeMock = nullptr;
        }

   device::VideoOutputPortType::setImpl(nullptr);
        if (p_videoOutputPortTypeMock != nullptr)
        {
            delete p_videoOutputPortTypeMock;
            p_videoOutputPortTypeMock = nullptr;
        }

//    device::FrontPanelIndicator::setImpl(nullptr);
//   // device::FrontPanelIndicator::getInstance().impl = &frontPanelIndicatorMock;

//    if (p_frontPanelIndicatorMock!= nullptr)
//    {
//       delete p_frontPanelIndicatorMock;
//       p_frontPanelIndicatorMock = nullptr;
//    }

}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[in] params Method parameters
 * @param[out] results Method results
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, JsonObject &params, JsonObject &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string message;
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   params.ToString(message);
   TEST_LOG("Invoking %s.%s, parameters %s\n", callsign, method, message.c_str());

   results = JsonObject();
   status = jsonrpc.Invoke<JsonObject, JsonObject>(INVOKE_TIMEOUT, std::string(method), params, results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;
}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[in] params Method parameters
 * @param[out] results Method results
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, JsonObject &params, Core::JSON::String &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string message;
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   params.ToString(message);
   TEST_LOG("Invoking %s.%s, parameters %s\n", callsign, method, message.c_str());

   status = jsonrpc.Invoke<JsonObject, Core::JSON::String>(INVOKE_TIMEOUT, std::string(method), params, results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;

}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[out] results Method results
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, Core::JSON::Boolean &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   TEST_LOG("Invoking %s.%s \n", callsign, method);

   status = jsonrpc.Invoke<void, Core::JSON::Boolean>(INVOKE_TIMEOUT, std::string(method), results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;
}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[out] results Method results
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, Core::JSON::String &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   TEST_LOG("Invoking %s.%s \n", callsign, method);

   status = jsonrpc.Invoke<void, Core::JSON::String>(INVOKE_TIMEOUT, std::string(method), results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;

}
/**
* @brief Activate a service plugin
*
* @param[in] callsign Service callsign
* @return Zero (Core::ERROR_NONE) on succes or another value on error
*/
uint32_t L2TestMocks::ActivateService(const char *callsign)
{
   JsonObject params;
   JsonObject result;
   uint32_t status = Core::ERROR_GENERAL;

   if(callsign != NULL)
   {
     params["callsign"] = callsign;
     status = InvokeServiceMethod("Controller.1", "activate", params, result);
   }

   return status;
}

/**
* @brief Deactivate a service plugin
*
* @param[in] callsign Service callsign
* @return Zero (Core::ERROR_NONE) on succes or another value on error
*/
uint32_t L2TestMocks::DeactivateService(const char *callsign)
{
   JsonObject params;
   JsonObject result;
   uint32_t status = Core::ERROR_GENERAL;

   if(callsign != NULL)
   {
      params["callsign"] = callsign;
      status = InvokeServiceMethod("Controller.1", "deactivate", params, result);
   }
   return status;
}


