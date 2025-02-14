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
#include <interfaces/IPowerManager.h>

#define JSON_TIMEOUT   (1000)
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);
#define SYSTEM_CALLSIGN  _T("org.rdk.PowerManager.1")
#define L2TEST_CALLSIGN _T("L2tests.1")
#define KED_FP_POWER 10

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
using ThermalTemperature = WPEFramework::Exchange::IPowerManager::ThermalTemperature;

typedef enum : uint32_t {
    POWERMANAGERL2TEST_SYSTEMSTATE_CHANGED = 0x00000001,
    POWERMANAGERL2TEST_THERMALSTATE_CHANGED=0x00000002,
    POWERMANAGERL2TEST_LOGUPLOADSTATE_CHANGED=0x00000004,
    POWERMANAGERL2TEST_SYSTEMSTATE_PRECHANGE = 0x00000005,
    POWERMANAGERL2TEST_EVENT_REBOOTING = 0x00000006,
    POWERMANAGERL2TEST_NETWORK_STANDBYMODECHANGED = 0x00000007,
    POWERMANAGERL2TEST_STATE_INVALID = 0x00000000
}PowerManagerL2test_async_events_t;

class PwrMgr_Notification : public Exchange::IPowerManager::INotification {
    private:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled;

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::IPowerManager::INotification)
        END_INTERFACE_MAP

    public:
        PwrMgr_Notification(){}
        ~PwrMgr_Notification(){}

        void OnPowerModeChanged(const PowerState &currentState, const PowerState &newState) override
        {
            TEST_LOG("OnPowerModeChanged event triggered ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("OnPowerModeChanged currentState: %u, newState: %u\n", currentState, newState);
            /* Notify the requester thread. */
            m_event_signalled |= POWERMANAGERL2TEST_SYSTEMSTATE_CHANGED;
            m_condition_variable.notify_one();
        }

        void OnPowerModePreChange(const PowerState &currentState, const PowerState &newState) override
        {
            TEST_LOG("OnPowerModePreChange event triggered ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            TEST_LOG("OnPowerModePreChange currentState: %u, newState: %u\n", currentState, newState);
            /* Notify the requester thread. */
            m_event_signalled |= POWERMANAGERL2TEST_SYSTEMSTATE_PRECHANGE;
            m_condition_variable.notify_one();
        }

        void OnDeepSleepTimeout(const int &wakeupTimeout) override
        {
            LOGINFO("OnDeepSleepTimeout: wakeupTimeout %d\n", wakeupTimeout);
        }

        void OnNetworkStandbyModeChanged(const bool &enabled) override
        {
            LOGINFO("OnNetworkStandbyModeChanged: enabled %d\n", enabled);
        }

        void OnThermalModeChanged(const ThermalTemperature &currentThermalLevel, const ThermalTemperature &newThermalLevel, const float &currentTemperature) override
        {
            TEST_LOG("OnThermalModeChanged event triggered ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);
            LOGINFO("OnThermalModeChanged received: currentThermalLevel %u, newThermalLevel %u, currentTemperature %f\n", currentThermalLevel, newThermalLevel, currentTemperature);

            /* Notify the requester thread. */
            m_event_signalled |= POWERMANAGERL2TEST_THERMALSTATE_CHANGED;
            m_condition_variable.notify_one();
        }

        void OnRebootBegin(const string &rebootReasonCustom, const string &rebootReasonOther, const string &rebootRequestor) override
        {
            TEST_LOG("OnRebootBegin event triggered ***\n");
            std::unique_lock<std::mutex> lock(m_mutex);

            LOGINFO("OnRebootBegin: rebootReasonCustom %s, rebootReasonOther %s, rebootRequestor %s\n", rebootReasonCustom.c_str(), rebootReasonOther.c_str(), rebootRequestor.c_str());
            /* Notify the requester thread. */
            m_event_signalled |= POWERMANAGERL2TEST_EVENT_REBOOTING;
            m_condition_variable.notify_one();
        }

        uint32_t WaitForRequestStatus(uint32_t timeout_ms, PowerManagerL2test_async_events_t expected_status)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto now = std::chrono::system_clock::now();
            std::chrono::milliseconds timeout(timeout_ms);
            uint32_t signalled = POWERMANAGERL2TEST_STATE_INVALID;

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

};
/**
 * @brief Internal test mock class
 *
 * Note that this is for internal test use only and doesn't mock any actual
 * concrete interface.
 */
class AsyncHandlerMock
{
    public:
        AsyncHandlerMock()
        {
        }

        MOCK_METHOD(void, OnPowerModeChanged, (const PowerState &currentState, const PowerState &newState));
        MOCK_METHOD(void, OnPowerModePreChange, (const PowerState &currentState, const PowerState &newState));
        MOCK_METHOD(void, OnDeepSleepTimeout, (const int &wakeupTimeout));
        MOCK_METHOD(void, OnNetworkStandbyModeChanged,(const bool &enabled));
        MOCK_METHOD(void, OnThermalModeChanged,(const ThermalTemperature &currentThermalLevel, const ThermalTemperature &newThermalLevel, const float &currentTemperature));
        MOCK_METHOD(void, OnRebootBegin,(const string &rebootReasonCustom, const string &rebootReasonOther, const string &rebootRequestor));
};

/* Systemservice L2 test class declaration */
class PowerManager_L2Test : public L2TestMocks {
protected:
    IARM_EventHandler_t thermMgrEventsHandler = nullptr;
    IARM_EventHandler_t powerEventHandler = nullptr;
    IARM_EventHandler_t powerPreChangeEventHandler = nullptr;
    IARM_EventHandler_t RebootingEventHandler = nullptr;
    IARM_EventHandler_t nwStndbyModeEventHandler = nullptr;
    IARM_EventHandler_t deepSleepTimeoutEventHandler = nullptr;

    PowerManager_L2Test();
    virtual ~PowerManager_L2Test() override;

    public:
        /**
         * @brief called when System state
         * changed notification received from IARM
         */
        void OnPowerModeChanged(const PowerState &currentState, const PowerState &newState);

        /**
         * @brief called when System state
         * changed notification received from IARM
         */
        void OnPowerModePreChange(const PowerState &currentState, const PowerState &newState);

        /**
         * @brief called when System state
         * changed notification received from IARM
         */
        void OnDeepSleepTimeout(const int &wakeupTimeout);

        /**
         * @brief called when System state
         * changed notification received from IARM
         */
        void OnNetworkStandbyModeChanged(const bool &enabled);

        /**
         * @brief called when System state
         * changed notification received from IARM
         */
        void OnThermalModeChanged(const ThermalTemperature &currentThermalLevel, const ThermalTemperature &newThermalLevel, const float &currentTemperature);

        /**
         * @brief called when System state
         * changed notification received from IARM
         */
        void OnRebootBegin(const string &rebootReasonCustom, const string &rebootReasonOther, const string &rebootRequestor);

        /**
         * @brief waits for various status change on asynchronous calls
         */
      uint32_t WaitForRequestStatus(uint32_t timeout_ms,PowerManagerL2test_async_events_t expected_status);
      void Test_PowerStateChange( Exchange::IPowerManager* PowerManagerPlugin );
      void Test_TemperatureThresholds( Exchange::IPowerManager* PowerManagerPlugin );
      void Test_OvertempGraceInterval( Exchange::IPowerManager* PowerManagerPlugin );
      void Test_WakeupSrcConfig( Exchange::IPowerManager* PowerManagerPlugin );
      void Test_PerformReboot( Exchange::IPowerManager* PowerManagerPlugin );
      void Test_NetworkStandbyMode( Exchange::IPowerManager* PowerManagerPlugin );
      Core::Sink<PwrMgr_Notification> mNotification;

    private:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled;
};


/**
 * @brief Constructor for PowerManagers L2 test class
 */
PowerManager_L2Test::PowerManager_L2Test()
        : L2TestMocks()
{
        uint32_t status = Core::ERROR_GENERAL;
        m_event_signalled = POWERMANAGERL2TEST_STATE_INVALID;

        /* Set all the asynchronouse event handler with IARM bus to handle various events*/
        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                if ((string(IARM_BUS_PWRMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_PWRMGR_EVENT_THERMAL_MODECHANGED)) {
                    thermMgrEventsHandler = handler;
                }
                if ((string(IARM_BUS_PWRMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_PWRMGR_EVENT_MODECHANGED)) {
                    powerEventHandler = handler;
                }
                if ((string(IARM_BUS_PWRMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_PWRMGR_EVENT_REBOOTING)) {
                    RebootingEventHandler = handler;
                }
                if ((string(IARM_BUS_PWRMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED)) {
                    nwStndbyModeEventHandler = handler;
                }
                if ((string(IARM_BUS_PWRMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_PWRMGR_EVENT_DEEPSLEEP_TIMEOUT)) {
                    deepSleepTimeoutEventHandler = handler;
                }
                return IARM_RESULT_SUCCESS;
            }));

         /* Activate plugin in constructor */
         status = ActivateService("org.rdk.PowerManager");
         EXPECT_EQ(Core::ERROR_NONE, status);

}

/**
 * @brief Destructor for PowerManagers L2 test class
 */
PowerManager_L2Test::~PowerManager_L2Test()
{
    uint32_t status = Core::ERROR_GENERAL;
    m_event_signalled = POWERMANAGERL2TEST_STATE_INVALID;

    ON_CALL(*p_rBusApiImplMock, rbus_close(::testing::_ ))
    .WillByDefault(
    ::testing::Return(RBUS_ERROR_SUCCESS));

    status = DeactivateService("org.rdk.PowerManager");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

void PowerManager_L2Test::OnPowerModeChanged(const PowerState &currentState, const PowerState &newState)
{
    TEST_LOG("OnPowerModeChanged event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    TEST_LOG("OnPowerModeChanged currentState: %u, newState: %u\n", currentState, newState);
    /* Notify the requester thread. */
    m_event_signalled |= POWERMANAGERL2TEST_SYSTEMSTATE_CHANGED;
    m_condition_variable.notify_one();
}

void PowerManager_L2Test::OnPowerModePreChange(const PowerState &currentState, const PowerState &newState)
{
    TEST_LOG("OnPowerModePreChange event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    TEST_LOG("OnPowerModePreChange currentState: %u, newState: %u\n", currentState, newState);
    /* Notify the requester thread. */
    m_event_signalled |= POWERMANAGERL2TEST_SYSTEMSTATE_PRECHANGE;
    m_condition_variable.notify_one();
}

void PowerManager_L2Test::OnDeepSleepTimeout(const int &wakeupTimeout)
{
    LOGINFO("OnDeepSleepTimeout: wakeupTimeout %d\n", wakeupTimeout);
}

void PowerManager_L2Test::OnNetworkStandbyModeChanged(const bool &enabled)
{
    LOGINFO("onNetworkStandbyModeChanged: enabled %d\n", enabled);
}

void PowerManager_L2Test::OnThermalModeChanged(const ThermalTemperature &currentThermalLevel, const ThermalTemperature &newThermalLevel, const float &currentTemperature)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    LOGINFO("OnThermalModeChanged received: currentThermalLevel %u, newThermalLevel %u, currentTemperature %f\n", currentThermalLevel, newThermalLevel, currentTemperature);

    /* Notify the requester thread. */
    m_event_signalled |= POWERMANAGERL2TEST_THERMALSTATE_CHANGED;
    m_condition_variable.notify_one();
}

void PowerManager_L2Test::OnRebootBegin(const string &rebootReasonCustom, const string &rebootReasonOther, const string &rebootRequestor)
{
    TEST_LOG("OnRebootBegin event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    LOGINFO("OnRebootBegin: rebootReasonCustom %s, rebootReasonOther %s, rebootRequestor %s\n", rebootReasonCustom.c_str(), rebootReasonOther.c_str(), rebootRequestor.c_str());
    /* Notify the requester thread. */
    m_event_signalled |= POWERMANAGERL2TEST_EVENT_REBOOTING;
    m_condition_variable.notify_one();
}
/**
 * @brief waits for various status change on asynchronous calls
 *
 * @param[in] timeout_ms timeout for waiting
 */
uint32_t PowerManager_L2Test::WaitForRequestStatus(uint32_t timeout_ms,PowerManagerL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(timeout_ms);
    uint32_t signalled = POWERMANAGERL2TEST_STATE_INVALID;

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

/* COM-RPC tests */
void PowerManager_L2Test::Test_OvertempGraceInterval( Exchange::IPowerManager* PowerManagerPlugin )
{
    uint32_t status = Core::ERROR_GENERAL;

    int graceInterval = 100;
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_SetOvertempGraceInterval)));
               auto param = static_cast<IARM_Bus_PWRMgr_SetOvertempGraceInterval_Param_t*>(arg);
               param->graceInterval = 100;
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->SetOvertempGraceInterval(graceInterval);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

    int graceInterval1 = 100;
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_GetOvertempGraceInterval)));
               auto param = static_cast<IARM_Bus_PWRMgr_GetOvertempGraceInterval_Param_t*>(arg);
               param->graceInterval = 100;
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->GetOvertempGraceInterval(graceInterval1);
    EXPECT_EQ(graceInterval1, graceInterval);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/* COM-RPC tests */
void PowerManager_L2Test::Test_TemperatureThresholds( Exchange::IPowerManager* PowerManagerPlugin )
{
    uint32_t signalled = POWERMANAGERL2TEST_STATE_INVALID;
    uint32_t status = Core::ERROR_GENERAL;
    float high = 95, critical = 99;

    /* Set Threshold */
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
      .WillByDefault(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
                EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_SetTemperatureThresholds)));
                auto param = static_cast<IARM_Bus_PWRMgr_SetTempThresholds_Param_t*>(arg);
                EXPECT_EQ(param->tempHigh, 95);
                EXPECT_EQ(param->tempCritical, 99);
                return IARM_RESULT_SUCCESS;
            });

    status = PowerManagerPlugin->SetTemperatureThresholds(high, critical);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

    IARM_Bus_PWRMgr_EventData_t param;
    param.data.therm.newLevel = IARM_BUS_PWRMGR_TEMPERATURE_HIGH;
    param.data.therm.curLevel = IARM_BUS_PWRMGR_TEMPERATURE_NORMAL;
    param.data.therm.curTemperature = 100;
    thermMgrEventsHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_THERMAL_MODECHANGED, &param, sizeof(param));

    signalled = mNotification.WaitForRequestStatus(JSON_TIMEOUT,POWERMANAGERL2TEST_THERMALSTATE_CHANGED);
    EXPECT_TRUE(signalled & POWERMANAGERL2TEST_THERMALSTATE_CHANGED);

    float high1 = 95, critical1 = 99;
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
      .WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_GetTemperatureThresholds)));
                auto param = static_cast<IARM_Bus_PWRMgr_GetTempThresholds_Param_t*>(arg);
                param->tempHigh = 95;
                param->tempCritical = 99;
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->GetTemperatureThresholds(high1, critical1);
    EXPECT_EQ(high1, high);
    EXPECT_EQ(critical1, critical);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

    float temperature = 0.0;

    /* Get Thermal state */
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
        .WillByDefault(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
                EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_GetThermalState)));
                auto param = static_cast<IARM_Bus_PWRMgr_GetThermalState_Param_t*>(arg);
               param->curTemperature = 100;
                return IARM_RESULT_SUCCESS;
            });

    status = PowerManagerPlugin->GetThermalState(temperature);
    EXPECT_EQ(temperature, 100);
    EXPECT_EQ(status,Core::ERROR_NONE);

    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

}

