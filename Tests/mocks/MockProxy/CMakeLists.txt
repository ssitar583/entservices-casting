###
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2024 RDK Management
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
###

# New library for TestMocklib
    add_library(TestMocklib SHARED
        IarmProxy.cpp  # Added IarmProxy.cpp to this new library
        procProxy.cpp  # Added procProxy.cpp to this new library
        RbusProxy.cpp  # Added RbusProxy.cpp to this new library
    )

# Include directories for the new TestMocklib
target_include_directories(TestMocklib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_INSTALL_PREFIX}/include/WPEFramework/core
    ${CMAKE_SOURCE_DIR}/helpers
    ${CMAKE_SOURCE_DIR}/Tests/mocks
    ${CMAKE_SOURCE_DIR}/Tests/mocks/thunder
    ${CMAKE_INSTALL_PREFIX}/include
    ${CMAKE_INSTALL_PREFIX}/include/WPEFramework
)

# Set target properties for TestMocklib
set_target_properties(TestMocklib PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES)

# Install TestMocklib
install(TARGETS TestMocklib
    DESTINATION lib)

