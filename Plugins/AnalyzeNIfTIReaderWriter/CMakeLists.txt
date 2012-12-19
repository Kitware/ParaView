
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

# fixme: I am not sure this is needed after modularization.
IF(VTK_USE_SYSTEM_ZLIB)
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
ENDIF(VTK_USE_SYSTEM_ZLIB)

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
  GUI_RESOURCE_FILES     AnalyzeNIfTIReaders.xml
                         AnalyzeNIfTIWriters.xml
  )
