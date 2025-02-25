#!/usr/bin/make -f
# Makefile for DGL #
# ---------------- #
# Created by falkTX
#

include ../Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I. -Isrc
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I. -Isrc -DDONT_SET_USING_DGL_NAMESPACE -Wno-unused-parameter
BUILD_CXX_FLAGS += -Isrc/pugl-upstream/include
LINK_FLAGS      += $(DGL_LIBS)

ifeq ($(USE_OPENGL3),true)
BUILD_CXX_FLAGS += -DDGL_USE_OPENGL3
endif

# TODO fix these after pugl-upstream is done
BUILD_CXX_FLAGS += -Wno-attributes -Wno-extra -Wno-missing-field-initializers
ifneq ($(MACOS),true)
BUILD_CXX_FLAGS += -Wno-narrowing
endif

# ifneq ($(MACOS_OLD),true)
# needed by sofd right now, fix later
# BUILD_CXX_FLAGS += -Wno-type-limits -fpermissive
# endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_common = \
	../build/dgl/Application.cpp.o \
	../build/dgl/ApplicationPrivateData.cpp.o \
	../build/dgl/Color.cpp.o \
	../build/dgl/EventHandlers.cpp.o \
	../build/dgl/Geometry.cpp.o \
	../build/dgl/ImageBase.cpp.o \
	../build/dgl/ImageBaseWidgets.cpp.o \
	../build/dgl/Resources.cpp.o \
	../build/dgl/SubWidget.cpp.o \
	../build/dgl/SubWidgetPrivateData.cpp.o \
	../build/dgl/TopLevelWidget.cpp.o \
	../build/dgl/TopLevelWidgetPrivateData.cpp.o \
	../build/dgl/Widget.cpp.o \
	../build/dgl/WidgetPrivateData.cpp.o \
	../build/dgl/Window.cpp.o \
	../build/dgl/WindowPrivateData.cpp.o

# ---------------------------------------------------------------------------------------------------------------------

OBJS_cairo = $(OBJS_common) \
	../build/dgl/Cairo.cpp.cairo.o

ifeq ($(MACOS),true)
OBJS_cairo += ../build/dgl/pugl.mm.cairo.o
else
OBJS_cairo += ../build/dgl/pugl.cpp.cairo.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_opengl = $(OBJS_common) \
	../build/dgl/OpenGL.cpp.opengl.o \
	../build/dgl/NanoVG.cpp.opengl.o

ifeq ($(MACOS),true)
OBJS_opengl += ../build/dgl/pugl.mm.opengl.o
else
OBJS_opengl += ../build/dgl/pugl.cpp.opengl.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_stub = $(OBJS_common)

ifeq ($(MACOS),true)
OBJS_stub += ../build/dgl/pugl.mm.o
else
OBJS_stub += ../build/dgl/pugl.cpp.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_vulkan = $(OBJS_common) \
	../build/dgl/Vulkan.cpp.vulkan.o

ifeq ($(MACOS),true)
OBJS_vulkan += ../build/dgl/pugl.mm.vulkan.o
else
OBJS_vulkan += ../build/dgl/pugl.cpp.vulkan.o
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_CAIRO),true)
TARGETS += ../build/libdgl-cairo.a
endif

ifeq ($(HAVE_OPENGL),true)
TARGETS += ../build/libdgl-opengl.a
# Compat name, to be removed soon
TARGETS += ../build/libdgl.a
endif

ifeq ($(HAVE_STUB),true)
TARGETS += ../build/libdgl-stub.a
endif

ifeq ($(HAVE_VULKAN),true)
TARGETS += ../build/libdgl-vulkan.a
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

cairo:  ../build/libdgl-cairo.a
opengl: ../build/libdgl-opengl.a
stub:   ../build/libdgl-stub.a
vulkan: ../build/libdgl-vulkan.a

# ---------------------------------------------------------------------------------------------------------------------

../build/libdgl-cairo.a: $(OBJS_cairo)
	-@mkdir -p ../build
	@echo "Creating libdgl-cairo.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

../build/libdgl-opengl.a: $(OBJS_opengl)
	-@mkdir -p ../build
	@echo "Creating libdgl-opengl.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

../build/libdgl-stub.a: $(OBJS_stub)
	-@mkdir -p ../build
	@echo "Creating libdgl-stub.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

../build/libdgl-vulkan.a: $(OBJS_vulkan)
	-@mkdir -p ../build
	@echo "Creating libdgl-vulkan.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# Compat name, to be removed soon
../build/libdgl.a: ../build/libdgl-opengl.a
	@echo "Symlinking libdgl.a"
	$(SILENT)ln -sf $< $@

# ---------------------------------------------------------------------------------------------------------------------

../build/dgl/%.c.o: src/%.c
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

../build/dgl/%.cpp.o: src/%.cpp
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

../build/dgl/%.mm.o: src/%.mm
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

../build/dgl/%.cpp.cairo.o: src/%.cpp
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (Cairo variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -c -o $@

../build/dgl/%.mm.cairo.o: src/%.mm
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (Cairo variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

../build/dgl/%.cpp.opengl.o: src/%.cpp
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (OpenGL variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -o $@

../build/dgl/%.mm.opengl.o: src/%.mm
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (OpenGL variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

../build/dgl/%.cpp.vulkan.o: src/%.cpp
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (Vulkan variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(VULKAN_FLAGS) -DDGL_VULKAN -c -o $@

../build/dgl/%.mm.vulkan.o: src/%.mm
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (Vulkan variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(VULKAN_FLAGS) -DDGL_VULKAN -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf ../build/dgl ../build/libdgl*.*

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_common:%.o=%.d)
-include $(OBJS_cairo:%.o=%.d)
-include $(OBJS_opengl:%.o=%.d)
-include $(OBJS_stub:%.o=%.d)
-include $(OBJS_vulkan:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
