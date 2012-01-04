ifdef PARALLEL
MPIINC  = -I/usr1/local/alpha/mpi/include
MPILIB  = -L/usr1/local/alpha/mpi/lib/alpha/ch_p4/
endif
CXX  = cxx
OPT  = $(MPIINC) -preempt_symbol -fast -tune host # -arch ev5
MOPT  = $(MPIINC) $(OPT) -O4 -inline speed # -speculate all # -om -non_shared
OPTXX   = -ansi_args -assume trusted_short_alignment -D_FASTMATH -float -fp_reorder -ifo -O3 -readonly_strings -tune host
MOPTXX  = $(OPTXX) -O4 -inline speed # -speculate all # -om -non_shared
OPTF  = -fast -tune host # -arch ev5
MOPTF  = $(OPTF) -O5 # -speculate all # -om -non_shared
DEBUG  = $(MPIINC) -preempt_symbol -gall -g2 -trapuv # -ieee # -check
DEBUGF  = -g2 -cpp # -ieee
LD  = $(CXX)
DEBUGLD  = $(DEBUG)
MOPTLD  = $(MOPTXX)
OPTLD  = $(OPTXX)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
VECLIB  = -L../../Veclib -lvec
LIBS  = $(VECLIB) $(FFTLIB)  -ldxml -lm -lfor
