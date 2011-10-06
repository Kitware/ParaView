## CMAKE file for AMR plugins

cmake_minimum_required(VERSION 2.8)

set(PLUGIN_NAME AMRTools)
set(PLUGIN_VERSION "1.0")

## Find ParaView
if(NOT ParaView_SOURCE_DIR)
    find_package(ParaView REQUIRED)
    include(${PARAVIEW_USE_FILE})
endif()

## Set the list of source
set(SMSOURCES
    vtkAMRBaseParticlesReader.cxx
    vtkAMRBaseReader.cxx
    vtkAMRDataSetCache.cxx
    vtkAMREnzoParticlesReader.cxx
    vtkAMREnzoReader.cxx
    vtkAMRFlashParticlesReader.cxx
    vtkAMRFlashReader.cxx
    )
    
## Mark abstract classes
set_source_files_properties(
    vtkAMRBaseParticlesReader.cxx
    vtkAMRBaseReader.cxx
    ABSTRACT
    )
    
## Mark helper classes to exclude from wrapping
set_source_files_properties(
    vtkARMEnzoReaderInternal
    vtkAMRFlashReaderInternal
    WRAP_EXCLUDE
    )
    
## Set the Server Manager XML
set(SM_XML
    Readers.xml
    )
    
## Add the plugin to the build
ADD_PARAVIEW_PLUGIN(
    ${PLUGIN_NAME} ${PLUGIN_VERSION}
    SERVER_MANAGER_XML ${SM_XML}
    SERVER_MANAGER_SOURCES ${SMSOURCES}
    SERVER_SOURCES
      vtkAMRFlashReaderInternal.cxx
      vtkAMREnzoReaderInternal.cxx
    )
    
