ARCH   = $(shell uname)
VPATH   = ./
LIB   = libmatsolve.a
INCDIR   = ../../include
TARFILE   = matsolve.tar

# Compilers and options

CPPFLAGS = -I./ -I$(INCDIR) -I$(HOME)/Nektar/include -g  -gstabs+

ifeq ($(ARCH),SunOS)      # SunOS 5.*
include ../../Flags/SunOS.inc
#OPT += -Qoption iropt -O4+vector
endif

ifeq ($(ARCH),ap3000)      # ap30000
include ../../Flags/ap3000.inc
#OPT += -Qoption iropt -O4+vector
endif


ifeq ($(ARCH),IRIX)      # Silicon Graphics IRIX
include ../../Flags/IRIX.inc
endif

ifeq ($(ARCH),IRIX64)      # Silicon Graphics IRIX64
include ../../Flags/IRIX64.inc
endif

ifeq ($(ARCH),AIX)      # IBM AIX
include ../../Flags/AIX.inc
endif

ifeq ($(ARCH),cray-pvp)      # Cray PVP
include ../../Flags/CRAYPVP.inc
endif

ifeq ($(ARCH),cray-t3d)      # Cray T3D
include ../../Flags/T3D.inc
endif

ifeq ($(ARCH),unicosmk)      # Cray T3E
include ../../Flags/T3E.inc
endif

ifeq ($(ARCH),alpha)      # DEC Alpha AXP
include ../../Flags/DECAXP.inc
endif

ifeq ($(ARCH),HP-UX)      # HP-UX
include ../../Flags/HP-UX.inc
endif

ifeq ($(ARCH),GCC)      # Generic GCC platform
include ../../Flags/GCC.inc
endif

ifeq ($(ARCH),Linux)      # Linux ==  Generic GCC platform
include ../../Flags/GCC.inc
endif

ifeq ($(ARCH),Linux-alpha)          # Linux on alpha processor
include ../../Flags/GCC.inc
endif

ifeq ($(ARCH),SUPER-UX)      # NEC machine
include ../../Flags/Nec.inc
endif


# These are the module names:

ALL   = GMatrix  MatSolve

# ---------------------------------------------------------------- #
# The following expand to give the names of all sections (ALL),    #
# all library member dependencies (LMOD), all associated           #
# object files (OBJS), and all source code files (SRC).            #
# ---------------------------------------------------------------- #

LMOD  = $(foreach routine, $(ALL), $(LIB)($(routine).o))
OBJS  = $(foreach routine, $(ALL), $(routine).o)

# ----------------------------------------------------------------------------
# Make library, default action.
#
$(LIB): $(LMOD)
  ranlib $(LIB)

# ----------------------------------------------------------------------------
# Install library & associated header files.
#
install:
  cp -f GMatrix.h     $(INCDIR)  ;
  cp -f SClevel.h    $(INCDIR)  ;
  cp -f MatSolve.h    $(INCDIR)  ;
  cp -f libmatsolve.a   ../$(ARCH);

help:
  @echo ''
  @echo ' Run make :'
  @echo ''

tar: Makefile
  @echo 'creating a tape archive in' $(TARFILE)
  @tar cvf $(TARFILE) Makefile *.cpp *.h
  gzip -9 $(TARFILE)

clean:
  -rm -f *.o *~
