# CMake version and policy issues
CMAKE_MINIMUM_REQUIRED( VERSION 2.4 )
IF( COMMAND CMAKE_POLICY )
  CMAKE_POLICY( SET CMP0003 NEW )
ENDIF( COMMAND CMAKE_POLICY )


# Hide CMAKE_INSTALL_PREFIX as it is an uncessary variable
MARK_AS_ADVANCED ( FORCE CMAKE_INSTALL_PREFIX )


# Set the project/library name here.  Classes should be declared as
# "class vtkManta_EXPORT vtkFoo", where vtkManta is the name of the project set here.
PROJECT ( vtkManta )

# Choose either VTK (VTK_DIR required and ParaView_DIR ignored)
# or ParaView (ParaView_DIR required and VTK_DIR ignored) to build against
SET ( BUILD_AGAINST_PACKAGE "ParaView" CACHE STRING "Build against either VTK or ParaView." )

IF(VTK_USE_CARBON)
  SET(RenderWindowType "vtkCarbonRenderWindow")
ELSEIF(VTK_USE_COCOA)
  SET(RenderWindowType "vtkCocoaRenderWindow")
ELSEIF(VTK_USE_X)
  SET(RenderWindowType "vtkXOpenGLRenderWindow")
ELSE()
  SET(RenderWindowType "vtkWin32OpenGLRenderWindow")
ENDIF()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkMantaRenderWindow.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkMantaRenderWindow.h"
  @ONLY
  IMMEDIATE
  )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
  
# Set vtkManta source files.
SET ( MANTA_CLASSES
  vtkMantaActor.cxx
  vtkMantaCamera.cxx
  vtkMantaLight.cxx
  vtkMantaPolyDataMapper.cxx
  vtkMantaProperty.cxx
  vtkMantaRenderer.cxx
  vtkMantaRenderWindow.cxx
  vtkMantaTexture.cxx
  #Manta/vtkColorMapTexture.cxx
  #Manta/vtkColorMapTexture.h
  )

# List specific VTK libraries to link against
SET ( vtkManta_LIBS vtkRendering vtkParallel vtkGraphics)

IF (BUILD_AGAINST_PACKAGE MATCHES "ParaView")
  SET ( MANTA_CLASSES ${MANTA_CLASSES} vtkMantaLODActor.cxx)
  SET ( vtkManta_LIBS ${vtkManta_LIBS} vtkPVServerManager)
ENDIF (BUILD_AGAINST_PACKAGE MATCHES "ParaView")

SET ( MANTA_SRCS
  ${MANTA_CLASSES}
  vtkMantaObjectFactory.cxx
  )

# testing is OFF by default to avoid time-consuming build of many tests
OPTION ( BUILD_TESTING "Build vtkManta Testing" OFF )

# Locate Manta build-include and build-library directories
SET ( MANTA_BUILD_NAMES
  buildmanta build_manta build-manta buildManta build_Manta build-Manta
  )

SET ( MANTA_BUILD_SEARCH_PATH "" )
FOREACH ( manta_build_name ${MANTA_BUILD_NAMES} )
  SET ( MANTA_BUILD_SEARCH_PATH
    ${MANTA_BUILD_SEARCH_PATH}
    ../${manta_build_name}
    /bin/${manta_build_name}
    /usr/bin/${manta_build_name}
    /usr/local/bin/${manta_build_name}
    )
ENDFOREACH ( manta_build_name )

#FIND_PACKAGE(Manta REQUIRED)

FIND_PATH ( MANTA_BUILD MantaConfigure.cmake ${MANTA_BUILD_SEARCH_PATH} DOC "Manta build directory" )
IF ( MANTA_BUILD )
  SET ( MANTA_BUILD_INCLUDE ${MANTA_BUILD}/include )
  SET ( MANTA_BUILD_LIB ${MANTA_BUILD}/lib )
ENDIF ( MANTA_BUILD )
# end: Locate Manta build-include and build-library directories

# Locate Manta source directory through Manta build directory
FIND_PATH ( MANTA_SOURCE manta-commands.txt ../manta ../Manta DOC "Manta source directory" )
IF ( MANTA_BUILD AND NOT MANTA_SOURCE )
  INCLUDE ( ${MANTA_BUILD}/CMakeFiles/CMakeDirectoryInformation.cmake )
  SET ( MANTA_SOURCE ${CMAKE_RELATIVE_PATH_TOP_SOURCE} )
