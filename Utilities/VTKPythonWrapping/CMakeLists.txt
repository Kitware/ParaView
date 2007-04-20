PROJECT(VTKPythonWrapping)

IF(NOT VTKPythonWrapping_INSTALL_LIBRARIES)
  SET(VTKPythonWrapping_INSTALL_LIBRARIES 1)
ENDIF(NOT VTKPythonWrapping_INSTALL_LIBRARIES)
IF(NOT VTKPythonWrapping_INSTALL_LIB_DIR)
  SET(VTKPythonWrapping_INSTALL_LIB_DIR "/lib/${PROJECT_NAME}")
ENDIF(NOT VTKPythonWrapping_INSTALL_LIB_DIR)
IF(NOT VTKPythonWrapping_INSTALL_BIN_DIR)
  SET(VTKPythonWrapping_INSTALL_BIN_DIR "/bin")
ENDIF(NOT VTKPythonWrapping_INSTALL_BIN_DIR)

# We only wrap subportion of VTK, so include only that
INCLUDE_DIRECTORIES(
  ${VTK_SOURCE_DIR}/Wrapping
  ${VTK_SOURCE_DIR}
  ${VTK_BINARY_DIR}
  ${VTK_SOURCE_DIR}/Common
  ${VTK_SOURCE_DIR}/IO
  ${VTK_SOURCE_DIR}/Filtering
  ${VTK_SOURCE_DIR}/Graphics
  ${VTK_BINARY_DIR}/Rendering
  ${PYTHON_INCLUDE_PATH}
  )

# OpenGL include directories.
IF(APPLE)
  IF(VTK_USE_X)
    SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
      ${OPENGL_INCLUDE_DIR})
  ENDIF(VTK_USE_X)
ELSE(APPLE)
  SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
    ${OPENGL_INCLUDE_DIR})
ENDIF(APPLE)

IF(VTK_USE_X)
  # X include directories.
  SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
    ${CMAKE_Xlib_INCLUDE_PATH} ${CMAKE_Xutil_INCLUDE_PATH})
ENDIF(VTK_USE_X)

IF(VTK_HAVE_VP1000)
  # VolumePro VP 1000 include directory.
  SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
    ${VLI_INCLUDE_PATH_FOR_VP1000})
ENDIF(VTK_HAVE_VP1000)

IF(VTK_USE_MANGLED_MESA)
  # Mangled Mesa include directory.
  SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
    ${MESA_INCLUDE_PATH})
ELSE(VTK_USE_MANGLED_MESA)
  # Off-Screen Mesa include directory.
  IF(VTK_OPENGL_HAS_OSMESA)
    IF(OSMESA_INCLUDE_DIR)
      SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
        ${OSMESA_INCLUDE_DIR})
    ENDIF(OSMESA_INCLUDE_DIR)
  ENDIF(VTK_OPENGL_HAS_OSMESA)
ENDIF(VTK_USE_MANGLED_MESA)

INCLUDE_DIRECTORIES(${PVWRAP_OPEN_GL_DIRS})
SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")

ADD_EXECUTABLE(vtkWrapPython 
  ${VTK_SOURCE_DIR}/Wrapping/vtkWrapPython.c 
  ${VTK_SOURCE_DIR}/Wrapping/vtkParse.tab.c)

TARGET_LINK_LIBRARIES(vtkWrapPython
  ${PYTHON_LIBRARY}
  )

ADD_EXECUTABLE(vtkWrapPythonInit 
  ${VTK_SOURCE_DIR}/Wrapping/vtkWrapPythonInit.c)


