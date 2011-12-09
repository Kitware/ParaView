COMPATH  = /usr/bin
CXX  = $(COMPATH)/g++
CC  = $(COMPATH)/gcc
FC      = /opt/xt-pe/1.3.28/bin/snos64/f77
#FC  = $(COMPATH)/g77

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
DEBUG  = -g
DEBUGF  = -g
LD  = $(CXX)
DEBUGLD  = $(DEBUG)
MOPTLD  = $(MOPTXX)
OPTLD  = $(OPTXX)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
LLAPACK = -L/usr/local/lib -llapack
LBLAS   = -L/usr/local/lib -lblas
LPG     = -L/usr/local/lib -lpgftnrtl -lpgc
LIBS  = -lvec $(FFTLIB) $(LLAPACK) $(LBLAS) -lg2c
