message  += Define PROCS otherwise PROCS=m. You may want to specify APPRENTICE xor PAT. Also VERBOSE and in that case message level MLEVEL (0-4, default 2), C/C++ optimization report level REPORT (istvf), f90 listing level LISTING (0-6,c,g,l,m,s,w,x,T).
CXX  = CC
ifdef MPIPRG
MPICXX  = $(CXX)
MPICC  = $(CC)
ifndef PROCS
PROCS  = -Xm
else
PROCS  := -X$(PROCS)
endif
endif
FC  = f90
OPT  = -h scalar3
MOPT  = -h scalar3,vector3,aggress,inline3,reduction,pipeline3,unroll,split
OPTXX  = $(OPT)
MOPTXX  = $(MOPT)
OPTF  = -dp -Oscalar3
MOPTF  = -dp -a pad -Oscalar3,vector3,aggress,inline3,unroll2,pipeline3,split2
DEBUG  = -Gn -h indef
DEBUGF  = -G0 -RabcnsCE -dp -ei # -eiI
LD  = $(CXX)
DEBUGLD  = -Wl"-Dstreams=on" $(PROCS)
MOPTLD  = $(DEBUGLD)
OPTLD  = $(DEBUGLD)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
LIBS  = -lvec $(FFTLIB)

ifdef APPRENTICE
OPT  += -h apprentice
MOPT  += -h apprentice
OPTF  += -eA
MOPTF  += -eA
LIBS  += -lapp
endif
ifdef PAT
LIBS  += -lpat pat.cld
endif
ifdef VERBOSE
ifndef MLEVEL
MLEVEL  := 2
endif
OPT  += -h msglevel_$(MLEVEL)
MOPT  += -h msglevel_$(MLEVEL)
OPTF  += -m$(MLEVEL)
MOPTF  += -m$(MLEVEL)
ifdef REPORT
OPT  += -h report=$(REPORT)
MOPT  += -h report=$(REPORT)
OPTF  += -Omsgs,negmsgs
MOPTF  += -Omsgs,negmsgs
endif
ifdef LISTING
OPTF  += -r $(LISTING)
MOPTF  += -r $(LISTING)
endif
endif
