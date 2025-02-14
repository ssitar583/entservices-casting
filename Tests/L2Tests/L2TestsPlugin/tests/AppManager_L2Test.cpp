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
#include <interfaces/IStore2.h>
#include <interfaces/ILifeCycleManager.h>
#include <interfaces/IAppManager.h>

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

/* Notification verification in tests will be disabled in Phase 1 since notification callback functionality will not be implemented.
 * Just adding the notification framework at this time; formalization is required when adding the implementation. then remove below define
 */
//#define ENABLE_NOTIFICATION_VERIFICATION

/* In phase 1, state change will not happened so,few test case it will fail, now commented the verification, once life cyle state change
 * is ready then remove this define and enable the verification.
 */
//#define ENABLE_LIFECYCLE_STATE_CHANGE_VERIFICATION

#define JSON_TIMEOUT   (1000)
#define APPMANAGER_CALLSIGN  _T("org.rdk.AppManager")
#define APPMANAGERL2TEST_CALLSIGN _T("L2tests.1")

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::IStore2;
using ::WPEFramework::Exchange::IAppManager;

MATCHER_P(MatchRequestStatusString, data, "")
{
    std::string actual = arg;
    TEST_LOG("Expected: %s, Actual: %s", data.c_str(), actual.c_str());
    EXPECT_STREQ(data.c_str(), actual.c_str());
    return data == actual;
}

MATCHER_P(MatchRequestStatusNumber, data, "")
{
    uint32_t actual = arg;
    TEST_LOG("Expected: %u, Actual: %u", data, actual);
    EXPECT_EQ(data, actual);
    return data == actual;
}

MATCHER_P(MatchRequestStatusBool, expected, "")
{
    bool actual = arg;
    std::string expected_str = expected ? "true" : "false";
    std::string actual_str = actual ? "true" : "false";
    TEST_LOG("Expected: %s, Actual: %s", expected_str.c_str(), actual_str.c_str());
    EXPECT_STREQ(expected_str.c_str(), actual_str.c_str());
    return expected == actual;
}

typedef enum : uint32_t {
    APPMANAGER_L2TEST_STATE_INVALID               = 0x00000000,
    APPMANAGER_L2TEST_ONAPPINSTALLED              = 0x00000001,
    APPMANAGER_L2TEST_ONAPPUNINSTALLED            = 0x00000002,
    APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED  = 0x00000003,
    APPMANAGER_L2TEST_ONAPPLAUNCHREQUEST          = 0x00000004,
    APPMANAGER_L2TEST_ONAPPUNLOADED               = 0x00000005
}Appmanager_L2test_Event;

class AsyncHandlerMock_AppManager
{
    public:
        AsyncHandlerMock_AppManager()
        {
        }
        MOCK_METHOD(void, onAppInstalled, (const string& appId, const string& version));
        MOCK_METHOD(void, onAppUninstalled, (const string& appId));
        MOCK_METHOD(void, onAppLifecycleStateChanged, (const string& appId, const string& appInstanceId,const Exchange::IAppManager::AppLifecycleState newState, const Exchange::IAppManager::AppLifecycleState oldState, const Exchange::IAppManager::AppErrorReason errorReason));
        MOCK_METHOD(void, onAppLaunchRequest, (const string& appId, const string& intent, const string& source));
        MOCK_METHOD(void, onAppUnloaded, (const string& appId, const string& appInstanceId));
};

class AppManagerTest : public L2TestMocks {
protected:
    virtual ~AppManagerTest() override;

    public:
    AppManagerTest();

      void onAppInstalled(const string& appId, const string& version);
      void onAppUninstalled(const string& appId);
      void onAppLifecycleStateChanged(const string& appId, const string& appInstanceId,const Exchange::IAppManager::AppLifecycleState newState, const Exchange::IAppManager::AppLifecycleState oldState, const Exchange::IAppManager::AppErrorReason errorReason);
      void onAppLaunchRequest(const string& appId, const string& intent, const string& source);
      void onAppUnloaded(const string& appId, const string& appInstanceId);

      uint32_t WaitForRequestStatus(uint32_t timeOutMs,Appmanager_L2test_Event expectedStatus);
      uint32_t CreateAppManagerInterfaceObjectUsingComRPCConnection();

    private:
        /** @brief Mutex */
        std::mutex mMutex;

        /** @brief Condition variable */
        std::condition_variable mConditionVariable;

        /** @brief Event signalled flag */
        uint32_t mEventSignalled;

    protected:
        /** @brief Pointer to the IShell interface */
        PluginHost::IShell *mControllerAppmanager;

        /** @brief Pointer to the IAppManager interface */
        Exchange::IAppManager *mAppmanagerPlugin;
};

AppManagerTest:: AppManagerTest():L2TestMocks()
{
        Core::JSONRPC::Message message;
        string response;
        uint32_t status = Core::ERROR_GENERAL;

         /* Activate plugin in constructor */
         status = ActivateService("org.rdk.PersistentStore");
         EXPECT_EQ(Core::ERROR_NONE, status);
         status = ActivateService("org.rdk.LifeCycleManager");
         EXPECT_EQ(Core::ERROR_NONE, status);
         status = ActivateService("org.rdk.AppManager");
         EXPECT_EQ(Core::ERROR_NONE, status);
}

/**
 * @brief Destructor for SystemServices L2 test class
 */
