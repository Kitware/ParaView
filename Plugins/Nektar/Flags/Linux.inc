COMPATH  = /usr/bin
CXX  = $(COMPATH)/g++
#CXX  = $(COMPATH)/c++
CC  = $(COMPATH)/gcc
FC      = $(COMPATH)/gfortran
#FC      = $(COMPATH)/f77
# commented by LG in order to compile on front node on XT3 PSC
#FC  = $(COMPATH)/f77

#MPILIB  = -L/usr/local/bin/
MPI_DIR = /home/venkatv/software/mpich2_joe

MPILIB  = -L$(MPI_DIR)/lib/
MPIINC  = -I$(MPI_DIR)/include/

ifdef MPIPRG
# CFM default rule
message  += CFM default uses MPICH, ch_p4, sun4u.
CXX  = $(MPI_DIR)/bin/mpicxx
CC  = $(MPI_DIR)/bin/mpicc
FC  = $(MPI_DIR)/bin/mpif77
#CXX  = /usr/local/bin/mpiCC
#CC  = /usr/local/bin/mpicc
#FC  = /usr/local/bin/mpif77

endif
message  += You can specify MCPU=-mcpu=pentiumpro,ev5 etc or MCPU=-march=pentium. See the gcc info pages for more information.

OPTXX  = -fPIC -O3 $(MCPU) -ffast-math
MOPTXX  = -fPIC -O3 $(MCPU) -ffast-math -funroll-loops -fstrict-aliasing
OPT  = -fPIC -O3 $(MCPU) -ffast-math
MOPT  = -fPIC -O3 $(MCPU) -ffast-math -funroll-loops -fstrict-aliasing
OPTF  = -fPIC -O3 $(MCPU) -ffast-math
MOPTF  = -fPIC -O3 $(MCPU) -ffast-math -funroll-loops

#OPTXX  = -O3 $(MCPU) -ffast-math
#MOPTXX  = -O3 $(MCPU) -ffast-math -funroll-loops
#OPT  = -O3 $(MCPU) -ffast-math
#MOPT  = -O3 $(MCPU) -ffast-math -funroll-loops
#OPTF  = -O3 $(MCPU) -ffast-math
#MOPTF  = -O3 $(MCPU) -ffast-math -funroll-loops

DEBUG  = -g
DEBUGF  = -g
LD  = $(CXX)
DEBUGLD  = $(DEBUG)
MOPTLD  = $(MOPTXX)
OPTLD  = $(OPTXX)
ifdef FFTPRG
FFTLIB  = -lfftw
endif


LLAPACK = -llapack
LBLAS = -lblas



#LLAPACK = -L/usr/local/lib -llapack
#LBLAS   = -L/usr/local/lib -lblas
LPG     = -L/usr/local/lib   -lpgftnrtl -lpgc
LIBS  = -lvec $(FFTLIB)      $(LLAPACK) $(LBLAS) -lg2c
