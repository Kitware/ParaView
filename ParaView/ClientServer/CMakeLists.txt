
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
  vtk3DWidget
  vtkAbstractMapper
  vtkAbstractMapper3D
  vtkAbstractTransform
  vtkActor
  vtkActor2D
  vtkAllToNRedistributePolyData
  vtkArrowSource
  vtkAxes
  vtkBoxWidget
  vtkClientCompositeManager
  vtkClipDataSet
  vtkCollectPolyData
  vtkCompositeManager
  vtkCompositer
  vtkCompressCompositer
  vtkConeSource
  vtkDataObject
  vtkDataReader
  vtkDataSet
  vtkDataSetReader
  vtkDataSetSource
  vtkDataSetSurfaceFilter
  vtkDataSetToDataSetFilter
  vtkDataSetToPolyDataFilter
  vtkDataSetToUnstructuredGridFilter
  vtkDecimatePro
  vtkExtentTranslator
  vtkExtractGrid
  vtkGenericRenderWindowInteractor
  vtkGeometryFilter
  vtkGraphicsFactory
  vtkHomogeneousTransform
  vtkImplicitFunction
  vtkImplicitPlaneWidget
  vtkInteractorObserver
  vtkKWApplication
  vtkKWObject
  vtkLineSource
  vtkLineWidget
  vtkLinearTransform
  vtkLookupTable
  vtkMapper
  vtkMatrix4x4
  vtkMultiDisplayManager
  vtkObject
  vtkObjectBase
  vtkOpenGLActor
  vtkOpenGLPolyDataMapper
  vtkOpenGLProperty
  vtkOpenGLRenderWindow
  vtkOpenGLRenderer
  vtkPVApplication
  vtkPVArrowSource
  vtkPVClientServerModule
  vtkPVClipDataSet
  vtkPVDuplicatePolyData
  vtkPVExtentTranslator
  vtkPVGenericRenderWindowInteractor
  vtkPVGeometryFilter
  vtkPVLODActor
  vtkPVMPIProcessModule
  vtkPVProcessModule
  vtkPVTreeComposite
  vtkPVUpdateSuppressor
  vtkPieceScalars
  vtkPlane
  vtkPlaneWidget
  vtkPlanes
  vtkPointSet
  vtkPointSource
  vtkPointWidget
  vtkPolyData
  vtkPolyDataMapper
  vtkPolyDataSource
  vtkPolyDataSourceWidget
  vtkPolyDataToPolyDataFilter
  vtkProcessObject
  vtkProp
  vtkProp3D
  vtkProperty
  vtkQuadricClustering
  vtkRedistributePolyData
  vtkRenderWindow
  vtkRenderWindowInteractor
  vtkRenderer
  vtkScalarsToColors
  vtkShrinkPolyData
  vtkSource
  vtkSphere
  vtkSphereSource
  vtkSphereWidget
  vtkStreamTracer
  vtkStructuredGridOutlineFilter
  vtkStructuredGridSource
  vtkStructuredGridToPolyDataFilter
  vtkStructuredGridToStructuredGridFilter
  vtkTransform
  vtkTreeCompositer
  vtkTubeFilter
  vtkUnstructuredGridSource
  vtkViewport
  vtkWeightedRedistributePolyData
  vtkWindow
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
  FOREACH(DIR ${VTK_INCLUDE_DIRS} ${PARAVIEW_SOURCE_DIR}/ParaView ${PARAVIEW_SOURCE_DIR}/Widgets ${PARAVIEW_SOURCE_DIR}/Filters)
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
TARGET_LINK_LIBRARIES (vtkParaViewCSWrapped vtkClientServer vtkKWParaViewTCL vtkHybrid vtkParallel )