ENDIF ( MANTA_BUILD AND NOT MANTA_SOURCE )

# Include Manta source and build-include directories
INCLUDE_DIRECTORIES(${MANTA_SOURCE} ${MANTA_BUILD_INCLUDE})
LINK_DIRECTORIES ( ${MANTA_BUILD_LIB} )

# Set specific Manta libraries to link against
SET ( MANTA_TARGET_LINK_LIBRARIES
  Manta_Factory
  Manta_UserInterface
  Manta_Engine
  Manta_Model
  Manta_Image
  Manta_Interface
  Manta_Core
  )

IF ( BUILD_AGAINST_PACKAGE MATCHES "VTK" )

  # Show VTK-build dependent flags while hiding ParaView-build dependent flags 
  MARK_AS_ADVANCED ( CLEAR VTK_DIR )
  MARK_AS_ADVANCED ( FORCE ParaView_DIR )
  MARK_AS_ADVANCED ( FORCE BUILD_AS_PLUGIN )
  MARK_AS_ADVANCED ( FORCE DART_TESTING_TIMEOUT )

  # Locate VTK build directory through FindVTK.cmake
  INCLUDE ( ${CMAKE_ROOT}/Modules/FindVTK.cmake )

  # Locate VTK build directory just in case FindVTK.cmake fails to do so
  IF ( NOT VTK_DIR )

    SET ( VTK_BUILD_SEARCH_PATH
      ../buildVTK ../build_VTK ../build-VTK 
      ../buildvtk ../build_VTK ../build-VTK
      )       
    FIND_PATH ( VTK_DIR UseVTK.cmake ${VTK_BUILD_SEARCH_PATH} DOC "VTK build directory" )
    
  ENDIF ( NOT VTK_DIR )
  
  # Make sure VTK is found. Otherwise key 'g' would not appear for generating CMakefile(s).
  FIND_PACKAGE ( VTK REQUIRED )
  
  IF ( VTK_DIR )
    
    INCLUDE ( ${VTK_USE_FILE} )

    # Set the directory for exporting vtkManta executables and libraries
    # to VTK's build directory (just if necessary)
    SET ( EXES_LIBS_EXPORT_DIR ${VTK_DIR} )
    MARK_AS_ADVANCED ( EXES_LIBS_EXPORT_DIR )

    # define a flag exported to vtkMantaConfigure.h to determine if some ParaView-dependent
    # vtkManta header files are included in some files
    SET ( BUILD_AGAINST_PARAVIEW 0 )

    # Give user option of building shared or static libraries.  Default to the choice made for VTK.
    OPTION ( BUILD_SHARED_LIBS "Build with shared libraries." ${VTK_BUILD_SHARED_LIBS} )

  ENDIF ( VTK_DIR )

  # Locate VTK_DATA_ROOT as the directory of VTKData.
  FIND_PATH( VTK_DATA_ROOT VTKData.readme $ENV{VTK_DATA_ROOT} )
  IF ( NOT VTK_DATA_ROOT )
    FIND_PATH ( VTK_DATA_ROOT VTKData.readme ../VTKData DOC "VTK Data root directory" )
  ENDIF ( NOT VTK_DATA_ROOT )