AppManagerTest::~AppManagerTest()
{
    uint32_t status = Core::ERROR_GENERAL;

    ON_CALL(*p_rBusApiImplMock, rbus_close(::testing::_ ))
    .WillByDefault(
    ::testing::Return(RBUS_ERROR_SUCCESS));

    status = DeactivateService("org.rdk.AppManager");
    EXPECT_EQ(Core::ERROR_NONE, status);

    status = DeactivateService("org.rdk.LifeCycleManager");
    EXPECT_EQ(Core::ERROR_NONE, status);

    status = DeactivateService("org.rdk.PersistentStore");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

uint32_t AppManagerTest::WaitForRequestStatus(uint32_t timeOutMs, Appmanager_L2test_Event expectedStatus)
{
    std::unique_lock<std::mutex> lock(mMutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(timeOutMs);
    uint32_t signalled = APPMANAGER_L2TEST_STATE_INVALID;

   while (!(expectedStatus & mEventSignalled))
   {
      if (mConditionVariable.wait_until(lock, now + timeout) == std::cv_status::timeout)
      {
         TEST_LOG("Timeout waiting for request status event");
         break;
      }
   }

    signalled = mEventSignalled;
    return signalled;
}

uint32_t AppManagerTest::CreateAppManagerInterfaceObjectUsingComRPCConnection()
{
    uint32_t returnValue =  Core::ERROR_GENERAL;
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> appManagerEngine;
    Core::ProxyType<RPC::CommunicatorClient> appManagerClient;

    TEST_LOG("Creating appManagerEngine");
    appManagerEngine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    appManagerClient = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(appManagerEngine));

    TEST_LOG("Creating appManagerEngine Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    appManagerEngine->Announcements(mappManagerClient->Announcement());
#endif
    if (!appManagerClient.IsValid())
    {
        TEST_LOG("Invalid appManagerClient");
    }
    else
    {
        mControllerAppmanager = appManagerClient->Open<PluginHost::IShell>(_T("org.rdk.AppManager"), ~0, 3000);
        if (mControllerAppmanager)
        {
        mAppmanagerPlugin = mControllerAppmanager->QueryInterface<Exchange::IAppManager>();
        returnValue = Core::ERROR_NONE;
        }
    }
    return returnValue;
}

void AppManagerTest::onAppInstalled(const string& appId, const string& version)
{
    TEST_LOG("onAppInstalled event triggered ***\n");
    std::unique_lock<std::mutex> lock(mMutex);

    TEST_LOG("onAppInstalled received appId: %s version: %s\n", appId.c_str(), version.c_str());

    /* Notify the requester thread. */
    mEventSignalled |= APPMANAGER_L2TEST_ONAPPINSTALLED;
    mConditionVariable.notify_one();
}

void AppManagerTest::onAppUninstalled(const string& appId)
{
    TEST_LOG("onAppUninstalled event triggered ***\n");
    std::unique_lock<std::mutex> lock(mMutex);

    TEST_LOG("onAppUninstalled received appId: %s \n", appId.c_str());

    /* Notify the requester thread. */
    mEventSignalled |= APPMANAGER_L2TEST_ONAPPUNINSTALLED;
    mConditionVariable.notify_one();
}

void AppManagerTest::onAppLifecycleStateChanged(const string& appId, const string& appInstanceId,const Exchange::IAppManager::AppLifecycleState newState, const Exchange::IAppManager::AppLifecycleState oldState, const Exchange::IAppManager::AppErrorReason errorReason)
{
    TEST_LOG("OnAppLifecycleStateChanged event triggered ***\n");
    std::unique_lock<std::mutex> lock(mMutex);

    TEST_LOG("OnAppLifecycleStateChanged received appId: %s appInstanceId: %s newState: %u oldState: %u errorReason: %u\n",
                           appId.c_str(),
                           appInstanceId.c_str(),
                           newState,
                           oldState,
                           errorReason);

    /* Notify the requester thread. */
    mEventSignalled |= APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED;
    mConditionVariable.notify_one();
}

void AppManagerTest::onAppLaunchRequest(const string& appId, const string& intent, const string& source)
{
    TEST_LOG("onAppLaunchRequest event triggered ***\n");
    std::unique_lock<std::mutex> lock(mMutex);

    TEST_LOG("onAppLaunchRequest received appId: %s intent: %s source: %s\n", appId.c_str(), intent.c_str(), source.c_str());

    /* Notify the requester thread. */
    mEventSignalled |= APPMANAGER_L2TEST_ONAPPLAUNCHREQUEST;
    mConditionVariable.notify_one();
}

void AppManagerTest::onAppUnloaded(const string& appId, const string& appInstanceId)
{
    TEST_LOG("OnAppUnloaded event triggered ***\n");
    std::unique_lock<std::mutex> lock(mMutex);

    TEST_LOG("OnAppUnloaded received appId: %s appInstanceId: %s\n", appId.c_str(), appInstanceId.c_str());

    /* Notify the requester thread. */
    mEventSignalled |= APPMANAGER_L2TEST_ONAPPUNLOADED;
    mConditionVariable.notify_one();
}

/* Begin: Test cases using JSON-RPC */

/********************************************************
************Test case Details **************************
** 1. Test SetAppProperty and GetAppProperty success scenario
** 2. Call SetAppProperty method using json-rpc with passing valid arguments and expected sucess
** 3. Call GetAppProperty method using json-rpc with passing valid arguments and expected sucess
** 4. Compare the value with expected value
*******************************************************/
TEST_F(AppManagerTest, SetAndGetAppPropertySuccessCaseUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;
    Core::JSON::String resultString;
    std::string propValue = "";
    string appId = "com.example.myapp";
    string key = "delay";
    string value = "\"10\"";

    TEST_LOG("Testing SetAppPropertyJsonRpcConnectionSuccessCase ");

    JsonObject params;
    params["appId"] = appId;
    params["key"] = key;
    params["value"] = value;
    /* Invoking setAppProperty method */
    status = InvokeServiceMethod("org.rdk.AppManager", "setAppProperty", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

    /* Invoking getAppProperty method */
    JsonObject params1;
    params1["appId"] = appId;
    params1["key"] = key;
    status = InvokeServiceMethod("org.rdk.AppManager", "getAppProperty", params1, resultString);
    EXPECT_EQ(status,Core::ERROR_NONE);

    propValue = resultString.Value();
    TEST_LOG("propValue: %s expected value: %s", propValue.c_str(), value.c_str());
    EXPECT_EQ(propValue, value);
    EXPECT_TRUE(propValue == value);
}

/********************************************************
************Test case Details **************************
** 1. Test SetAppProperty and GetAppProperty Failure scenario
** 2. Call SetAppProperty method using json-rpc with passing Empty AppId and expected core::ERROR_GENERAL
** 3. Call GetAppProperty method using json-rpc with passing Empty AppId and expected core::ERROR_GENERAL
*******************************************************/
TEST_F(AppManagerTest, SetAndGetAppPropertyEmptyAppIdUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;
    Core::JSON::String resultString;
    std::string propValue = "";
    string appId = "";
    string key = "delay";
    string value = "\"10\"";

    TEST_LOG("Testing SetAndGetAppPropertyEmptyAppIdUsingJsonRpc ");

    /* Invoking setAppProperty method */
    JsonObject params;
    params["appId"] = appId;
    params["key"] = key;
    params["value"] = value;
    status = InvokeServiceMethod("org.rdk.AppManager", "setAppProperty", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_GENERAL);

    /* Invoking getAppProperty method */
    JsonObject params1;
    params1["appId"] = appId;
    params1["key"] = key;
    status = InvokeServiceMethod("org.rdk.AppManager", "getAppProperty", params1, resultString);
    EXPECT_EQ(status,Core::ERROR_GENERAL);
}

/********************************************************
************Test case Details **************************
** 1. Test SetAppProperty and GetAppProperty Failure scenario
** 2. Call SetAppProperty method using json-rpc with passing Empty key and expected core::ERROR_GENERAL
** 3. Call GetAppProperty method using json-rpc with passing Empty key and expected core::ERROR_GENERAL
*******************************************************/
TEST_F(AppManagerTest, SetAndGetAppPropertyEmptyKeyUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;
    Core::JSON::String resultString;
    std::string propValue = "";
    string appId = "com.example.myapp";
    string key = "";
    string value = "\"10\"";

    TEST_LOG("Testing SetAndGetAppPropertyEmptyKeyUsingJsonRpc");

    /* Invoking setAppProperty method */
    JsonObject params;
    params["appId"] = appId;
    params["key"] = key;
    params["value"] = value;
    status = InvokeServiceMethod("org.rdk.AppManager", "setAppProperty", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_GENERAL);

    /* Invoking getAppProperty method */
    JsonObject params1;
    params1["appId"] = appId;
    params1["key"] = key;
    status = InvokeServiceMethod("org.rdk.AppManager", "getAppProperty", params1, resultString);
    EXPECT_EQ(status,Core::ERROR_GENERAL);
}

/********************************************************
************Test case Details **************************
** Verify PreLoad, Launch, Close App mehods sequence
** 1. Subscribe the onAppLifecycleStateChanged event
** 2. Setting mock values for the onAppLifecycleStateChanged
** 3. Call preloadApp method using json-rpc with passing valid arguments and expected success
** 4. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 5. Call launchApp method using json-rpc with passing valid arguments and expected success
** 6. Waiting for the onAppLifecycleStateChanged event to receive with Active state
** 7. Call closeApp method using json-rpc with passing valid arguments and expected success
** 8. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 9. UnSubscribe the onAppLifecycleStateChanged event
*******************************************************/
TEST_F(AppManagerTest, PreLoadLaunchAndCloseAppSuccessCaseUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_AppManager> asyncHandler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;

    TEST_LOG("Testing PreLoadLaunchAndCloseAppSuccessCaseUsingJsonRpc ");

    /* Calling PreloadApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    uint32_t signalled = APPMANAGER_L2TEST_STATE_INVALID;
    string expectedAppId = "com.example.myapp";
    string expectedAppInstanceId = "1";
    Exchange::IAppManager::AppLifecycleState expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    Exchange::IAppManager::AppLifecycleState expectedOldState = IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    Exchange::IAppManager::AppErrorReason expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    /* Subscribe onAppLifecycleStateChanged */
    status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                       _T("onAppLifecycleStateChanged"),
                                       [this, &asyncHandler](const JsonObject& parameters) {
                                           string appId = parameters["appId"].String();
                                           string appInstanceId = parameters["appInstanceId"].String();
                                           Exchange::IAppManager::AppLifecycleState newState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["newState"].Number());
                                           Exchange::IAppManager::AppLifecycleState oldState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["oldState"].Number());
                                           Exchange::IAppManager::AppErrorReason errorReason = static_cast<Exchange::IAppManager::AppErrorReason>(parameters["errorReason"].Number());
                                           asyncHandler.onAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
                                       });
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject preLoadparams;
    preLoadparams["appId"] = "com.example.myapp";
    preLoadparams["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "preloadApp", preLoadparams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

    /* Calling LaunchApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject params;
    params["appId"] = "com.example.myapp";
    params["intent"] = "start";
    params["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "launchApp", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    /* Calling CloseApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject closeParams;
    closeParams["appId"] = "com.example.myapp";
    status = InvokeServiceMethod("org.rdk.AppManager", "closeApp", closeParams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    /* Unregister for events. */
    jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onAppLifecycleStateChanged"));
    EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */
}

/********************************************************
************Test case Details **************************
** Verify PreLoad, Close and Terminate App mehods sequence
** 1. Subscribe the onAppLifecycleStateChanged event
** 2. Setting mock values for the onAppLifecycleStateChanged
** 3. Call preloadApp method using json-rpc with passing valid arguments and expected success
** 4. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 5. Call preloadApp method using json-rpc with passing valid arguments and expected success
** 6 Waiting for the onAppLifecycleStateChanged event to receive with Active state
** 7 Call terminateApp method using json-rpc with passing valid arguments and expected success
** 8 Waiting for the onAppLifecycleStateChanged event to receive with Terminated state
** 9 UnSubscribe the onAppLifecycleStateChanged event
*******************************************************/
TEST_F(AppManagerTest, PreLoadAndTerminateAppSuccessCaseUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_AppManager> asyncHandler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;

    TEST_LOG("Testing PreLoadAndTerminateAppSuccessCaseUsingJsonRpc ");

    /* Calling PreloadApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    uint32_t signalled = APPMANAGER_L2TEST_STATE_INVALID;
    string expectedAppId = "com.example.myapp";
    string expectedAppInstanceId = "1";
    Exchange::IAppManager::AppLifecycleState expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    Exchange::IAppManager::AppLifecycleState expectedOldState = IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    Exchange::IAppManager::AppErrorReason expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    /* Subscribe onAppLifecycleStateChanged */
    status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                       _T("onAppLifecycleStateChanged"),
                                       [this, &asyncHandler](const JsonObject& parameters) {
                                           string appId = parameters["appId"].String();
                                           string appInstanceId = parameters["appInstanceId"].String();
                                           Exchange::IAppManager::AppLifecycleState newState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["newState"].Number());
                                           Exchange::IAppManager::AppLifecycleState oldState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["oldState"].Number());
                                           Exchange::IAppManager::AppErrorReason errorReason = static_cast<Exchange::IAppManager::AppErrorReason>(parameters["errorReason"].Number());
                                           asyncHandler.onAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
                                       });
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject preLoadparams;
    preLoadparams["appId"] = "com.example.myapp";
    preLoadparams["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "preloadApp", preLoadparams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

    /* Calling LaunchApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject params;
    params["appId"] = "com.example.myapp";
    params["intent"] = "start";
    params["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "launchApp", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    /* Calling CloseApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject closeParams;
    closeParams["appId"] = "com.example.myapp";
    status = InvokeServiceMethod("org.rdk.AppManager", "closeApp", closeParams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

    /* Calling TerminateApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_TERMINATED;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject terminateParams;
    terminateParams["appId"] = "com.example.myapp";
    status = InvokeServiceMethod("org.rdk.AppManager", "terminateApp", terminateParams, resultJson);
#ifdef ENABLE_LIFECYCLE_STATE_CHANGE_VERIFICATION
    EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_LIFECYCLE_STATE_CHANGE_VERIFICATION */

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    /* Unregister for events. */
    jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onAppLifecycleStateChanged"));
    EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */
}

