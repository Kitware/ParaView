# CMake version and policy issues
CMAKE_MINIMUM_REQUIRED( VERSION 2.4 )
IF( COMMAND CMAKE_POLICY )
  CMAKE_POLICY( SET CMP0003 NEW )
ENDIF( COMMAND CMAKE_POLICY )

# build the VTK lib slightly differently since we are building from
#the PV source
SET ( BUILD_AGAINST_PARAVIEW 1 )
ADD_SUBDIRECTORY(VTK)

ADD_SUBDIRECTORY(ParaView)
