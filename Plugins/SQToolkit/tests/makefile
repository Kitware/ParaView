
#WORK
PV3_HOME=/home/burlen/ext/ParaView/ParaView3
VTK_HOME=$(PV3_HOME)/VTK
PV3_BUILD=/home/burlen/ext/ParaView/PV3-3.9
VTK_BUILD=/home/burlen/ext/ParaView/PV3-3.9

PV3_INC=\
  -I$(PV3_HOME)/Servers/Filters

VTK_INC=\
  -I$(VTK_BUILD)/VTK\
  -I$(VTK_BUILD)/VTK/Utilities\
  -I$(VTK_HOME)/Parallel\
  -I$(VTK_HOME)/GenericFiltering\
  -I$(VTK_HOME)/Views\
  -I$(VTK_HOME)/Imaging\
  -I$(VTK_HOME)/GUISupport\
  -I$(VTK_HOME)/Infovis\
  -I$(VTK_HOME)/Hybrid\
  -I$(VTK_HOME)/VolumeRendering\
  -I$(VTK_HOME)/Examples\
  -I$(VTK_HOME)/Wrapping\
  -I$(VTK_HOME)/IO\
  -I$(VTK_HOME)/Filtering\
  -I$(VTK_HOME)/Common\
  -I$(VTK_HOME)/Common/Testing/Cxx\
  -I$(VTK_HOME)/Widgets\
  -I$(VTK_HOME)/Rendering\
  -I$(VTK_HOME)/Rendering/Testing/Cxx\
  -I$(VTK_HOME)/Patented\
  -I$(VTK_HOME)/Graphics\
  -I$(VTK_HOME)/Utilities

PV3_LIB=\
  -L$(PV3_BUILD)/bin\
  -lvtkPVFilters

VTK_LIB=\
  -L$(VTK_BUILD)/bin\
  -lvtkCommon\
  -lvtkFiltering\
  -lvtkHybrid\
  -lvtkParallel\
  -lvtkGraphics\
  -lvtkImaging\
  -lvtkRendering\
  -lvtkGraphics\
  -lvtkImaging\
  -lvtkWidgets\
  -lvtkftgl\
  -lvtkfreetype\
  -lGL\
  -lXt\
  -lSM\
  -lICE\
  -lX11\
  -lXext\
  -lvtkIO\
  -lvtkDICOMParser\
  -lvtkmetaio\
  -lvtksqlite\
  -lvtkpng\
  -lvtktiff\
  -lvtkzlib\
  -lvtkjpeg\
  -lvtkexpat\
  -lvtkexoIIc\
  -lvtkNetCDF\
  -lpthread\
  -lrt\
  -lvtkverdict\
  -lvtkFiltering\
  -lvtksys\
  -lpthread\
  -ldl\
  -lm

#MPICH_HOME=/opt/mpich2-1.0.8
MPICH_INC=-DMPICH_IGNORE_CXX_SEEK -I$(MPICH_HOME)/include
MPICH_LIB=-L$(MPICH_HOME)/lib -Wl,-rpath -Wl,$(MPICH_HOME)/lib -lmpichcxx -lmpich -lpthread -lrt -lrt

# MESA_HOME=/home/burlen/ext2/Mesa-7.2
MESA_LIB=-Wl,-rpath,$(MESA_HOME)/lib -L$(MESA_HOME)/lib -lGL -lGLU -lOSMesa -lglut -lGLw

XDMF_INC=-I$(XDMF_HOME)/include
XDMF_LIB=-L$(XDMF_HOME)/lib -lXdmf

LIB=$(MESA_LIB) $(VTK_LIB) $(MPICH_LIBS) 
INC=$(MPICH_INC) $(VTK_INC)

#-Wl,-rpath,/home/burlen/ext/kitware_cvs/PV3-mpich2-1.0.7/bin:/opt/mpich2-1.0.7/lib

FLAGS_MIN=-g -Wall -Wno-deprecated -rdynamic
FLAGS= $(FLAGS_MIN) -Wl,-rpath,$(VTK_BUILD)/bin -Wl,-rpath,$(MPICH_HOME)/lib

TestCartesianDecomp : TestCartesianDecomp.cpp ../CartesianExtent.cxx ../CartesianBounds.cxx ../CartesianDecomp.cxx ../CartesianDataBlock.cxx
	g++ -g -Wall $(FLAGS) -I../ $(VTK_INC) $(MPICH_INC) $^ -L$(VTK_BUILD)/bin -L$(MPICH_HOME)/lib -lmpich -lvtkCommon -lvtkFiltering -o $@

