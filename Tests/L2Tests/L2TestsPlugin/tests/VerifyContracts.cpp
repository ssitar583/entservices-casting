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
#include <vector>
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <fstream>
#include <iostream>
#include "Network.h"
#include <time.h>
#include <chrono>
#include <string>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>

#define JSON_TIMEOUT   (1000)
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using namespace std;

/* Contract test verification test class declaration */
class CTVerifier_Test : public L2TestMocks {
protected:
    IARM_EventHandler_t dsHdmiStatusEventHandler;
    IARM_EventHandler_t dsHdmiVideoModeEventHandler;
    std::thread runFlaskThread;
    std::mutex mtx;
    std::atomic<bool> runningFlaskThread{true};  // Atomic flag for controlling the loop
    CTVerifier_Test();
    virtual ~CTVerifier_Test() override;

protected:
    void SetUp() override {
    }

    void TearDown() override {
        runningFlaskThread = false;  // Stop the thread loop
        if (runFlaskThread.joinable()) {
            runFlaskThread.join();  // Wait for the thread to finish
        }
    }

public:
    void createDeviceFiles();
    void enableTTS();
    void activateAllPlugins();
    void deactivateAllPlugins();
    bool parseContractTestResults(string consumer, string& git_hash_str, int duration);
    void runPactVerifyCmd(string consumer, string& provider_version);
    static void RunFlaskServer(void);
    void shutdownFlaskServer();
    // Add all supported plugins here
    vector<string> vtrPlugins = {
        "org.rdk.HdcpProfile", "org.rdk.Network", "org.rdk.System",
        "org.rdk.PersistentStore", "org.rdk.TextToSpeech", "org.rdk.UserSettings",
        "org.rdk.Bluetooth", "org.rdk.AVInput", "org.rdk.AVOutput", "org.rdk.DisplaySettings"};
};

/**
 * @brief Constructor for CTVerifier_Test class
 */
CTVerifier_Test::CTVerifier_Test()
        : L2TestMocks()
{
}

/**
 * @brief Destructor for CTVerifier_Test class
 */
CTVerifier_Test::~CTVerifier_Test()
{
}

void CTVerifier_Test::activateAllPlugins()
{
    createDeviceFiles();
    /* Activate all plugins */
    for (vector<string>::iterator i = vtrPlugins.begin(); i != vtrPlugins.end(); i++)
    {
        cout << "Activating plugin: " << *i  << endl;
        uint32_t status = ActivateService((*i).c_str());
        EXPECT_EQ(Core::ERROR_NONE, status);
    }
    enableTTS();
}

void CTVerifier_Test::deactivateAllPlugins()
{
    /* Deactivate all plugins */
    for (vector<string>::iterator i = vtrPlugins.begin(); i != vtrPlugins.end(); i++)
    {
        cout << "Deactivating plugin: " << *i  << endl;
        uint32_t status = DeactivateService((*i).c_str());
        //EXPECT_EQ(Core::ERROR_NONE, status);
    }
}

/* Contract test verification test class declaration */
class CTVerifier_CallMocks : public CTVerifier_Test {

protected:
    CTVerifier_CallMocks()
    {
    }
    virtual ~CTVerifier_CallMocks() override
    {
    }
};

/**
 * @brief Create device properties and version files needed by various plugins
 */
void CTVerifier_Test::createDeviceFiles()
{
    std::ofstream deviceProperties("/etc/device.properties");
    deviceProperties << "BUILD_TYPE=dev\n";
    deviceProperties << "FORCE_MTLS=true\n";
    deviceProperties << "DEVICE_NAME=PLATCO\nDEVICE_TYPE=mediaclient\nMFG_NAME=Element\nWIFI_SUPPORT=true\n";
    deviceProperties << "MOCA_INTERFACE=true\nWIFI_INTERFACE=wlan0\nMOCA_INTERFACE=eth0\nETHERNET_INTERFACE=eth0\n";
    deviceProperties << "RDK_PROFILE=TV";
    deviceProperties.close();

    std::ofstream version("/version.txt");
    version << "imagename:XUSHTC11MWR_VBN_2401_sprint_20240211231329sdy_NG\n";
    version << "VERSION=7.0.0.0\nBUILD_TIME=2024-02-11 23:13:29\n";
    version.close();
}

/**
 * @brief Enable TTS with dummy configuration
 */
