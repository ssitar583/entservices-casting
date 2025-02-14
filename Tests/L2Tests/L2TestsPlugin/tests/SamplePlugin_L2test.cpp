/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Synamedia Ltd.
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
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "L2Tests.h"
#include "L2TestsMock.h"

#include <mutex>
#include <condition_variable>
#include <fstream>


#include <interfaces/ISamplePlugin.h>

#define JSON_TIMEOUT   (1000)
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);
#define SAMPLEPLUGIN_CALLSIGN  _T("SamplePlugin")
#define SAMPLEPLUGINL2TEST_CALLSIGN _T("L2tests.1")


using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;

typedef enum : uint32_t 
{
    EXCITING_THING_HAPPEND = 0x00000001,
}SamplePluginL2test_async_events_t;

class AsyncHandlerMock_SamplePlugin {
public:
    AsyncHandlerMock_SamplePlugin() {}

    MOCK_METHOD(void, SomethingHappend, (const Exchange::ISamplePlugin::INotification::Source event));
};

class SamplePluginNotificationHandler : public Exchange::ISamplePlugin::INotification {
private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled;

    BEGIN_INTERFACE_MAP(Notification)
    INTERFACE_ENTRY(Exchange::ISamplePlugin::INotification)
    END_INTERFACE_MAP


public:
    SamplePluginNotificationHandler() {}
    ~SamplePluginNotificationHandler(){}

    void SomethingHappend(const Exchange::ISamplePlugin::INotification::Source event) override 
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (event == Exchange::ISamplePlugin::INotification::EXCITING_THING_HAPPENED) 
        {
            m_event_signalled |= EXCITING_THING_HAPPEND;
        }
        else 
        {
            TEST_LOG("Received unexpected event: %d", event);
        }
        m_condition_variable.notify_one();
    }

   uint32_t WaitForRequestStatus(uint32_t timeout_ms, SamplePluginL2test_async_events_t expected_status) 
   {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        std::chrono::milliseconds timeout(timeout_ms);

        while (!(expected_status & m_event_signalled)) {
            if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
                TEST_LOG("Timeout waiting for request status event");
                break;
            }
        }
        TEST_LOG("Finished waiting. signal status: %d", m_event_signalled);
        return m_event_signalled;
    }
};


class SamplePluginTest : public L2TestMocks {
protected:
    virtual   ~SamplePluginTest() override;
    IARM_EventHandler_t _eventHandler = nullptr;

public:
    SamplePluginTest(); 
    void SomethingHappend(const Exchange::ISamplePlugin::INotification::Source event);
    uint32_t WaitForRequestStatus(uint32_t timeout_ms, SamplePluginL2test_async_events_t expected_status);

private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled;
};

SamplePluginTest::SamplePluginTest()
        : L2TestMocks()
{
    Core::JSONRPC::Message message;
        string response;
        uint32_t status = Core::ERROR_GENERAL;
        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                   _eventHandler = handler;
                return IARM_RESULT_SUCCESS;
         }));
         status = ActivateService("SamplePlugin");
         EXPECT_EQ(Core::ERROR_NONE, status);
}

SamplePluginTest::~SamplePluginTest()
{
    uint32_t status = Core::ERROR_GENERAL;

    status = DeactivateService("SamplePlugin");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

TEST_F(SamplePluginTest, Greeter)
{
    JsonObject params;
    std::string message ="Hi";
    std::string result ;
    JsonObject result_1;
    uint32_t status = Core::ERROR_GENERAL;
    params["message"] = message;

    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SAMPLEPLUGIN_CALLSIGN, SAMPLEPLUGINL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_SamplePlugin> async_handler;
    Core::Sink<SamplePluginNotificationHandler> notification_handler;

    // Subscribe to the notification
    status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT, _T("SomethingHappend"), 
        [this,&async_handler](const JsonObject event) {
            std::string eventString;
            event.ToString(eventString);
            TEST_LOG("Event received in subscription callback: %s", eventString.c_str());
            Exchange::ISamplePlugin::INotification::Source expected_event = Exchange::ISamplePlugin::INotification::EXCITING_THING_HAPPENED;
            async_handler.SomethingHappend(expected_event);
            });

    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(async_handler, SomethingHappend(Exchange::ISamplePlugin::INotification::EXCITING_THING_HAPPENED))
    .WillOnce(Invoke(this, &SamplePluginTest::SomethingHappend));

    status = InvokeServiceMethod("SamplePlugin", "greeter", params, result_1);
    EXPECT_EQ(Core::ERROR_NONE, status);

    IARM_Bus_PWRMgr_EventData_t param;
    param.data.state.curState = IARM_BUS_PWRMGR_POWERSTATE_ON;
    param.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP;
    TEST_LOG("TEST_F Greeter calling _eventHandler for IARM_BUS_PWRMGR_EVENT_MODECHANGED");
    _eventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &param,sizeof(IARM_Bus_PWRMgr_EventData_t));

    // Wait for the event to be received
    uint32_t signalled = WaitForRequestStatus(JSON_TIMEOUT, EXCITING_THING_HAPPEND);
    EXPECT_TRUE(signalled & EXCITING_THING_HAPPEND);

    // Unsubscribe from the notification
    jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("SomethingHappend"));
    EXPECT_EQ(Core::ERROR_NONE, status);
}

uint32_t SamplePluginTest::WaitForRequestStatus(uint32_t timeout_ms, SamplePluginL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(timeout_ms);

    while (!(expected_status & m_event_signalled)) {
        if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
            TEST_LOG("Timeout waiting for request status event");
            break;
        }
    }
    return m_event_signalled;
}

void SamplePluginTest::SomethingHappend(const Exchange::ISamplePlugin::INotification::Source event) 
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (event == Exchange::ISamplePlugin::INotification::EXCITING_THING_HAPPENED)
    {
        m_event_signalled |= EXCITING_THING_HAPPEND;
    }
    else
    {
        TEST_LOG("Received unexpected event: %d", event);
    }
    m_condition_variable.notify_one();
}

