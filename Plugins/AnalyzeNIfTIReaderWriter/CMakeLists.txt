
# create a plugin that adds readers and writers
# for Analyze and NIfTI files to the ParaView GUI
# it is added in the file dialog when doing opens/saves.

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIRS}
  )
  INCLUDE_DIRECTORIES(SYSTEM
    ${MPI_INCLUDE_PATH}
  )

vtk_module_load(vtkzlib)

INCLUDE_DIRECTORIES(${vtkzlib_INCLUDE_DIRS})

ELSE ()
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF ()

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

# Add testing if necessary
if (BUILD_TESTING)
  # Analyze reader test was conflicting with the reader from VisIt. So
  # disabling it.
  if(NOT PARAVIEW_USE_VISITBRIDGE)
    add_subdirectory(Testing)
  endif ()
endif()