/* COM-RPC tests */
void PowerManager_L2Test::Test_PowerStateChange( Exchange::IPowerManager* PowerManagerPlugin )
{
    uint32_t signalled = POWERMANAGERL2TEST_STATE_INVALID;
    uint32_t status = Core::ERROR_GENERAL;

    PowerState currentState = PowerState::POWER_STATE_ON;
    PowerState prevState = PowerState::POWER_STATE_STANDBY;
    const string  standbyReason = "";
    int keyCode = KED_FP_POWER;

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_SetPowerState)));
               auto param = static_cast<_IARM_Bus_PWRMgr_SetPowerState_Param_t*>(arg);
               param->newState = IARM_BUS_PWRMGR_POWERSTATE_ON;
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->SetPowerState(keyCode,currentState,standbyReason);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

    IARM_Bus_PWRMgr_EventData_t param;
    param.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_ON;
    param.data.state.curState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY;

    powerEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &param, sizeof(param));
    signalled = mNotification.WaitForRequestStatus(JSON_TIMEOUT,POWERMANAGERL2TEST_SYSTEMSTATE_CHANGED);
    EXPECT_TRUE(signalled & POWERMANAGERL2TEST_SYSTEMSTATE_CHANGED);

    PowerState currentState1 = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState1 = PowerState::POWER_STATE_UNKNOWN;
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_GetPowerState)));
               auto param = static_cast<_IARM_Bus_PWRMgr_GetPowerState_Param_t*>(arg);
               param->prevState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY;
               param->curState = IARM_BUS_PWRMGR_POWERSTATE_ON;
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->GetPowerState(currentState1, prevState1);
    EXPECT_EQ(currentState1, currentState);
    EXPECT_EQ(prevState1, prevState);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/* COM-RPC tests */
