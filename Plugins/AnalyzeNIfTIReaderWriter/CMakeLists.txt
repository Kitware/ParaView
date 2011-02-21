
# create a plugin that adds readers and writers
# for Analyze and NIfTI files to the ParaView GUI
# it is added in the file dialog when doing opens/saves.

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
    ${MPI_INCLUDE_PATH}
  )
  
IF(VTK_USE_SYSTEM_ZLIB)
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
ENDIF(VTK_USE_SYSTEM_ZLIB)

ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
   FIND_PACKAGE(Qt4)
   IF(QT4_FOUND)
      INCLUDE(${QT_USE_FILE})

      INCLUDE_DIRECTORIES(${PARAVIEW_INCLUDE_DIRS}
                          ${VTK_INCLUDE_DIR}
                          ${pqCore_BINARY_DIR}
                          ${pqCore_SOURCE_DIR}
                          ${pqComponents_BINARY_DIR}
                          ${pqComponents_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
       )
   ENDIF(QT4_FOUND)
ENDIF(PARAVIEW_BUILD_QT_GUI)

ADD_PARAVIEW_PLUGIN(AnalyzeReader "1.0"
  SERVER_MANAGER_SOURCES vtkAnalyzeReader.cxx vtknifti1_io.cxx vtkznzlib.cxx
  SERVER_MANAGER_XML readers.xml
  GUI_RESOURCE_FILES AnalyzeReader.xml
  )

ADD_PARAVIEW_PLUGIN(AnalyzeWriter "1.0"
  SERVER_MANAGER_SOURCES vtkAnalyzeWriter.cxx vtknifti1_io.cxx vtkznzlib.cxx
  SERVER_MANAGER_XML writers.xml
  GUI_RESOURCE_FILES AnalyzeWriter.xml
  )

ADD_PARAVIEW_PLUGIN(NIfTIReader "1.0"
  SERVER_MANAGER_SOURCES vtkNIfTIReader.cxx vtknifti1_io.cxx vtkznzlib.cxx
  SERVER_MANAGER_XML readers.xml
  GUI_RESOURCE_FILES NIfTIReader.xml
  )

ADD_PARAVIEW_PLUGIN(NIfTIWriter "1.0"
  SERVER_MANAGER_SOURCES vtkNIfTIWriter.cxx vtknifti1_io.cxx vtkznzlib.cxx
  SERVER_MANAGER_XML writers.xml
  GUI_RESOURCE_FILES NIfTIWriter.xml
  )
