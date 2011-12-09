COMPATH  = /usr/bin
CXX  = icc
CC  = icc
FC  = ifort

MPILIB  = -L/opt/mpt-1.11.1/lib
MPIINC  = -I/opt/mpt-1.11.1/include/

ifdef MPIPRG
# CFM default rule
message  += CFM default uses MPICH, ch_p4, sun4u.
CXX  = icc
CC  = icc
FC  = ifort
endif
message  += You can specify MCPU=-mcpu=pentiumpro,ev5 etc or MCPU=-march=pentium. See the gcc info pages for more information.
OPTXX  = -O3
MOPTXX  = -O3
OPT  = -O3
MOPT  = -O3
OPTF  = -O3
MOPTF  = -O3
DEBUG  = -g
DEBUGF  = -g
LD  = $(CXX)
DEBUGLD  = $(DEBUG)
MOPTLD  = $(MOPTXX)
OPTLD  = $(OPTXX)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
LLAPACK = -L/opt/intel-fc/9.0.025 -lmkl_lapack
LIBS  = -lvec $(FFTLIB) $(LLAPACK) -lmkl -lmkl_ipf -lguide -lstdc++ -lipr  -lg2c