SET(VTK_COMMON_SOURCES
  ${VTK_SOURCE_DIR}/Common/vtkAbstractArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkAbstractTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkAmoebaMinimizer.cxx
  ${VTK_SOURCE_DIR}/Common/vtkAnimationCue.cxx
  ${VTK_SOURCE_DIR}/Common/vtkAnimationScene.cxx
  ${VTK_SOURCE_DIR}/Common/vtkArrayIterator.cxx
  ${VTK_SOURCE_DIR}/Common/vtkAssemblyNode.cxx
  ${VTK_SOURCE_DIR}/Common/vtkAssemblyPath.cxx
  ${VTK_SOURCE_DIR}/Common/vtkAssemblyPaths.cxx
  ${VTK_SOURCE_DIR}/Common/vtkBitArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkBitArrayIterator.cxx
  ${VTK_SOURCE_DIR}/Common/vtkBoundingBox.cxx
  ${VTK_SOURCE_DIR}/Common/vtkBox.cxx
  ${VTK_SOURCE_DIR}/Common/vtkByteSwap.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCallbackCommand.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCharArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCollectionIterator.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCommand.cxx
  ${VTK_SOURCE_DIR}/Common/vtkContourValues.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCriticalSection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCylindricalTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDataArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDataArrayCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDataArrayCollectionIterator.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDataArraySelection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDebugLeaks.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDebugLeaksManager.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDirectory.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDoubleArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDynamicLoader.cxx
  ${VTK_SOURCE_DIR}/Common/vtkEdgeTable.cxx
  ${VTK_SOURCE_DIR}/Common/vtkErrorCode.cxx
  ${VTK_SOURCE_DIR}/Common/vtkEventForwarderCommand.cxx
  ${VTK_SOURCE_DIR}/Common/vtkExtentSplitter.cxx
  ${VTK_SOURCE_DIR}/Common/vtkExtentTranslator.cxx
  ${VTK_SOURCE_DIR}/Common/vtkFastNumericConversion.cxx
  ${VTK_SOURCE_DIR}/Common/vtkFileOutputWindow.cxx
  ${VTK_SOURCE_DIR}/Common/vtkFloatArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkFunctionParser.cxx
  ${VTK_SOURCE_DIR}/Common/vtkFunctionSet.cxx
  ${VTK_SOURCE_DIR}/Common/vtkGarbageCollector.cxx
  ${VTK_SOURCE_DIR}/Common/vtkGarbageCollectorManager.cxx
  ${VTK_SOURCE_DIR}/Common/vtkGeneralTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkHeap.cxx
  ${VTK_SOURCE_DIR}/Common/vtkHomogeneousTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIOStream.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIdList.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIdListCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIdTypeArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIdentityTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkImplicitFunction.cxx
  ${VTK_SOURCE_DIR}/Common/vtkImplicitFunctionCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIndent.cxx
  ${VTK_SOURCE_DIR}/Common/vtkInitialValueProblemSolver.cxx
  ${VTK_SOURCE_DIR}/Common/vtkInstantiator.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIntArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkLargeInteger.cxx
  ${VTK_SOURCE_DIR}/Common/vtkLinearTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkLogLookupTable.cxx
  ${VTK_SOURCE_DIR}/Common/vtkLongArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkLookupTable.cxx
  ${VTK_SOURCE_DIR}/Common/vtkMath.cxx
  ${VTK_SOURCE_DIR}/Common/vtkMatrix4x4.cxx
  ${VTK_SOURCE_DIR}/Common/vtkMatrixToHomogeneousTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkMatrixToLinearTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkMultiThreader.cxx
  ${VTK_SOURCE_DIR}/Common/vtkMutexLock.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOStrStreamWrapper.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOStreamWrapper.cxx
  ${VTK_SOURCE_DIR}/Common/vtkObject.cxx
  ${VTK_SOURCE_DIR}/Common/vtkObjectBase.cxx
  ${VTK_SOURCE_DIR}/Common/vtkObjectFactory.cxx
  ${VTK_SOURCE_DIR}/Common/vtkObjectFactoryCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOldStyleCallbackCommand.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOutputWindow.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOverrideInformation.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOverrideInformationCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricBoy.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricConicSpiral.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricCrossCap.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricDini.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricEllipsoid.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricEnneper.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricFigure8Klein.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricFunction.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricKlein.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricMobius.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricRandomHills.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricRoman.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricSuperEllipsoid.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricSuperToroid.cxx
  ${VTK_SOURCE_DIR}/Common/vtkParametricTorus.cxx
  ${VTK_SOURCE_DIR}/Common/vtkPerspectiveTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkPlane.cxx
  ${VTK_SOURCE_DIR}/Common/vtkPlaneCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkPlanes.cxx
  ${VTK_SOURCE_DIR}/Common/vtkPoints.cxx
  ${VTK_SOURCE_DIR}/Common/vtkPriorityQueue.cxx
  ${VTK_SOURCE_DIR}/Common/vtkProp.cxx
  ${VTK_SOURCE_DIR}/Common/vtkPropCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkProperty2D.cxx
  ${VTK_SOURCE_DIR}/Common/vtkQuadric.cxx
  ${VTK_SOURCE_DIR}/Common/vtkReferenceCount.cxx
  ${VTK_SOURCE_DIR}/Common/vtkRungeKutta2.cxx
  ${VTK_SOURCE_DIR}/Common/vtkRungeKutta4.cxx
  ${VTK_SOURCE_DIR}/Common/vtkRungeKutta45.cxx
  ${VTK_SOURCE_DIR}/Common/vtkScalarsToColors.cxx
  ${VTK_SOURCE_DIR}/Common/vtkShortArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkSignedCharArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkSmartPointerBase.cxx
  ${VTK_SOURCE_DIR}/Common/vtkSphericalTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkStdString.cxx
  ${VTK_SOURCE_DIR}/Common/vtkStringArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkStructuredData.cxx
  ${VTK_SOURCE_DIR}/Common/vtkStructuredVisibilityConstraint.cxx
  ${VTK_SOURCE_DIR}/Common/vtkTableExtentTranslator.cxx
  ${VTK_SOURCE_DIR}/Common/vtkTensor.cxx
  ${VTK_SOURCE_DIR}/Common/vtkThreadMessager.cxx
  ${VTK_SOURCE_DIR}/Common/vtkTimeStamp.cxx
  ${VTK_SOURCE_DIR}/Common/vtkTimerLog.cxx
  ${VTK_SOURCE_DIR}/Common/vtkTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkTransformCollection.cxx
  ${VTK_SOURCE_DIR}/Common/vtkUnsignedCharArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkUnsignedIntArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkUnsignedLongArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkUnsignedShortArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkVariant.cxx
  ${VTK_SOURCE_DIR}/Common/vtkVariantArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkVersion.cxx
  ${VTK_SOURCE_DIR}/Common/vtkVoidArray.cxx
  ${VTK_SOURCE_DIR}/Common/vtkWarpTransform.cxx
  ${VTK_SOURCE_DIR}/Common/vtkWindow.cxx
  ${VTK_SOURCE_DIR}/Common/vtkWindowLevelLookupTable.cxx
  ${VTK_SOURCE_DIR}/Common/vtkXMLFileOutputWindow.cxx
  )

