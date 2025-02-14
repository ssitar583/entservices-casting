# If not stated otherwise in this file or this component's LICENSE file the
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


set(PLUGIN_NAME L2Tests)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(THUNDER_PORT 9998)
find_package(${NAMESPACE}Plugins REQUIRED)

set(SRC_FILES Module.cpp L2Tests.cpp L2TestsMock.cpp)

if(PLUGIN_SAMPLEPLUGIN)
    set(SRC_FILES ${SRC_FILES} tests/SamplePlugin_L2test.cpp)
endif()

if(PLUGIN_SYSTEMSERVICES)
    set(SRC_FILES ${SRC_FILES} tests/SystemService_L2Test.cpp)
endif()
# if(PLUGIN_TEXTTOSPEECH)
#     set(SRC_FILES ${SRC_FILES} tests/TextToSpeech_L2test.cpp)
# endif()
if(PLUGIN_USBACCESS)
    set(SRC_FILES ${SRC_FILES} tests/UsbAccess_L2Test.cpp)
endif()

if(PLUGIN_TELEMETRY)
    set(SRC_FILES ${SRC_FILES} tests/Telemetry_L2Test.cpp)
endif()

if(PLUGIN_WAREHOUSE)
    set(SRC_FILES ${SRC_FILES} tests/Warehouse_L2Test.cpp)
endif()

if(PLUGIN_HDCPPROFILE)
    set(SRC_FILES ${SRC_FILES} tests/VerifyContracts.cpp)
endif()

if(PLUGIN_USERSETTINGS)
    set(SRC_FILES ${SRC_FILES} tests/UserSettings_L2Test.cpp)
endif()

if(PLUGIN_POWERMANAGER)
    set(SRC_FILES ${SRC_FILES} tests/PowerManager_L2Test.cpp)
endif()

if(PLUGIN_USBDEVICE)
    set(SRC_FILES ${SRC_FILES} tests/UsbDevice_L2Test.cpp)
endif()

# if(PLUGIN_USB_MASS_STORAGE)
#     set(SRC_FILES ${SRC_FILES} tests/UsbMassStorage_L2Test.cpp)
# endif()
if(PLUGIN_AVOUTPUT)
    set(SRC_FILES ${SRC_FILES} tests/AVOutputTV_L2Test.cpp)
endif()

if(PLUGIN_FIRMWAREUPDATE)
    set(SRC_FILES ${SRC_FILES} tests/FirmwareUpdate_L2Test.cpp)
endif()

# if (PLUGIN_ANALYTICS)
#     set(SRC_FILES ${SRC_FILES} tests/Analytics_L2Test.cpp)
# endif()

if(PLUGIN_DISPLAYSETTINGS)
    set(SRC_FILES ${SRC_FILES} tests/DisplaySettings_L2Test.cpp)
endif()

if(PLUGIN_APPMANAGER)
    set(SRC_FILES ${SRC_FILES} tests/AppManager_L2Test.cpp)
endif()

add_library(${MODULE_NAME} SHARED ${SRC_FILES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME}
            PRIVATE
            MODULE_NAME=Plugin_${PLUGIN_NAME}
            THUNDER_PORT="${THUNDER_PORT}")

target_compile_options(${MODULE_NAME} PRIVATE -Wno-error)
target_link_libraries(${MODULE_NAME} PRIVATE gmock_main ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

if (NOT L2_TEST_OOP_RPC)
    find_library(TESTMOCKLIB_LIBRARIES NAMES TestMocklib)
    if (TESTMOCKLIB_LIBRARIES)
        message ("Found mock library - ${TESTMOCKLIB_LIBRARIES}")
        target_link_libraries(${MODULE_NAME} PRIVATE ${TESTMOCKLIB_LIBRARIES})
    else (TESTMOCKLIB_LIBRARIES)
         message ("Require ${TESTMOCKLIB_LIBRARIES} library")
    endif (TESTMOCKLIB_LIBRARIES)
endif (NOT L2_TEST_OOP_RPC)

find_library(MOCKACCESSOR_LIBRARIES NAMES MockAccessor)
if (MOCKACCESSOR_LIBRARIES)
    message ("Found MockAccessor library - ${MOCKACCESSOR_LIBRARIES}")
    target_link_libraries(${MODULE_NAME} PRIVATE ${MOCKACCESSOR_LIBRARIES})
else (MOCKACCESSOR_LIBRARIES)
     message ("Require ${MOCKACCESSOR_LIBRARIES} library")
endif (MOCKACCESSOR_LIBRARIES)

target_include_directories(${MODULE_NAME} PRIVATE ../../../helpers)
target_include_directories(${MODULE_NAME} PRIVATE ./)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks/thunder)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks/devicesettings)
target_include_directories(${MODULE_NAME} PRIVATE ../../../Network)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks/MockPlugin)
 
install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})



