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
vtkClipPlane \
vtkCutMaterial \
vtkCutPlane \
vtkDummyRenderWindowInteractor \
vtkFastGeometryFilter \
vtkImageOutlineFilter \
vtkInteractorStyleCamera \
vtkInteractorStyleGridExtent \
vtkInteractorStyleImageExtent \
vtkInteractorStylePlane \
vtkInteractorStylePlaneSource \
vtkInteractorStyleSphere \
vtkPOPReader \
vtkPVActorComposite \
vtkPVApplication \
vtkPVArrayCalculator \
vtkPVContour \
vtkPVData \
vtkPVDataSetReaderInterface \
vtkPVEnSightReaderInterface \
vtkPVGlyph3D \
vtkPVInputMenu \
vtkPVMethodInterface \
vtkPVRenderView \
vtkPVSelectionList \
vtkPVSource \
vtkPVSourceCollection \
vtkPVSourceInterface \
vtkPVSourceList \
vtkPVThreshold \
vtkPVWindow \
vtkSimpleFieldDataToAttributeDataFilter \
vtkSingleContourFilter \
vtkStringList 


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

VTK_LIB_FILE  = vtk${ME}${VTK_LIB_EXT}
TCL_LIB_FILE  = vtk${ME}Tcl${VTK_LIB_EXT}


#------------------------------------------------------------------------------
# Include the generic stuff here

include ../Kits.make

vtktcl.o: ${VTKSRC}/tcl/vtktcl.c
	$(CC) -I${VTKBIN}/common $(CC_FLAGS) -c -o vtktcl.o $(VTKSRC)/tcl/vtktcl.c

ParaView: Makefile ParaView.o vtktcl.o ${TOOLKIT_DEPENDS} 
	echo ${TCL_LIB_FILE}; $(CXX) -o ParaView ParaView.o \
	${TCL_LIB_FILE} ${KIT_LIBS} \
	$(VIEWSDIR)/ParaView/vtkKWParaViewTcl$(VTK_LIB_EXT) \
	$(VIEWSDIR)/Widgets/vtkKWWidgetsTcl$(VTK_LIB_EXT) \
	vtktcl.o \
	${USE_PATENTED_LIBS}	\
	${USE_IMAGING_LIBS}	\
	${USE_GRAPHICS_LIBS}	\
	${USE_CONTRIB_LIBS}	\
        ${USE_PARALLEL_LIBS}    \
	-L${VTKBIN}/common -lVTKCommonTcl  -lVTKCommon \
	${LDLIBS} ${CONTROLLER_API_LIBS} \
	${TK_LIB} ${TCL_LIB} ${DL_LIBS} ${THREAD_LIBS} \
	-L. ${CXX_FLAGS} ${VTK_SHLIB_LINK_FLAGS} ${XLDFLAGS} ${LDLIBS} \
	${XLIBS} ${GRAPHICS_API_LIBS} -lXext -lXt ${X_PRE_LIBS} -lX11 ${X_EXTRA_LIBS} -lm



#	-L/. -lVTKKWParaView ./vtkKWParaViewTcl  \
#	-L../Widgets -lVTKKWWidgets ../Widgets/vtkKWWidgetsTcl \
#	${XLIBS} ${GRAPHICS_API_LIBS} \
#	${TK_LIB} ${TCL_LIB} ${DL_LIBS} ${THREAD_LIBS} \
#	-L. ${CXX_FLAGS} ${VTK_SHLIB_LINK_FLAGS} ${XLDFLAGS} ${LDLIBS} \
#	${XLIBS} -lXext -lXt ${X_PRE_LIBS} -lX11 ${X_EXTRA_LIBS} -lm









