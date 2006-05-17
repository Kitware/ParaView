# ---------------------------------------------------------------------------
# KWWidgets_WRAP_TCL
# Macro around various VTK wrapping macros and wrapping-related settings

MACRO(KWWidgets_WRAP_TCL target src_list_name sources commands)

  IF(VTK_MAJOR_VERSION AND VTK_MAJOR_VERSION LESS 5)
      
    # Attempt at supporting 4.4. 
    # This won't work anyway, as the lexer in VTK 4.x was updated later on
    # to support export macros not starting with VTK_ (say, KWWidgets_Export).

    VTK_WRAP_TCL2(
      ${target}
      SOURCES ${src_list_name} ${sources}
      COMMANDS ${commands})
    
  ELSE(VTK_MAJOR_VERSION AND VTK_MAJOR_VERSION LESS 5)
    
    IF(NOT VTK_CMAKE_DIR)
      SET(VTK_CMAKE_DIR "${VTK_SOURCE_DIR}/CMake")
    ENDIF(NOT VTK_CMAKE_DIR)
    INCLUDE("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")
    
    # VTK 5.0 really need some help. The evil VTK_WRAP_TCL3_INIT_DIR hack was
    # fixed on the 5.0 branch but people have downloaded earlier 5.0 as well.
    # Furthermore, old 5.0 can not be used once it has been installed, since
    # vtkWrapperInit.data.in is not installed properly: report that sad fact.

    IF("${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}" EQUAL "5.0")
      IF(VTK_INSTALL_PREFIX)
        IF(NOT EXISTS "${VTK_CMAKE_DIR}/vtkWrapperInit.data.in")
          MESSAGE("Sorry, you are using a VTK 5.0 that can not be used properly once it has been installed. You can either download a more recent VTK 5.0 snapshot from the CVS repository, or simply point KWWidgets to your VTK build directory instead of your VTK install directory.")
        ENDIF(NOT EXISTS "${VTK_CMAKE_DIR}/vtkWrapperInit.data.in")
      ELSE(VTK_INSTALL_PREFIX)
        SET(VTK_WRAP_TCL3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
        SET(VTK_WRAP_PYTHON3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
      ENDIF(VTK_INSTALL_PREFIX)
    ENDIF("${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}" EQUAL "5.0")

    VTK_WRAP_TCL3(
      ${target}
      ${src_list_name} "${sources}" 
      "${commands}" 
      ${ARGN})
    
  ENDIF(VTK_MAJOR_VERSION AND VTK_MAJOR_VERSION LESS 5)

ENDMACRO(KWWidgets_WRAP_TCL)
