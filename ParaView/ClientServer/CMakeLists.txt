
SET(ClientServer_SRCS
  vtkClientServerID.h
  vtkClientServerInterpreter.cxx
  vtkClientServerInterpreter.h
  vtkClientServerStream.cxx
  vtkClientServerStream.h
  vtkType.h
  vtkTypeFromNative.h
  )

ADD_LIBRARY(vtkClientServer ${ClientServer_SRCS})
TARGET_LINK_LIBRARIES(vtkClientServer vtkKWCommonTCL vtkCommon)

SET(VTK_CLIENT_SERVER_BUILD_SHARED ${BUILD_SHARED_LIBS})
CONFIGURE_FILE(
  ${PARAVIEW_SOURCE_DIR}/ClientServer/vtkClientServerConfigure.h.in
  ${PARAVIEW_BINARY_DIR}/ClientServer/vtkClientServerConfigure.h
  @ONLY IMMEDIATE
  )

SET (ClientServer_WRAP_CLASSES
#
 vtkObjectBase
 vtkObject
#
 vtkSource
 vtkProcessObject
 vtkPolyDataSource
 vtkSphereSource
#
 vtkActor2D
 vtkProp
#
 vtkPolyDataToPolyDataFilter
 vtkShrinkPolyData
#
 vtkDecimatePro
#
 vtkDataSetSource
 vtkDataSetToDataSetFilter
 vtkPieceScalars
#
 vtkGraphicsFactory
#
 vtkDataSetReader
 vtkDataReader
#
 vtkStreamTracer
 vtkDataSetToPolyDataFilter
#
 vtkExtractGrid
 vtkStructuredGridToStructuredGridFilter
 vtkStructuredGridSource
#
 vtkStructuredGridOutlineFilter
 vtkStructuredGridToPolyDataFilter
#
 vtkGeometryFilter
#
 vtkLineSource
 vtkConeSource

#
 vtkTubeFilter
#
 vtkLineWidget
 vtk3DWidget
 vtkInteractorObserver


 vtkActor
 vtkMapper
 vtkAbstractMapper
 vtkAbstractMapper3D
 vtkPolyDataMapper
 vtkRenderer
 vtkRenderWindow
 vtkRenderWindowInteractor
 vtkOpenGLRenderer
 vtkOpenGLRenderWindow
 vtkOpenGLActor
 vtkOpenGLPolyDataMapper
 vtkWindow
 vtkViewport
 vtkProp3D
 vtkProperty
vtkPolyData
vtkDataSet
vtkDataObject
vtkDataSetSurfaceFilter
vtkArrowSource
vtkExtentTranslator
vtkOpenGLProperty
vtkPointSet
# paraview classes
vtkGenericRenderWindowInteractor
vtkPVArrowSource
vtkPVExtentTranslator
vtkPVGenericRenderWindowInteractor
vtkPVGeometryFilter
vtkPVUpdateSuppressor
vtkPVLODActor
vtkClientCompositeManager
vtkPVApplication
vtkPVProcessModule
vtkPVClientServerModule
vtkPVMPIProcessModule
vtkKWObject
vtkKWApplication
)

IF(VTK_USE_X)
  SET(ClientServer_WRAP_CLASSES ${ClientServer_WRAP_CLASSES}
    vtkXOpenGLRenderWindow
    vtkXRenderWindowInteractor
    )
ELSE(VTK_USE_X)
  IF(WIN32)
    SET(ClientServer_WRAP_CLASSES ${ClientServer_WRAP_CLASSES}
      vtkWin32OpenGLRenderWindow
      vtkWin32RenderWindowInteractor
    )
  ENDIF(WIN32)
ENDIF(VTK_USE_X)

FIND_FILE(VTK_WRAP_HINTS hints
          ${VTK_DIR}/../VTK/Wrapping
          ${VTK_DIR}/../vtknew/Wrapping )


#
# Find where the header files are located
#
FOREACH(CLASS ${ClientServer_WRAP_CLASSES})
  SET(ClientServer_CLASS_FOUND 0)
  FOREACH(DIR ${VTK_INCLUDE_DIRS} ${PARAVIEW_SOURCE_DIR}/ParaView ${PARAVIEW_SOURCE_DIR}/Widgets)
    IF (NOT ClientServer_CLASS_FOUND)
      IF (EXISTS ${DIR}/${CLASS}.h)
        SET(ClientServer_RESOLVED_CLASSES ${ClientServer_RESOLVED_CLASSES} ${DIR}/${CLASS}.h)
        SET(ClientServer_CLASS_FOUND 1)
      ENDIF (EXISTS ${DIR}/${CLASS}.h)
    ENDIF (NOT ClientServer_CLASS_FOUND)
  ENDFOREACH(DIR)
ENDFOREACH(CLASS)

VTK_WRAP_ClientServer(vtkParaViewCSWrapped vtkClientServerWrappedList ${ClientServer_RESOLVED_CLASSES})

ADD_LIBRARY(vtkParaViewCSWrapped ${vtkClientServerWrappedList})
TARGET_LINK_LIBRARIES (vtkParaViewCSWrapped vtkClientServer vtkKWParaViewTCL vtkHybrid vtkParallel)