void CTVerifier_Test::enableTTS()
{
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject result;
    JsonObject params_ttsen;
    JsonObject enableTTS;
    bool enable = true;
    enableTTS["enabletts"] = JsonValue((bool)enable);
    status = InvokeServiceMethod("org.rdk.TextToSpeech.1", "enabletts", enableTTS, result);
    status = InvokeServiceMethod("org.rdk.TextToSpeech.1", "isttsenabled", params_ttsen, result);

    JsonObject setTTSConfig;
    setTTSConfig["language"] = "en-US";
    setTTSConfig["voice"] = "carol";
    setTTSConfig["ttsendpoint"] = "https://sky-tts-run-api-gateway-3m02gblb.ew.gateway.dev/tts";
    setTTSConfig["ttsendpointsecured"] = "https://sky-tts-run-api-gateway-3m02gblb.ew.gateway.dev/tts";
    setTTSConfig["volume"] = "95";
    setTTSConfig["primvolduckpercent"] = "50";
    setTTSConfig["rate"] = "50";
    setTTSConfig["speechrate"] = "medium";
    status = InvokeServiceMethod("org.rdk.TextToSpeech.1", "setttsconfiguration", setTTSConfig, result);
    status = InvokeServiceMethod("org.rdk.TextToSpeech.1", "getttsconfiguration", params_ttsen, result);
}

class CTVerifierMain : public CTVerifier_CallMocks {
};

/********************************************************
************Test case Details **************************
** 1. Run verify contracts using the pact_verifier_cli
*******************************************************/

TEST_F(CTVerifierMain, VerifyContractsOnAllPlugins)
{
    char const *pact_token = getenv("PACTFLOW_TOKEN");
    string pact_token_str = "";
    if (pact_token == NULL)
    {
        std::cout << "PACTFLOW_TOKEN is NULL\n";
    }
    else
    {
        std::string s(pact_token);
        pact_token_str = s;
    }

    if(pact_token_str.empty())
    {
        cout << "PACTFLOW_TOKEN is empty, don't run contract tests\n";
        return;
    }
    // Start the thread that will start python flask service
    runFlaskThread = std::thread(RunFlaskServer);

    #include "MockCallSetup.h"

    activateAllPlugins();

    //get short hash from long hash in GITHUB_SHA
    string git_hash_str = "";
    char const *l_hash = getenv("GITHUB_SHA");
    if (l_hash == NULL)
    {
        cout << "GITHUB_SHA is NULL\n";
    }
    else
    {
        cout << "GITHUB_SHA: " << l_hash << "\n";
        std::string s(l_hash);
        git_hash_str = s.substr(0, 7);
    }
    cout << "git_hash: " << git_hash_str << "\n";

    // //sleep for 10 seconds
    // while(1)
    // {
    //     cout << "Sleeping for 10 seconds\n";
    //     sleep(10);
    // }

    string test_pact_cmd = "~/bin/pact_verifier_cli --version";
    system(test_pact_cmd.c_str());
    std::remove("contract-test-statistics.txt");

    //*******************Ripple********************* */
    string provider_version = "0.1.0-" + git_hash_str;
    runPactVerifyCmd("ripple", provider_version);

    //*******************Immerse-UI********************* */
    runPactVerifyCmd("immerse-ui", provider_version);

    //*******************netflix-native-app********************* */
    runPactVerifyCmd("netflix-native-app", provider_version);

    shutdownFlaskServer();  // Shutdown flask thread
    deactivateAllPlugins();
}

void CTVerifier_Test::runPactVerifyCmd(string consumer, string& provider_version)
{
    char const *pact_token = getenv("PACTFLOW_TOKEN");
    std::string pact_token_str(pact_token);

    string pact_verify_cmd = "~/bin/pact_verifier_cli ";
	pact_verify_cmd += "--provider-version=" + provider_version + " --provider-branch=test-github-rdke ";
    pact_verify_cmd += "--loglevel=info --provider-name=rdk_service ";
    pact_verify_cmd += "--broker-url=https://skyai.pactflow.io --token=" + pact_token_str + " ";
    pact_verify_cmd += "--hostname=127.0.0.1 --port=9998 ";
    pact_verify_cmd += "--state-change-url=http://127.0.0.1:5003/ ";

    if(consumer == "ripple")
    {
        pact_verify_cmd += "--transport=websocket ";
        pact_verify_cmd += "--filter-consumer ripple ";
    }
    else if(consumer == "netflix-native-app")
    {
        pact_verify_cmd += "--transport=http ";
        pact_verify_cmd += "--filter-consumer netflix-native-app ";
    }
    else if(consumer == "immerse-ui")
    {
        pact_verify_cmd += "--transport=http ";
        pact_verify_cmd += "--filter-consumer immerse-ui ";
    }
    else
    {
        cout << "Invalid consumer\n";
        return;
    }
    pact_verify_cmd += "--publish ";
    std::string result_file = "contract-test-results-" + consumer + ".json";
    pact_verify_cmd += "--json " + result_file + " ";
    pact_verify_cmd += "--full-log ";
    // pact_verify_cmd += R"(--consumer-version-selectors="{\"tag\": \"20.297.000.00-1292e575\", \"latest\": true}" )";
    pact_verify_cmd += R"(--consumer-version-selectors="{mainBranch: true}" )";

    // Record the start time
    auto start_time = std::chrono::high_resolution_clock::now();

    cout << "pact_verify_cmd: " << pact_verify_cmd << "\n";
    int stat = system(pact_verify_cmd.c_str());
    cout << "pact_verify_cmd stat: " << stat << "\n";
    //EXPECT_GE(stat, 0);

    // Record the end time
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the duration
    std::chrono::duration<double> duration = end_time - start_time;

    //Parse the contract-test-results.json file and check if all the tests passed
    parseContractTestResults(consumer, provider_version, duration.count());
}

