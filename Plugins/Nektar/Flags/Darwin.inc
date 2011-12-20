COMPATH  = /usr/bin
CXX  = $(COMPATH)/g++
CC  = $(COMPATH)/gcc
ifeq ($(MACHTYPE),i386)
FC  = /sw/bin/g95
FFLAGS = -DNO_DLAMCH
CPPFLAGS := $(CPPFLAGS) -D__APPLE_INTEL__
else
FC   = /sw/bin/g77
endif

MPILIB  = -L/usr/local/bin/
MPIINC  = -I/usr/local/include/

ifdef MPIPRG
# CFM default rule
message  += CFM default uses MPICH, ch_p4, sun4u.
CXX  = /usr/local/bin/mpiCC
CC  = /usr/local/bin/mpicc
FC  = /usr/local/bin/mpif77
endif
message  += You can specify MCPU=-mcpu=pentiumpro,ev5 etc or MCPU=-march=pentium. See the gcc info pages for more information.
OPTXX  = -O3 $(MCPU) -ffast-math
MOPTXX  = -O3 $(MCPU) -ffast-math -funroll-loops -fstrict-aliasing
OPT  = -O3 $(MCPU) -ffast-math
MOPT  = -O3 $(MCPU) -ffast-math -funroll-loops -fstrict-aliasing
OPTF  = -O3 $(MCPU) -ffast-math
MOPTF  = -O3 $(MCPU) -ffast-math -funroll-loops
DEBUG  = -g  -gstabs+
DEBUGF  = -g $(FPPFLAGS)
LD  = $(CXX) -bind_at_load
DEBUGLD  = $(DEBUG)
MOPTLD  = $(MOPTXX)
OPTLD  = $(OPTXX)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
LPG     = -L/usr/local/lib -lpgftnrtl -lpgc
ifeq ($(MACHTYPE),i386)
LIBS  = -lvec $(FFTLIB) -framework Accelerate -L/sw/lib -lg95
else
LIBS  = -lvec $(FFTLIB) -framework Accelerate -L/sw/lib -lg2c
endif
