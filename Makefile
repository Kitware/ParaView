VTKSRC = /viz/ccl/vtk
VTKBIN = /viz/ccl/vtk
VIEWSDIR = /viz/ccl/Views


all: doWidgets doParaView

doParaView: doWidgets
	cd ParaView; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
		VIEWSDIR=$(VIEWSDIR) targets.make
	cd ParaView; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
		VIEWSDIR=$(VIEWSDIR) all
	cd ParaView; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
		VIEWSDIR=$(VIEWSDIR) ParaView

doWidgets:
	cd Widgets;  ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
		targets.make
	cd Widgets;  ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) all