void PowerManager_L2Test::Test_WakeupSrcConfig( Exchange::IPowerManager* PowerManagerPlugin )
{
    uint32_t status = Core::ERROR_GENERAL;
    int powerMode = (1<<IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP),
            srcType = (1<<WAKEUPSRC_VOICE),
            config = (1<<WAKEUPSRC_VOICE);

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_SetWakeupSrcConfig)));
               auto param = static_cast<IARM_Bus_PWRMgr_WakeupSrcConfig_Param_t*>(arg);
                EXPECT_EQ(param->pwrMode, (1<<IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP));
                EXPECT_EQ(param->srcType, (1<<WAKEUPSRC_VOICE));
                EXPECT_EQ(param->config, (1<<WAKEUPSRC_VOICE));
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->SetWakeupSrcConfig(powerMode, srcType, config);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

    int powerMode1, srcType1, config1;
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_GetWakeupSrcConfig)));
               auto param = static_cast<IARM_Bus_PWRMgr_WakeupSrcConfig_Param_t*>(arg);
                param->pwrMode = (1<<IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                param->srcType = (1<<WAKEUPSRC_VOICE);
                param->config = (1<<WAKEUPSRC_VOICE);
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->GetWakeupSrcConfig(powerMode1, srcType1, config1);
    EXPECT_EQ(powerMode, powerMode1);
    EXPECT_EQ(srcType, srcType1);
    EXPECT_EQ(config, config1);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/* COM-RPC tests */
void PowerManager_L2Test::Test_PerformReboot( Exchange::IPowerManager* PowerManagerPlugin )
{
    uint32_t status = Core::ERROR_GENERAL;
    uint32_t signalled = POWERMANAGERL2TEST_STATE_INVALID;
    string requestor = "PowerManager_L2Test";
    string customReason = "MAINTENANCE_REBOOT";
    string otherReason = "MAINTENANCE_REBOOT";

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_Reboot)));
               auto param = static_cast<IARM_Bus_PWRMgr_RebootParam_t*>(arg);
                EXPECT_EQ(strcmp(param->requestor, "PowerManager_L2Test"),0);
                EXPECT_EQ(strcmp(param->reboot_reason_custom, "MAINTENANCE_REBOOT"),0);
                EXPECT_EQ(strcmp(param->reboot_reason_other, "MAINTENANCE_REBOOT"),0);
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->Reboot(requestor,customReason,otherReason);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

    IARM_Bus_PWRMgr_RebootParam_t param = {};
    strncpy(param.requestor, "PowerManager", sizeof(param.requestor));
    strncpy(param.reboot_reason_custom, "MAINTENANCE_REBOOT", sizeof(param.reboot_reason_custom));
    strncpy(param.reboot_reason_other, "MAINTENANCE_REBOOT", sizeof(param.reboot_reason_other));

    RebootingEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_REBOOTING, &param, sizeof(param));
    signalled = mNotification.WaitForRequestStatus(JSON_TIMEOUT,POWERMANAGERL2TEST_EVENT_REBOOTING);
    EXPECT_TRUE(signalled & POWERMANAGERL2TEST_EVENT_REBOOTING);
}

