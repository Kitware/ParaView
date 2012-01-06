message  = Extra build flags:\n\n
#COMPATH  = /opt/xt-pe/1.3.10/bin/snos64/
CXX  = mpicxx
CC  = mpicc
FC  = mpif90
ifdef MPIPRG
# CFM default rule
message  += CFM default uses LAM,\n to use MPICH define MPICH.\n\n
ifdef MPICH
MPICXX  = CC
MPICC  = cc
MPILIB  =
else
MPICXX  = CC
MPICC  = cc
MPILIB  =
endif
endif
# LLAPACK  = #-llapack -lsci
LLAPACK = -L$(TACC_ACML_LIB) -lacml
LBLAS  = #-lblas


#use with mvapich/1.0.1 and pgi/7.2-5
BLACSdir=$(TACC_SCALAPACK_LIB)
SLALAPACKdir=$(TACC_SCALAPACK_LIB)
MPILIB=
PLAT=LINUX
SCALAPACK  = $(SLALAPACKdir)libscalapack.a $(BLACSdir)blacsCinit_MPI-$(PLAT)-0.a $(BLACSdir)blacs_MPI-$(PLAT)-0.a $(BLACSdir)blacsCinit_MPI-$(PLAT)-0.a
GOTO_BLAS = -L$(TACC_GOTOBLAS_LIB) -lgoto_lp64
LAPACKLIB  =   $(SCALAPACK) $(GOTO_BLAS)  $(TACC_ACML_LIB)/libacml.a $(SCALAPACK)

F90LIB = -L/share/apps/pgi/7.1/linux86-64/7.1-2/lib -lpgf90 -lpgf90_rpm1 -lpgf902 -lpgf90rtl -lpgftnrtl -lnspgc -lpgc -lrt -lpthread -lm


message  += You can specify MCPU=-mcpu=pentiumpro,ev5 etc.\n or MCPU=-march=pentium. See the gcc info pages for more information.
OPTXX  = -O2 -tp barcelona-64 #$(MCPU)  -fastsse   -malign-double -ffast-math
MOPTXX  = -O3  -fastsse -Munroll -tp barcelona-64   #$(MCPU)  -malign-double -ffast-math -funroll-loops -fstrict-aliasing
OPT = $(OPTXX)
MOPT = $(MOPTXX)
OPTF  =
MOPTF  =
OPTF  = -O2 -tp barcelona-64 #$(MCPU) -malign-double -ffast-math # -fno-globals
MOPTF  = -O3 -fastsse -Munroll -tp barcelona-64   #$(MCPU) -malign-double -ffast-math -funroll-loops # -fno-globals
DEBUG  = -g #-Mprof=func
DEBUGF  = -g #-Mprof=func
LD  = $(CXX)
DEBUGLD  = $(DEBUG)
MOPTLD  = $(MOPTXX)
OPTLD  = $(OPTXX)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
LPDIO   = #-L/usr/local/packages/pdio/nektar/lib -lpdio
# LIBS    = $(FFTLIB) $(MPILIB) $(LLAPACK) $(LBLAS) -lpgf90 -lpghpf2 -lpghpf_mpi -lpgftnrtl -lvec $(LPDIO)
LIBS    = $(FFTLIB) $(MPILIB) $(LAPACKLIB) $(F90LIB)
#LIBS  = $(FFTLIB) $(MPILIB) $(LLAPACK) $(LBLAS) -L/share/apps/pgi/7.1/linux86-64/7.1-2/lib -lpgf90 -lpgf90_rpm1 -lpgf902 -lpgf90rtl -lpgftnrtl -lnspgc -lpgc -lrt -lpthread  -lvec -lm
# LIBS  = $(FFTLIB) $(MPILIB) $(LLAPACK) $(LBLAS) -lvec
XLIBDIRS = -L/usr/X11/lib
XINCDIRS = -I/usr/include/X11
