BR_DIR = /home/david/pisignage-BR/buildroot-2018.11.1
CC=${BR_DIR}/output/host/usr/bin/arm-linux-cc
CXX=${BR_DIR}/output/host/usr/bin/arm-linux-g++

OBJS=main.o

OBJS+= ../compositor/ilclient/ilclient.o ../compositor/ilclient/ilcore.o 
LIB+= ../compositor/ilclient/libilclient.a

OBJS+= ../compositor/tricks.o

OBJS+= ../compositor/pijpegdecoder.o ../compositor/piImageResizer.o ../compositor/PiOMXUser.o
OBJS+= ../PiSignageLogging.o

BIN=omxtest.bin

CFLAGS+= -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX
CFLAGS+= -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS+= -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT
CFLAGS+= -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST
CFLAGS+= -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi

LDFLAGS+=-lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lboost_system -lboost_filesystem
LDFLAGS+= -lrt -lm -lvcilcs -lvchostif 

INCLUDES+= -I./
INCLUDES+= -I../
INCLUDES+= -I../compositor
INCLUDES+= -I${BR_DIR}/output/staging/usr/include/

all: $(BIN) $(LIB)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

%.o: %.cc
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

%.bin: $(OBJS)
	$(CXX) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

%.a: $(OBJS)
	$(AR) r $@ $^

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN) $(LIB)