SET(VTK_FILTERS_SOURCES
  ${VTK_SOURCE_DIR}/Filtering/vtkAbstractGraph.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkAbstractGraphAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkAbstractMapper.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkActor2D.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkActor2DCollection.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkAlgorithmOutput.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkBiQuadraticQuad.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkBiQuadraticQuadraticHexahedron.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkBiQuadraticQuadraticWedge.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCachedStreamingDemandDrivenPipeline.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCardinalSpline.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCastToConcrete.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCell.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCell3D.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCellArray.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCellData.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCellLinks.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCellLocator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCellTypes.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkColorTransferFunction.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCompositeDataIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCompositeDataPipeline.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCompositeDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCone.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkConvexPointSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCoordinate.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkCylinder.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDataObject.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDataObjectAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDataObjectCollection.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDataObjectTypes.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetAttributes.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetCollection.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkDemandDrivenPipeline.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkEmptyCell.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkExecutive.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkExplicitCell.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkFieldData.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkFilteringInformationKeyManager.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericAdaptorCell.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericAttribute.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericAttributeCollection.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericCell.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericCellIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericCellTessellator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericDataSetAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericEdgeTable.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericPointIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericSubdivisionErrorMetric.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGeometricErrorMetric.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGraph.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGraphAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGraphDFSIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGraphIdList.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkAttributesErrorMetric.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHexagonalPrism.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHexahedron.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHierarchicalDataInformation.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHierarchicalDataIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHierarchicalBoxDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHierarchicalDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHierarchicalDataSetAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHyperOctree.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHyperOctreeAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHyperOctreeCursor.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHyperOctreePointsGrabber.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImageAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImageData.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImageIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImageProgressIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImageToStructuredPoints.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImplicitBoolean.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImplicitDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImplicitSelectionLoop.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImplicitSum.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImplicitVolume.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImplicitWindowFunction.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformation.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationDataObjectKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationDoubleKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationDoubleVectorKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationExecutivePortKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationExecutivePortVectorKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIdTypeKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationInformationKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationInformationVectorKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIntegerKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIntegerPointerKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIntegerVectorKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationKeyVectorKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationObjectBaseKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationRequestKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationStringKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationUnsignedLongKey.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationVector.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkInterpolatedVelocityField.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkKochanekSpline.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkLine.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkLocator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkMapper2D.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkMergePoints.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkMultiBlockDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkMultiBlockDataSetAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkMultiGroupDataInformation.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkMultiGroupDataIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkMultiGroupDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkMultiGroupDataSetAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkNonLinearCell.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkOrderedTriangulator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkParametricSpline.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPentagonalPrism.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPerlinNoise.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPiecewiseFunction.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPiecewiseFunctionAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPiecewiseFunctionShiftScale.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPixel.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPointData.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPointLocator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPointSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPointSetAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPolyData.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPolyDataAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPolyDataCollection.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPolyLine.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPolyVertex.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPolygon.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkProcessObject.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPropAssembly.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkPyramid.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuad.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticEdge.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticHexahedron.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticPyramid.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticLinearQuad.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticQuad.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticTetra.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticTriangle.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticLinearWedge.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkQuadraticWedge.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkRectilinearGrid.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkRectilinearGridAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkScalarTree.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkSelection.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkSimpleCellTessellator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkSimpleScalarTree.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkSmoothErrorMetric.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkSphere.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkSpline.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkStreamingDemandDrivenPipeline.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredGrid.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredGridAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredPoints.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredPointsCollection.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkSuperquadric.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTable.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTableAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTemporalDataSet.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTemporalDataSetAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTetra.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkThreadedImageAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTree.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTreeAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTreeDFSIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTriangle.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTriangleStrip.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTriQuadraticHexahedron.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkTrivialProducer.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkUniformGrid.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkUnstructuredGrid.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkUnstructuredGridAlgorithm.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkVertex.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkVertexLinks.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkViewDependentErrorMetric.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkVoxel.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkWedge.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkViewport.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericInterpolatedVelocityField.cxx
  )

