As part of rdkservices open source activity and logical grouping of services into various entservices-* repos, the below listed change to L1 and L2 Test are effective hence forth.

# Changes Done:
Since the mock part is common across various plugins/repos and common for L1, L2 & etc, the gtest and gmock related stubs (including platform interface mocks) are moved to a new repo called "entservices-testframework" and L1 & L2 test files of each plugin moved to corresponding repos, you can find them inside Tests directory of each entservices-*.
Hence, any modifications/additions related to mocks should be commited to entservices-testframework repo @ rdkcentral and any modifications/additions related to test case should be commited to Test directory of corresponding entservices repo.

# Individual Repo Handling
Each individual entservices-* repo was added with a .yml file to trigger L1, L2, L2-OOP test job in github workflow. This yml file triggers below mentioned build jobs in addition to regular build jobs (thunder, thunder tools & etc,).

a/ Build mocks => To create TestMock Lib from all required mock relates stubs and copy to install/usr/lib path.
b/ Build entservices-<repo-name> => To create Test Lib of .so type from all applicable test files which are enabled for plugin test.
c/ Build entservices-testframework => To create L1/L2  executable by linking the plugins/test .so files.

This ensures everything in-tact in repo level across multiple related plugins when there is a new change comes in.

# testframework Repo Handling
The entservices-testframework repo contains yml files corresponds to L1, L2 & L2-OOP to trigger test job in github workflow.

This yml file triggers below mentioned build jobs in addition to regular build jobs (thunder, thunder tools & etc,).

a/ Build mocks => To create TestMock Lib from all required mock relates stubs and copy to install/usr/lib path.
b/ Build entservices-* => Jobs to checkout/build all individual repo's plugin & test files which are enabled for plugin test and copy all required libs to install/usr/lib path.
c/ Build entservices-testframework => To create L1/L2  executable by linking the plugins/test .so files.

This ensures everything in-tact across multiple repos when there is a new change comes either in mocks or test case or plugins.

##### Steps to run L1, L2, L2-OOP test locally #####
1. checkout the entservices-<repo-name> to your working directory in your build machine.
example: git clone https://github.com/rdkcentral/entservices-testframework.git

2. switch to entservices-<repo-name> directory
example: cd entservices-testframework

3. check and ensure current working branch points to develop
example: git branch

4. Run below curl command to download act executable to your repo.
example: curl -SL https://raw.githubusercontent.com/nektos/act/master/install.sh | bash

5. 5a/to run L1 test
example: ./bin/act -W .github/workflows/L1-tests.yml -s GITHUB_TOKEN=<your access token>

5. 5b/to run L2 test
example: ./bin/act -W .github/workflows/L2-tests.yml -s GITHUB_TOKEN=<your access token>

5. 5c/to run L2 test OOP
example: ./bin/act -W .github/workflows/L2-tests-oop.yml -s GITHUB_TOKEN=<your access token>

NOTES:
a/ If you face any secret token related error while run your yml, pls comment the below mentioned line
#token: ${{ secrets.RDKE_GITHUB_TOKEN }}
b/ Coverage Report of both L1 and L2 test are uploaded to artifacts server.
c/ For the case, which has modification in plugin and/or test files as well in entservices-testframework mock files, change the ref key of checkout job to point your own branch instead of develop, in both entservices-testframework and entservices-* repo and ensure L1, L2, L2-OOP test jobs are passing for your PR.
example: ref: feature/L1-test
