# NOTE !!! when adding a class, it must be added in one location.
# Place it in the correct list based on whether it is only a .h file
# and whether it is abstract or concrete.
#
ME = KWParaView

srcdir        = .

ABSTRACT_H = 

CONCRETE_H = \

ABSTRACT = \
vtkInteractorStyleExtent 

CONCRETE = \
vtkDummyRenderWindowInteractor \
vtkInteractorStyleGridExtent \
vtkInteractorStyleImageExtent \
vtkInteractorStylePlane \
vtkInteractorStylePlaneSource \
vtkInteractorStyleSphere \
vtkKWRenderView \
vtkPVApplication \
vtkPVRenderView \
vtkPVSlave \
vtkPVWindow 


#------------------------------------------------------------------------------
# Autoconfig defines that can be overridden in user.make

include ${VTKBIN}/system.make

#------------------------------------------------------------------------------
# Include user-editable defines.

include ${VTKBIN}/user.make

#------------------------------------------------------------------------------
# Include the generated targets

include targets.make

# add in any extra libraies here
KIT_LIBS =

# standard stuff here
KIT_OBJ = ${SRC_OBJ} 
KIT_TCL_OBJ = ${TCL_OBJ}
KIT_NEWS = ${TCL_NEWS}
KIT_FLAGS = -I../Widgets -I../../Widgets
KIT_EXTRA_DEPENDS = ../Widgets

VTK_LIB_FILE  = libVTK${ME}${VTK_LIB_EXT}
TCL_LIB_FILE  = vtk${ME}Tcl


#------------------------------------------------------------------------------
# Include the generic stuff here

include ../Kits.make
pvd:
	echo  Makefile ParaView.o ${TOOLKIT_DEPENDS} 

ParaView: Makefile ParaView.o pvinit.o ${TOOLKIT_DEPENDS} 
	$(CXX) -o ParaView ParaView.o pvinit.o \
	-L/. -lVTKKWParaView ./vtkKWParaViewTcl  \
	-L../Widgets -lVTKKWWidgets ../Widgets/vtkKWWidgetsTcl \
	${USE_CONTRIB_LIBS}	\
	${USE_PATENTED_LIBS}	\
	${USE_IMAGING_LIBS}	\
	${USE_GRAPHICS_LIBS}	\
	-L${VTKBIN}/common -lVTKCommonTcl  -lVTKCommon \
	${XLIBS} ${GRAPHICS_API_LIBS} \
	${TK_LIB} ${TCL_LIB} ${DL_LIBS} ${THREAD_LIBS} \
	-L. ${CXX_FLAGS} ${VTK_SHLIB_LINK_FLAGS} ${XLDFLAGS} ${LDLIBS} \
	${XLIBS} -lXext -lXt ${X_PRE_LIBS} -lX11 ${X_EXTRA_LIBS} -lm