ELSE ( BUILD_AGAINST_PACKAGE MATCHES "VTK" )

  # Show ParaView-build dependent flags while hiding VTK-build dependent flags
  MARK_AS_ADVANCED ( CLEAR BUILD_AS_PLUGIN )
  MARK_AS_ADVANCED ( CLEAR ParaView_DIR )
  MARK_AS_ADVANCED ( FORCE VTK_DIR )
  MARK_AS_ADVANCED ( FORCE DART_TESTING_TIMEOUT )

  # Locate ParaView build directory through FindParaView.cmake (to be implemented)
  # INCLUDE ( FindParaView.cmake )

  # Locate ParaView build directory just in case FindParaView.cmake fails to do so
  IF ( NOT ParaView_DIR )

    SET ( PARAVIEW_BUILD_SEARCH_PATH
          ../buildParaView  ../build_ParaView  ../build-ParaView
          ../buildparaView  ../build_paraView  ../build-paraView
        )
    FIND_PATH ( ParaView_DIR ParaViewConfig.cmake ${PARAVIEW_BUILD_SEARCH_PATH} DOC "ParaView build directory" )

    IF ( ParaView_DIR )
      INCLUDE ( ${ParaView_DIR}/ParaViewConfig.cmake )
    ENDIF ( ParaView_DIR )

  ENDIF ( NOT ParaView_DIR )

  # Make sure ParaView is found. Otherwise key 'g' would not appear for generating CMakefile(s).
  IF (ParaView_SOURCE_DIR)
    INCLUDE_DIRECTORIES(
      ${PARAVIEW_INCLUDE_DIRS}
      ${PARAVIEW_GUI_INCLUDE_DIRS}
      ${PARAVIEW_KWSYS_INCLUDE_DIRS}
      ${VTK_INCLUDE_DIR}
      ${MPI_INCLUDE_PATH}
    )
  ELSE (ParaView_SOURCE_DIR)
    FIND_PACKAGE(ParaView REQUIRED)
    INCLUDE(${PARAVIEW_USE_FILE})
  ENDIF (ParaView_SOURCE_DIR)

  IF ( ParaView_DIR )

    INCLUDE ( ${PARAVIEW_USE_FILE} )
  
    # Set the directory for exporting vtkManta executables and libraries
    # to ParaView's build directory (just if necessary)
    SET ( EXES_LIBS_EXPORT_DIR ${ParaView_DIR} )
    MARK_AS_ADVANCED ( EXES_LIBS_EXPORT_DIR )

    # define a flag exported to vtkMantaConfigure.h to determine if some ParaView-dependent
    # vtkManta header files are included in some files
    SET ( BUILD_AGAINST_PARAVIEW 1 )

    # Give user option of building shared or static libraries.  Default to the choice made for ParaView.
    OPTION ( BUILD_SHARED_LIBS "Build with shared libraries." ${PARAVIEW_BUILD_SHARED_LIBS} )

    SET(MANTA_PLUGIN_SS_SOURCES
      ${MANTA_CLASSES}
      vtkSMMantaViewProxy.cxx
      vtkSMMantaClientServerViewProxy.cxx
      vtkSMMantaParallelViewProxy.cxx
      vtkServerSideFactory.cxx
      )      
        
    SET(SERVER_ARGS 
      SERVER_MANAGER_XML MantaWrapping.xml 
      SERVER_MANAGER_SOURCES ${MANTA_PLUGIN_SS_SOURCES} )
    
    IF(PARAVIEW_BUILD_QT_GUI)
      #set up client side pieces of the plugin
      
      INCLUDE(${QT_USE_FILE})
      
      SET(MANTA_PLUGIN_CS_SOURCES
        pqMantaView.cxx
        )
      
      QT4_WRAP_CPP(MOC_SRCS pqMantaView.h)
      
      ADD_PARAVIEW_VIEW_MODULE(
        IFACES 
        IFACE_SRCS 
        VIEW_TYPE pqMantaView
        VIEW_NAME Manta          
        VIEW_XML_GROUP views
        VIEW_XML_NAME Manta
        )
      
      SET(CLIENT_ARGS
        GUI_INTERFACES ${IFACES}
        GUI_SOURCES ${MANTA_PLUGIN_CS_SOURCES} ${MOC_SRCS} ${IFACE_SRCS} )
      
    ENDIF(PARAVIEW_BUILD_QT_GUI)
    
    ADD_PARAVIEW_PLUGIN ( MantaView "1.1" 
      ${SERVER_ARGS}
      ${CLIENT_ARGS}
      )
    
    TARGET_LINK_LIBRARIES ( MantaView vtkManta )
    
  ENDIF ( ParaView_DIR )
  
ENDIF ( BUILD_AGAINST_PACKAGE MATCHES "VTK" )


# Set the default build type to 'Release'
IF ( NOT CMAKE_BUILD_TYPE )
  SET ( CMAKE_BUILD_TYPE "Release" CACHE STRING 
        "Choose the build type: Debug / Release / RelWithDebInfo / MinSizeRel."
        FORCE
      ) 
ENDIF ( NOT CMAKE_BUILD_TYPE )

  
# --------------------------------------------------------------
# Most users should NOT need to change anything below this line.
# --------------------------------------------------------------

