/*=========================================================================

  Program:   ParaView
  Module:    ParaViewCoreVTKExtensionsPrintSelf.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConfig.h"

#include "vtkAMRDualClip.h"
#include "vtkAMRDualContour.h"
#include "vtkAMRDualGridHelper.h"
#include "vtkAppendArcLength.h"
#include "vtkAppendRectilinearGrid.h"
#include "vtkAttributeDataReductionFilter.h"
#include "vtkAttributeDataToTableFilter.h"
#include "vtkBSPCutsGenerator.h"
#include "vtkBlockDeliveryPreprocessor.h"
#include "vtkCSVExporter.h"
#include "vtkCSVWriter.h"
#include "vtkCacheSizeKeeper.h"
#include "vtkCameraInterpolator2.h"
#include "vtkCameraManipulator.h"
#include "vtkCameraManipulatorGUIHelper.h"
#include "vtkCellIntegrator.h"
#include "vtkCleanArrays.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkCompositeDataToUnstructuredGridFilter.h"
#include "vtkDataSetToRectilinearGrid.h"
//#include "vtkEnzoReader.h"
#include "vtkEquivalenceSet.h"
#include "vtkExodusFileSeriesReader.h"
#include "vtkExtractHistogram.h"
#include "vtkExtractScatterPlot.h"
#include "vtkFileSequenceParser.h"
#include "vtkFileSeriesReader.h"
#include "vtkFileSeriesWriter.h"
#include "vtkFlashContour.h"
//#include "vtkFlashReader.h"
#include "vtkGridConnectivity.h"
#include "vtkHierarchicalFractal.h"
#include "vtkImageCompressor.h"
#include "vtkIntegrateAttributes.h"
#include "vtkIntegrateFlowThroughSurface.h"
#include "vtkInteractorStyleTransferFunctionEditor.h"
#include "vtkIntersectFragments.h"
#include "vtkIsoVolume.h"
#include "vtkKdTreeGenerator.h"
#include "vtkKdTreeManager.h"
#include "vtkMarkSelectedRows.h"
#include "vtkMaterialInterfaceCommBuffer.h"
#include "vtkMaterialInterfaceFilter.h"
#include "vtkMaterialInterfaceIdList.h"
#include "vtkMaterialInterfacePieceLoading.h"
#include "vtkMaterialInterfacePieceTransaction.h"
#include "vtkMaterialInterfacePieceTransactionMatrix.h"
#include "vtkMaterialInterfaceProcessLoading.h"
#include "vtkMaterialInterfaceProcessRing.h"
#include "vtkMaterialInterfaceToProcMap.h"
#include "vtkMergeArrays.h"
#include "vtkMergeCompositeDataSet.h"
#include "vtkMinMax.h"
#include "vtkMultiProcessControllerHelper.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkPConvertSelection.h"
#include "vtkPEnSightGoldBinaryReader.h"
#include "vtkPEnSightGoldReader.h"
#include "vtkPEnSightReader.h"
#include "vtkPExtractHistogram.h"
#include "vtkPGenericEnSightReader.h"
#include "vtkPPhastaReader.h"
#include "vtkPSciVizContingencyStats.h"
#include "vtkPSciVizDescriptiveStats.h"
#include "vtkPSciVizKMeans.h"
#include "vtkPSciVizMultiCorrelativeStats.h"
#include "vtkPSciVizPCAStats.h"
#include "vtkPVAMRDualClip.h"
#include "vtkPVArrayCalculator.h"
#include "vtkPVAxesActor.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVBox.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVClipClosedSurface.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVCompositeDataPipeline.h"
#include "vtkPVConnectivityFilter.h"
#include "vtkPVContourFilter.h"
#include "vtkPVCueManipulator.h"
#include "vtkPVDReader.h"
#include "vtkPVDefaultPass.h"
#include "vtkPVEnSightMasterServerReader.h"
#include "vtkPVEnSightMasterServerReader2.h"
#include "vtkPVEnSightMasterServerTranslator.h"
#include "vtkPVExponentialKeyFrame.h"
#include "vtkPVExtractVOI.h"
#include "vtkPVFrustumActor.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVJoystickFly.h"
#include "vtkPVJoystickFlyIn.h"
#include "vtkPVJoystickFlyOut.h"
#include "vtkPVLODActor.h"
#include "vtkPVLODVolume.h"
#include "vtkPVLinearExtrusionFilter.h"
#include "vtkPVMergeTables.h"
#include "vtkPVNullSource.h"
#include "vtkPVPlane.h"
#include "vtkPVPostFilter.h"
#include "vtkPVPostFilterExecutive.h"
#include "vtkPVRecoverGeometryWireframe.h"
#include "vtkPVScalarBarActor.h"
#include "vtkPVSelectionSource.h"
#include "vtkPVTextSource.h"
#include "vtkPVTrackballMoveActor.h"
#include "vtkPVTrackballMultiRotate.h"
#include "vtkPVTrackballPan.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkPVTransform.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkParallelSerialWriter.h"
#include "vtkPhastaReader.h"
#include "vtkPlotEdges.h"
#include "vtkPointHandleRepresentationSphere.h"
#include "vtkPolyLineToRectilinearGridFilter.h"
#include "vtkQuerySelectionSource.h"
#include "vtkRectilinearGridConnectivity.h"
#include "vtkReductionFilter.h"
#include "vtkSciVizStatistics.h"
#include "vtkSelectionConverter.h"
#include "vtkSelectionSerializer.h"
#include "vtkSortedTableStreamer.h"
#include "vtkSpyPlotBlock.h"
#include "vtkSpyPlotBlockIterator.h"
#include "vtkSpyPlotHistoryReader.h"
#include "vtkSpyPlotIStream.h"
#include "vtkSpyPlotReader.h"
#include "vtkSpyPlotReaderMap.h"
#include "vtkSpyPlotUniReader.h"
#include "vtkSquirtCompressor.h"
#include "vtkSurfaceVectors.h"
#include "vtkTilesHelper.h"
#include "vtkTimeToTextConvertor.h"
#include "vtkTrackballPan.h"
#include "vtkTransferFunctionEditorRepresentation.h"
#include "vtkTransferFunctionEditorRepresentation1D.h"
#include "vtkTransferFunctionEditorRepresentationShapes1D.h"
#include "vtkTransferFunctionEditorRepresentationShapes2D.h"
#include "vtkTransferFunctionEditorRepresentationSimple1D.h"
#include "vtkTransferFunctionEditorWidget.h"
#include "vtkTransferFunctionEditorWidget1D.h"
#include "vtkTransferFunctionEditorWidgetShapes1D.h"
#include "vtkTransferFunctionEditorWidgetShapes2D.h"
#include "vtkTransferFunctionEditorWidgetSimple1D.h"
#include "vtkTransferFunctionViewer.h"
#include "vtkUndoElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStack.h"
#include "vtkUpdateSuppressorPipeline.h"
#include "vtkVRMLSource.h"
#include "vtkVolumeRepresentationPreprocessor.h"
#include "vtkXMLCollectionReader.h"
#include "vtkXMLPVDWriter.h"
#include "vtkZlibImageCompressor.h"

#ifdef PARAVIEW_USE_MPI
#include "vtkAllToNRedistributeCompositePolyData.h"
#include "vtkAllToNRedistributePolyData.h"
#include "vtkBalancedRedistributePolyData.h"
#include "vtkMPICompositeManager.h"
#include "vtkRedistributePolyData.h"
#include "vtkWeightedRedistributePolyData.h"
#ifdef PARAVIEW_USE_ICE_T
#include "vtkIceTCompositePass.h"
#include "vtkIceTContext.h"
#endif
#endif

#define PRINT_SELF(classname)                                                                      \
  c = classname::New();                                                                            \
  c->Print(cout);                                                                                  \
  c->Delete();

int ParaViewCoreVTKExtensionsPrintSelf(int, char* [])
{
  vtkObject* c;

  PRINT_SELF(vtkAMRDualClip);
  PRINT_SELF(vtkAMRDualContour);
  PRINT_SELF(vtkAMRDualGridHelper);
  PRINT_SELF(vtkAppendArcLength);
  PRINT_SELF(vtkAppendRectilinearGrid);
  PRINT_SELF(vtkAttributeDataReductionFilter);
  PRINT_SELF(vtkAttributeDataToTableFilter);
  PRINT_SELF(vtkBlockDeliveryPreprocessor);
  PRINT_SELF(vtkBSPCutsGenerator);
  PRINT_SELF(vtkCameraInterpolator2);
  PRINT_SELF(vtkCameraManipulator);
  PRINT_SELF(vtkCameraManipulatorGUIHelper);
  PRINT_SELF(vtkCellIntegrator);
  PRINT_SELF(vtkCleanArrays);
  PRINT_SELF(vtkCleanUnstructuredGrid);
  PRINT_SELF(vtkCompositeDataToUnstructuredGridFilter);
  PRINT_SELF(vtkCSVExporter);
  PRINT_SELF(vtkCSVWriter);
  PRINT_SELF(vtkDataSetToRectilinearGrid);
  // PRINT_SELF(vtkEnzoReader);
  PRINT_SELF(vtkEquivalenceSet);
  PRINT_SELF(vtkExodusFileSeriesReader);
  PRINT_SELF(vtkExtractHistogram);
  PRINT_SELF(vtkExtractScatterPlot);
  PRINT_SELF(vtkFileSequenceParser);
  PRINT_SELF(vtkFileSeriesReader);
  PRINT_SELF(vtkFileSeriesWriter);
  PRINT_SELF(vtkFlashContour);
  // PRINT_SELF(vtkFlashReader);
  PRINT_SELF(vtkGridConnectivity);
  PRINT_SELF(vtkHierarchicalFractal);
  PRINT_SELF(vtkImageCompressor);
  PRINT_SELF(vtkIntegrateAttributes);
  PRINT_SELF(vtkIntegrateFlowThroughSurface);
  PRINT_SELF(vtkInteractorStyleTransferFunctionEditor);
  PRINT_SELF(vtkIntersectFragments);
  PRINT_SELF(vtkIsoVolume);
  PRINT_SELF(vtkKdTreeGenerator);
  PRINT_SELF(vtkKdTreeManager);
  PRINT_SELF(vtkMarkSelectedRows);
  // PRINT_SELF(vtkMaterialInterfaceCommBuffer);
  PRINT_SELF(vtkMaterialInterfaceFilter);
  // PRINT_SELF(vtkMaterialInterfaceIdList);
  // PRINT_SELF(vtkMaterialInterfacePieceLoading);
  // PRINT_SELF(vtkMaterialInterfacePieceTransaction);
  // PRINT_SELF(vtkMaterialInterfacePieceTransactionMatrix);
  // PRINT_SELF(vtkMaterialInterfaceProcessLoading);
  // PRINT_SELF(vtkMaterialInterfaceProcessRing);
  // PRINT_SELF(vtkMaterialInterfaceToProcMap);
  PRINT_SELF(vtkMergeArrays);
  PRINT_SELF(vtkMergeCompositeDataSet);
  PRINT_SELF(vtkMinMax);
  PRINT_SELF(vtkMultiProcessControllerHelper);
  PRINT_SELF(vtkOrderedCompositeDistributor);
  PRINT_SELF(vtkParallelSerialWriter);
  PRINT_SELF(vtkPConvertSelection);
  PRINT_SELF(vtkPEnSightGoldBinaryReader);
  PRINT_SELF(vtkPEnSightGoldReader);
  PRINT_SELF(vtkPEnSightReader);
  PRINT_SELF(vtkPExtractHistogram);
  PRINT_SELF(vtkPGenericEnSightReader);
  PRINT_SELF(vtkPhastaReader);
  PRINT_SELF(vtkPlotEdges);
  PRINT_SELF(vtkPointHandleRepresentationSphere);
  PRINT_SELF(vtkPolyLineToRectilinearGridFilter);
  PRINT_SELF(vtkPPhastaReader);
  PRINT_SELF(vtkPSciVizContingencyStats);
  PRINT_SELF(vtkPSciVizDescriptiveStats);
  PRINT_SELF(vtkPSciVizKMeans);
  PRINT_SELF(vtkPSciVizMultiCorrelativeStats);
  PRINT_SELF(vtkPSciVizPCAStats);
  PRINT_SELF(vtkPVAMRDualClip);
  PRINT_SELF(vtkPVArrayCalculator);
  PRINT_SELF(vtkPVAxesActor);
  PRINT_SELF(vtkPVAxesWidget);
  PRINT_SELF(vtkPVBox);
  PRINT_SELF(vtkPVCenterAxesActor);
  PRINT_SELF(vtkPVClipClosedSurface);
  PRINT_SELF(vtkPVClipDataSet);
  PRINT_SELF(vtkPVCompositeDataPipeline);
  PRINT_SELF(vtkPVConnectivityFilter);
  PRINT_SELF(vtkPVContourFilter);
  PRINT_SELF(vtkPVCueManipulator);
  PRINT_SELF(vtkPVDefaultPass);
  PRINT_SELF(vtkPVDReader);
  PRINT_SELF(vtkPVEnSightMasterServerReader);
  PRINT_SELF(vtkPVEnSightMasterServerReader2);
  PRINT_SELF(vtkPVEnSightMasterServerTranslator);
  PRINT_SELF(vtkPVExponentialKeyFrame);
  PRINT_SELF(vtkPVExtractVOI);
  PRINT_SELF(vtkPVFrustumActor);
  PRINT_SELF(vtkPVGeometryFilter);
  PRINT_SELF(vtkPVInteractorStyle);
  PRINT_SELF(vtkPVJoystickFly);
  PRINT_SELF(vtkPVJoystickFlyIn);
  PRINT_SELF(vtkPVJoystickFlyOut);
  PRINT_SELF(vtkPVLinearExtrusionFilter);
  PRINT_SELF(vtkPVLODActor);
  PRINT_SELF(vtkPVLODVolume);
  PRINT_SELF(vtkPVMergeTables);
  PRINT_SELF(vtkPVNullSource);
  PRINT_SELF(vtkPVPlane);
  PRINT_SELF(vtkPVPostFilter);
  PRINT_SELF(vtkPVPostFilterExecutive);
  PRINT_SELF(vtkPVRecoverGeometryWireframe);
  PRINT_SELF(vtkPVScalarBarActor);
  PRINT_SELF(vtkPVSelectionSource);
  PRINT_SELF(vtkPVTextSource);
  PRINT_SELF(vtkPVTrackballMoveActor);
  PRINT_SELF(vtkPVTrackballMultiRotate);
  PRINT_SELF(vtkPVTrackballPan);
  PRINT_SELF(vtkPVTrackballRoll);
  PRINT_SELF(vtkPVTrackballRotate);
  PRINT_SELF(vtkPVTrackballZoom);
  PRINT_SELF(vtkPVTransform);
  PRINT_SELF(vtkPVTrivialProducer);
  PRINT_SELF(vtkPVUpdateSuppressor);
  PRINT_SELF(vtkQuerySelectionSource);
  PRINT_SELF(vtkRectilinearGridConnectivity);
  PRINT_SELF(vtkReductionFilter);
  PRINT_SELF(vtkSciVizStatistics);
  PRINT_SELF(vtkSelectionConverter);
  PRINT_SELF(vtkSelectionSerializer);
  PRINT_SELF(vtkSortedTableStreamer);
  // PRINT_SELF(vtkSpyPlotBlock);
  // PRINT_SELF(vtkSpyPlotBlockIterator);
  PRINT_SELF(vtkSpyPlotHistoryReader);
  // PRINT_SELF(vtkSpyPlotIStream);
  PRINT_SELF(vtkSpyPlotReader);
  // PRINT_SELF(vtkSpyPlotReaderMap);
  PRINT_SELF(vtkSpyPlotUniReader);
  PRINT_SELF(vtkSquirtCompressor);
  PRINT_SELF(vtkSurfaceVectors);
  // PRINT_SELF(vtkTilesHelper);
  PRINT_SELF(vtkTimeToTextConvertor);
  PRINT_SELF(vtkTrackballPan);
  PRINT_SELF(vtkTransferFunctionEditorRepresentation);
  PRINT_SELF(vtkTransferFunctionEditorRepresentation1D);
  PRINT_SELF(vtkTransferFunctionEditorRepresentationShapes1D);
  PRINT_SELF(vtkTransferFunctionEditorRepresentationShapes2D);
  PRINT_SELF(vtkTransferFunctionEditorRepresentationSimple1D);
  PRINT_SELF(vtkTransferFunctionEditorWidget);
  PRINT_SELF(vtkTransferFunctionEditorWidget1D);
  PRINT_SELF(vtkTransferFunctionEditorWidgetShapes1D);
  PRINT_SELF(vtkTransferFunctionEditorWidgetShapes2D);
  PRINT_SELF(vtkTransferFunctionEditorWidgetSimple1D);
  PRINT_SELF(vtkTransferFunctionViewer);
  PRINT_SELF(vtkUndoElement);
  PRINT_SELF(vtkUndoSet);
  PRINT_SELF(vtkUndoStack);
  PRINT_SELF(vtkUpdateSuppressorPipeline);
  PRINT_SELF(vtkVolumeRepresentationPreprocessor);
  PRINT_SELF(vtkVRMLSource);
  PRINT_SELF(vtkXMLCollectionReader);
  PRINT_SELF(vtkXMLPVDWriter);
  PRINT_SELF(vtkZlibImageCompressor);
#ifdef PARAVIEW_USE_MPI
  PRINT_SELF(vtkAllToNRedistributeCompositePolyData);
  PRINT_SELF(vtkAllToNRedistributePolyData);
  PRINT_SELF(vtkBalancedRedistributePolyData);
  PRINT_SELF(vtkRedistributePolyData);
  PRINT_SELF(vtkWeightedRedistributePolyData);
  PRINT_SELF(vtkMPICompositeManager);
#ifdef PARAVIEW_USE_ICE_T
  PRINT_SELF(vtkIceTCompositePass);
  PRINT_SELF(vtkIceTContext);
#endif
#endif

  return 0;
}