/* COM-RPC tests */
void PowerManager_L2Test::Test_NetworkStandbyMode( Exchange::IPowerManager* PowerManagerPlugin )
{
    uint32_t status = Core::ERROR_GENERAL;
    uint32_t signalled = POWERMANAGERL2TEST_STATE_INVALID;
    bool standbyMode = true;

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_SetNetworkStandbyMode)));
               auto param = static_cast<IARM_Bus_PWRMgr_NetworkStandbyMode_Param_t*>(arg);
                EXPECT_EQ(param->bStandbyMode, true);
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->SetNetworkStandbyMode(standbyMode);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

    IARM_Bus_PWRMgr_EventData_t param;
    param.data.bNetworkStandbyMode = true;

    nwStndbyModeEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED, &param, sizeof(param));
    signalled = mNotification.WaitForRequestStatus(JSON_TIMEOUT,POWERMANAGERL2TEST_NETWORK_STANDBYMODECHANGED);
    EXPECT_TRUE(signalled & POWERMANAGERL2TEST_NETWORK_STANDBYMODECHANGED);

    bool standbyMode1;
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
   .   WillByDefault(
          [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
              EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_PWRMGR_NAME)));
              EXPECT_EQ(string(methodName), string(_T(IARM_BUS_PWRMGR_API_GetNetworkStandbyMode)));
               auto param = static_cast<IARM_Bus_PWRMgr_NetworkStandbyMode_Param_t*>(arg);
                param->bStandbyMode = true;
               return IARM_RESULT_SUCCESS;
    });
    status = PowerManagerPlugin->GetNetworkStandbyMode(standbyMode1);
    EXPECT_EQ(standbyMode, standbyMode1);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/********************************************************
************Test case Details **************************
** 1. Get temperature from systemservice
** 2. Set temperature threshold
** 3. Temperature threshold change event triggered from IARM
** 4. Verify that threshold change event is notified
*******************************************************/

