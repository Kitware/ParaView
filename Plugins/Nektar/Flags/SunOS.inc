CXX  = CC
ifdef MPIPRG
# CFM default rule
message  += CFM default uses MPICH, ch_shmem, sun4u. Youcan specify MPICH, CH_dev (ch_shmem, ch_shmem-g, ch_p4).
ifdef MPICH
ifdef CH_dev
MPICXX  = /users/build/msgpass/mpich/lib/solaris/$(CH_dev)/mpiC++
MPICC  = /users/build/msgpass/mpich/lib/solaris/$(CH_dev)/mpicc
endif
else
# CFM default rule
MPICXX  = /users/build/msgpass/mpich/lib/solaris/ch_shmem/mpiC++
MPICC  = /users/build/msgpass/mpich/lib/solaris/ch_shmem/mpicc
endif
endif
KERNEL  = $(shell uname -m)
# CFM default rule
message  += CFM default uses -xarch=v8plusa for sun4u, generic otherwise.
ifeq ($(KERNEL),sun4u)
XARCH  = v8plusa
else
XARCH  = generic
endif
message  += You may want to specify XPUSE or XPCOLLECT
ifdef XPUSE
PROF  = -xprofile=use:$(XPUSE)
endif
ifdef XPCOLLECT
PROF  = -xprofile=collect:$(XPCOLLECT)
endif
message  += You may want to specify XTARGET, XARCH, XCHIP, XCACHE and RESTRICT.
ifdef XCACHE
override XCACHE  := -xcache=$(XCACHE)
complete_target += 1
endif
ifdef XCHIP
override XCHIP  := -xchip=$(XCHIP)
complete_target += 1
endif
ifdef XARCH
ifeq ($(XARCH),v8plus)
ISA  := v8plus
endif
ifeq ($(XARCH),v8plusa)
ISA  := v8plus
endif
override XARCH  := -xarch=$(XARCH)
complete_target += 1
endif
# No need for -xtarget= if all of the 3 above are specified.
ifdef XTARGET
ifeq ($(complete_target),3)
override XTARGET :=
else
override XTARGET := -xtarget=$(XTARGET)
endif
endif
OPTXX  = -fast -xO4 $(XTARGET) $(XARCH) $(XCHIP) $(XCACHE) $(PROF)
MOPTXX  = $(OPTXX) -fsimple=2 -xunroll=8
OPT  = $(OPTXX) -xdepend
MOPT  = $(OPT) -fsimple=2 -xunroll=8
OPTF  = -fast -xO4 -xdepend $(XTARGET) $(XARCH) $(XCHIP) $(XCACHE)  $(PROF)
MOPTF  = $(OPTF) -fsimple=2 -xunroll=8
ifeq ($(ISA),v8plus)
MOPTXX  += -xsafe=mem
endif
ifdef RESTRICT
ifneq ($(RESTRICT),1)
MOPT  += -xrestrict=$(RESTRICT)
else
MOPT  += -xrestrict
endif
endif
DEBUG  = -g
DEBUGF  = -g
LD  = $(CXX)
DEBUGLD  = $(DEBUG) # $(XARCH)
MOPTLD  = $(MOPTXX)
OPTLD  = $(OPTXX)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
message += You may want to specify STATIC=1 (for libsunperf)
ifdef STATIC
BSTATIC = -Bstatic
Bdynamic = -Bdynamic
endif
LIBS  = -lvec $(FFTLIB) $(MPILIB) $(BSTATIC) -xlic_lib=sunperf $(BDYNAMIC) -lF77 -lM77 -lV77 -xlic_lib=sunmath -lm # -lfast
