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

set(PLUGIN_NAME MiracastPlayer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

set(PLUGIN_MIRACASTPLAYER_STARTUPORDER "" CACHE STRING "To configure startup order of MiracastPlayer plugin")
set(PLUGIN_MIRACASTPLAYER_AUTOSTART "false" CACHE STRING "Automatically start MiracastPlayer plugin")

add_definitions(-DMIRACAST_PLAYER_API_VERSION_NUMBER_MAJOR=2)
add_definitions(-DMIRACAST_PLAYER_API_VERSION_NUMBER_MINOR=0)
add_definitions(-DMIRACAST_PLAYER_API_VERSION_NUMBER_PATCH=0)

set(MIRACAST_PLAYER_IMPLEMENTATION ${PLUGIN_NAME}Implementation)
add_definitions(-DPLUGIN_MIRACAST_PLAYER_IMPLEMENTATION_NAME="${MIRACAST_PLAYER_IMPLEMENTATION}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(IARMBus)
find_package(GLIB REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMERBASE REQUIRED gstreamer-app-1.0)

find_package(GStreamer REQUIRED)

find_library(GLIB_LIBRARY NAMES glib-2.0 )

if (USE_THUNDER_R4)
    find_package(${NAMESPACE}COM REQUIRED)
else ()
    find_package(${NAMESPACE}Protocols REQUIRED)
endif (USE_THUNDER_R4)

add_library(${MODULE_NAME} SHARED MiracastPlayer.cpp Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})

include_directories(../../helpers)

target_link_libraries(${MODULE_NAME} PRIVATE CompileSettingsDebug::CompileSettingsDebug ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

add_library(${PLUGIN_IMPLEMENTATION} SHARED Module.cpp MiracastPlayerImplementation.cpp ../common/MiracastLogger.cpp ../common/MiracastCommon.cpp RTSP/MiracastRTSPMsg.cpp)

target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)
set_source_files_properties(MiracastPlayer.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

if (USE_THUNDER_R4)
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}COM::${NAMESPACE}COM)
else ()
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Protocols::${NAMESPACE}Protocols)
endif (USE_THUNDER_R4)

if (RDK_SERVICES_L1_TEST)
	target_sources(${PLUGIN_IMPLEMENTATION}
		PRIVATE
		Test/MiracastGstPlayer.cpp
		)
else()
	target_sources(${PLUGIN_IMPLEMENTATION}
		PRIVATE
		Generic/MiracastGstPlayer.cpp
		)
endif()

target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ./)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ../common)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE RTSP)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ../../helpers)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_INCLUDE_DIRS})
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${GLIB_INCLUDE_DIRS})
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${GSTREAMER_INCLUDES})
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${GSTREAMERBASE_INCLUDE_DIRS})

target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${GLIB_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${GSTREAMER_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${GSTREAMERBASE_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE -lpthread)

if(WPEFRAMEWORK_SECURITYUTIL_FOUND)
	target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}SecurityUtil)
endif()

if(NOT RDK_SERVICES_L1_TEST AND NOT RDK_SERVICE_L2_TEST)
	target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE MiracastPlayerHal)
endif()

install(TARGETS ${PLUGIN_IMPLEMENTATION}
		DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})