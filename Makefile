#!/usr/bin/make -f
# Makefile for DPF #
# ---------------- #
# Created by falkTX
#

include Makefile.base.mk

all: dgl examples gen

# --------------------------------------------------------------

ifneq ($(CROSS_COMPILING),true)
CAN_GENERATE_TTL = true
else ifneq ($(EXE_WRAPPER),)
CAN_GENERATE_TTL = true
endif

dgl:
	$(MAKE) -C dgl

examples: dgl
	$(MAKE) all -C examples/CVPort
	$(MAKE) all -C examples/FileHandling
	$(MAKE) all -C examples/Info
	$(MAKE) all -C examples/Latency
	$(MAKE) all -C examples/Meters
	$(MAKE) all -C examples/Metronome
	$(MAKE) all -C examples/MidiThrough
	$(MAKE) all -C examples/Parameters
	$(MAKE) all -C examples/SendNote
	$(MAKE) all -C examples/States
ifeq ($(HAVE_CAIRO),true)
	$(MAKE) all -C examples/CairoUI
endif
# ifneq ($(MACOS_OR_WINDOWS),true)
# 	# ExternalUI is WIP
# 	$(MAKE) all -C examples/ExternalUI
# 	install -d bin/d_extui-dssi
# 	install -d bin/d_extui.lv2
# 	install -m 755 examples/ExternalUI/ExternalLauncher.sh bin/d_extui.sh
# 	install -m 755 examples/ExternalUI/ExternalLauncher.sh bin/d_extui-dssi/d_extui.sh
# 	install -m 755 examples/ExternalUI/ExternalLauncher.sh bin/d_extui.lv2/d_extui.sh
# endif

ifeq ($(CAN_GENERATE_TTL),true)
gen: examples utils/lv2_ttl_generator
	@$(CURDIR)/utils/generate-ttl.sh
ifeq ($(MACOS),true)
	@$(CURDIR)/utils/generate-vst-bundles.sh
endif

utils/lv2_ttl_generator:
	$(MAKE) -C utils/lv2-ttl-generator
else
gen:
endif

tests: dgl
	$(MAKE) -C tests

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C dgl
	$(MAKE) clean -C examples/CVPort
	$(MAKE) clean -C examples/CairoUI
	$(MAKE) clean -C examples/FileHandling
	$(MAKE) clean -C examples/Info
	$(MAKE) clean -C examples/Latency
	$(MAKE) clean -C examples/Meters
	$(MAKE) clean -C examples/Metronome
	$(MAKE) clean -C examples/MidiThrough
	$(MAKE) clean -C examples/Parameters
	$(MAKE) clean -C examples/SendNote
	$(MAKE) clean -C examples/States
	$(MAKE) clean -C utils/lv2-ttl-generator
ifneq ($(MACOS_OR_WINDOWS),true)
	$(MAKE) clean -C examples/ExternalUI
endif
	rm -rf bin build

# --------------------------------------------------------------

.PHONY: dgl examples tests
