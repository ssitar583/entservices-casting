# Unit Tests #

## How to run L1 tests locally using act command ##

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

##### Add below change in L1-tests.yml (Skip this step if you are in Comcast VPN) #####
##### Please do not deliver below changes to rdk-e/rdkservices develop branch. These changes are only for local run. #####

```shell script
Add ubuntu-latest in runs-on section and add cmake installation steps.

###### Patch code ######
   L1-tests:
     name: Run L1-tests
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

```

##### Run below command #####

```shell script
act -W .github/workflows/L1-tests.yml -s GITHUB_TOKEN=[token]
```


## How to install new packages in Docker container ##

```shell script

```
## FAQ ##

1. The commands to build and run tests locally.
  run_ut.sh script does the following:
   - Downloads Dockerfile and creates docker session. (run_ut.sh)
   - Calls required scripts for building code and running L1 tests. (runL1Tests.sh)
   - Installs reuired packages. (installPackages.sh)
   - Clones and builds the Thunder components. (buildThunderComponents.sh)
   - Clones and builds RDK Services. (buildRdkservices.sh)
   - Runs the L1 tests and generates the coverage report.(generateL1Coverage.sh)

2. The commands to build and run tests in github are in [L1-tests.yml](.github/workflows/L1-tests.yml).
 L1-tests.yml trigger the workflow and follows below steps.
   - It pulls the comcast container(partners.artifactory.comcast.com/rdk-docker/ci-container-image:latest)
     and opens the docker session.
   - Checkout and builds the Thunder, ThunderInterfaces, rdkservices repos.
   - Finally runs the rdkservice's L1 tests.
   - Generates the coverage report.
   - Uploads the test results to the automatics server. https://rdkeorchestrationservice.as-g8.cf.comcast.net/rdke_orchestration_api/push_unit_test_results

3. External dependencies need to be mocked.
 For each external header, add one into the [tests.cmake](../tests.cmake) (preserve the original path parts, if needed).
 For each external declaration, add one into the [mocks folder](./mocks).
 For the queries on mocks please refer to the [gMock Cookbook](http://google.github.io/googletest/gmock_cook_book.html).

4. For the queries on how to write tests please refer to the [GoogleTest User’s Guide](https://google.github.io/googletest/).
 Common recommendations:
   - Tests should be fast.
   - Tests should be independent and repeatable.
   - Find the smallest piece of logic you can test, isolate it from other parts.
   - Use One-Assert-per-Test and Arrange-Act-Assert patterns.
     If two or more tests operate on similar data, use a test fixture.
   - Having more constraints than necessary is bad.
     If a test over-specifies, it doesn’t leave enough freedom to the implementation.
     Use `ON_CALL` by default, and only use `EXPECT_CALL` when intend to verify that the call is made.

5. Before review:
   - Enable [ClangFormat](./.clang-format) and make sure the tests code is formatted.
   - Make sure the code builds without warnings.
   - At the bottom of the workflow summary page on GitHub, there is a section with artifacts.
     Artifacts include coverage report and valgrind log.
     They help to understand how much code is covered by tests and whether there are memory leaks.
     Make sure you check both.
