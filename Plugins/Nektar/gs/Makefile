#
# GSLIB - GLobal Message Library by Henry Tufo
#
#
# This makefile is compatible with GNU Make version 3.59 by
# Richard Stallman and Roland McGrath.
#
# ---------------------------------------------------------------------

ARCH  := $(shell uname)
MACH  := $(shell uname -m)
ifeq ($(ARCH),OSF1)
override ARCH := $(shell uname -m)
endif

ifeq ($(ARCH),Linux)
  ifeq ($(MACH),ia64)
  ARCH =  Linux-ia64
  endif
endif
HYBRID  = ../
VPATH  = src
LIB  = libgs.a
INCDIR  =
TARFILE  = $(patsubst %.a, %.tar, $(LIB))
MPIPRG  = yes
PARALLEL = yes

# Compilers and options

defines  = MPISRC SAFE # DEBUG VV INFO
CPPFLAGS = -I./include

override defines := r8 $(defines)

include $(HYBRID)/Flags/$(ARCH).inc

ifeq ($(ARCH),cray-pvp)      # Cray PVP
include $(HYBRID)/Flags/CRAYPVP.inc
endif

ifeq ($(ARCH),cray-t3d)      # Cray T3D
include $(HYBRID)/Flags/T3D.inc
endif

ifeq ($(ARCH),unicosmk)      # Cray T3E
include $(HYBRID)/Flags/T3E.inc
endif

ifeq ($(ARCH),alpha)      # DEC Alpha AXP
include $(HYBRID)/Flags/DECAXP.inc
endif

ifeq ($(ARCH),XT3)                      # CRAY XT3
include $(HYBRID)/Flags/XT3.inc
endif

ifeq ($(ARCH),XT5)                      # CRAY XT5
include $(HYBRID)/Flags/XT5.inc
endif


ifeq ($(ARCH),RANGER)                   # RANGER. TACC
include ../Flags/RANGER.inc
endif


ifeq ($(ARCH),BgP)                     # BlueGene/P ALCF
include ../Flags/BgP.inc
endif



ifdef    defines
DEF     = $(foreach def, $(defines) $(ADDONS), -D$(def))
override CPPFLAGS :=  $(DEF) $(CPPFLAGS)
endif

# These are the module names:

GS = bit_mask bss_malloc ivec error comm gs queue stack # ex

ifeq ($(LTYPE),g)  # turn on debug flag
CPPFLAGS := -DDEBUG $(CPPFLAGS)
CFLAGS := $(DEBUG)
else      # Maximal optimization flags
ifeq ($(LTYPE),mopt)
CFLAGS := $(MOPT)
else      # Regular optimization flags
ifeq ($(LTYPE),opt)
CFLAGS := $(OPT)
endif
endif
endif

# override CPPFLAGS :=  $(CPPFLAGS) -I$(MPIINC)
override CFLAGS :=  $(CFLAGS) $(MPIINC)

# ---------------------------------------------------------------- #
# The following expand to give the names of all sections (ALL),    #
# all library member dependencies (LMOD), all associated           #
# object files (OBJS), and all source code files (SRC).            #
# ---------------------------------------------------------------- #

ALL   = $(GS)

LMOD  = $(foreach routine, $(ALL), $(LIB)($(routine).o))
OBJS  = $(foreach routine, $(ALL), $(routine).o)

# -----------------------------------------------------------

help:
  @echo ''
  @echo ' GSlib Makefiles:'
  @echo ''
  @echo ' Run make with one of the following options:'
  @echo '  a) dbx....dbx version'
  @echo '  b) opt....optimised version'
  @echo '  c) mopt...maximally (?) optimized version'
  @echo ''
  @echo '$(message)'

$(LIB): $(LMOD)

tester:  driver.c $(LIB)
  $(MPICC) $(CFLAGS) $(CPPFLAGS) -o tester driver.c -L. -lgs $(MPILIB)

dbx:
  $(MAKE) LTYPE=g $(LIB)
  @echo '$(message)'

opt:
  $(MAKE) LTYPE=opt $(LIB)
  @echo '$(message)'

mopt:
  $(MAKE) LTYPE=mopt $(LIB)
  @echo '$(message)'

testerdbx:
  $(MAKE) LTYPE=g tester
  @echo '$(message)'

testeropt:
  $(MAKE) LTYPE=opt tester
  @echo '$(message)'

testermopt:
  $(MAKE) LTYPE=mopt tester
  @echo '$(message)'

tar: Makefile
  @echo 'creating a tape archive in' $(TARFILE)
  @tar cvf $(TARFILE) Makefile driver.c src/*.c include/*.h

clean:
  -rm -f *.o *.a

srcclean:
  -rm -f src/*~ include/*~

veryclean: clean srcclean
