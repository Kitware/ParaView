IF(NOT VTK_USE_N_WAY_ARRAYS)
  MESSAGE(SEND_ERROR "Array plugin requires VTK_USE_N_WAY_ARRAYS")
ENDIF(NOT VTK_USE_N_WAY_ARRAYS)

IF(NOT VTK_USE_BOOST)
  MESSAGE(SEND_ERROR "Array plugin requires VTK_USE_BOOST")
ENDIF(NOT VTK_USE_BOOST)

INCLUDE_DIRECTORIES(
  ${PARAVIEW_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIR}
  )

ADD_PARAVIEW_PLUGIN(
  ArrayPlugin "1.0"
  SERVER_MANAGER_XML
    OverViewArray.xml
  SERVER_MANAGER_SOURCES
    ${VTK_SOURCE_DIR}/Filtering/vtkArrayDataAlgorithm.h
    ${VTK_SOURCE_DIR}/Infovis/vtkAdjacencyMatrixToEdgeTable.h
    ${VTK_SOURCE_DIR}/Infovis/vtkArrayToTable.h
    ${VTK_SOURCE_DIR}/Infovis/vtkBoostLogWeighting.h
    ${VTK_SOURCE_DIR}/Infovis/vtkBoostRandomSparseArraySource.h
    ${VTK_SOURCE_DIR}/Infovis/vtkCosineSimilarity.h
    ${VTK_SOURCE_DIR}/Infovis/vtkDiagonalMatrixSource.h
    ${VTK_SOURCE_DIR}/Infovis/vtkExtractFactoredArray.h
    ${VTK_SOURCE_DIR}/Infovis/vtkMatricizeArray.h
    ${VTK_SOURCE_DIR}/Infovis/vtkMatrixColumnNorm.h
    ${VTK_SOURCE_DIR}/Infovis/vtkNormalizeMatrixVectors.h
    ${VTK_SOURCE_DIR}/Infovis/vtkTableToSparseArray.h
    ${VTK_SOURCE_DIR}/Infovis/vtkTransposeMatrix.h
  )

TARGET_LINK_LIBRARIES(
  ArrayPlugin
  vtkInfovis
  )

