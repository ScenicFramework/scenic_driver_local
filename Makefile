# Variables to override
#
# CC            C compiler
# CROSSCOMPILE	crosscompiler prefix, if any
# CFLAGS	compiler flags for compiling all C files
# ERL_CFLAGS	additional compiler flags for files using Erlang header files
# ERL_EI_INCLUDE_DIR include path to ei.h (Required for crosscompile)
# ERL_EI_LIBDIR path to libei.a (Required for crosscompile)
# LDFLAGS	linker flags for linking all binaries
# ERL_LDFLAGS	additional linker flags for projects referencing Erlang libraries


MIX = mix
PREFIX = $(MIX_APP_PATH)/priv
DEFAULT_TARGETS ?= $(PREFIX) $(PREFIX)/scenic_driver_local

# # Look for the EI library and header files
# # For crosscompiled builds, ERL_EI_INCLUDE_DIR and ERL_EI_LIBDIR must be
# # passed into the Makefile.
# ifeq ($(ERL_EI_INCLUDE_DIR),)
# ERL_ROOT_DIR = $(shell erl -eval "io:format(\"~s~n\", [code:root_dir()])" -s init stop -noshell)
# ifeq ($(ERL_ROOT_DIR),)
# 	$(error Could not find the Erlang installation. Check to see that 'erl' is in your PATH)
# endif
# ERL_EI_INCLUDE_DIR = "$(ERL_ROOT_DIR)/usr/include"
# ERL_EI_LIBDIR = "$(ERL_ROOT_DIR)/usr/lib"
# endif

# # Set Erlang-specific compile and linker flags
# ERL_CFLAGS ?= -I$(ERL_EI_INCLUDE_DIR)
# ERL_LDFLAGS ?= -L$(ERL_EI_LIBDIR) -lei


$(info SCENIC_LOCAL_TARGET: $(SCENIC_LOCAL_TARGET))
ifdef SCENIC_LOCAL_GL
$(info SCENIC_LOCAL_GL: $(SCENIC_LOCAL_GL))
endif

DEVICE_SRCS =

FONT_SRCS = \
	c_src/font/font.c

IMAGE_SRCS = \
	c_src/image/image.c

TOMMYDS_SRCS = \
	c_src/tommyds/src/tommyhashlin.c \
	c_src/tommyds/src/tommyhash.c

SCENIC_SRCS = \
	c_src/scenic/comms.c \
	c_src/scenic/ops/scenic_ops.c \
	c_src/scenic/ops/script_ops.c \
	c_src/scenic/script.c \
	c_src/scenic/unix_comms.c \
	c_src/scenic/utils.c

ifeq ($(SCENIC_LOCAL_TARGET),glfw)
	CFLAGS = -O3 -std=c99

	ifndef MIX_ENV
		MIX_ENV = dev
	endif

	ifdef DEBUG
		CFLAGS +=  -pedantic -Weverything -Wall -Wextra -Wno-unused-parameter -Wno-gnu
	endif

	ifeq ($(MIX_ENV),dev)
		CFLAGS += -g
	endif

	LDFLAGS += `pkg-config --static --libs glfw3 glew`
	CFLAGS += `pkg-config --static --cflags glfw3 glew`

	ifneq ($(OS),Windows_NT)
		CFLAGS += -fPIC

		ifeq ($(shell uname),Darwin)
			LDFLAGS += -framework Cocoa -framework OpenGL -Wno-deprecated
		else
			LDFLAGS += -lGL -lm -lrt
		endif
	endif

	DEVICE_SRCS += \
		c_src/device/glfw.c \
		c_src/device/gl_helpers.c \
		c_src/nanovg/nanovg.c
	FONT_SRCS += c_src/font/nvg_font_ops.c
	IMAGE_SRCS += c_src/image/nvg_image_ops.c
	SCENIC_SRCS += c_src/scenic/ops/nvg_script_ops.c

