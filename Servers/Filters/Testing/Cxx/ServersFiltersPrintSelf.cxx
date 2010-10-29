/*=========================================================================

  Program:   ParaView
  Module:    ServersFiltersPrintSelf.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConfig.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAMRDualClip.h"
#include "vtkAMRDualContour.h"
#include "vtkAMRDualGridHelper.h"
#include "vtkAppendRectilinearGrid.h"
#include "vtkAppendArcLength.h"
#include "vtkAttributeDataReductionFilter.h"
#include "vtkAttributeDataToTableFilter.h"
#include "vtkBlockDeliveryPreprocessor.h"
#include "vtkBSPCutsGenerator.h"
#include "vtkCameraInterpolator2.h"
#include "vtkCameraManipulator.h"
#include "vtkChartRepresentation.h"
#include "vtkCleanArrays.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkClientServerMoveData.h"
#include "vtkCompleteArrays.h"
#include "vtkCompositeAnimationPlayer.h"
#include "vtkCompositeDataToUnstructuredGridFilter.h"
#include "vtkCompositeRepresentation.h"
#include "vtkContextNamedOptions.h"
#include "vtkCSVExporter.h"
#include "vtkCSVWriter.h"
#include "vtkCubeAxesRepresentation.h"
#include "vtkDataLabelRepresentation.h"
#include "vtkDataSetToRectilinearGrid.h"
#include "vtkEnzoReader.h"
#include "vtkEquivalenceSet.h"
#include "vtkExodusFileSeriesReader.h"
#include "vtkExtractHistogram.h"
#include "vtkExtractScatterPlot.h"
#include "vtkFileSeriesReader.h"
#include "vtkFileSeriesWriter.h"
#include "vtkFlashContour.h"
#include "vtkFlashReader.h"
#include "vtkGlyph3DRepresentation.h"
#include "vtkHierarchicalFractal.h"
#include "vtkGeometryRepresentation.h"
#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkGridConnectivity.h"
#include "vtkImageSliceDataDeliveryFilter.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageSliceRepresentation.h"
#include "vtkImageVolumeRepresentation.h"
#include "vtkIntegrateAttributes.h"
#include "vtkIntegrateFlowThroughSurface.h"
#include "vtkInteractorStyleTransferFunctionEditor.h"
#include "vtkIntersectFragments.h"
#include "vtkIsoVolume.h"
#include "vtkKdTreeGenerator.h"
#include "vtkKdTreeManager.h"
#include "vtkMarkSelectedRows.h"
#include "vtkMaterialInterfaceFilter.h"
#include "vtkMergeArrays.h"
#include "vtkMergeCompositeDataSet.h"
#include "vtkMinMax.h"
#include "vtkMPICompositeManager.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiViewManager.h"
#include "vtkNetworkImageSource.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkOutlineRepresentation.h"
#include "vtkPConvertSelection.h"
#include "vtkPExtractHistogram.h"
#include "vtkParallelSerialWriter.h"
#include "vtkPEnSightGoldBinaryReader.h"
#include "vtkPEnSightGoldReader.h"
#include "vtkPGenericEnSightReader.h"
#include "vtkPhastaReader.h"
#include "vtkPlotEdges.h"
#include "vtkPointHandleRepresentationSphere.h"
#include "vtkPolyLineToRectilinearGridFilter.h"
#include "vtkPPhastaReader.h"
#include "vtkPSciVizContingencyStats.h"
#include "vtkPSciVizDescriptiveStats.h"
#include "vtkPSciVizMultiCorrelativeStats.h"
#include "vtkPSciVizPCAStats.h"
#include "vtkPSciVizKMeans.h"
#include "vtkPVAMRDualClip.h"
#include "vtkPVAnimationScene.h"
#include "vtkPVArrayCalculator.h"
#include "vtkPVArrowSource.h"
#include "vtkPVAxesActor.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVCacheKeeperPipeline.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVClipClosedSurface.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVConnectivityFilter.h"
#include "vtkPVContourFilter.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkPVDataRepresentationPipeline.h"
#include "vtkPVDefaultPass.h"
#include "vtkPVDReader.h"
#include "vtkPVEnSightMasterServerReader.h"
#include "vtkPVEnSightMasterServerReader2.h"
#include "vtkPVEnSightMasterServerTranslator.h"
#include "vtkPVExtentTranslator.h"
#include "vtkPVExtractSelection.h"
#include "vtkPVExtractVOI.h"
#include "vtkPVFrustumActor.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVGlyphFilter.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVJoystickFly.h"
#include "vtkPVJoystickFlyIn.h"
#include "vtkPVJoystickFlyOut.h"
#include "vtkPVLastSelectionInformation.h"
#include "vtkPVLinearExtrusionFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVLODVolume.h"
#include "vtkPVMain.h"
#include "vtkPVMergeTables.h"
#include "vtkPVNullSource.h"
#include "vtkPVParallelCoordinatesRepresentation.h"
#include "vtkPVRecoverGeometryWireframe.h"
#include "vtkPVRepresentedDataInformation.h"
#include "vtkPVScalarBarActor.h"
#include "vtkPVSelectionSource.h"
#include "vtkPVServerArrayHelper.h"
#include "vtkPVServerArraySelection.h"
#include "vtkPVServerFileListing.h"
#include "vtkPVServerObject.h"
#include "vtkPVServerSelectTimeSet.h"
#include "vtkPVServerTimeSteps.h"
#include "vtkPVStringArrayHelper.h"
#include "vtkPVTextSource.h"
#include "vtkPVTrackballMoveActor.h"
#include "vtkPVTrackballMultiRotate.h"
#include "vtkPVTrackballPan.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkPVHardwareSelector.h"
#include "vtkQuerySelectionSource.h"
#include "vtkRealtimeAnimationPlayer.h"
#include "vtkRectilinearGridConnectivity.h"
#include "vtkReductionFilter.h"
#include "vtkScatterPlotMapper.h"
#include "vtkScatterPlotPainter.h"
#include "vtkSelectionConverter.h"
#include "vtkSelectionDeliveryFilter.h"
#include "vtkSelectionRepresentation.h"
#include "vtkSequenceAnimationPlayer.h"
#include "vtkSortedTableStreamer.h"
#include "vtkSpreadSheetRepresentation.h"
#include "vtkSpyPlotReader.h"
#include "vtkSpyPlotUniReader.h"
#include "vtkSquirtCompressor.h"
#include "vtkZlibImageCompressor.h"
#include "vtkSurfaceVectors.h"
#include "vtkTextSourceRepresentation.h"
#include "vtkTilesHelper.h"
#include "vtkTableFFT.h"
#include "vtkTexturePainter.h"
#include "vtkTimestepsAnimationPlayer.h"
#include "vtkTimeToTextConvertor.h"
#include "vtkTrackballPan.h"
#include "vtkTransferFunctionEditorRepresentation1D.h"
#include "vtkTransferFunctionEditorRepresentation.h"
#include "vtkTransferFunctionEditorRepresentationShapes1D.h"
#include "vtkTransferFunctionEditorRepresentationShapes2D.h"
#include "vtkTransferFunctionEditorRepresentationSimple1D.h"
#include "vtkTransferFunctionEditorWidget1D.h"
#include "vtkTransferFunctionEditorWidget.h"
#include "vtkTransferFunctionEditorWidgetShapes1D.h"
#include "vtkTransferFunctionEditorWidgetShapes2D.h"
#include "vtkTransferFunctionEditorWidgetSimple1D.h"
#include "vtkTransferFunctionViewer.h"
#include "vtkUnstructuredDataDeliveryFilter.h"
#include "vtkUnstructuredGridVolumeRepresentation.h"
#include "vtkUpdateSuppressorPipeline.h"
#include "vtkVolumeRepresentationPreprocessor.h"
#include "vtkVRMLSource.h"
#include "vtkXMLCollectionReader.h"
#include "vtkXMLPVAnimationWriter.h"
#include "vtkXMLPVDWriter.h"
#include "vtkXYChartRepresentation.h"

#ifdef PARAVIEW_ENABLE_PYTHON
#include "vtkPythonCalculator.h"
#include "vtkPythonProgrammableFilter.h"
#endif

#ifdef VTK_USE_MPI
# include "vtkBalancedRedistributePolyData.h"
# include "vtkAllToNRedistributePolyData.h"
# include "vtkAllToNRedistributeCompositePolyData.h"
# include "vtkRedistributePolyData.h"
# include "vtkWeightedRedistributePolyData.h"
# ifdef PARAVIEW_USE_ICE_T
//vtkCaveRenderManager.h"
#  include "vtkIceTCompositePass.h"
#  include "vtkIceTContext.h"
#  include "vtkIceTSynchronizedRenderers.h"
# endif
#endif

#define PRINT_SELF(classname)\
  c = classname::New(); c->Print(cout); c->Delete();

int main(int , char *[])
{
  vtkObject *c;

  PRINT_SELF(vtk3DWidgetRepresentation);
  PRINT_SELF(vtkAMRDualClip);
  PRINT_SELF(vtkAMRDualContour);
  PRINT_SELF(vtkAMRDualGridHelper);
  PRINT_SELF(vtkAppendRectilinearGrid);
  PRINT_SELF(vtkAppendArcLength);
  PRINT_SELF(vtkAttributeDataReductionFilter);
  PRINT_SELF(vtkAttributeDataToTableFilter);
  PRINT_SELF(vtkBlockDeliveryPreprocessor);
  PRINT_SELF(vtkBSPCutsGenerator);
  PRINT_SELF(vtkCameraInterpolator2);
  PRINT_SELF(vtkCameraManipulator);
  PRINT_SELF(vtkChartRepresentation);
  PRINT_SELF(vtkCleanArrays);
  PRINT_SELF(vtkCleanUnstructuredGrid);
  PRINT_SELF(vtkClientServerMoveData);
  PRINT_SELF(vtkCompleteArrays);
  PRINT_SELF(vtkCompositeAnimationPlayer);
  PRINT_SELF(vtkCompositeDataToUnstructuredGridFilter);
  PRINT_SELF(vtkCompositeRepresentation);
  PRINT_SELF(vtkContextNamedOptions);
  PRINT_SELF(vtkCSVExporter);
  PRINT_SELF(vtkCSVWriter);
  PRINT_SELF(vtkCubeAxesRepresentation);
  PRINT_SELF(vtkDataLabelRepresentation);
  PRINT_SELF(vtkDataSetToRectilinearGrid);
  PRINT_SELF(vtkEnzoReader);
  PRINT_SELF(vtkEquivalenceSet);
  PRINT_SELF(vtkExodusFileSeriesReader);
  PRINT_SELF(vtkExtractHistogram);
  PRINT_SELF(vtkExtractScatterPlot);
  PRINT_SELF(vtkFileSeriesReader);
  PRINT_SELF(vtkFileSeriesWriter);
  PRINT_SELF(vtkFlashContour);
  PRINT_SELF(vtkFlashReader);
  PRINT_SELF(vtkGlyph3DRepresentation);
  PRINT_SELF(vtkHierarchicalFractal);
  PRINT_SELF(vtkGeometryRepresentation);
  PRINT_SELF(vtkGeometryRepresentationWithFaces);
  PRINT_SELF(vtkGridConnectivity);
  PRINT_SELF(vtkImageSliceDataDeliveryFilter);
  PRINT_SELF(vtkImageSliceMapper);
  PRINT_SELF(vtkImageSliceRepresentation);
  PRINT_SELF(vtkImageVolumeRepresentation);
  PRINT_SELF(vtkIntegrateAttributes);
  PRINT_SELF(vtkIntegrateFlowThroughSurface);
  PRINT_SELF(vtkInteractorStyleTransferFunctionEditor);
  PRINT_SELF(vtkIntersectFragments);
  PRINT_SELF(vtkIsoVolume);
  PRINT_SELF(vtkKdTreeGenerator);
  PRINT_SELF(vtkKdTreeManager);
  PRINT_SELF(vtkMarkSelectedRows);
  PRINT_SELF(vtkMaterialInterfaceFilter);
  PRINT_SELF(vtkMergeArrays);
  PRINT_SELF(vtkMergeCompositeDataSet);
  PRINT_SELF(vtkMinMax);
  PRINT_SELF(vtkMPICompositeManager);
  PRINT_SELF(vtkMPIMoveData);
  PRINT_SELF(vtkMultiViewManager);
  PRINT_SELF(vtkNetworkImageSource);
  PRINT_SELF(vtkOrderedCompositeDistributor);
  PRINT_SELF(vtkOutlineRepresentation);
  PRINT_SELF(vtkPConvertSelection);
  PRINT_SELF(vtkPExtractHistogram);
  PRINT_SELF(vtkParallelSerialWriter);
  PRINT_SELF(vtkPEnSightGoldBinaryReader);
  PRINT_SELF(vtkPEnSightGoldReader);
  PRINT_SELF(vtkPGenericEnSightReader);
  PRINT_SELF(vtkPhastaReader);
  PRINT_SELF(vtkPlotEdges);
  PRINT_SELF(vtkPointHandleRepresentationSphere);
  PRINT_SELF(vtkPolyLineToRectilinearGridFilter);
  PRINT_SELF(vtkPPhastaReader);
  PRINT_SELF(vtkPSciVizContingencyStats);
  PRINT_SELF(vtkPSciVizDescriptiveStats);
  PRINT_SELF(vtkPSciVizMultiCorrelativeStats);
  PRINT_SELF(vtkPSciVizPCAStats);
  PRINT_SELF(vtkPSciVizKMeans);
  PRINT_SELF(vtkPVAMRDualClip);
  PRINT_SELF(vtkPVAnimationScene);
  PRINT_SELF(vtkPVArrayCalculator);
  PRINT_SELF(vtkPVArrowSource);
  PRINT_SELF(vtkPVAxesActor);
  PRINT_SELF(vtkPVAxesWidget);
  PRINT_SELF(vtkPVCacheKeeper);
  PRINT_SELF(vtkPVCacheKeeperPipeline);
  PRINT_SELF(vtkPVCenterAxesActor);
  PRINT_SELF(vtkPVClipClosedSurface);
  PRINT_SELF(vtkPVClipDataSet);
  PRINT_SELF(vtkPVConnectivityFilter);
  PRINT_SELF(vtkPVContourFilter);
  PRINT_SELF(vtkPVCompositeRepresentation);
  PRINT_SELF(vtkPVDataRepresentationPipeline);
  PRINT_SELF(vtkPVDefaultPass);
  PRINT_SELF(vtkPVDReader);
  PRINT_SELF(vtkPVEnSightMasterServerReader);
  PRINT_SELF(vtkPVEnSightMasterServerReader2);
  PRINT_SELF(vtkPVEnSightMasterServerTranslator);
  PRINT_SELF(vtkPVExtentTranslator);
  PRINT_SELF(vtkPVExtractSelection);
  PRINT_SELF(vtkPVExtractVOI);
  PRINT_SELF(vtkPVFrustumActor);
  PRINT_SELF(vtkPVGenericRenderWindowInteractor);
  PRINT_SELF(vtkPVGeometryFilter);
  PRINT_SELF(vtkPVGeometryInformation);
  PRINT_SELF(vtkPVGlyphFilter);
  PRINT_SELF(vtkPVInteractorStyle);
  PRINT_SELF(vtkPVJoystickFly);
  PRINT_SELF(vtkPVJoystickFlyIn);
  PRINT_SELF(vtkPVJoystickFlyOut);
  PRINT_SELF(vtkPVLastSelectionInformation);
  PRINT_SELF(vtkPVLinearExtrusionFilter);
  PRINT_SELF(vtkPVLODActor);
  PRINT_SELF(vtkPVLODVolume);
  PRINT_SELF(vtkPVMain);
  PRINT_SELF(vtkPVMergeTables);
  PRINT_SELF(vtkPVNullSource);
  PRINT_SELF(vtkPVParallelCoordinatesRepresentation);
  PRINT_SELF(vtkPVRecoverGeometryWireframe);
  PRINT_SELF(vtkPVRepresentedDataInformation);
  PRINT_SELF(vtkPVScalarBarActor);
  PRINT_SELF(vtkPVSelectionSource);
  PRINT_SELF(vtkPVServerArrayHelper);
  PRINT_SELF(vtkPVServerArraySelection);
  PRINT_SELF(vtkPVServerFileListing);
  PRINT_SELF(vtkPVServerObject);
  PRINT_SELF(vtkPVServerSelectTimeSet);
  PRINT_SELF(vtkPVServerTimeSteps);
  PRINT_SELF(vtkPVStringArrayHelper);
  PRINT_SELF(vtkPVTextSource);
  PRINT_SELF(vtkPVTrackballMoveActor);
  PRINT_SELF(vtkPVTrackballMultiRotate);
  PRINT_SELF(vtkPVTrackballPan);
  PRINT_SELF(vtkPVTrackballRoll);
  PRINT_SELF(vtkPVTrackballRotate);
  PRINT_SELF(vtkPVTrackballZoom);
  PRINT_SELF(vtkPVUpdateSuppressor);
  PRINT_SELF(vtkPVHardwareSelector);
  PRINT_SELF(vtkQuerySelectionSource);
  PRINT_SELF(vtkRealtimeAnimationPlayer);
  PRINT_SELF(vtkRectilinearGridConnectivity);
  PRINT_SELF(vtkReductionFilter);
  PRINT_SELF(vtkScatterPlotMapper);
  PRINT_SELF(vtkScatterPlotPainter);
  PRINT_SELF(vtkSelectionConverter);
  PRINT_SELF(vtkSelectionDeliveryFilter);
  PRINT_SELF(vtkSelectionRepresentation);
  PRINT_SELF(vtkSequenceAnimationPlayer);
  PRINT_SELF(vtkSortedTableStreamer);
  PRINT_SELF(vtkSpreadSheetRepresentation);
  PRINT_SELF(vtkSpyPlotReader);
  PRINT_SELF(vtkSpyPlotUniReader);
  PRINT_SELF(vtkSquirtCompressor);
  PRINT_SELF(vtkZlibImageCompressor);
  PRINT_SELF(vtkSurfaceVectors);
  PRINT_SELF(vtkTextSourceRepresentation);
  PRINT_SELF(vtkTilesHelper);
  PRINT_SELF(vtkTableFFT);
  PRINT_SELF(vtkTexturePainter);
  PRINT_SELF(vtkTimestepsAnimationPlayer);
  PRINT_SELF(vtkTimeToTextConvertor);
  PRINT_SELF(vtkTrackballPan);
  PRINT_SELF(vtkTransferFunctionEditorRepresentation1D);
  PRINT_SELF(vtkTransferFunctionEditorRepresentation);
  PRINT_SELF(vtkTransferFunctionEditorRepresentationShapes1D);
  PRINT_SELF(vtkTransferFunctionEditorRepresentationShapes2D);
  PRINT_SELF(vtkTransferFunctionEditorRepresentationSimple1D);
  PRINT_SELF(vtkTransferFunctionEditorWidget1D);
  PRINT_SELF(vtkTransferFunctionEditorWidget);
  PRINT_SELF(vtkTransferFunctionEditorWidgetShapes1D);
  PRINT_SELF(vtkTransferFunctionEditorWidgetShapes2D);
  PRINT_SELF(vtkTransferFunctionEditorWidgetSimple1D);
  PRINT_SELF(vtkTransferFunctionViewer);
  PRINT_SELF(vtkUnstructuredDataDeliveryFilter);
  PRINT_SELF(vtkUnstructuredGridVolumeRepresentation);
  PRINT_SELF(vtkUpdateSuppressorPipeline);
  PRINT_SELF(vtkVolumeRepresentationPreprocessor);
  PRINT_SELF(vtkVRMLSource);
  PRINT_SELF(vtkXMLCollectionReader);
  PRINT_SELF(vtkXMLPVAnimationWriter);
  PRINT_SELF(vtkXMLPVDWriter);
  PRINT_SELF(vtkXYChartRepresentation);

#ifdef PARAVIEW_ENABLE_PYTHON
  PRINT_SELF(vtkPythonCalculator);
  PRINT_SELF(vtkPythonProgrammableFilter);
#endif

#ifdef VTK_USE_MPI
  PRINT_SELF(vtkBalancedRedistributePolyData);
  PRINT_SELF(vtkAllToNRedistributePolyData);
  PRINT_SELF(vtkAllToNRedistributeCompositePolyData);
  PRINT_SELF(vtkRedistributePolyData);
  PRINT_SELF(vtkWeightedRedistributePolyData);
# ifdef PARAVIEW_USE_ICE_T
  //vtkCaveRenderManager);
  PRINT_SELF(vtkIceTCompositePass);
  PRINT_SELF(vtkIceTContext);
  PRINT_SELF(vtkIceTSynchronizedRenderers);
# endif
#endif

  return 0;
}
