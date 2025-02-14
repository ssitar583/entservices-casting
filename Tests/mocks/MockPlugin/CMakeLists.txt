# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 Synamedia Ltd.
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

set(PLUGIN_NAME MockPlugin)
#set(MODULE_NAME ${PLUGIN_NAME})
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_MOCKPLUGIN_AUTOSTART "false" CACHE STRING "Automatically start MockPlugin plugin")
set(PLUGIN_MOCKPLUGIN_STARTUPORDER "50" CACHE STRING "To configure startup order of Mock plugin")

# Find required packages
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)

# Build the main plugin that runs inside the WPEFramework daemon
add_library(${MODULE_NAME} SHARED
    MockPlugin.cpp
    Module.cpp
)

# Include directories
target_include_directories(${MODULE_NAME} PRIVATE ${IARMBUS_INCLUDE_DIRS} ../../../helpers ../../mocks)

# Link libraries
target_link_libraries(${MODULE_NAME}
    PRIVATE
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)

find_library(MOCKACCESSOR_LIBRARIES NAMES MockAccessor)
if (MOCKACCESSOR_LIBRARIES)
    message ("Found MockAccessor library - ${MOCKACCESSOR_LIBRARIES}")
    target_link_libraries(${MODULE_NAME} PRIVATE ${MOCKACCESSOR_LIBRARIES})
else (MOCKACCESSOR_LIBRARIES)
     message ("Require ${MOCKACCESSOR_LIBRARIES} library")
endif (MOCKACCESSOR_LIBRARIES)

install(TARGETS
    ${MODULE_NAME}
    DESTINATION
    lib/${STORAGE_DIRECTORY}/plugins
)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})

# Write configuration
write_config(${PLUGIN_NAME})
