#pragma once

// std inc
#include <gmock/gmock.h>

// interface inc
#include <interfaces/IPowerManager.h>

// service / internal inc
#include "PluginInterfaceBuilder.h"

using ::testing::NiceMock;

namespace Core = WPEFramework::Core;
namespace Plugin = WPEFramework::Plugin;
namespace Exchange = WPEFramework::Exchange;

class PowerManagerMock : public WPEFramework::Exchange::IPowerManager {

public:
    MOCK_METHOD(uint32_t, Register, (Exchange::IPowerManager::INotification * notification), (override));
    MOCK_METHOD(uint32_t, Unregister, (Exchange::IPowerManager::INotification * notification), (override));

    MOCK_METHOD(uint32_t, SetPowerState, (const int& keyCode, const PowerState& powerState, const string& reason), (override));
    MOCK_METHOD(uint32_t, GetPowerState, (PowerState & currentState, PowerState& previousState), (override, const));
    MOCK_METHOD(uint32_t, GetThermalState, (float& currentTemperature), (override, const));
    MOCK_METHOD(uint32_t, SetTemperatureThresholds, (float high, float critical), (override));
    MOCK_METHOD(uint32_t, GetTemperatureThresholds, (float& high, float& critical), (override, const));
    MOCK_METHOD(uint32_t, SetOvertempGraceInterval, (const int graceInterval), (override));
    MOCK_METHOD(uint32_t, GetOvertempGraceInterval, (int& graceInterval), (override, const));
    MOCK_METHOD(uint32_t, SetDeepSleepTimer, (const int timeOut), (override));
    MOCK_METHOD(uint32_t, GetLastWakeupReason, (WakeupReason & wakeupReason), (override, const));
    MOCK_METHOD(uint32_t, GetLastWakeupKeyCode, (int& keycode), (override, const));
    MOCK_METHOD(uint32_t, Reboot, (const string& rebootRequestor, const string& rebootReasonCustom, const string& rebootReasonOther), (override));
    MOCK_METHOD(uint32_t, SetNetworkStandbyMode, (const bool& standbyMode), (override));
    MOCK_METHOD(uint32_t, GetNetworkStandbyMode, (bool& standbyMode), (override));
    MOCK_METHOD(uint32_t, SetWakeupSrcConfig, (const int& powerMode, const int& wakeSrcType, int config), (override));
    MOCK_METHOD(uint32_t, GetWakeupSrcConfig, (int& powerMode, int& srcType, int& config), (override, const));
    MOCK_METHOD(uint32_t, SetSystemMode, (const SystemMode& currentMode, const SystemMode& newMode), (override, const));
    MOCK_METHOD(uint32_t, GetPowerStateBeforeReboot, (PowerState & powerStateBeforeReboot), (override));

    BEGIN_INTERFACE_MAP(PowerManagerMock)
    INTERFACE_ENTRY(Exchange::IPowerManager)
    END_INTERFACE_MAP

    static std::map<std::string, Core::ProxyType<Exchange::IPowerManager>>& mockInstances()
    {
        static std::map<std::string, Core::ProxyType<Exchange::IPowerManager>> mocks;
        return mocks;
    }

    static std::string testId()
    {
        auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        if (!testInfo) {
            return {};
        }

        // unique std::string {testClass#testName}
        return std::string(testInfo->test_suite_name()) + "#" + std::string(testInfo->name());
    }

    static Exchange::IPowerManager* Get()
    {
        std::string id = testId();

        // ASSERT_FALSE(id.empty()) << "testId should have been valid for all testcases";
        ASSERT(!id.empty());

        auto& mocks = mockInstances();

        auto it = mocks.find(id);
        if (it == mocks.end()) {
            // create new
            mocks.insert(std::pair<std::string, Core::ProxyType<PowerManagerMock>>(id, Core::ProxyType<PowerManagerMock>::Create()));
            it = mocks.find(id);
            ASSERT(it != mocks.end());
        }

        return &(*(it->second));
    }

    static PowerManagerMock& Mock()
    {
        /*PowerManagerMock *mock = dynamic_cast<PowerManagerMock *>(Get());*/
        // TODO: avoid static_cast
        PowerManagerMock* mock = static_cast<PowerManagerMock*>(Get());
        /*ASSERT_NE(mock, nullptr);*/
        return *mock;
    }

    static void Delete(void)
    {
        std::string id = testId();

        ASSERT_FALSE(id.empty()) << "testId should have been valid for all testcases";

        auto& mocks = mockInstances();

        auto it = mocks.find(id);
        if (it != mocks.end()) {
            // ProxyType<> Release gets called on destructor
            mocks.erase(it);
        }
    }
};

