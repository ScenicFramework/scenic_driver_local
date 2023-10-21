MIX = mix
PREFIX = $(MIX_APP_PATH)/priv
DEFAULT_TARGETS ?= $(PREFIX) $(PREFIX)/scenic_driver_local

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
	c_src/scenic/scenic_ops.c \
	c_src/scenic/script_ops.c \
	c_src/scenic/script.c \
	c_src/scenic/unix_comms.c \
	c_src/scenic/utils.c

NVG_COMMON_SRCS = \
	c_src/device/nvg/gl_helpers.c \
	c_src/device/nvg/nanovg/nanovg.c \
	c_src/device/nvg/nvg_font_ops.c \
	c_src/device/nvg/nvg_image_ops.c \
	c_src/device/nvg/nvg_scenic.c \
	c_src/device/nvg/nvg_script_ops.c

CAIRO_COMMON_SRCS = \
	c_src/device/cairo/cairo_common.c \
	c_src/device/cairo/cairo_font_ops.c \
	c_src/device/cairo/cairo_image_ops.c \
	c_src/device/cairo/cairo_script_ops.c

ifeq ($(SCENIC_LOCAL_TARGET),cairo-gtk)
	CFLAGS = -O3 -std=gnu99

	ifndef MIX_ENV
		MIX_ENV = dev
	endif

	ifdef DEBUG
		CFLAGS += -O0 -pedantic -Wall -Wextra -Wno-unused-parameter
	endif

	ifeq ($(MIX_ENV),dev)
		CFLAGS += -g
	endif

	LDFLAGS += `pkg-config --static --libs freetype2 cairo gtk+-3.0`
	CFLAGS += `pkg-config --static --cflags freetype2 cairo gtk+-3.0`
	LDFLAGS += -lm

	DEVICE_SRCS += \
		$(CAIRO_COMMON_SRCS) \
		c_src/device/cairo/cairo_gtk.c

else ifeq ($(SCENIC_LOCAL_TARGET),cairo-fb)
	LDFLAGS += `pkg-config --static --libs freetype2 cairo`
	CFLAGS += `pkg-config --static --cflags freetype2 cairo`
	LDFLAGS += -lm
	CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter -pedantic
	CFLAGS += -std=gnu99

	DEVICE_SRCS += \
		$(CAIRO_COMMON_SRCS) \
		c_src/device/cairo/cairo_fb.c

else ifeq ($(SCENIC_LOCAL_TARGET),glfw)
$(info )
$(info **********************************************************************************)
$(info SCENIC_LOCAL_TARGET=glfw is deprecated. Please use `SCENIC_LOCAL_TARGET=cairo-gtk`)
$(info **********************************************************************************)
$(info )

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
		$(NVG_COMMON_SRCS) \
		c_src/device/nvg/glfw.c

else ifeq ($(SCENIC_LOCAL_TARGET),bcm)
$(info )
$(info ********************************************************************************)
$(info SCENIC_LOCAL_TARGET=bcm is deprecated. Please use `SCENIC_LOCAL_TARGET=cairo-fb`)
$(info ********************************************************************************)
$(info )

	LDFLAGS += -lGLESv2 -lEGL -lm -lvchostif -lbcm_host
	CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter -pedantic
	CFLAGS += -std=gnu99

	DEVICE_SRCS += \
		$(NVG_COMMON_SRCS) \
		c_src/device/nvg/bcm.c

	ifeq ($(SCENIC_LOCAL_GL),gles2)
		CFLAGS += -DSCENIC_GLES2
	else
		CFLAGS += -DSCENIC_GLES3
	endif

else ifeq ($(SCENIC_LOCAL_TARGET),drm)
$(info )
$(info ********************************************************************************)
$(info SCENIC_LOCAL_TARGET=drm is deprecated. Please use `SCENIC_LOCAL_TARGET=cairo-fb`)
$(info ********************************************************************************)
$(info )

	LDFLAGS += -lGLESv2 -lEGL -lm -lvchostif -ldrm -lgbm
	CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter -pedantic
	CFLAGS += -std=gnu99
	CFLAGS += -fPIC -I$(NERVES_SDK_SYSROOT)/usr/include/drm

	DEVICE_SRCS += \
		$(NVG_COMMON_SRCS) \
		c_src/device/nvg/drm.c

	ifeq ($(SCENIC_LOCAL_GL),gles2)
		CFLAGS += -DSCENIC_GLES2
	else
		CFLAGS += -DSCENIC_GLES3
	endif

endif

CFLAGS += \
	-Ic_src \
	-Ic_src/device \
	-Ic_src/font \
	-Ic_src/image \
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

