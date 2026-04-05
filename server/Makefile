###########################################################################
# Fadecandy Server

TARGET := fcserver

CPP_FILES += \
	src/main.cpp \
	src/tcpnetserver.cpp \
	src/usbdevice.cpp \
	src/fcdevice.cpp \
	src/enttecdmxdevice.cpp \
	src/fcserver.cpp \
	src/version.cpp \
	src/tinythread.cpp \
	src/spidevice.cpp \
	src/apa102spidevice.cpp

INCLUDES += -Isrc
CLEAN_FILES += src/*.d src/*.o
CPPFLAGS += -Wno-strict-aliasing

###########################################################################
# System Support

SYS := $(shell $(CXX) -dumpmachine)

ifneq (, $(findstring linux, $(SYS)))
UNAME := Linux
endif
ifneq (, $(findstring mingw, $(SYS)))
UNAME := MINGW32
endif
ifneq (, $(findstring darwin, $(SYS)))
UNAME := Darwin
endif

LIBS += -lstdc++ -lm
VERSION := $(shell git describe --match "fcserver-*")
CXXFLAGS += -DFCSERVER_VERSION=$(VERSION)

ifeq ($(UNAME), Darwin)
	LDFLAGS += -m32
	CPPFLAGS += -m32 -DHAVE_POLL_H
	ifneq ("$(shell which llvm-gcc)", "")
		CPPFLAGS += -Wno-tautological-constant-out-of-range-compare
		CXXFLAGS += -std=gnu++0x
	endif
else
	CXXFLAGS += -std=gnu++0x
endif

ifneq ("$(DEBUG)", "")
	TARGET := debug-$(TARGET)
	CPPFLAGS += -g -DDEBUG -DENABLE_LOGGING
else
	STRIP_CMD := strip $(TARGET)
	CPPFLAGS += -Os -DNDEBUG
	LDFLAGS += -Os
endif

###########################################################################
# System rapidjson (header-only)

INCLUDES += $(shell pkg-config --cflags RapidJSON 2>/dev/null || echo -I/usr/include)

###########################################################################
# System libusb

INCLUDES += $(shell pkg-config --cflags libusb-1.0)
LIBS += $(shell pkg-config --libs libusb-1.0)

###########################################################################
# Platform-specific

ifeq ($(UNAME), Linux)
	LIBS += -lpthread
	CPPFLAGS += -DOS_LINUX
endif

ifeq ($(UNAME), Darwin)
	LIBS += -framework CoreFoundation -framework IOKit -lobjc
	CPPFLAGS += -DOS_DARWIN
endif

###########################################################################
# Build Rules

CPPFLAGS += -MMD $(INCLUDES)
CXXFLAGS += -felide-constructors -fno-exceptions -fno-rtti

OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(STRIP_CMD)
	rm -f src/version.o

-include $(OBJS:.o=.d)

clean:
	rm -f $(CLEAN_FILES) $(TARGET)

.PHONY: clean all