/********************************************************
************Test case Details **************************
** Verify PreLoadApp method with empty AppId
** 1. Call preloadApp method using json-rpc with passing Empty AppId and expected failiure
*******************************************************/
TEST_F(AppManagerTest, PreLoadAppEmptyAppIdUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_AppManager> asyncHandler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;

    TEST_LOG("Testing PreLoadAppEmptyAppIdUsingJsonRpc ");

    /* Calling PreloadApp */
    JsonObject preLoadparams;
    preLoadparams["appId"] = "";
    preLoadparams["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "preloadApp", preLoadparams, resultJson);
    EXPECT_EQ(status,Core::ERROR_GENERAL);
}

/********************************************************
************Test case Details **************************
** Verify LaunchApp method with empty AppId
** 1. Subscribe the onAppLifecycleStateChanged event
** 2. Setting mock values for the onAppLifecycleStateChanged
** 3. Call preloadApp method using json-rpc with passing valid arguments and expected success
** 4. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 5. Call launchApp method using json-rpc with passing Empty AppId and expected failiure
** 6. UnSubscribe the onAppLifecycleStateChanged event
*******************************************************/
TEST_F(AppManagerTest, LaunchAppEmptyAppIdUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_AppManager> asyncHandler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;

    TEST_LOG("Testing LaunchAppEmptyAppIdUsingJsonRpc ");

    /* Calling PreloadApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    uint32_t signalled = APPMANAGER_L2TEST_STATE_INVALID;
    string expectedAppId = "com.example.myapp";
    string expectedAppInstanceId = "1";
    Exchange::IAppManager::AppLifecycleState expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    Exchange::IAppManager::AppLifecycleState expectedOldState = IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    Exchange::IAppManager::AppErrorReason expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    /* Subscribe onAppLifecycleStateChanged */
    status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                       _T("onAppLifecycleStateChanged"),
                                       [this, &asyncHandler](const JsonObject& parameters) {
                                           string appId = parameters["appId"].String();
                                           string appInstanceId = parameters["appInstanceId"].String();
                                           Exchange::IAppManager::AppLifecycleState newState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["newState"].Number());
                                           Exchange::IAppManager::AppLifecycleState oldState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["oldState"].Number());
                                           Exchange::IAppManager::AppErrorReason errorReason = static_cast<Exchange::IAppManager::AppErrorReason>(parameters["errorReason"].Number());
                                           asyncHandler.onAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
                                       });
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject preLoadparams;
    preLoadparams["appId"] = "com.example.myapp";
    preLoadparams["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "preloadApp", preLoadparams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

    /* Calling LaunchApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject params;
    params["appId"] = "";
    params["intent"] = "start";
    params["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "launchApp", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_GENERAL);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    /* Unregister for events. */
    jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onAppLifecycleStateChanged"));
    EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */
}

