message  += You must setenv TARGET cray-t3d; you may want to specify APPRENTICE.
FC   = cf77
CFLAGS  += -h scalar3,aggress,indef,inline3,msglevel_2,report=is,unroll
FFLAGS  += -dp -Oscalar3
LD   = $(CC)
LDFLAGS   = $(CFLAGS) -D rdahead=on
LIBS     = -L$(LIBDIR) -lspec -lvec -lsci -L/usr/local/mpp/lib/BENCHLIB -l_vect -lm
ifdef APPRENTICE
CFLAGS  += -h apprentice
FFLAGS  += -Wf"-Ta"
LIBS  += -lapp
endif
