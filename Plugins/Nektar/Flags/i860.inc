CC   = icc
FC       = if77
options  += -O4 -Msafeptr -Mxp -Minfo=loop -Knoieee -Mvect=cachesize:4096,recog,transform,smallvect:256
AR       = ar860
LD       = $(CC)
LDFLAGS  = $(CFLAGS)
LIBS     = -L$(LIBDIR) -lspec -lInTools -lvec -lkmath -lf -lm