else ifeq ($(SCENIC_LOCAL_TARGET),bcm)
	LDFLAGS += -lGLESv2 -lEGL -lm -lvchostif -lbcm_host
	CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter -pedantic
	CFLAGS += -std=gnu99

	DEVICE_SRCS += \
		c_src/device/bcm.c \
		c_src/device/gl_helpers.c \
		c_src/nanovg/nanovg.c
	FONT_SRCS += c_src/font/nvg_font_ops.c
	IMAGE_SRCS += c_src/image/nvg_image_ops.c
	SCENIC_SRCS += c_src/scenic/ops/nvg_script_ops.c

	ifeq ($(SCENIC_LOCAL_GL),gles2)
		CFLAGS += -DSCENIC_GLES2
	else
		CFLAGS += -DSCENIC_GLES3
	endif

else ifeq ($(SCENIC_LOCAL_TARGET),drm)
	# drm is the forward looking default
	LDFLAGS += -lGLESv2 -lEGL -lm -lvchostif -ldrm -lgbm
	CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter -pedantic
	CFLAGS += -std=gnu99
	CFLAGS += -fPIC -I$(NERVES_SDK_SYSROOT)/usr/include/drm

	DEVICE_SRCS += \
		c_src/device/drm.c \
		c_src/device/gl_helpers.c \
		c_src/nanovg/nanovg.c
	FONT_SRCS += c_src/font/nvg_font_ops.c
	IMAGE_SRCS += c_src/image/nvg_image_ops.c
	SCENIC_SRCS += c_src/scenic/ops/nvg_script_ops.c

	ifeq ($(SCENIC_LOCAL_GL),gles2)
		CFLAGS += -DSCENIC_GLES2
	else
		CFLAGS += -DSCENIC_GLES3
	endif
else ifeq ($(SCENIC_LOCAL_TARGET),cairo)
	LDFLAGS += `pkg-config --static --libs freetype2 cairo`
	CFLAGS += `pkg-config --static --cflags freetype2 cairo`
	LDFLAGS += -lm
	CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter -pedantic
	CFLAGS += -std=gnu99

	DEVICE_SRCS += c_src/device/cairo.c
	FONT_SRCS += c_src/font/cairo_font_ops.c
	IMAGE_SRCS += c_src/image/cairo_image_ops.c
	SCENIC_SRCS += c_src/scenic/ops/cairo_script_ops.c
else
$(info ------ no SCENIC_LOCAL_TARGET set ------)
$(info If you get here, then you are probably using a custom Nerves system)
$(info Please export/set SCENIC_LOCAL_TARGET to one of [glfw, bcm, drm])
$(info If you are running on a desktop machine, pick: glfw)
$(info For any varient of rpi <= 3, pick: bcm)
$(info For any varient of rpi >= 4, pick: drm)
$(info For any varient of bbb, pick: drm)
$(info example for a custom rpi3 build system:)
$(info export SCENIC_LOCAL_TARGET=bcm)
$(info For bbb, you also need to set SCENIC_LOCAL_GL=gles2)
$(info For >= rpi4, you also need to set SCENIC_LOCAL_GL=gles3)
$(info ----------------------------------------)
endif

# ifeq ($(SCENIC_LOCAL_TARGET),drm_gles3)
# 	LDFLAGS += -lGLESv2 -lEGL -lm -lvchostif -ldrm -lgbm
# 	CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter -pedantic
# 	CFLAGS += -std=gnu99

# 	CFLAGS += -fPIC -I$(NERVES_SDK_SYSROOT)/usr/include/drm

# 	SRCS = c_src/device/drm_gles3.c
# endif

# $(info $(shell printenv))
CFLAGS += \
	-Ic_src \
	-Ic_src/device \
	-Ic_src/font \
	-Ic_src/image \
	-Ic_src/nanovg \
	-Ic_src/scenic \
	-Ic_src/tommyds/src

SRCS = \
	$(DEVICE_SRCS) \
	$(FONT_SRCS) \
	$(IMAGE_SRCS) \
	$(TOMMYDS_SRCS) \
	$(SCENIC_SRCS) \
	c_src/main.c

calling_from_make:
	mix compile

all: $(DEFAULT_TARGETS)

$(PREFIX):
	mkdir -p $@

$(PREFIX)/scenic_driver_local: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	$(RM) -rf $(PREFIX)

.PHONY: all clean calling_from_make