SET(Kit_SRCS
  ${VTK_COMMON_SOURCES}
  ${VTK_FILTERS_SOURCES}
  "${VTK_SOURCE_DIR}/Graphics/vtkProgrammableFilter.cxx"
  "${VTK_SOURCE_DIR}/IO/vtkXMLParser.cxx"
  )

SET(VTK_COMMON_ABSTRACTS
  ${VTK_SOURCE_DIR}/Common/vtkAbstractArray
  ${VTK_SOURCE_DIR}/Common/vtkArrayIterator
  ${VTK_SOURCE_DIR}/Common/vtkArrayIteratorTemplate
  ${VTK_SOURCE_DIR}/Common/vtkAbstractTransform
  ${VTK_SOURCE_DIR}/Common/vtkCallbackCommand
  ${VTK_SOURCE_DIR}/Common/vtkCommand
  ${VTK_SOURCE_DIR}/Common/vtkDataArray
  ${VTK_SOURCE_DIR}/Common/vtkEventForwarderCommand
  ${VTK_SOURCE_DIR}/Common/vtkFunctionSet
  ${VTK_SOURCE_DIR}/Common/vtkHomogeneousTransform
  ${VTK_SOURCE_DIR}/Common/vtkImplicitFunction
  ${VTK_SOURCE_DIR}/Common/vtkInitialValueProblemSolver
  ${VTK_SOURCE_DIR}/Common/vtkLinearTransform
  ${VTK_SOURCE_DIR}/Common/vtkObjectBase
  ${VTK_SOURCE_DIR}/Common/vtkObjectFactory
  ${VTK_SOURCE_DIR}/Common/vtkOldStyleCallbackCommand
  ${VTK_SOURCE_DIR}/Common/vtkOverrideInformation
  ${VTK_SOURCE_DIR}/Common/vtkOverrideInformationCollection
  ${VTK_SOURCE_DIR}/Common/vtkParametricFunction
  ${VTK_SOURCE_DIR}/Common/vtkProp
  ${VTK_SOURCE_DIR}/Common/vtkScalarsToColors
  ${VTK_SOURCE_DIR}/Common/vtkStructuredData
  ${VTK_SOURCE_DIR}/Common/vtkWarpTransform
  ${VTK_SOURCE_DIR}/Common/vtkWindow
  )

