#
# Makefile for Visualization Toolkit sources. 
# 
#------------------------------------------------------------------------------
#

SHELL = /bin/sh
.SUFFIXES: .cxx .java .class

#------------------------------------------------------------------------------

CC_FLAGS = ${CPPFLAGS} ${USER_CFLAGS} ${CFLAGS} ${USE_TOOLKIT_FLAGS} \
	   ${GRAPHICS_API_FLAGS} ${TK_INCLUDE} ${TCL_INCLUDE} \
	   -I${VTKBIN}/common -DVTK_USE_TKWIDGET


CXX_FLAGS = ${CPPFLAGS} ${USER_CXXFLAGS} ${CXXFLAGS} -I${srcdir} \
	${KIT_FLAGS} -I. ${USE_TOOLKIT_FLAGS} ${GRAPHICS_API_FLAGS} \
	 -I${VTKSRC}/common -I${VTKSRC}/imaging -I${VTKSRC}/graphics \
	-I${VTKSRC}/patented -I${VTKSRC}/contrib ${TK_INCLUDE} ${TCL_INCLUDE} \
	-I${VTKBIN}/common -DVTK_USE_TKWIDGET

all: ${BUILD_TCL} 

.c.o:
	${CC} ${CC_FLAGS} -c $< -o $@
.cxx.o:
	${CXX} ${CXX_FLAGS} -c $< -o $@

#------------------------------------------------------------------------------
depend: $(VTKBIN)/targets
	$(VTKBIN)/targets $(VTKSRC) extra ${srcdir} ${VTKBIN}/common ${KIT_EXTRA_DEPENDS} concrete $(CONCRETE) abstract $(ABSTRACT) concrete_h $(CONCRETE_H) abstract_h $(ABSTRACT_H)


targets.make: $(VTKBIN)/targets Makefile
	$(VTKBIN)/targets ${VTKSRC} extra ${srcdir} ${VTKBIN}/common ${KIT_EXTRA_DEPENDS} concrete $(CONCRETE) abstract $(ABSTRACT) concrete_h $(CONCRETE_H) abstract_h $(ABSTRACT_H)

#------------------------------------------------------------------------------
# rules for the normal library
#
vtk${ME}.a: ${SRC_OBJ} ${KIT_OBJ}
	${AR} cr vtk${ME}.a ${KIT_OBJ}
	${RANLIB} vtk$(ME).a


vtk$(ME)$(SHLIB_SUFFIX): ${KIT_OBJ}
	rm -f vtk$(ME)$(SHLIB_SUFFIX)
	$(CXX) ${CXX_FLAGS} ${VTK_SHLIB_BUILD_FLAGS} -o \
	vtk$(ME)$(SHLIB_SUFFIX) \
	   ${KIT_OBJ} ${SHLIB_LD_LIBS}

#------------------------------------------------------------------------------
# rules for the tcl library
#
build_tcl: ${TCL_LIB_FILE}

tcl/${ME}Init.cxx: $(VTKBIN)/wrap/vtkWrapTclInit ${KIT_NEWS} Makefile
	$(VTKBIN)/wrap/vtkWrapTclInit VTK${ME}Tcl ${KIT_NEWS} > tcl/${ME}Init.cxx

vtk${ME}Tcl.a: tcl/${ME}Init.o ${KIT_LIBS} ${KIT_TCL_OBJ} ${SRC_OBJ} ${KIT_OBJ}
	${AR} cr vtk${ME}Tcl.a tcl/${ME}Init.o ${KIT_LIBS} ${KIT_TCL_OBJ} ${KIT_OBJ}
	${RANLIB} vtk$(ME)Tcl.a

vtk$(ME)Tcl$(SHLIB_SUFFIX): tcl/${ME}Init.o ${KIT_LIBS} ${KIT_TCL_OBJ} ${SRC_OBJ} ${KIT_OBJ}
	rm -f vtk$(ME)Tcl$(SHLIB_SUFFIX)
	$(CXX) ${CXX_FLAGS} ${VTK_SHLIB_BUILD_FLAGS} -o \
	vtk$(ME)Tcl$(SHLIB_SUFFIX) tcl/${ME}Init.o  \
	${KIT_LIBS} ${KIT_TCL_OBJ} ${KIT_OBJ}


LDLIBS =  \
	${USE_LOCAL_LIBS}	\
	${USE_CONTRIB_LIBS}	\
	${USE_PATENTED_LIBS}	\
	${USE_IMAGING_LIBS}	\
	${USE_GRAPHICS_LIBS}	\
        -L${VTKBIN}/common -lVTKCommonTcl -lVTKCommon 
