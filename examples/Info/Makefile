#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_info

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	InfoExamplePlugin.cpp

FILES_UI  = \
	InfoExampleUI.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_OPENGL),true)
TARGETS += jack
TARGETS += lv2_sep
else
TARGETS += lv2_dsp
endif # HAVE_OPENGL

TARGETS += vst

all: $(TARGETS)

# --------------------------------------------------------------
