VTKSRC = /home/lawcc/vtk
VTKBIN = /home/lawcc/vtk

all: doWidgets doParaView

doParaView: doWidgets
	cd ParaView; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
	 	targets.make
	cd ParaView; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) all
	cd ParaView; ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) ParaView

doWidgets:
	cd Widgets;  ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) \
		targets.make
	cd Widgets;  ${MAKE} -${MAKEFLAGS} VTKBIN=$(VTKBIN) VTKSRC=$(VTKSRC) all