/********************************************************
************Test case Details **************************
** Verify Close App with empty AppId
** 1. Subscribe the onAppLifecycleStateChanged event
** 2. Setting mock values for the onAppLifecycleStateChanged
** 3. Call preloadApp method using json-rpc with passing valid arguments and expected success
** 4. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 5. Call launchApp method using json-rpc with passing valid arguments and expected success
** 6. Waiting for the onAppLifecycleStateChanged event to receive with Active state
** 7. Call closeApp method using json-rpc with passing Empty AppId and expected failiure
** 8. UnSubscribe the onAppLifecycleStateChanged event
*******************************************************/
TEST_F(AppManagerTest, CloseAppEmptyAppIdUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_AppManager> asyncHandler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;

    TEST_LOG("Testing CloseAppEmptyAppIdUsingJsonRpc ");

    /* Calling PreloadApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    uint32_t signalled = APPMANAGER_L2TEST_STATE_INVALID;
    string expectedAppId = "com.example.myapp";
    string expectedAppInstanceId = "1";
    Exchange::IAppManager::AppLifecycleState expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    Exchange::IAppManager::AppLifecycleState expectedOldState = IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    Exchange::IAppManager::AppErrorReason expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    /* Subscribe onAppLifecycleStateChanged */
    status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                       _T("onAppLifecycleStateChanged"),
                                       [this, &asyncHandler](const JsonObject& parameters) {
                                           string appId = parameters["appId"].String();
                                           string appInstanceId = parameters["appInstanceId"].String();
                                           Exchange::IAppManager::AppLifecycleState newState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["newState"].Number());
                                           Exchange::IAppManager::AppLifecycleState oldState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["oldState"].Number());
                                           Exchange::IAppManager::AppErrorReason errorReason = static_cast<Exchange::IAppManager::AppErrorReason>(parameters["errorReason"].Number());
                                           asyncHandler.onAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
                                       });
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject preLoadparams;
    preLoadparams["appId"] = "com.example.myapp";
    preLoadparams["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "preloadApp", preLoadparams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

    /* Calling LaunchApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject params;
    params["appId"] = "com.example.myapp";
    params["intent"] = "start";
    params["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "launchApp", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    /* Calling CloseApp */
    JsonObject closeParams;
    closeParams["appId"] = "";
    status = InvokeServiceMethod("org.rdk.AppManager", "closeApp", closeParams, resultJson);
    EXPECT_EQ(status,Core::ERROR_GENERAL);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    /* Unregister for events. */
    jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onAppLifecycleStateChanged"));
    EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */
}