SET(VTK_FILTERS_ABSTRACTS
  ${VTK_SOURCE_DIR}/Filtering/vtkAbstractGraph
  ${VTK_SOURCE_DIR}/Filtering/vtkAbstractMapper
  ${VTK_SOURCE_DIR}/Filtering/vtkCell
  ${VTK_SOURCE_DIR}/Filtering/vtkCell3D
  ${VTK_SOURCE_DIR}/Filtering/vtkCompositeDataIterator
  ${VTK_SOURCE_DIR}/Filtering/vtkCompositeDataSet
  ${VTK_SOURCE_DIR}/Filtering/vtkDataObjectSource
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSet
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetSource
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetToDataSetFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetToImageFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetToPolyDataFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetToStructuredGridFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetToStructuredPointsFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkDataSetToUnstructuredGridFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkExecutive
  ${VTK_SOURCE_DIR}/Filtering/vtkExplicitCell
  ${VTK_SOURCE_DIR}/Filtering/vtkFilteringInformationKeyManager
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericAdaptorCell
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericAttribute
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericCellIterator
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericCellTessellator
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericDataSet
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericDataSetAlgorithm
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericPointIterator
  ${VTK_SOURCE_DIR}/Filtering/vtkGenericSubdivisionErrorMetric
  ${VTK_SOURCE_DIR}/Filtering/vtkHierarchicalDataSetAlgorithm
  ${VTK_SOURCE_DIR}/Filtering/vtkHyperOctreeAlgorithm
  ${VTK_SOURCE_DIR}/Filtering/vtkHyperOctreeCursor
  ${VTK_SOURCE_DIR}/Filtering/vtkHyperOctreePointsGrabber
  ${VTK_SOURCE_DIR}/Filtering/vtkImageAlgorithm
  ${VTK_SOURCE_DIR}/Filtering/vtkImageInPlaceFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkImageIterator
  ${VTK_SOURCE_DIR}/Filtering/vtkImageMultipleInputFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkImageMultipleInputOutputFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkImageProgressIterator
  ${VTK_SOURCE_DIR}/Filtering/vtkImageSource
  ${VTK_SOURCE_DIR}/Filtering/vtkImageToImageFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkImageTwoInputFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationDataObjectKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationDoubleKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationDoubleVectorKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationExecutivePortKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationExecutivePortVectorKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIdTypeKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationInformationKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationInformationVectorKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIntegerKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIntegerPointerKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationIntegerVectorKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationKeyVectorKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationObjectBaseKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationRequestKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationStringKey
  ${VTK_SOURCE_DIR}/Filtering/vtkInformationUnsignedLongKey
  ${VTK_SOURCE_DIR}/Filtering/vtkLocator
  ${VTK_SOURCE_DIR}/Filtering/vtkMapper2D
  ${VTK_SOURCE_DIR}/Filtering/vtkMultiGroupDataSetAlgorithm
  ${VTK_SOURCE_DIR}/Filtering/vtkNonLinearCell
  ${VTK_SOURCE_DIR}/Filtering/vtkPointSet
  ${VTK_SOURCE_DIR}/Filtering/vtkPointSetSource
  ${VTK_SOURCE_DIR}/Filtering/vtkPointSetToPointSetFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkPolyDataSource
  ${VTK_SOURCE_DIR}/Filtering/vtkPolyDataToPolyDataFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkProcessObject
  ${VTK_SOURCE_DIR}/Filtering/vtkRectilinearGridSource
  ${VTK_SOURCE_DIR}/Filtering/vtkRectilinearGridToPolyDataFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkScalarTree
  ${VTK_SOURCE_DIR}/Filtering/vtkSimpleImageToImageFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkSource
  ${VTK_SOURCE_DIR}/Filtering/vtkSpline
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredGridSource
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredGridToPolyDataFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredGridToStructuredGridFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredPointsSource
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredPointsToPolyDataFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredPointsToStructuredPointsFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkStructuredPointsToUnstructuredGridFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkThreadedImageAlgorithm
  ${VTK_SOURCE_DIR}/Filtering/vtkUnstructuredGridSource
  ${VTK_SOURCE_DIR}/Filtering/vtkUnstructuredGridToPolyDataFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkUnstructuredGridToUnstructuredGridFilter
  ${VTK_SOURCE_DIR}/Filtering/vtkViewport
  )

SET_SOURCE_FILES_PROPERTIES(
  ${VTK_COMMON_ABSTRACTS}
  ${VTK_FILTERS_ABSTRACTS}
  ABSTRACT
  )

