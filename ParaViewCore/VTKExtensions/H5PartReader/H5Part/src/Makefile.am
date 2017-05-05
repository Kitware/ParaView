# src level Makefile.am

# PATH SETTING (IMPORTED FROM CONFIGURE)
HDF5ROOT = @HDF5ROOT@

# COMPILERS
CC = @CC@

#INCLUDES
HDFINC = -I$(HDF5ROOT)/include
MPIINC = @MPIINC@

INC = $(HDFINC) ${MPIINC}

# H5Part header file location
H5PINC = -I@prefix@/include

# COMPILER FLAGS
CFLAGS = @CFLAGS@ ${INC}

# H5Part compiled library location
H5PLIB = -L@prefix@/lib

# HDF5 LIBRARY
HDFLIB = -L$(HDF5ROOT)/lib -lhdf5 -lz $(SZLIB) @LDFLAGS@

# SZ LIBRARY
SZLIB = @SZLIB@

# Extra files that I wish to include in the dist tar ball.
EXTRA_DIST = TestUnderscoreC.c TestUnderscore.f $(EXTRA_HEADERS)

# Files that I don't want to include in the dist tar ball
nodist_include_HEADERS = @UNDERSCORE_H@

# What to build... Will be determined by configure script.
lib_LIBRARIES = @MTARGET@

# Listing of all possible targets that I may build.
EXTRA_LIBRARIES = libH5Part.a libH5PartF.a

# Header files that I wish to install in $(prefix)/include
include_HEADERS = H5Part.h H5PartTypes.h H5PartErrors.h H5Block.h H5BlockTypes.h H5BlockErrors.h H5Part.inc H5PartF90.inc H5BlockF90.inc @UNDERSCORE_H@

# Listing of all possible headers that I may include
EXTRA_HEADERS = H5PartPrivate.h H5BlockPrivate.h

# Listing of sources
libH5Part_a_SOURCES = H5Part.c H5Block.c
libH5PartF_a_SOURCES = H5PartF.c H5BlockF.c


H5Part.inc: H5PartF90.inc
 grep "INTEGER.* FUNCTION" $< | while read type func name rest; do echo "      $$type $$name"; done > $@

# Specific building instruction (What compilers to use...)
# ------------ Serial Lib build commands ------------
libH5Part.a: H5Part.o H5Block.o
 ${AR} rucs $@ $^

libH5PartF.a: H5Part.o H5PartF.o H5Block.o H5BlockF.o
 ${AR} rucs $@ $^

%.o : %.c
 $(CC) $(CFLAGS) $(INC) -c $<

H5Part.o: H5Part.c H5Part.h H5PartPrivate.h H5PartTypes.h
H5PartF.o: H5PartF.c Underscore.h H5Part.h
H5Block.o: H5Block.c H5Part.h H5PartPrivate.h H5PartTypes.h H5Block.h H5BlockTypes.h
H5BlockF.o: H5BlockF.c Underscore.h H5Block.h


# ----------- Build Parallel H5Part Stuff ------------

libpH5Part.a: libH5Part.a
 ${RM} $@
 ln -s $^ $@

libpH5PartF.a: libH5PartF.a
 ${RM} $@
 ln -s $^ $@

clean:
 rm -f *~ *.o *.a *.so

distclean: clean
 rm -f *.a
 rm -rf .deps
 rm -rf .libs
 rm -f Underscore.h
 rm -f H5Part_py_wrap.c
 rm -f H5Part.py
 rm -f Makefile
