#if not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Synamedia
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

cmake_minimum_required(VERSION 3.8)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/e39786088138f2749d64e9e90e0f9902daa77c40.zip
)
FetchContent_MakeAvailable(googletest)

if(L2_TEST_OOP_RPC)
    add_library(MockAccessor SHARED
        MockAccessor.cpp
    )

    # Include directories for the new MockAccessor
    target_include_directories(MockAccessor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_INSTALL_PREFIX}/include/WPEFramework/core
        ${CMAKE_SOURCE_DIR}/helpers
        ${CMAKE_SOURCE_DIR}/Tests/mocks
        ${CMAKE_SOURCE_DIR}/Tests/mocks/thunder
        ${CMAKE_INSTALL_PREFIX}/include
        ${CMAKE_INSTALL_PREFIX}/include/WPEFramework
    )

    # Set target properties for MockAccessor
    set_target_properties(MockAccessor PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

    # Install MockAccessor
    install(TARGETS MockAccessor
        DESTINATION lib)

    add_subdirectory(MockProxy)

else(L2_TEST_OOP_RPC)

    set(MODULE_NAME TestMocklib)

    add_library(${MODULE_NAME} SHARED
        Rfc.cpp
        Iarm.cpp
        Wraps.cpp
        RBus.cpp
        Telemetry.cpp
        readprocMockInterface.cpp
        devicesettings.cpp
        systemaudioplatform.cpp
        Udev.cpp
        btmgr.cpp
        libusb.cpp
        tvSettings.cpp
        tr181api.cpp
    )
    if(RDK_SERVICES_L1_TEST AND USE_THUNDER_R4)
        add_library(${MODULE_NAME} SHARED
            Communicator.cpp
        )
    endif()
    target_link_libraries(${MODULE_NAME} PRIVATE gmock_main)

    # Set target properties for TestMocklib
    set_target_properties(${MODULE_NAME} PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES)

    # Install TestMocklib
    install(TARGETS ${MODULE_NAME}
        DESTINATION lib)

endif(L2_TEST_OOP_RPC)
