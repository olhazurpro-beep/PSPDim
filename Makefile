TARGET = nightshift
OBJS = main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -D_PSP_FW_VERSION=600
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

LIBS = -lpspsystemctrl_kernel -lpspsdk -lpspdisplay -lpspkernel

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