/*
 * @brief Parse the contract-test-results.json file and write the results to contract-test-statistics.txt
 */
bool CTVerifier_Test::parseContractTestResults(string consumer, string& provider_version, int duration)
{
    //Read json file to str::string
    string result_file = "contract-test-results-" + consumer + ".json";
    std::ifstream t(result_file);
    //If file doesn't exist, return
    if (!t.is_open())
    {
        std::cout << result_file + " file not found\n";
        return false;
    }
    std::string input((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    // std::cout << "input: " << input << std::endl;

    JsonObject parameters; 
    parameters.FromString(input);
    std::string temp;
    parameters.ToString(temp);
    // std::cout << "parameters: " << temp << std::endl;
    JsonArray err_arr = parameters["errors"].Array();
    uint32_t failed_apis = err_arr.Length();
    std::cout << "failed_apis: " << err_arr.Length() << std::endl;

    //Find number of occurences of "" in a string input
    size_t pos = 0;
    int count = 0;
    std::string search_str = "\"\"";
    JsonArray output_arr = parameters["output"].Array();
    for (uint32_t i = 0; i < output_arr.Length(); i++)
    {
        string output_str = output_arr[i].String();
        // std::cout << "output_str: " << output_str << std::endl;
        //check if the stirng is empty
        if (output_str.empty())
            count++;
    }
    //Reduce count by 1 since the first element after header is empty
    if(count > 0)
        count--;

    //Find number of occurences of "Test Name:" in a string input
    // std::string search_str = "Test Name:";
    // while ((pos = input.find(search_str, pos)) != std::string::npos) {
    //     count++;
    //     pos += search_str.length();
    // }
    std::cout << "Total Interactions: " << count << std::endl;

    //Get current date & time to a string
    time_t now = time(0);
    string dt = ctime(&now);
    cout << "The local date and time is: " << dt << endl;

    //Write test results in to a file
    std::ofstream test_results("contract-test-statistics.txt", std::ios::app);
    test_results << "********************Contract Test Results********************" << "\n";
    test_results << "Date & Time: " << dt << "\n";
    test_results << "rdkservices version: " << provider_version << "\n\n";
    test_results << "Consumer: " + consumer + " - Provider: rdk_service" << "\n";
    test_results << "Total Interactions: " << count << "\n";
    test_results << "Failed Interactions: " << failed_apis << "\n";
    test_results << "Duration: " << duration << " seconds" << "\n";
    test_results << "*************************************************************" << "\n";
    test_results.close();
    //return true if all tests passed
    return (failed_apis == 0);
}

/**
 * @brief Start the Flask server at 5003 port and listen for provider state strings
 */
void CTVerifier_Test::RunFlaskServer(void) {
    std::cout << "Starting Flask server from verify contracts ..." << std::endl;
    // Run the Flask server
    std::string venvPath = "venv/bin/activate";
    // std::string cmd = "venv/bin/python provider.py";
    std::string cmd = "venv/bin/python rdkservices/Tests/L2Tests/pact/providerStates/providerStateService/start.py";
    system(cmd.c_str());
    std::cout << "Flask server stopped" << std::endl;
}

/*
 * @brief Shutdown the Flask server
 */
void CTVerifier_Test::shutdownFlaskServer()
{
    runningFlaskThread = false;  // Stop the thread loop
    // The curl command to send a POST request to the Flask shutdown endpoint
    std::string command = "curl -X POST http://localhost:5003/shutdown";

    // Execute the command using system()
    int result = system(command.c_str());

    // Check if the system call was successful
    if (result == 0) {
        std::cout << "POST request sent successfully!" << std::endl;
    } else {
        std::cerr << "Failed to send POST request!" << std::endl;
    }
}
