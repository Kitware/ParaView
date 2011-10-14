## VECLIB - Library of Vector Routines
#
# The following collection of C functions is a (partial) implementation
# of the Intel VECLIB library packaged with the iPSC/860 hypercube. For
# documentation, see the iPSC/860 Programmers Reference Manual.
#
# Ron Henderson
# Department of Mechanical and Aerospace Engineering
# Princeton University
# Princeton, New Jersey  08540
#
#
# This makefile is compatible with GNU Make version 3.59 by
# Richard Stallman and Roland McGrath.
#
# ---------------------------------------------------------------------

ARCH  := $(shell uname)
ifeq ($(ARCH),OSF1)
override ARCH := $(shell uname -m)
endif
MACHTYPE = $(shell uname -m)
HYBRID   = ..
VPATH   = math:triads:relational:reduction:conversion:misc:memory
LIB   = libvec.a
LIBDIR   = ../Hlib/$(ARCH)
INCDIR   = ../include
TARFILE   = $(patsubst %.a, %.tar, $(LIB))
ifndef RFFT
RFFT   = # vendFFT
endif

# These are the module names:

MATH  = xfill   xneg   xvneg  xvadd  xvsub  xsmul  xvmul  xvrecp  \
    xvdiv   xzero  xsadd  xsdiv  xvabs  xvamax xvsin  xvcos   \
    xvsqrt  xvlg10 xvexp

TRIADS  = xsvtvm  xsvtvp  xsvvmt xsvvpt xsvvtm xsvvtp xvvtvm \
    xvvtvp  xvvpvt  xvvmvt xsvtsp xvvvtm

RELATE  = xseq    xsle    xslt   xsne

REDUCE  = xsum    ixmax   ixmin  icount ifirst xvpoly

CONVERT  = xvfloa  vdble   vsngl  xvrand xvreal xvimag xvcmplx xbrev

MISC  = xscatr  xgathr  xramp  xcndst xmask  polint xfft   \
    mxm     mxv     mxva   spline veclib dclock zbesj  mach

ifeq ($(RFFT),vendFFT)
ifeq ($(ARCH),IRIX64)
FFT  = mips-fftdf
endif
ifeq ($(ARCH),IRIX)
FFT  = mips-fftdf
endif
ifeq ($(ARCH),AIX)
FFT  = ibm-fftdf
endif
ifeq ($(ARCH),cray-pvp)
FFT  = scilibd-fftdf
endif
ifeq ($(ARCH),cray-t3d)
FFT  = scilibd-fftdf
endif
ifeq ($(ARCH),unicosmk)
FFT  = scfftd-fftdf
endif
else
FFT  = fftdf
endif

MEMORY  = xvector xmatrix icopy

# ---------------------------------------------------------------- #
# The following expand to give the names of all sections (ALL),    #
# all library member dependencies (LMOD), all associated           #
# object files (OBJS), and all source code files (SRC).            #
# ---------------------------------------------------------------- #


ALL   = $(MATH) $(TRIADS) $(RELATE) $(REDUCE) $(CONVERT) $(MISC) \
  $(MEMORY) $(FFT)

LMOD  = $(foreach routine, $(ALL), $(LIB)($(routine).o))
OBJS  = $(foreach routine, $(ALL), $(routine).o)

# -----------------------------------------------------------

# Compilers and options

CFLAGS   = $(OPT)
FFLAGS   = $(OPTF)
CPPFLAGS = $(defines) -I$(INCDIR)

# -----------------------------------------------------------
# Override certain variables on various computer systems

include  ../Flags/${ARCH}.inc

ifeq ($(ARCH),SunOS)      # SunOS 5.*
include ../Flags/SunOS.inc
#OPT  += -Qoption iropt -O4+vector
endif

ifeq ($(ARCH),IRIX)      # Silicon Graphics IRIX
include ../Flags/IRIX.inc
endif

ifeq ($(ARCH),IRIX64)      # Silicon Graphics IRIX64
include ../Flags/IRIX64.inc
endif

ifeq ($(ARCH),AIX)      # IBM AIX
include ../Flags/AIX.inc
endif

ifeq ($(ARCH),cray-pvp)      # Cray PVP
include ../Flags/CRAYPVP.inc
endif

ifeq ($(ARCH),cray-t3d)      # Cray T3D
include ../Flags/T3D.inc
defines += -DCYBENCHLIB
endif

ifeq ($(ARCH),unicosmk)      # Cray T3E
include ../Flags/T3E.inc
endif

ifeq ($(ARCH),alpha)      # DEC Alpha AXP
include ../Flags/DECAXP.inc
endif

ifeq ($(ARCH),HP-UX)      # HP-UX
include ../Flags/HP-UX.inc
LIB     = libVec.a
endif

ifeq ($(ARCH),GCC)      # Generic GCC platform
include ../Flags/GCC.inc
endif

ifeq ($(ARCH),Linux)      # Linux = Generic
include ../Flags/Linux.inc
endif

ifeq ($(ARCH),XT3)                      # XT3 = CRAY XT3
include ../Flags/XT3.inc
endif

ifeq ($(ARCH),XT5)                      # XT3 = CRAY XT3
include ../Flags/XT5.inc
endif


ifeq ($(ARCH),RANGER)                   # RANGER, TACC
include ../Flags/RANGER.inc
endif



# ------------------------------------------------------------------
#               DO NOT MAKE CHANGES BELOW THIS LINE
# ------------------------------------------------------------------

message  += You can specify DBX for a debugging version or OPTM for a maximally (?) optimized one.
ifdef DBX
override CFLAGS := $(DEBUG)
override FFLAGS := $(DEBUGF)
endif
ifdef OPTM
override CFLAGS := $(MOPT)
override FFLAGS := $(MOPTF)
endif

$(LIB): $(LMOD)
  ranlib $(LIB)
  @echo '$(message)'

# ------------------------------------------------------------------
#                 Special targets
# ------------------------------------------------------------------

clean:
  -rm -f *.o *~

tar: Makefile
  @echo 'creating a tape archive in' $(TARFILE)
  -tar cvf $(TARFILE) README Makefile \
  math/*.c triads/*.c relational/*.c reduction/*.c conversion/*.c \
  misc/*.[cfF] memory/*.c

install: $(LIB)
  /bin/cp $(LIB) $(LIBDIR)

# ------------------------------------------------------------------
#                 Machine Targets
# ------------------------------------------------------------------

cray:
        $(MAKE) ARCH=cray-pvp
t3d:
        $(MAKE) ARCH=cray-t3d

# ------------------------------------------------------------------
#                 Special rules to take care of -o behavior in CRAYs
# ------------------------------------------------------------------

.f.o:
  $(FC) $(FFLAGS) -c $<

.F.o:
  $(FC) $(FFLAGS) $(defines) -c $<