SET(VTK_COMMON_WRAP_EXCLUDES
  ${VTK_SOURCE_DIR}/Common/vtkArrayIteratorTemplate.txx
  ${VTK_SOURCE_DIR}/Common/vtkBoundingBox.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCallbackCommand.cxx
  ${VTK_SOURCE_DIR}/Common/vtkCommand.cxx
  ${VTK_SOURCE_DIR}/Common/vtkDebugLeaksManager.cxx
  ${VTK_SOURCE_DIR}/Common/vtkErrorCode.cxx
  ${VTK_SOURCE_DIR}/Common/vtkEventForwarderCommand.cxx
  ${VTK_SOURCE_DIR}/Common/vtkGarbageCollectorManager.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIOStream.cxx
  ${VTK_SOURCE_DIR}/Common/vtkIndent.cxx
  ${VTK_SOURCE_DIR}/Common/vtkLargeInteger.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOStrStreamWrapper.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOStreamWrapper.cxx
  ${VTK_SOURCE_DIR}/Common/vtkOldStyleCallbackCommand.cxx
  ${VTK_SOURCE_DIR}/Common/vtkSmartPointerBase.cxx
  ${VTK_SOURCE_DIR}/Common/vtkStdString.cxx
  ${VTK_SOURCE_DIR}/Common/vtkTimeStamp.cxx
  ${VTK_SOURCE_DIR}/Common/vtkVariant.cxx
  )

SET(VTK_FILTERS_WRAP_EXCLUDES
  ${VTK_SOURCE_DIR}/Filtering/vtkFilteringInformationKeyManager.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkHierarchicalDataIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImageIterator.cxx
  ${VTK_SOURCE_DIR}/Filtering/vtkImageProgressIterator.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  ${VTK_COMMON_WRAP_EXCLUDES}
  ${VTK_FILTERS_WRAP_EXCLUDES}
  WRAP_EXCLUDE
  )

SET(EXE_EXT "")
IF(WIN32)
  SET(EXE_EXT ".exe")
ENDIF(WIN32)

SET(MAKE_SYSTEM)
IF(CMAKE_BUILD_TOOL MATCHES "make")
  SET(MAKE_SYSTEM 1)
ENDIF(CMAKE_BUILD_TOOL MATCHES "make")

SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF(MAKE_SYSTEM OR UNIX)
  SET(CFG_INIT "")
ENDIF(MAKE_SYSTEM OR UNIX)

SET(VTK_WRAP_PYTHON_EXE 
  "${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/vtkWrapPython${EXE_EXT}"
  CACHE INTERNAL "Location of program to do Python wrapping")
SET(VTK_WRAP_PYTHON_INIT_EXE 
  "${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/vtkWrapPythonInit${EXE_EXT}"
  CACHE INTERNAL "Location of program to do Python wrapping")

# Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
SET(VTK_WRAP_PYTHON_FIND_LIBS 1)

INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
IF (PARAVIEW_ENABLE_PYTHON AND NOT VTK_WRAP_PYTHON)
  SET(KIT Common)
  SET(VTK_WRAP_PYTHON3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
  SET(Kit_PYTHON_EXTRA_SRCS
    "${VTK_SOURCE_DIR}/${KIT}/vtkPythonUtil.cxx")
  SET(KIT_PYTHON_LIBS
    vtkIO
    vtkRendering
    vtkGraphics
    )

  IF(UNIX)
    # On UNIX, Python needs the math library.
    SET(KIT_PYTHON_LIBS ${KIT_PYTHON_LIBS} -lm)

    # If using X with Tk, we need to link to X when we link
    # to Tk in order to support static linking.
    IF(CMAKE_HAS_X)
      SET(KIT_PYTHON_LIBS ${KIT_PYTHON_LIBS} -lXt ${CMAKE_X_LIBS})
    ENDIF(CMAKE_HAS_X)
  ENDIF(UNIX)
  SET(KIT_PYTHON_DEPS)
  SET(VTK_INSTALL_NO_LIBRARIES 1)
  IF(VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTK_INSTALL_NO_LIBRARIES)
  ENDIF(VTKPythonWrapping_INSTALL_BIN_DIR)
  SET(VTK_INSTALL_LIB_DIR "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_BIN_DIR_CM24 "${VTK_INSTALL_BIN_DIR}")
  #STRING(REGEX REPLACE "^/" "" VTK_INSTALL_LIB_DIR_CM24 "${VTK_INSTALL_LIB_DIR}")
  SET (VTK_INSTALL_LIB_DIR_CM24 ${PV_INSTALL_LIB_DIR_CM24})

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON AND NOT VTK_WRAP_PYTHON)

IF (PARAVIEW_ENABLE_PYTHON)

  # Install yje vtk${KIT}Python library.
  IF(NOT PV_INSTALL_NO_LIBRARIES)
    SET (targets_to_install ${targets_to_install} vtk${KIT}Python)
    IF(PV_INSTALL_HAS_CMAKE_24)
      
      INSTALL(TARGETS ${targets_to_install}
        RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
        LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
        ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
    ELSE(PV_INSTALL_HAS_CMAKE_24)
      INSTALL_TARGETS(${PV_INSTALL_LIB_DIR}
        vtk${KIT}
        ${targets_to_install}
        )
    ENDIF(PV_INSTALL_HAS_CMAKE_24)

  ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

  SET(PV_PYTHON_MODULE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/paraview")
  SET(PV_PYTHON_MODULE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/paraview")
  SET(PV_PYTHON_MODULE_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "Path to paraview module.")
  MAKE_DIRECTORY(${PV_PYTHON_MODULE_BINARY_DIR})
  # Handle out-of-source builds correctly.
  #
  #  1. Create a list of Python files to be installed/copied.
  #  2. Copy them to the binary directory. Since paraview cannot be built
  #     in place, we must always copy the files to the binary directory.
  #  3. Use Python's compileall to compile the copied files.
  #
  # *** Step 1 has to be done carefully to avoid missing out files ***


  # List all python source files.
  # All files paraview/*.py included in the paraview module.
  SET(PV_PYTHON_FILES
    __init__
    numeric
    util
    )

  # Now copy these files if necessary.
  SET(PV_PYTHON_SOURCE_FILES)
  SET(PV_PYTHON_OUTPUT_FILES)
  FOREACH(file ${VTK_PYTHON_FILES})
    SET(src "${PV_PYTHON_MODULE_SOURCE_DIR}/${file}.py")
    SET(PV_PYTHON_SOURCE_FILES ${VTK_PYTHON_SOURCE_FILES} ${src})
  ENDFOREACH(file)

  IF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    #MESSAGE("In source build -- no need to copy Python files.")
  ELSE ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    FOREACH(file ${PV_PYTHON_FILES})
      SET(src "${PV_PYTHON_MODULE_SOURCE_DIR}/${file}.py")
      SET(tgt "${PV_PYTHON_MODULE_BINARY_DIR}/${file}.py")
      ADD_CUSTOM_COMMAND(DEPENDS ${src}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${src} ${tgt}
        OUTPUT ${tgt}
        COMMENT "source copy")
      SET(PV_PYTHON_OUTPUT_FILES ${PV_PYTHON_OUTPUT_FILES} ${tgt})
    ENDFOREACH(file)
  ENDIF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")

  # Byte compile the Python files.
  CONFIGURE_FILE(${PV_PYTHON_MODULE_SOURCE_DIR}/compile_all_pv.py.in
    ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py
    @ONLY IMMEDIATE)

  ADD_CUSTOM_COMMAND(
    WORKING_DIRECTORY ${PV_PYTHON_MODULE_BINARY_DIR}
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS compile_all_pv.py
    DEPENDS ${PV_PYTHON_SOURCE_FILES} ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py
    ${PV_PYTHON_OUTPUT_FILES}
    OUTPUT "${PV_PYTHON_MODULE_BINARY_DIR}/pv_compile_complete"
    )

  ADD_CUSTOM_TARGET(paraview_pyc ALL 
    DEPENDS "${PV_PYTHON_MODULE_BINARY_DIR}/pv_compile_complete")

  # Install the paraview module files.
  IF (NOT PV_INSTALL_NO_LIBRARIES)
    IF(PV_INSTALL_HAS_CMAKE_24)
     MESSAGE ("PV_INSTALL_LIB_DIR_CM24 = ${PV_INSTALL_LIB_DIR_CM24}")
      INSTALL(DIRECTORY ${PV_PYTHON_MODULE_BINARY_DIR} 
          DESTINATION ${PV_INSTALL_LIB_DIR_CM24}
          COMPONENT Runtime
          )
    ELSE(PV_INSTALL_HAS_CMAKE_24)
      INSTALL_FILES(${PV_INSTALL_LIB_DIR}/paraview FILES ${PV_PYTHON_OUTPUT_FILES})
    ENDIF(PV_INSTALL_HAS_CMAKE_24)
  ENDIF (NOT PV_INSTALL_NO_LIBRARIES)
ENDIF (PARAVIEW_ENABLE_PYTHON)
