
# create a plugin that adds readers and writers
# for Analyze and NIfTI files to the ParaView GUI
# it is added in the file dialog when doing opens/saves.

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIRS}
    ${MPI_INCLUDE_PATH}
  )

vtk_module_load(vtkzlib)

INCLUDE_DIRECTORIES(${vtkzlib_INCLUDE_DIRS})

ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

ADD_PARAVIEW_PLUGIN(AnalyzeNIfTIIO "1.0"
  SERVER_MANAGER_SOURCES vtkAnalyzeReader.cxx
                         vtkAnalyzeWriter.cxx
                         vtkNIfTIReader.cxx
                         vtkNIfTIWriter.cxx
                         vtknifti1_io.cxx
                         vtkznzlib.cxx
  SERVER_MANAGER_XML     AnalyzeNIfTISM.xml
  REQUIRED_ON_SERVER)

target_link_libraries(AnalyzeNIfTIIO LINK_PRIVATE ${vtkzlib_LIBRARIES})
