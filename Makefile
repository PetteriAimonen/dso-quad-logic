# Makefile for DSO203 example application
# Petteri Aimonen <jpa@dso.mail.kapsi.fi> 2011

# Name of the target application
NAME = LOGICAPP

# Names of the object files (add all .c files you want to include)
OBJS = main.o ds203_io.o dsosignalstream.o \
xposhandler.o textdrawable.o signalgraph.o \
breaklines.o cursor.o window.o grid.o \
cxxglue.o libc_glue.o fix16.o

# Linker script (choose which application position to use)
LFLAGS  = -L linker_scripts -T app3.lds

# Any libraries to include
LIBS = -lm -lgcc baselibc/libc.a

# Include directories for .h files
CFLAGS = -I baselibc/include -I stm32_headers -I DS203 -I libfixmath

# Include directories for .hh files
CXXFLAGS = -I streams -I gui

# DS203 generic stuff
OBJS += startup.o BIOS.o Interrupt.o

# Names of the toolchain programs
CC      = arm-none-eabi-gcc
CXX     = arm-none-eabi-g++
CP      = arm-none-eabi-objcopy
OD      = arm-none-eabi-objdump

# Processor type
CFLAGS += -mcpu=cortex-m3 -mthumb -mno-thumb-interwork

# Optimization & debug settings
CFLAGS += -fno-common -O1 -g

# Compiler warnings
CFLAGS += -Wall -Werror -Wno-unused

# Flags for C++
CXXFLAGS += -fno-exceptions -fno-rtti -std=gnu++0x

# Default linker arguments (disables GCC-provided startup.c, creates .map file)
LFLAGS += -nostartfiles -nostdlib -Wl,-Map=build/$(NAME).map -eReset_Handler

# Directory for .o files
VPATH = build
_OBJS = $(addprefix build/,$(OBJS))

all: $(NAME).HEX

clean:
	rm -f $(NAME).HEX build/*

$(NAME).HEX: build/$(NAME).elf
	$(CP) -O ihex $< $@

build/$(NAME).elf: ${_OBJS}
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LFLAGS) -o $@ ${_OBJS} ${LIBS}

# Rebuild all objects if a common header changes
$(_OBJS): DS203/*.h Makefile

# C files

build/%.o: %.c *.h
	$(CC) $(CFLAGS) -std=gnu99 -c -o $@ $<

build/%.o: DS203/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: DS203/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: libfixmath/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# C++ files

build/%.o: gui/%.cc gui/*.hh streams/*.hh
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

build/%.o: streams/%.cc gui/*.hh streams/*.hh
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

build/%.o: %.cc gui/*.hh streams/*.hh
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

# Installing

deploy: $(NAME).HEX
	mount /mnt/dso
	cp $< /mnt/dso
	umount /mnt/dso


# The rest is for the developer unit tests
HOSTCXX = g++
HOSTCXXFLAGS = -I. -Istreams -Igui -Wall -g -O0 $(CXXFLAGS)

run_tests: build/dsosignalstream_tests build/xposhandler_tests
	$(foreach test, $^, \
	echo $(test) && \
	./$(test) > /dev/null && \
	) true

build/%_tests: gui/%_tests.cc gui/%.cc gui/*.hh streams/*.hh
	$(HOSTCXX) $(HOSTCXXFLAGS) -o $@ gui/$*_tests.cc gui/$*.cc

build/%_tests: streams/%_tests.cc streams/%.cc streams/*.hh
	$(HOSTCXX) $(HOSTCXXFLAGS) -o $@ streams/$*_tests.cc streams/$*.cc