/********************************************************
************Test case Details **************************
** Verify TerminateApp with empty AppId
** 1. Subscribe the onAppLifecycleStateChanged event
** 2. Setting mock values for the onAppLifecycleStateChanged
** 3. Call preloadApp method using json-rpc with passing valid arguments and expected success
** 4. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 5. Call launchApp method using json-rpc with passing valid arguments and expected success
** 6. Waiting for the onAppLifecycleStateChanged event to receive with Active state
** 7. Call closeApp method using json-rpc with passing valid arguments and expected success
** 8. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 9. Call terminateApp method using json-rpc with passing Empty AppId and expected failiure
** 10. UnSubscribe the onAppLifecycleStateChanged event
*******************************************************/
TEST_F(AppManagerTest, TerminateAppEmptyAppIdUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_AppManager> asyncHandler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;

    TEST_LOG("Testing TerminateAppEmptyAppIdUsingJsonRpc ");

    /* Calling PreloadApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    uint32_t signalled = APPMANAGER_L2TEST_STATE_INVALID;
    string expectedAppId = "com.example.myapp";
    string expectedAppInstanceId = "1";
    Exchange::IAppManager::AppLifecycleState expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    Exchange::IAppManager::AppLifecycleState expectedOldState = IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    Exchange::IAppManager::AppErrorReason expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    /* Subscribe onAppLifecycleStateChanged */
    status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                       _T("onAppLifecycleStateChanged"),
                                       [this, &asyncHandler](const JsonObject& parameters) {
                                           string appId = parameters["appId"].String();
                                           string appInstanceId = parameters["appInstanceId"].String();
                                           Exchange::IAppManager::AppLifecycleState newState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["newState"].Number());
                                           Exchange::IAppManager::AppLifecycleState oldState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["oldState"].Number());
                                           Exchange::IAppManager::AppErrorReason errorReason = static_cast<Exchange::IAppManager::AppErrorReason>(parameters["errorReason"].Number());
                                           asyncHandler.onAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
                                       });
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject preLoadparams;
    preLoadparams["appId"] = "com.example.myapp";
    preLoadparams["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "preloadApp", preLoadparams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    /* Calling TerminateApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_TERMINATED;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject terminateParams;
    terminateParams["appId"] = "";
    status = InvokeServiceMethod("org.rdk.AppManager", "terminateApp", terminateParams, resultJson);
    EXPECT_EQ(status,Core::ERROR_GENERAL);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    /* Unregister for events. */
    jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onAppLifecycleStateChanged"));
    EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */
}

