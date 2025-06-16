# If not stated otherwise in this file or this component's license file the
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

set(PLUGIN_NAME MiracastService)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

set(PLUGIN_MIRACASTSERVICE_STARTUPORDER "" CACHE STRING "To configure startup order of MiracastService plugin")
set(PLUGIN_MIRACASTSERVICE_AUTOSTART "false" CACHE STRING "Automatically start MiracastService plugin")

add_definitions(-DMIRACAST_SERVICE_API_VERSION_NUMBER_MAJOR=2)
add_definitions(-DMIRACAST_SERVICE_API_VERSION_NUMBER_MINOR=0)
add_definitions(-DMIRACAST_SERVICE_API_VERSION_NUMBER_PATCH=0)

set(MIRACAST_SERVICE_IMPLEMENTATION ${PLUGIN_NAME}Implementation)
add_definitions(-DPLUGIN_MIRACAST_SERVICE_IMPLEMENTATION_NAME="${MIRACAST_SERVICE_IMPLEMENTATION}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(IARMBus)
find_package(GLIB REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(WPEFrameworkSecurityUtil)

find_library(GLIB_LIBRARY NAMES glib-2.0 )

if (USE_THUNDER_R4)
    find_package(${NAMESPACE}COM REQUIRED)
else ()
    find_package(${NAMESPACE}Protocols REQUIRED)
endif (USE_THUNDER_R4)

add_library(${MODULE_NAME} SHARED MiracastService.cpp Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})

include_directories(../../helpers)

target_link_libraries(${MODULE_NAME} PRIVATE CompileSettingsDebug::CompileSettingsDebug ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

add_library(${PLUGIN_IMPLEMENTATION} SHARED MiracastServiceImplementation.cpp Module.cpp ../common/MiracastCommon.cpp ../common/MiracastLogger.cpp MiracastController.cpp P2P/MiracastP2P.cpp)

target_link_libraries(${PLUGIN_IMPLEMENTATION}
        PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)
set_source_files_properties(MiracastService.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

if (USE_THUNDER_R4)
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}COM::${NAMESPACE}COM)
else ()
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Protocols::${NAMESPACE}Protocols)
endif (USE_THUNDER_R4)

target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ./)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ../common)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE P2P)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ../../helpers)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_INCLUDE_DIRS})
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${GLIB_INCLUDE_DIRS})

target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${GLIB_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE -lpthread)

if(NOT RDK_SERVICES_L1_TEST)
    target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE -lwpa_client)
endif()

install(TARGETS ${PLUGIN_IMPLEMENTATION}
		DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})