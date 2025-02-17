################################################################################
# sky-miracastappplugin
################################################################################
LOCAL_PATH = $(shell pwd)
LDFLAGS += -lcurl -ldl -lrt -lpthread -lstdc++ -fuse-ld=gold -lessos -lcrypto -lssl
CXXFLAGS += -Wno-attributes -Wall -g -fpermissive -std=c++1y -fPIC
CFLAGS += -Wall -std=c99 $(CXXFLAGS)
BUILD_DIR = $(LOCAL_PATH)/build
################################################################################
# sky miracastapp plugin files
################################################################################

skyAppPlugin_SRCS = \
				 ./utils/CrashLogger.cpp \
				 ./utils/AppLaunchDetails.cpp \
				 ./utils/util_base64.cpp \
				 ./utils/SkyAsProxy.cpp \
				 ./utils/MiracastAppLogging.cpp \
				 ./monitors/MiracastAppStateMonitor.cpp \
				 ./Platform/MiracastApplication.cpp \
				 ./Platform/MiracastAppInterface.cpp \
				 ./Platform/MiracastAppMainStub.cpp \
				 ./Platform/Graphics/MiracastGraphicsDelegate.cpp \
				 ./MiracastApp.cpp \
				 ./MiracastAppMgr.cpp \
				 ../MiracastPlayer/RTSP/MiracastRTSPMsg.cpp \
				 ../MiracastPlayer/Generic/MiracastGstPlayer.cpp \
				 ../common/MiracastCommon.cpp \
				 $(NULL)

skyAppPlugin_INCLUDES = \
	       -I$(STAGING_DIR)/usr/include/ \
	       -I$(STAGING_DIR)/usr/include/glib-2.0 \
	       -I$(STAGING_DIR)/usr/lib/glib-2.0/include \
		   -I$(STAGING_DIR)/usr/include/rdk/iarmbus \
		   -I$(STAGING_DIR)/usr/include/libsoup-2.4/ \
		   -I$(LOCAL_PATH)/utils/ \
		   -I$(LOCAL_PATH)/monitors/ \
		   -I$(LOCAL_PATH)/Platform/ \
		   -I$(LOCAL_PATH)/Platform/Graphics/ \
		   -I$(LOCAL_PATH)/ \
		   -I$(LOCAL_PATH)/../MiracastPlayer/ \
		   -I$(LOCAL_PATH)/../MiracastPlayer/RTSP \
		   -I$(LOCAL_PATH)/../MiracastPlayer/Generic \
		   -I$(LOCAL_PATH)/../common \
		   $(NULL)

GST_INCLUDES =  -I$(STAGING_DIR)/usr/include/gstreamer-1.0 \
                -I$(STAGING_DIR)/usr/include/ \
                -I$(STAGING_DIR)/usr/include/glib-2.0 \
				-I$(STAGING_DIR)/usr/lib/gstreamer-1.0/include \
				-I$(STAGING_DIR)/usr/lib/glib-2.0/include

OBJS := $(skyAppPlugin_SRCS:%=$(BUILD_DIR)/%.o)

skyAppPlugin_LIBS = -L$(STAGING_DIR)/usr/lib/ \
                                 -lglib-2.0 \
								 -lgobject-2.0 \
								 -lIARMBus \
								 -lWPEFrameworkCore \
								 -lwayland-client \
								 -lwayland_skyq_client \
								 -lsoup-2.4 \
								 -lEGL \
								 -lMiracastPlayerHal

GST_LIBS = -L$(STAGING_DIR)/usr/lib -L$(STAGING_DIR)/usr/lib/gstreamer-1.0 \
           -lsec_api -lgstreamer-1.0 -lgstbase-1.0 -lglib-2.0 \
           -lgobject-2.0 -lgstapp-1.0 -lgstallocators-1.0 -lgstsecmemallocator -lrdkgstreamerutils -lrdkgstreamerutilsplatform

# ---------------------------------------------------------------------
#  Build rules
# ---------------------------------------------------------------------
BUILD_TARGETS = skyMiracastApplication
DIR_CREATE = @mkdir -p $(@D)

.PHONY: all install clean

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(DIR_CREATE)
	$(CXX) $(CXXFLAGS) $(skyAppPlugin_INCLUDES) $(GST_INCLUDES) -c $< -o $@

$(BUILD_DIR)/$(BUILD_TARGETS): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(skyAppPlugin_LIBS) $(GST_LIBS) $(LDFLAGS)

all: $(BUILD_TARGETS)

clean: rm -rf $(BUILD_DIR)/*
