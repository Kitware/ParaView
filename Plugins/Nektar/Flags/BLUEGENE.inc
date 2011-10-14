CC  = xlC
FC  = xlf
CXX  = xlC
MPICXX  = mpCC
MPICC  = mpcc
ifdef PARALLEL
CC     = $(MPICC)
CXX    = $(MPICXX)
endif

qoptions = #-qinlglue -qproto
soptions= #-qstrict
message = Extra gmake options:\n\n
# Large memory options
message += You can specify LDATA=0xn0000000 to ask for n 256 segments of data (up to 8, default 1) if you have more than 256MB of data. You can specify LSTACK=Ox1000000 to ask for the maximum 256MB of stack.\n\n
ifdef LDATA
largeopt += -bmaxdata:$(LDATA)
endif
ifdef LSTACK
largeopt += -bmaxstack:$(LSTACK)
endif
# Set for pass1/2 of the feedback based optimizations XOR fdpr
message += You can specify PDF1, PFD2 or FDPR - exclusively.\n\n
ifdef PDF1
profopt = -qpdf1
pdflib  = -lpdf
else
ifdef PDF2
profopt = -qpdf2
else
ifdef FDPR
profopt = -qfdpr
endif
endif
endif
# Interprocedural optimization
message += You can specify IPA.\n\n
ifdef IPA
ipa  = -qipa
endif
# Set for extra debugging control
message += You can specify EXTRADEBUG for trapping etc.\n\n
ifdef EXTRADEBUG
debug  = -qcheck=all -qfloat=nans -qextcheck -qflttrap=overflow:underflow:zerodivide:invalid:inexact:enable #:imprecise
endif
hot  = -qhot $(dcache) $(icache) $(tlb) $(l2cache)
CPPFLAGS += -DSTRDUP
options  = $(profopt) $(ipa)
OPT   =  -g -O3 -Q $(options) $(qoptions) $(soptions)
#OPT  = -qcpluscmt -O3 -Q $(options) $(qoptions) $(soptions)
MOPT  = -O5 -qnoipa -Q $(options) $(qoptions) $(soptions)
OPTXX  = $(OPT)
MOPTXX  = $(MOPT)
OPTF  =  -O2 -Q $(options) $(soptions)
#-qextname
MOPTF  =  -O5 -qnoipa -Q $(options) $(soptions)
#-qextname
DEBUG  = -qcpluscmt -g -qfullpath $(debug)
DEBUGF  = $(largeopt)
#-qextname
# C using the KAP C preprocessor
message  += You can specify KAP to use both the KAP C & Fortran preprocessor or just KAPC for the C preprocessor.\n\n
ifdef KAP
KAPC  = 1
KAPF  = 1
endif
ifdef KAPC
CC  = kxlc
MOPT  += +K5 +Ktmpdir=/tmp $(KCFLAGS) # +Ksif
KLIBS  = -L/usr/lpp/kapc/lib -lkapio
endif
message += You can specify HOT for compiler hot transformations or you can specify KAPF for the KAP Fortran preprocessor.\n\n
ifndef HOT
ifdef KAPF
#Fortran using the KAP Fortran preprocessor
MOPTF  += -Pk $(KFFLAGS)
endif
else
#Fortran using the compiler's hot transformations
MOPTF  += $(hot)
endif
LD  = $(CXX)
DEBUGLD  = $(DEBUG) $(largeopt)
MOPTLD  = $(MOPTXX) $(largeopt)
OPTLD  = $(OPTXX) $(largeopt)
ifdef FFTPRG
FFTLIB  = -lfftw
endif
#LIBS  = -lvec $(FFTLIB) $(LLAPACK) $(LBLAS) $(LBLACS) $(pdflib) $(KLIBS) -lxlf90 -lxlf
LIBS  = -lvec $(FFTLIB) $(LLAPACK) $(LBLAS) $(LBLACS)  $(pdflib) $(KLIBS) -L/opt/ibmcmp/xlf/bg/10.1/blrts_lib -lxlf90 -lxl #-L/usr/local/apps/ipm/lib -lipm

message  += You can specify ARCH2 (sp2-wide/thin2/thin,sp1/f50/com/p690). CFM default is p690.\n\n
ifndef ARCH2
ARCH2  = p690
endif

