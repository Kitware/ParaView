CXX  = CC
ifdef MPIPRG
message  += CFM default uses SGIs MPI, to use MPICH define MPICH-std (IRIXN32/ch_shmem) or MPICH, IRIXlvl (IRIXN32, IRIX64), CH_dev (ch_shmem, ch_p4).
ifdef MPICH
ifdef IRIXlvl
ifdef CH_dev
MPICXX  = /users/build/msgpass/mpich/lib/$(IRIXlvl)/$(CH_dev)/mpiC++
MPICC  = /users/build/msgpass/mpich/lib/$(IRIXlvl)/$(CH_dev)/mpicc
endif
endif
else
ifdef MPICH-std
MPICXX  = /users/build/msgpass/mpich/lib/IRIXN32/ch_shmem/mpiC++
MPICC  = /users/build/msgpass/mpich/lib/IRIXN32/ch_shmem/mpicc
else
# CFM default rule
MPICXX  = $(CXX)
MPICC  = $(CC)
MPILIB  = -lmpi
endif
endif
endif
release  = $(shell uname -r)
message  += You may want to specify PLAT (host), ABI (n32), ISA (mips4) or LIST. You can also specify RESTRICT (typed).
ifndef PLAT
PLAT  := $(subst IP,ip,$(shell uname -m))
endif
# CFM default rule
message  += CFM default uses -n32 -mips4.
ifndef ABI
ABI  := n32
endif
ifndef ISA
ISA  := mips4
endif
ifndef RESTRICT
RESTRICT:=typed
endif
extras  = # -Ofast=$(PLAT) -LNO:fusion=2 # -LNO:gather_scatter=2:blocking=off -CG:body_ifc_ratio=10 # -INLINE:list=on -TENV:X=4
OPT  = -O2  -OPT:Olimit=0:roundoff=3:IEEE_arithmetic=3:alias=${RESTRICT}:ldx=on:liberal_ivdep:div_split:fast_exp
OPTXX  = $(OPT)
MOPT  = $(OPT) -TARG:platform=$(PLAT) $(extras)
MOPTXX  = $(MOPT)
OPTF  = -O2  -OPT:Olimit=0:roundoff=3:IEEE_arithmetic=3:ldx=on:liberal_ivdep:div_split:fast_exp
MOPTF  = $(OPTF) -TARG:platform=$(PLAT) $(extras)
ifdef LIST
CPPFLAGS += -LIST:notes=ON -clist
OPTF  += -flist
MOPTF  += -flist
endif
DEBUG  = -g
DEBUGF  = $(DEBUG)
CWOFF  = -woff 1174,1552
CCWOFF  = -woff 1174,3180,1552
FWOFF  = -woff 2274,2271
LD  = $(CXX)
DEBUGLD  = $(DEBUG)
MOPTLD  = $(MOPTXX)
OPTLD  = $(OPTXX)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
LIBS  = -lvec $(FFTLIB) -lcomplib.sgimath -lftn -lm
MPIINC = -I/usr/local/mpich-1.2.3/include
MPILIB = -L/usr/local/lib
