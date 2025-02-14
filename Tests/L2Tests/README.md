## How to run locally using act ##

Install [act](https://github.com/nektos/act) and docker:

##### Ubuntu #####

```shell script
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash
sudo apt update
sudo apt -y install docker.io
```

##### macOS #####

```shell script
brew install act
brew install --cask docker
```

Create a [personal access token](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token).

##### Add below change in L2-tests.yml in local server. (Skip this step if you are in Comcast VPN) #####
##### Please do not deliver below changes to rdk-e/rdkservices develop branch. These changes are only for local run. #####

```shell script
Add ubuntu-latest in runs-on section and add cmake installation steps.

  Note: Note: The L2-tests.yml is configured to run with the latest Thunder version, R4.4.1.

###### Patch code ######
   L2-tests:
     name: Run L2-tests
-    runs-on: comcast-ubuntu-latest
-    container:
-      image: partners.artifactory.comcast.com/rdk-docker/ci-container-image:latest
-      credentials:
-        username: ${{ secrets.ARTIFACTORY_COVERITY_USER }}
-        password: ${{ secrets.ARTIFACTORY_COVERITY_APIKEY }}
+    runs-on: ubuntu-latest
     strategy:
       matrix:
         compiler: [ gcc, clang ]
@@ -33,6 +28,10 @@ jobs:
             coverage: without-coverage

     steps:
+      - name: Set up Python
+        uses: actions/setup-python@v4
+        with:
+          python-version: '3.x'
+      - run: pip install jsonref
+
+      - name: Set up CMake
+        uses: jwlawson/actions-setup-cmake@v1.13
+        with:
+          cmake-version: '3.16.x'
+
+      - name: Install packages
+        run: >
+          sudo apt update
+          &&
+          sudo apt install -y libsqlite3-dev libcurl4-openssl-dev valgrind lcov clang libsystemd-dev libboost-all-dev libwebsocketpp-dev meson libcunit1 libcunit1-dev
+
       - name: Install GStreamer
         run: |
            apt update

```

##### Run below command #####

```shell script
act -W .github/workflows/L2-tests.yml -s GITHUB_TOKEN=[token]

The tests will run without valgrid by default. If you want to run tests with valgrind also then you need to remove below condition at "Run unit tests with valgrind" step from the yml file.

    if: ${{ !env.ACT }}

```

##### Get a bash shell in the container, if needed #####

```shell script
docker ps
docker exec -it <container name> /bin/bash
```

## FAQ ##

1. The commands to build and run tests are in [L2-tests.yml](../../.github/workflows/L2-tests.yml).
 For the queries on syntax please refer to the [Workflow syntax for GitHub Actions](https://docs.github.com/en/actions/using-workflows/workflow-syntax-for-github-actions).

2. External dependencies need to be mocked.
 For each external header, add one into the [l2tests.cmake](../../l2tests.cmake) (preserve the original path parts, if needed).
 For each external declaration, add one into the [mocks folder](../mocks).
 For the queries on mocks please refer to the [gMock Cookbook](http://google.github.io/googletest/gmock_cook_book.html).

3. For the queries on how to write tests please refer to the [GoogleTest User’s Guide](https://google.github.io/googletest/).
 Common recommendations:
   - Tests should be fast.
   - Tests should be independent and repeatable.
   - Find the L2 usecase/logic from your plugin which you can test.  Isolate each use case from other use cases.
   - Implement the Arrange-Act-Assert patterns for each step in a use case.
     If two or more tests operate on similar data, use a test fixture.
     Use `ON_CALL` by default, and only use `EXPECT_CALL` when intend to verify that the call is made.

4. To add L2 tests for a RDK service
 Required steps:
   - Set plugin ON by default in [l2tests.cmake](../../l2tests.cmake)
  	  Example : set(PLUGIN_SYSTEMSERVICES ON)

   - Add required empty headers,definitions and mock files in [l2tests.cmake](../../l2tests.cmake) to pass compilation for the added plugin
   - Update yml file as well to enable the plugin.
      Example : -DPLUGIN_SYSTEMSERVICES=ON

   - Add new L2 test file for the plugin in folder rdkservices/L2Tests/tests with name Pluginname_L2tests
       Example : SystemServices_L2test.cpp

   - Add a class (with pluginname_L2tests as name) which inherits the mock class for getting all the mocks required by plugin.
       Example :class Systemservice_L2test : public L2TestMocks

   -  Add Test fixtures for each usecase.

5. To run L2 tests for specific suite (plugin)
    During test/code development, we often do not want to run all the tests. Instead, to save time, we can use the following method to run specified test suite(s).

 Required steps:
   - In yml file in the step where it runs the L2test binary RDKServiceL2test add the test suite name you want to run
   Example :
       a. If we want to run single suite
          RDKServicesL2test System (this is internally considered as 'System*' and gtest framework runs all tests added with suite name starting with 'System')
       b. If we want to run multiple suites
          RDKServicesL2test System Usb (this is internally considered as 'System*:Usb*' and gtest framework runs all tests added with suite name starting with 'System' and all tests with suite name starting with 'Usb')


  Note: TEST_COMPLETION_TIMEOUT specifies a time limit for completing the specified tests.  This can be increased/decreased by changing the value in  [CMakeLists.txt](./CMakeLists.txt).



###### Out Of Process (OOP) L2 Test Mock Steps ################

###### Existing In-Process L2 Test Flow (FYR) #################
plugin (Being Tested Ex: SystemServices) -> mocks (Iarm.cpp, Rbus.cpp & etc) -> mockImpl of gmock (IarmBusMock.h) == All are running under WpeFramework process.

###### New OOP L2 Test Flow ###################################

plugin (Being Tested Ex: PowerManager) -> Proxys (IarmProxy.cpp, RbusProxy.cpp & etc) == Running separatly from WpeFramework process with different PID.
MockPlugin -> mockImpl of gmock (IarmBusMock.h) = Running under WpeFramework process
The communication between WpeFramework process and plugin process is done by COM-RPC machnism using interfaces of IIarm.h, IRBus.h & etc.

###### Detailed Steps for forward API calls from the plugin to the mocked component ###################
1. Identify all the interface mocks needed for your plugin to run successfully and create a new interface header file
   @rdkservices\Tests\mocks\interfaces
   Example: IIarm.h, IProc.h, IRBus.h
2. Add a cp command in L2-tests-oop.yml to copy your interface header file to ThunderInterface as shown below.
   Example: cp "$GITHUB_WORKSPACE/rdkservices/Tests/mocks/interfaces/IIarm.h" "$GITHUB_WORKSPACE/rdkservices-apis/apis/IARM/"
3. Add a proxy file @ rdkservices\Tests\mocks\MockProxy and do the following.
      a/ static methods definition and appropriate linking to function pointers.
      b/ create a RPC channel to communicate with Mock Plugin using your interface methods and Initialize it.
      Example: IarmProxy.cpp, RbusProxy.cpp, ProcProxy.cpp
4. In the Mock Plugin, override the interfaces declared in your mock interface and add implementation for each interface method
   @ rdkservices\Tests\mocks\MockPlugin\MockPlugin.cpp
   Example: MockPlugin::IARM_Bus_Init, MockPlugin::openproc, MockPlugin::rbus_close
5. Add your interface to mock plugin interface entry in rdkservices\Tests\mocks\MockAccessor.h
   Example: INTERFACE_ENTRY(Exchange::IIarm), INTERFACE_ENTRY(Exchange::IProc), INTERFACE_ENTRY(Exchange::IRBus)
6. Add your interface to the MockAccessor class @ rdkservices\Tests\mocks\MockAccessor.cpp
   Example: template class MockAccessor<IarmBusImpl>; template class MockAccessor<readprocImpl>; template class MockAccessor<RBusApiImpl>;
7. Set the gmock impl instance pointer to template class from L2TestsMock.cpp constructor and get it in MockPlugin::Initialize function.
   Example: MockAccessor<IarmBusImpl>::setPtr(p_iarmBusImplMock); _iarmImpl = MockAccessor<IarmBusImpl>::getPtr();
8. Add corresponding xyz_L2Test.cpp file in Test directory as usal and execute the test suite.
   Example: PowerManager_L2Test.cpp

###### Detailed Steps for Event Notifications #################
For Event Notifications, the same COM-RPC machanism which was establised in steps 3(b) of above will be reused and data flow will be from Mock Plugin to OOP plugin.
1. In interface header file declare INotification class and methods for each notification callback.
   Example: IIarm.h - IIarm::INotification::IARM_EventHandler
2. In the MockProxy, register for the notifications after opening the communicator client.
   Example: _mockPluginIarm->Register(&(Instance()._notificationHandler));
3. Register OOP plugin handler by using appropriate Register API call of your interface and store it to eventHandlerInfo list.
   Example: IarmBusMockProxy::IarmBusRegisterEventHandler
4. In MockPlugin.cpp, define another handler function specific to your interface and pass it to gmock implementation as
   notification callback handler.
   Example: MockPlugin::IARM_Bus_RegisterEventHandler, MockPlugin::sendNotificationIarm
5. Once Mock Plugin internal handler function called by gmock test case, invoke the EventHandler of INotification class.
   Example: MockPlugin::sendNotificationIarm
6. In Proxy::EventHandler, submit the job to IWorkerPool to invoke the OOP plugin handler callback function from worker thread context.
   Example: IarmBusMockProxy::IARM_EventHandler, IarmBusMockProxy::dispatchEvent, IarmBusMockProxy::Dispatch

###### General Note ###########################################
1. All the necessary configuration/compile time switch are preconfigured in L2-test-oop.yml, please refer to it for more info.
2. The above mentioned steps are validated/verified by using PowerManager and UserSettings plugins, please refer to it for more info.
3. L2_Test_OOP_RPC switch must be set to ON to enable this support (-DL2_TEST_OOP_RPC=ON, -DPLUGIN_SAMPLEPLUGIN if needed).
4. In L2TestMock.cpp, setImpl related stuffs of any mock interface should be either #if or #else of L2_TEST_OOP_RPC switch, not outside of it.
5. gmock MOCK_METHOD (whether it is existing mock like Iarm, Rbus & etc or new mocks getting introduced in the system newly)
   should be common for L1 Test, in-process L2 Test and OOP L2 Test.