/********************************************************
************Test case Details **************************
** Verify PreLoad, Launch, Close mehods sequence
** 1. Subscribe the onAppLifecycleStateChanged event
** 2. Setting mock values for the onAppLifecycleStateChanged
** 3. Call preloadApp method using json-rpc with passing valid arguments and expected success
** 4. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 5. Call launchApp method using json-rpc with passing valid arguments and expected success
** 6. Waiting for the onAppLifecycleStateChanged event to receive with Active state
** 7. Call closeApp method using json-rpc with passing valid arguments and expected success
** 8. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 9. UnSubscribe the onAppLifecycleStateChanged event
*******************************************************/
TEST_F(AppManagerTest, PreLoadLaunchAppSuccessCaseUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_AppManager> asyncHandler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;

    TEST_LOG("Testing PreLoadLaunchAppSuccessCaseUsingJsonRpc ");

    /* Calling PreloadApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    uint32_t signalled = APPMANAGER_L2TEST_STATE_INVALID;
    string expectedAppId = "com.example.myapp";
    string expectedAppInstanceId = "1";
    Exchange::IAppManager::AppLifecycleState expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    Exchange::IAppManager::AppLifecycleState expectedOldState = IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    Exchange::IAppManager::AppErrorReason expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    /* Subscribe onAppLifecycleStateChanged */
    status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                       _T("onAppLifecycleStateChanged"),
                                       [this, &asyncHandler](const JsonObject& parameters) {
                                           string appId = parameters["appId"].String();
                                           string appInstanceId = parameters["appInstanceId"].String();
                                           Exchange::IAppManager::AppLifecycleState newState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["newState"].Number());
                                           Exchange::IAppManager::AppLifecycleState oldState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["oldState"].Number());
                                           Exchange::IAppManager::AppErrorReason errorReason = static_cast<Exchange::IAppManager::AppErrorReason>(parameters["errorReason"].Number());
                                           asyncHandler.onAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
                                       });
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject preLoadparams;
    preLoadparams["appId"] = "com.example.myapp";
    preLoadparams["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "preloadApp", preLoadparams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

    /* Calling LaunchApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject params;
    params["appId"] = "com.example.myapp";
    params["intent"] = "start";
    params["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "launchApp", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    /* Calling CloseApp */
#ifdef ENABLE_NOTIFICATION_VERIFICATION
    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject closeParams;
    closeParams["appId"] = "com.example.myapp";
    status = InvokeServiceMethod("org.rdk.AppManager", "closeApp", closeParams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    /* Unregister for events. */
    jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onAppLifecycleStateChanged"));
    EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */
}

/********************************************************
************Test case Details **************************
** Verify PreLoad, Launch Apps methods and call GetLoadedApps method sequence
** 1. Subscribe the onAppLifecycleStateChanged event
** 2. Setting mock values for the onAppLifecycleStateChanged
** 3. Call preloadApp method using json-rpc with passing valid arguments and expected success
** 4. Waiting for the onAppLifecycleStateChanged event to receive with Running state
** 5. Call launchApp method using json-rpc with passing valid arguments and expected success
** 6. Waiting for the onAppLifecycleStateChanged event to receive with Active state
** 7. Call GetLoadedApps method using json-rpc and expected success and get the loaded Apps
** 9. UnSubscribe the onAppLifecycleStateChanged event
*******************************************************/
TEST_F(AppManagerTest, GetLoadedAppsSuccessCaseUsingJsonRpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(APPMANAGER_CALLSIGN, APPMANAGERL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_AppManager> asyncHandler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject resultJson;
    Core::JSON::String resultString;
    string apps = "";
    string expectedApps = "\"[{\\\"appId\\\":\\\"com.example.myapp\\\",\\\"type\\\":1,\\\"lifecycleState\\\":0,\\\"targetLifecycleState\\\":2,\\\"activeSessionId\\\":\\\"\\\",\\\"appInstanceId\\\":\\\"\\\"}]\"";

    TEST_LOG("Testing GetLoadedAppsSuccessCaseUsingJsonRpc ");

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    uint32_t signalled = APPMANAGER_L2TEST_STATE_INVALID;
    string expectedAppId = "com.example.myapp";
    string expectedAppInstanceId = "1";
    Exchange::IAppManager::AppLifecycleState expectedNewState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    Exchange::IAppManager::AppLifecycleState expectedOldState = IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    Exchange::IAppManager::AppErrorReason expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    /* Subscribe onAppLifecycleStateChanged */
    status = jsonrpc.Subscribe<JsonObject>(JSON_TIMEOUT,
                                       _T("onAppLifecycleStateChanged"),
                                       [this, &asyncHandler](const JsonObject& parameters) {
                                           string appId = parameters["appId"].String();
                                           string appInstanceId = parameters["appInstanceId"].String();
                                           Exchange::IAppManager::AppLifecycleState newState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["newState"].Number());
                                           Exchange::IAppManager::AppLifecycleState oldState = static_cast<Exchange::IAppManager::AppLifecycleState>(parameters["oldState"].Number());
                                           Exchange::IAppManager::AppErrorReason errorReason = static_cast<Exchange::IAppManager::AppErrorReason>(parameters["errorReason"].Number());
                                           asyncHandler.onAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
                                       });
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    /* Calling PreloadApp with appId: com.example.myapp*/
    JsonObject preLoadparams;
    preLoadparams["appId"] = "com.example.myapp";
    preLoadparams["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "preloadApp", preLoadparams, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);

    expectedNewState = IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedOldState = IAppManager::AppLifecycleState::APP_STATE_RUNNING;
    expectedErrorReason = IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    EXPECT_CALL(asyncHandler, onAppLifecycleStateChanged(MatchRequestStatusString(expectedAppId),
                                                          MatchRequestStatusString(expectedAppInstanceId),
                                                          MatchRequestStatusNumber(expectedNewState),
                                                          MatchRequestStatusNumber(expectedOldState),
                                                          MatchRequestStatusNumber(expectedErrorReason)))
    .WillOnce(Invoke(this, &AppManagerTest::onAppLifecycleStateChanged));
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    /* Calling LaunchApp */
    JsonObject params;
    params["appId"] = "com.example.myapp";
    params["intent"] = "start";
    params["launchArgs"] = "{\"key1\":\"38\"}";
    status = InvokeServiceMethod("org.rdk.AppManager", "launchApp", params, resultJson);
    EXPECT_EQ(status,Core::ERROR_NONE);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    signalled = WaitForRequestStatus(JSON_TIMEOUT,APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
    EXPECT_TRUE(signalled & APPMANAGER_L2TEST_ONAPPLIFECYCLESTATECHANGED);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */

    JsonObject params1;
    /* Calling GetLoadedApps */
    status = InvokeServiceMethod("org.rdk.AppManager", "getLoadedApps", params1, resultString);
    EXPECT_EQ(status,Core::ERROR_NONE);

    resultString.ToString(apps);
    TEST_LOG("apps: %s", apps.c_str());
    EXPECT_EQ(apps, expectedApps);
    EXPECT_TRUE(apps == expectedApps);

#ifdef ENABLE_NOTIFICATION_VERIFICATION
    /* Unregister for events. */
    jsonrpc.Unsubscribe(JSON_TIMEOUT, _T("onAppLifecycleStateChanged"));
    EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_NOTIFICATION_VERIFICATION */
}
/* End: Test cases using JSON-RPC */

/* Begin: Test cases using COM-RPC */

/********************************************************
************Test case Details **************************
** Verify Set and Get App property methods
** 1. Create COM-RPC connection
** 3. Call setAppProperty method with passing valid arguments and expected sucess
** 3. Call getAppProperty method with passing valid arguments and expected sucess
** 4. Compare with result value with expected value
** 5. Release interface object and COM-RPC connection
*******************************************************/
TEST_F(AppManagerTest, SetAndGetAppPropertyComRpcConnectionSuccessCase)
{
    uint32_t status = Core::ERROR_GENERAL;

    if (CreateAppManagerInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid appManagerClient");
    }
    else
    {
        ASSERT_TRUE(mControllerAppmanager!= nullptr);
        if (mControllerAppmanager)
        {
            ASSERT_TRUE(mAppmanagerPlugin!= nullptr);
            if (mAppmanagerPlugin)
            {
                mAppmanagerPlugin->AddRef();

                string appId = "com.example.myapp";
                string key = "delay";
                string value = "\"10\""; /* string as json string format */
                string expectedValue = "\"10\"";

                TEST_LOG("Calling SetAppProperty");
                status = mAppmanagerPlugin->SetAppProperty(appId, key, value);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                TEST_LOG("Calling GetAppProperty");
                value = "";
                status = mAppmanagerPlugin->GetAppProperty(appId, key, value);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }
                TEST_LOG("value: %s expectedValue:%s", value.c_str(), expectedValue.c_str());

                EXPECT_EQ(value, expectedValue);

                mAppmanagerPlugin->Release();
            }
            else
            {
                TEST_LOG("mAppmanagerPlugin is NULL");
            }
            mControllerAppmanager->Release();
        }
        else
        {
            TEST_LOG("mControllerAppmanager is NULL");
        }
    }
}

/********************************************************
************Test case Details **************************
** Verify PreLoad, Launch, Close and Termaite App mehods sequence
** 1. Create COM-RPC connection
** 2. Call preloadApp method passing valid arguments and expect sucess
** 3. Call launchApp method passing valid arguments and expect sucess
** 4. Call closeApp method passing valid arguments and expect sucess
** 5. Call terminateApp method passing valid arguments and expect sucess
** 6. Release interface object and COM-RPC connection
*******************************************************/
TEST_F(AppManagerTest, PreLoadLaunchCloseAndTerminateAppComRpcConnectionSuccessCase)
{
    uint32_t status = Core::ERROR_GENERAL;

    if (CreateAppManagerInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid appManagerClient");
    }
    else
    {
        ASSERT_TRUE(mControllerAppmanager!= nullptr);
        if (mControllerAppmanager)
        {
            ASSERT_TRUE(mAppmanagerPlugin!= nullptr);
            if (mAppmanagerPlugin)
            {
                mAppmanagerPlugin->AddRef();

                string appId = "com.example.myapp";
                string launchArgs = "{\"key1\":\"38\"}";
                string error = "";
                string value = "\"10\""; /* string as json string format */
                string expectedValue = "\"10\"";

                TEST_LOG("Calling PreloadApp");
                status = mAppmanagerPlugin->PreloadApp(appId, launchArgs, error);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                string intent = "start";
                TEST_LOG("Calling LaunchApp");
                value = "";
                status = mAppmanagerPlugin->LaunchApp(appId, intent, launchArgs);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                TEST_LOG("Calling GetLoadedApps");
                string appData = "";
                string expectedApps = "[{\"appId\":\"com.example.myapp\",\"type\":1,\"lifecycleState\":0,\"targetLifecycleState\":2,\"activeSessionId\":\"\",\"appInstanceId\":\"\"}]";
                status = mAppmanagerPlugin->GetLoadedApps(appData);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                TEST_LOG("appData: %s expectedApps:%s", appData.c_str(), expectedApps.c_str());

                EXPECT_EQ(appData, expectedApps);

                TEST_LOG("Calling CloseApp");
                value = "";
                status = mAppmanagerPlugin->CloseApp(appId);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                TEST_LOG("Calling TerminateApp");
                value = "";
                status = mAppmanagerPlugin->TerminateApp(appId);
#ifdef ENABLE_LIFECYCLE_STATE_CHANGE_VERIFICATION
                EXPECT_EQ(status,Core::ERROR_NONE);
#endif /* ENABLE_LIFECYCLE_STATE_CHANGE_VERIFICATION */
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                mAppmanagerPlugin->Release();
            }
            else
            {
                TEST_LOG("mAppmanagerPlugin is NULL");
            }
            mControllerAppmanager->Release();
        }
        else
        {
            TEST_LOG("mControllerAppmanager is NULL");
        }
    }
}

/********************************************************
************Test case Details **************************
** Verify PreLoad, Launch, Close App mehods sequence
** 1. Create COM-RPC connection
** 3. Call preloadApp method passing valid arguments and expect sucess
** 3. Call launchApp method passing valid arguments and expect sucess
** 4. Call GetLoadedApps method and expect success along with app data
** 5. Call closeApp method passing valid arguments and expect sucess
** 6. Call GetLoadedApps method and expect success along with empty app data
** 7. Release interface object and COM-RPC connection
*******************************************************/
TEST_F(AppManagerTest, PreLoadLaunchCloseAppComRpcConnectionSuccessCase)
{
    uint32_t status = Core::ERROR_GENERAL;

    if (CreateAppManagerInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE)
    {
        TEST_LOG("Invalid appManagerClient");
    }
    else
    {
        ASSERT_TRUE(mControllerAppmanager!= nullptr);
        if (mControllerAppmanager)
        {
            ASSERT_TRUE(mAppmanagerPlugin!= nullptr);
            if (mAppmanagerPlugin)
            {
                mAppmanagerPlugin->AddRef();

                string appId = "com.example.myapp";
                string launchArgs = "{\"key1\":\"38\"}";
                string error = "";
                string value = "\"10\""; /* string as json string format */
                string expectedValue = "\"10\"";

                TEST_LOG("Calling PreloadApp");
                status = mAppmanagerPlugin->PreloadApp(appId, launchArgs, error);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                string intent = "start";
                TEST_LOG("Calling LaunchApp");
                value = "";
                status = mAppmanagerPlugin->LaunchApp(appId, intent, launchArgs);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                TEST_LOG("Calling GetLoadedApps");
                string appDataAfterLaunch = "";
                string expectedAppData = "[{\"appId\":\"com.example.myapp\",\"type\":1,\"lifecycleState\":0,\"targetLifecycleState\":2,\"activeSessionId\":\"\",\"appInstanceId\":\"\"}]";
                status = mAppmanagerPlugin->GetLoadedApps(appDataAfterLaunch);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                TEST_LOG("appDataAfterLaunch: %s expectedAppData:%s", appDataAfterLaunch.c_str(), expectedAppData.c_str());

                EXPECT_EQ(appDataAfterLaunch, expectedAppData);

                TEST_LOG("Calling CloseApp");
                value = "";
                status = mAppmanagerPlugin->CloseApp(appId);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                TEST_LOG("Calling GetLoadedApps");
                string appDataAfterClose = "";
                string expectedAppDataAfterClose = "[{\"appId\":\"com.example.myapp\",\"type\":1,\"lifecycleState\":0,\"targetLifecycleState\":3,\"activeSessionId\":\"\",\"appInstanceId\":\"\"}]";
                status = mAppmanagerPlugin->GetLoadedApps(appDataAfterClose);
                EXPECT_EQ(status,Core::ERROR_NONE);
                if (status != Core::ERROR_NONE)
                {
                    std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
                    TEST_LOG("Err: %s", errorMsg.c_str());
                }

                TEST_LOG("appDataAfterClose: %s expectedAppDataAfterClose:%s", appDataAfterClose.c_str(), expectedAppDataAfterClose.c_str());

                EXPECT_EQ(appDataAfterClose, expectedAppDataAfterClose);

                mAppmanagerPlugin->Release();
            }
            else
            {
                TEST_LOG("mAppmanagerPlugin is NULL");
            }
            mControllerAppmanager->Release();
        }
        else
        {
            TEST_LOG("mControllerAppmanager is NULL");
        }
    }
}

/* End: Test cases using COM-RPC */


