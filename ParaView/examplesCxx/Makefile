# Generated automatically from Makefile.in by configure.
#
# examples/cc//Makefile.in  --
#
# Makefile for Visualization Toolkit C++ examples.
# 
#------------------------------------------------------------------------------
#
SHELL = /bin/sh
.SUFFIXES: .cxx

srcdir        = .
VTK_LIB_FILE  = /home7/amy/vtk/patented/libVTKPatented${VTK_LIB_EXT} /home7/amy/vtk/imaging/libVTKImaging${VTK_LIB_EXT} /home7/amy/vtk/contrib/libVTKContrib${VTK_LIB_EXT} /home7/amy/vtk/graphics/libVTKGraphics${VTK_LIB_EXT} /home7/amy/vtk/common/libVTKCommon${VTK_LIB_EXT}

#------------------------------------------------------------------------------
# Autoconfig defines that can be overridden in user.make

include /home7/amy/vtk/system.make

#------------------------------------------------------------------------------
# Include user-editable defines.

include /home7/amy/vtk/user.make

#-----------------------------------------------------------------------------

EXAMPLES = ParallelIso 


CXX_FLAGS = ${CPPFLAGS} ${USER_CXXFLAGS} ${CXXFLAGS} -I${srcdir} \
	${KIT_FLAGS} -I. ${GRAPHICS_API_FLAGS} ${CONTROLLER_API_FLAGS} \
	 -I/home7/amy/vtk/common -I/home7/amy/vtk/contrib \
	 -I/home7/amy/vtk//patented \
	-I/home7/amy/vtk/imaging -I/home7/amy/vtk/graphics

VTKLIB = ${VTK_LIB_FILE}

LDLIBS = ${VTK_LIB_FILE} ${XLIBS} ${GRAPHICS_API_LIBS} ${DL_LIBS} \
	${THREAD_LIBS} ${CONTROLLER_API_LIBS} -lXext -lXt -lX11  -lm

.cxx.o:
	${CXX} ${CXX_FLAGS} -c $< -o $@

#------------------------------------------------------------------------------
#

all: ${EXAMPLES}

ParallelIso: ParallelIso.o
	${CXX} -o ParallelIso ${CXX_FLAGS} ParallelIso.o ${XLDFLAGS} ${LDLIBS} \
	${XLIBS} -lXext -lXt ${X_PRE_LIBS} -lX11 ${X_EXTRA_LIBS} -lm

#------------------------------------------------------------------------------

clean:
	-rm -f ${EXAMPLES} *.o 

