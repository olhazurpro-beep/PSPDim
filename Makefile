TARGET = nightshift
OBJS = main.o

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

INCDIR =
CFLAGS = -O2 -G0 -Wall -fno-pic
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
# Линковка системных библиотек ядра
LIBS = -lpspsystemctrl_kernel -lpspdisplay_driver
LDFLAGS = -mno-crt0 -nostartfiles

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