# Include class headers and the configuration header.
INCLUDE_DIRECTORIES ( ${PROJECT_SOURCE_DIR} )
INCLUDE_DIRECTORIES ( ${PROJECT_BINARY_DIR} )

# Setup the output directories.
SET ( LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH
      "Single output directory for building all libraries."
    )
SET ( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH
      "Single output directory for building all executables."
    )
MARK_AS_ADVANCED ( LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH )

# Set VTKMANTA_BUILD_SHARED_LIBS to 1 or 0 for use in creating the configuration header.
IF ( BUILD_SHARED_LIBS )
  SET ( VTKMANTA_BUILD_SHARED_LIBS_CONFIG 1 )
ELSE ( BUILD_SHARED_LIBS )
  SET( VTKMANTA_BUILD_SHARED_LIBS_CONFIG 0 )
ENDIF ( BUILD_SHARED_LIBS )

IF ( VTK_FOUND )
  # Access the 'examples' directory for the secondary CMakeLists.txt 
  SUBDIRS(examples)

  # Build vtkManta tests
  IF ( BUILD_TESTING )
    ENABLE_TESTING()
    INCLUDE( CTest )
  ENDIF ( BUILD_TESTING )

  # If this is a build tree, provide an option for exporting this
  # project's executables and libraries to VTK's or ParaView's build directory.
  IF ( EXISTS ${EXES_LIBS_EXPORT_DIR}/bin )

    OPTION ( EXPORT_EXES_AND_LIBS
             "Export this project's executables and libraries to VTK's or ParaView's build directory."
             OFF
           )
    MARK_AS_ADVANCED ( EXPORT_EXES_AND_LIBS )
    IF ( EXPORT_EXES_AND_LIBS )
      SET ( LIBRARY_OUTPUT_PATH ${EXES_LIBS_EXPORT_DIR}/bin )
      SET ( EXECUTABLE_OUTPUT_PATH ${EXES_LIBS_EXPORT_DIR}/bin )
    ENDIF ( EXPORT_EXES_AND_LIBS )

  ENDIF ( EXISTS ${EXES_LIBS_EXPORT_DIR}/bin ) 

  # Create the header to configure the classes.
  CONFIGURE_FILE ( ${PROJECT_SOURCE_DIR}/vtkMantaConfigure.h.in
                   ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Configure.h
                   @ONLY
                   IMMEDIATE
                 )

  # Use wrapping hints for this project.
  SET ( VTK_WRAP_HINTS "${PROJECT_SOURCE_DIR}/hints" )

  # Create the instantiator for these classes.
  SET ( VTK_USE_INSTANTIATOR_NEW 1 )
  VTK_MAKE_INSTANTIATOR3 ( "${PROJECT_NAME}Instantiator"
                           vtkMantaInstantiator_SRCS
                           "${MANTA_SRCS}"
                           "VTK_${PROJECT_NAME}_EXPORT"
                           ${PROJECT_BINARY_DIR}
                           "${PROJECT_NAME}Configure.h"
                         )

  # Create vtkManta libraries (libvtkManta.so and
  # possibly libvtkMantaCS.so, libvtkMantaPlugin.so, libvtkMantaPython.so, libvtkMantaPythonD.so).
  ADD_LIBRARY ( ${PROJECT_NAME} ${MANTA_SRCS} ${vtkMantaInstantiator_SRCS} )
  TARGET_LINK_LIBRARIES ( ${PROJECT_NAME} ${vtkManta_LIBS} ${MANTA_TARGET_LINK_LIBRARIES} )

  
  # ------------------------------------
  # Below this line are wrapping stuffs.
  # ------------------------------------


  IF ( BUILD_AGAINST_PACKAGE MATCHES "VTK" )
    # Create Tcl wrappers if VTK is wrapped in Tcl.
    IF ( VTK_WRAP_TCL )

      INCLUDE ( ${VTK_CMAKE_DIR}/vtkWrapTcl.cmake )
      VTK_WRAP_TCL3 ( ${PROJECT_NAME}TCL vtkMantaTCL_SRCS "${MANTA_SRCS}" "" )
      ADD_LIBRARY ( ${PROJECT_NAME}TCL ${vtkMantaTCL_SRCS} )
      
      TARGET_LINK_LIBRARIES ( ${PROJECT_NAME}TCL ${PROJECT_NAME} ${MANTA_TARGET_LINK_LIBRARIES} )
      FOREACH ( c ${vtkManta_LIBS} )
        TARGET_LINK_LIBRARIES ( ${PROJECT_NAME}TCL ${c}TCL )
      ENDFOREACH ( c )

      # Create the list of kits that are required.
      SET ( VTKManta_KITS "" )
      FOREACH ( c ${vtkManta_LIBS} )
        SET ( VTKManta_KITS "${VTKManta_KITS} ${c}" )
      ENDFOREACH ( c )

      FOREACH ( c ${MANTA_SRCS} )
        GET_FILENAME_COMPONENT ( VTKManta_LAST_CLASS ${c} NAME_WE )
      ENDFOREACH ( c )

      IF ( CMAKE_CONFIGURATION_TYPES )
        FOREACH ( config ${CMAKE_CONFIGURATION_TYPES} )
          SET ( VTKManta_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}/${config}" )
          CONFIGURE_FILE ( ${PROJECT_SOURCE_DIR}/pkgIndex.tcl.in
            ${PROJECT_BINARY_DIR}/${config}/pkgIndex.tcl
            @ONLY
            IMMEDIATE
            )
        ENDFOREACH ( config )
      ELSE ( CMAKE_CONFIGURATION_TYPES )
        SET ( VTKManta_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}" )
        CONFIGURE_FILE ( ${PROJECT_SOURCE_DIR}/pkgIndex.tcl.in  
          ${PROJECT_BINARY_DIR}/pkgIndex.tcl     
          @ONLY
          IMMEDIATE
          )
      ENDIF ( CMAKE_CONFIGURATION_TYPES ) 

    ENDIF ( VTK_WRAP_TCL )


    # Create Python wrappers if VTK is wrapped in Python.
    IF ( VTK_WRAP_PYTHON )

      INCLUDE ( ${VTK_CMAKE_DIR}/vtkWrapPython.cmake )
      VTK_WRAP_PYTHON3 ( ${PROJECT_NAME}Python vtkMantaPYTHON_SRCS "${MANTA_SRCS}" )
      ADD_LIBRARY ( ${PROJECT_NAME}PythonD ${vtkMantaPYTHON_SRCS} )
      ADD_LIBRARY ( ${PROJECT_NAME}Python MODULE ${PROJECT_NAME}PythonInit.cxx )

      TARGET_LINK_LIBRARIES ( ${PROJECT_NAME}PythonD ${PROJECT_NAME} ${MANTA_TARGET_LINK_LIBRARIES} )
      FOREACH ( c ${vtkManta_LIBS} )
        TARGET_LINK_LIBRARIES ( ${PROJECT_NAME}PythonD ${c}PythonD )
      ENDFOREACH ( c )
      TARGET_LINK_LIBRARIES ( ${PROJECT_NAME}Python ${PROJECT_NAME}PythonD )

    ENDIF ( VTK_WRAP_PYTHON )


    # Create Java wrappers if VTK is wrapped in Java.
    IF ( VTK_WRAP_JAVA )
      INCLUDE ( ${VTK_CMAKE_DIR}/vtkWrapJava.cmake )
      SET ( VTK_WRAP_JAVA3_INIT_DIR "${PROJECT_SOURCE_DIR}" )
      SET ( VTK_JAVA_HOME ${PROJECT_BINARY_DIR}/java/${PROJECT_NAME} )
      MAKE_DIRECTORY ( ${VTK_JAVA_HOME} )
      VTK_WRAP_JAVA3 ( ${PROJECT_NAME}Java vtkMantaJAVA_SRCS "${MANTA_SRCS}" )
      ADD_LIBRARY ( ${PROJECT_NAME}Java ${vtkMantaJAVA_SRCS} )

      TARGET_LINK_LIBRARIES ( ${PROJECT_NAME}Java ${PROJECT_NAME} )
      FOREACH ( c ${vtkManta_LIBS} )
        TARGET_LINK_LIBRARIES ( ${PROJECT_NAME}Java ${c}Java )
      ENDFOREACH ( c )
    ENDIF ( VTK_WRAP_JAVA )

  ENDIF ( BUILD_AGAINST_PACKAGE MATCHES "VTK" )
  

ENDIF ( VTK_FOUND )

