VTKSRC = /home7/amy/vtk
VTKBIN = /home7/amy/vtk
VIEWSDIR = /home7/amy/Views


all: doWidgets doParaView

doTest: doWidgets
	cd Test; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
		VIEWSDIR=$(VIEWSDIR) targets.make
	cd Test; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
		VIEWSDIR=$(VIEWSDIR) all
	cd Test; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
		VIEWSDIR=$(VIEWSDIR) Test

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