TEST_F(PowerManager_L2Test,PowerManagerComRpc)
{
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> mEngine_PowerManager;
    Core::ProxyType<RPC::CommunicatorClient> mClient_PowerManager;
    PluginHost::IShell *mController_PowerManager;

    TEST_LOG("Creating mEngine_PowerManager");
    mEngine_PowerManager = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    mClient_PowerManager = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(mEngine_PowerManager));

    TEST_LOG("Creating mEngine_PowerManager Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    mEngine_PowerManager->Announcements(mClient_PowerManager->Announcement());
#endif

    if (!mClient_PowerManager.IsValid())
    {
        TEST_LOG("Invalid mClient_PowerManager");
    }
    else
    {
        mController_PowerManager = mClient_PowerManager->Open<PluginHost::IShell>(_T("org.rdk.PowerManager"), ~0, 3000);
        if (mController_PowerManager)
        {
            auto PowerManagerPlugin = mController_PowerManager->QueryInterface<Exchange::IPowerManager>();

            PowerManagerPlugin->AddRef();
            PowerManagerPlugin->Register(&mNotification);

            if (PowerManagerPlugin)
            {
                Test_PowerStateChange(PowerManagerPlugin);
                Test_TemperatureThresholds(PowerManagerPlugin);
                Test_OvertempGraceInterval(PowerManagerPlugin);
                Test_WakeupSrcConfig(PowerManagerPlugin);
                Test_PerformReboot(PowerManagerPlugin);
                Test_NetworkStandbyMode(PowerManagerPlugin);

                PowerManagerPlugin->Unregister(&mNotification);
                PowerManagerPlugin->Release();
            }
            else
            {
                TEST_LOG("PowerManagerPlugin is NULL");
            }
            mController_PowerManager->Release();
        }
        else
        {
            TEST_LOG("mController_PowerManager is NULL");
        }
    }
}

