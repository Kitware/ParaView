FC   = cf77
CFLAGS  += -v -h scalar3,task0,vector3,report=isv #,ivdep
FFLAGS  += -dp -Zv
LD   = $(CC)
LDFLAGS   = $(CFLAGS)
LIBS     = -L$(LIBDIR) -lspec -lvec -lsci -lm