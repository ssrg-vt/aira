LIB := libOpenCL_rt.so
VER_MAJOR := 0
VER_MINOR := 1

# Modify build process for different systems
ifeq ($(type),android)

TYPE := Android
ANDROID := /usr/local/android
include $(ANDROID)/apps/utils/Makefile.mk
include $(ANDROID)/apps/OpenCL/utils/Makefile.mk

CFLAGS += -O3 -Wall -D_ANDROID -I./include

else

TYPE := x86

# Let user specify location of OpenCL installation
ifeq ($(ocl),)
OCL=/usr
else
OCL=$(ocl)
endif

ifeq ($(ocl_inc),)
OCL_INC=$(OCL)/include
else
OCL_INC=$(ocl_inc)
endif

ifeq ($(ocl_lib),)
OCL_LIB=$(OCL)/lib
else
OCL_LIB=$(ocl_lib)
endif

CC := gcc
CFLAGS := -O3 -Wall -I$(OCL_INC) -L$(OCL_LIB) -I./include
LIBS := -lOpenCL

endif

PIC := -fPIC
SHARED := -shared -Wl,-soname,$(LIB)

SRC := $(shell ls src/*.c)
HDR := $(shell ls include/*.h)
OBJ := $(subst src,build,$(SRC:.c=.o))

all: $(LIB)

%/.dir:
	@echo "--> Building for $(TYPE) <--"
	@mkdir $*
	@touch $@

build/%.o: src/%.c $(HDR)
	@echo " [CC] $<"
	@$(CC) $(CFLAGS) $(PIC) -o $@ -c $< $(LIBS)

$(LIB): build/.dir $(OBJ) $(HDR)
	@echo " [LD] $@"
	@$(CC) $(CFLAGS) $(SHARED) -o $(LIB) $(OBJ) $(LIBS)
	@ln -fs $(LIB) $(LIB).$(VER_MAJOR).$(VER_MINOR);

clean:
	@echo " [CLEAN] $(LIB)"
	@rm -rf build $(LIB) $(LIB).$(VER_MAJOR).$(VER_MINOR)

.PHONY: all clean
