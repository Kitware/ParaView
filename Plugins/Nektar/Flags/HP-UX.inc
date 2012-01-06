CXX  = CC
MPICXX  = mpiCC
MPICC  = mpicc

message  += You may want to specify Aarch (1.1, 2.0 etc.), Smodel (889 etc.) or target (spp1200/spp1600/S2000/X2000) - otherwise target is assumed to be native system.
ifdef Aarch
options  += +DA$(Aarch)
endif
ifdef Smodel
options  += +DS$(Smodel)
endif
ifdef target
options  += +tm $(target)
endif

message  += You may want to specify REPORT (all, loop, private), INFO, CXPA, GPROF, FIRSTPASS, SECONDPASS.
ifdef REPORT
options += +Oreport=$(LIST)
endif
ifdef INFO
options  += +Oinfo
endif
ifdef CXPA
options  += +pa
endif
ifdef GPROF
options  += -G
endif
ifdef FIRSTPASS
options += +I
endif
ifdef SECONDPASS
options += +P +Ostatic_prediction
endif

OPT  = $(options) +O3 +Odataprefetch +Olibcalls +Onofltacc # +ESfic +ESsfc
MOPT  = $(options) +Oall +Odataprefetch +Oloop_transform +Onoparmsoverlap
OPTXX  = $(OPT) # +ESlit
MOPTXX  = $(MOPT) # +ESlit
OPTF  = $(options) +O3 +Odataprefetch +Olibcalls +Onofltacc +U77
MOPTF  = $(options) +Oall +Odataprefetch +Oloop_transform +Onoparmsoverlap +U77
DEBUG  = -g
DEBUGF  = -g
LD  = $(CXX)
OPTLD  = -Wl,-aarchive_shared -Wl,+FPD -Wl,+vnocompatwarnings $(OPT) # -Wl,+Bimmediate
MOPTLD  = $(OPTLD)
DEBUGLD  = $(DEBUG) -Wl,+vnocompatwarnings
ifdef FFTPRG
FFTLIB  = -lfftw
endif
message  += If you specify BLASLIB, -lblas -lvec will be used instead of -lveclib.
ifndef BLASLIB
BLAS  = -lveclib
else
BLAS  = -lblas -lvec
endif
LIBS    = -lVec $(FFTLIB) $(BLAS) -llapack -lm -lcnx_syscall -lpthread -lcps -lail -lisamstub
