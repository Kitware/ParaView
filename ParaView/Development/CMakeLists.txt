PROJECT(Development)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(Development_SRCS
   vtkAMRBox.h
   vtkUniformGrid.cxx

   vtkCompositeDataSet.cxx
   vtkCompositeDataIterator.cxx
   vtkCompositeDataVisitor.cxx 

   vtkMultiBlockDataSet.cxx
   vtkMultiBlockDataIterator.cxx
   vtkMultiBlockDataVisitor.cxx 

   vtkHierarchicalDataSet.cxx
   vtkHierarchicalDataIterator.cxx

   vtkHierarchicalBoxDataSet.cxx
   vtkHierarchicalBoxVisitor.cxx 

   vtkCompositeDataVisitorCommand.cxx
   vtkApplyFilterCommand.cxx
   vtkMultiBlockApplyFilterCommand.cxx
   vtkHierarchicalBoxApplyFilterCommand.cxx

   vtkHierarchicalBoxSource.cxx
   vtkHierarchicalBoxToHierarchicalBoxFilter.cxx
   vtkHierarchicalBoxToPolyDataFilter.cxx
   vtkHierarchicalBoxOutlineFilter.cxx
   vtkHierarchicalBoxCutter.cxx
   vtkHierarchicalBoxCellDataToPointData.cxx
   vtkHierarchicalBoxContour.cxx
   )

SET_SOURCE_FILES_PROPERTIES(
   vtkApplyFilterCommand.cxx
   vtkCompositeDataSet.cxx
   vtkCompositeDataIterator.cxx
   vtkCompositeDataVisitor.cxx 
   vtkCompositeDataVisitorCommand.cxx
   vtkHierarchicalDataSet.cxx
   vtkHierarchicalBoxToPolyDataFilter.cxx
   vtkHierarchicalBoxSource.cxx
   vtkHierarchicalBoxToHierarchicalBoxFilter.cxx
   ABSTRACT
   )

SET_SOURCE_FILES_PROPERTIES(
   vtkAMRBox.h
   WRAP_EXCLUDE
   )

IF(NOT PARAVIEW_NO_HDF5)
  SET(Development_SRCS
      ${Development_SRCS}
      vtkChomboReader.cxx
     )
ENDIF(NOT PARAVIEW_NO_HDF5)

VTK_WRAP_TCL2(vtkPVDevelopmentTCL DevelopmentTCL_SRCS ${Development_SRCS})
ADD_LIBRARY(vtkPVDevelopmentTCL ${DevelopmentTCL_SRCS} ${Development_SRCS})

TARGET_LINK_LIBRARIES(vtkPVDevelopmentTCL
  vtkHybridTCL
  )

IF(NOT PARAVIEW_NO_HDF5)
  TARGET_LINK_LIBRARIES(vtkPVDevelopmentTCL ${PARAVIEW_HDF5_LIBRARIES})
ENDIF(NOT PARAVIEW_NO_HDF5)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkPVDevelopmentTCL)
ENDIF(BUILD_SHARED_LIBS)