ifeq ($(ARCH2),p690)    # HPCx
dcache  = -qcache=type=d:level=1:assoc=2:line=128:size=32:cost=12
icache  = -qcache=type=i:level=1:assoc=0:line=128:size=64:cost=12
l2cache = -qcache=type=c:level=2:assoc=8:line=128:size=768
options  += -qarch=pwr4 -qtune=pwr4
LBLAS  = -lessl
LLAPACK  = -L/usr/local/lib -llapack
endif

ifeq ($(ARCH2),sp2-wide)  # IBM SP2 "wide" node
dcache  = -qcache=type=d:level=1:assoc=4:line=256:size=256
icache  = -qcache=type=i:level=1:assoc=2:line=128:size=32
tlb  = -qcache=type=d:level=2:assoc=2:line=4096:size=2048 \
    -qcache=type=i:level=2:assoc=2:line=4096:size=512
options  += -qarch=pwr2 -qtune=pwr2
KCFLAGS  += +Kargs=-r=3:-chs=256:-chl=256
KFFLAGS  += -Wp,-r=3,-inl,-f,-chs=256,-chl=256
LBLAS  = -lesslp2
#LLAPACK  = -L/usr/local/lib -llapack_esslp2
endif

ifeq ($(ARCH2),sp2-thin2)  # IBM SP2 "thin2" node
dcache  = -qcache=type=d:level=1:assoc=4:line=128:size=128
icache  = -qcache=type=i:level=1:assoc=2:line=128:size=32
tlb  = -qcache=type=d:level=2:assoc=2:line=4096:size=2048 \
    -qcache=type=i:level=2:assoc=2:line=4096:size=512
options  += -qarch=pwr2 -qtune=pwr2
KCFLAGS  += +Kargs=-r=3:-chs=128
KFFLAGS  += -Wp,-r=3,-inl,-f,-chs=128
LBLAS  = -lesslp2
endif

ifeq ($(ARCH2),sp2-thin)  # IBM SP2 "thin" node
dcache  = -qcache=type=d:level=1:assoc=4:line=128:size=64
icache  = -qcache=type=i:level=1:assoc=2:line=128:size=32
tlb  = -qcache=type=d:level=2:assoc=2:line=4096:size=2048 \
    -qcache=type=i:level=2:assoc=2:line=4096:size=512
options  += -qarch=pwr2 -qtune=pwr2s
KCFLAGS  += +Kargs=-r=3
KFFLAGS  += -Wp,-r=3,-inl,-f
LBLAS  = -lessl
endif

ifeq ($(ARCH2),sp1)    # IBM SP1
dcache  = -qcache=type=d:level=1:assoc=4:line=64:size=32
icache  = -qcache=type=i:level=1:assoc=2:line=64:size=32
tlb  = -qcache=type=d:level=2:assoc=2:line=4096:size=512 \
    -qcache=type=i:level=2:assoc=2:line=4096:size=512
options  += -qarch=pwr -qtune=pwr
KCFLAGS  += +Kargs=-r=3:-chs=32:-chl=64
KFFLAGS  += -Wp,-r=3,-inl,-f,-chs=32,-chl=64
LBLAS  = -lessl
endif

ifeq ($(ARCH2),f50)  # IBM SP2 "wide" node
dcache  = -qcache=type=d:level=1:assoc=4:line=32:size=32
icache  = -qcache=type=i:level=1:assoc=4:line=32:size=32
l2cache  = -qcache=type=d:level=2:assoc=8:line=64:size=256
options  += -qarch=ppc -qtune=604
KCFLAGS  += +Kargs=-r=3:-chs=32:-chl=32
KFFLAGS  += -Wp,-r=3,-inl,-f,-chs=32,-chl=32
LBLAS  = -lessl
endif

ifeq ($(ARCH2),BG)     # IBM Blue Gene
options =  -qarch=440d -qtune=440 -Wl,--allow-multiple-definition
LBLAS   = -L/usr/local/apps/lib -lmassv -lmass  -lesslbg   -L/usr/local/apps/V1R3/scalapack -lscalapack  /usr/local/apps/V1R3/lapack/lapack440.a /usr/local/apps/V1R3/lapack/blas440.a
LBLACS  =  /usr/local/apps/V1R3/blacs/blacs_MPI-BGL-0.a /usr/local/apps/V1R3/blacs/blacsCinit_MPI-BGL-0.a /usr/local/apps/V1R3/blacs/blacsF77init_MPI-BGL-0.a /usr/local/apps/V1R3/blacs/blacs_MPI-BGL-0.a
endif

ifeq ($(ARCH2),common)    # Generic RS/6000
options  += -qarch=com
LBLAS  = -lblas
endif
