MARCH   = $(shell uname)
MACHTYPE = $(shell uname -m)
ifeq ($(ARCH),OSF1)
override ARCH := $(MACHTYPE)
else
ifeq ($(MACHTYPE),alpha)
override ARCH := $(ARCH)-$(MACHTYPE)
endif
endif
HYBRID   = ../
VPATH   = ../SRC_PV_NEK
LIB   = libnektar
INCDIR   = ../../include
TARFILE   = $(LIB).tar

# Compilers and options

ifdef PARALLEL
MPIPRG  = yes
endif

defines  =  METIS

CPPFLAGS :=  -I$(INCDIR)

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
include ../../Flags/Linux.inc
endif

ifeq ($(ARCH),Linux-alpha)          # Linux on alpha processor
include ../../Flags/GCC.inc
endif

ifeq ($(ARCH),Darwin)
include ../../Flags/Darwin.inc
endif

ifeq ($(ARCH),SUPER-UX)      # NEC machine
include ../../Flags/Nec.inc
endif

ifeq ($(ARCH),XT3)                      # CRAY XT3
include ../../Flags/XT3.inc
endif

ifeq ($(ARCH),XT4)                      # CRAY XT4
include ../../Flags/XT4.inc
endif

ifeq ($(ARCH),XT5)                      # CRAY XT5
include ../../Flags/XT5.inc
endif



ifeq ($(ARCH),RANGER)                   # RANGER, TACC
include ../../Flags/RANGER.inc
endif

ifeq ($(ARCH),BgP)                      # BlueGene/P ALCF
include ../../Flags/BgP.inc
endif



ifeq ($(COMPRESS),1)
override defines := $(defines) COMPRESS
endif

ifeq ($(PARALLEL),1)
override defines := $(defines) PARALLEL
endif

ifeq ($(LEGENDRE),1)
override defines := $(defines) LEGENDRE
endif

ifeq ($(LANC),1)
override defines := $(defines) Lanczos
endif

ifdef    defines
DEF     = $(foreach def, $(defines) , -D$(def))
override CPPFLAGS := $(CPPFLAGS) $(DEF) -w
endif


# These are the module names:

ALL =   nek2vel_PV_interface gen_utils io \
        prepost mlevel bwoptim comm_PV_NEKTAR \
        dgalerkin  ScaLapackNektar_new test_partition_connectivity \
        sgs


export

help:
  @echo ''
  @echo ' Nektar Makefiles:'
  @echo ''
  @echo ' Run make with one of the following options:'
  @echo '  a) dbx....dbx version'
  @echo '  b) opt....optimised version'
  @echo '  c) mopt...maximally (?) optimized version'
  @echo ''
  @echo '$(message)'

dbx:
  $(MAKE) LTYPE=g -f MakeTEST
  @echo '$(message)'

opt:
  $(MAKE) LTYPE=opt -f MakeTEST
  @echo '$(message)'

mopt:
  $(MAKE) LTYPE=mopt -f MakeTEST
  @echo '$(message)'

tar: Makefile
  @echo 'creating a tape archive in' $(TARFILE)
  @tar cvf $(TARFILE) Makefile MakeTEST src/*.[cCfFy]

clean:
  -rm -f *.o *~
