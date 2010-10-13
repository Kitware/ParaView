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

#include "vtkAppendRectilinearGrid.h"
#include "vtkAttributeDataReductionFilter.h"
#include "vtkCameraManipulator.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkClientServerMoveData.h"
#include "vtkCompleteArrays.h"
#include "vtkCSVWriter.h"
#include "vtkExtractHistogram.h"
#include "vtkExtractScatterPlot.h"
#include "vtkHierarchicalFractal.h"
#include "vtkImageCompressor.h"
#include "vtkIntegrateAttributes.h"
#include "vtkIntegrateFlowThroughSurface.h"
#include "vtkInteractorStyleTransferFunctionEditor.h"
#include "vtkKdTreeGenerator.h"
#include "vtkKdTreeManager.h"
#include "vtkMPICompositeManager.h"
#include "vtkMPIMoveData.h"
#include "vtkMergeArrays.h"
#include "vtkMinMax.h"
#include "vtkMultiViewManager.h"
#include "vtkNetworkImageSource.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkPExtractHistogram.h"
#include "vtkPhastaReader.h"
#include "vtkPPhastaReader.h"
#include "vtkPointHandleRepresentationSphere.h"
#include "vtkPolyLineToRectilinearGridFilter.h"
#include "vtkPVAnimationScene.h"
#include "vtkPVArrowSource.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVConnectivityFilter.h"
#include "vtkPVDReader.h"
#include "vtkPVEnSightMasterServerReader.h"
#include "vtkPVEnSightMasterServerTranslator.h"
#include "vtkPVExtentTranslator.h"
#include "vtkPVExtractVOI.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVGlyphFilter.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVJoystickFlyIn.h"
#include "vtkPVJoystickFlyOut.h"
#include "vtkPVLinearExtrusionFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVLODVolume.h"
#include "vtkPVMain.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkPVServerArrayHelper.h"
#include "vtkPVServerArraySelection.h"
#include "vtkPVServerFileListing.h"
#include "vtkPVServerObject.h"
#include "vtkPVServerSelectTimeSet.h"
#include "vtkPVServerTimeSteps.h"
#include "vtkPVTextSource.h"
#include "vtkPVTrackballMoveActor.h"
#include "vtkPVTrackballPan.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkPVHardwareSelector.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkReductionFilter.h"
#include "vtkSpyPlotReader.h"
#include "vtkSpyPlotUniReader.h"
#include "vtkSquirtCompressor.h"
#include "vtkSurfaceVectors.h"
#include "vtkTimeToTextConvertor.h"
#include "vtkTransferFunctionEditorRepresentationShapes1D.h"
#include "vtkTransferFunctionEditorRepresentationShapes2D.h"
#include "vtkTransferFunctionEditorRepresentationSimple1D.h"
#include "vtkTransferFunctionEditorWidgetShapes1D.h"
#include "vtkTransferFunctionEditorWidgetShapes2D.h"
#include "vtkTransferFunctionEditorWidgetSimple1D.h"
#include "vtkTransferFunctionViewer.h"
#include "vtkUpdateSuppressorPipeline.h"
#include "vtkVRMLSource.h"
#include "vtkXMLCollectionReader.h"
#include "vtkXMLPVAnimationWriter.h"
#include "vtkXMLPVDWriter.h"

#ifdef VTK_USE_MPI
# include "vtkAllToNRedistributePolyData.h"
# include "vtkBalancedRedistributePolyData.h"
# include "vtkRedistributePolyData.h"
# ifdef PARAVIEW_USE_ICE_T
//#include "vtkExtractCTHPart2.h"
#  include "vtkIceTContext.h"
# endif
# include "vtkRedistributePolyData.h"
//#include "vtkStructuredCacheFilter.h"
# include "vtkWeightedRedistributePolyData.h"
#endif //VTK_USE_MPI

#ifdef PARAVIEW_ENABLE_PYTHON
# include "vtkPythonProgrammableFilter.h"
#endif //PARAVIEW_ENABLE_PYTHON

int main(int , char *[])
{
  vtkObject *c;
  c = vtkAppendRectilinearGrid::New(); c->Print(cout); c->Delete();
  c = vtkAttributeDataReductionFilter::New(); c->Print(cout); c->Delete();
  c = vtkCameraManipulator::New(); c->Print(cout); c->Delete();
  c = vtkCleanUnstructuredGrid::New(); c->Print(cout); c->Delete();
  c = vtkClientServerMoveData::New(); c->Print(cout); c->Delete();
  c = vtkCompleteArrays::New(); c->Print(cout); c->Delete();
  c = vtkCSVWriter::New(); c->Print(cout); c->Delete();
  c = vtkExtractHistogram::New(); c->Print(cout); c->Delete();
  c = vtkExtractScatterPlot::New(); c->Print(cout); c->Delete();
  c = vtkHierarchicalFractal::New(); c->Print(cout); c->Delete();
  c = vtkImageCompressor::New(); c->Print(cout); c->Delete();
  c = vtkIntegrateAttributes::New(); c->Print(cout); c->Delete();
  c = vtkIntegrateFlowThroughSurface::New(); c->Print(cout); c->Delete();
  c = vtkKdTreeGenerator::New(); c->Print(cout); c->Delete();
  c = vtkKdTreeManager::New(); c->Print(cout); c->Delete();
  c = vtkMergeArrays::New(); c->Print(cout); c->Delete();
  c = vtkMinMax::New(); c->Print(cout); c->Delete();
  c = vtkMPICompositeManager::New(); c->Print(cout); c->Delete();
  c = vtkMPIMoveData::New(); c->Print(cout); c->Delete();
  c = vtkMultiViewManager::New(); c->Print(cout); c->Delete();
  c = vtkNetworkImageSource::New(); c->Print(cout); c->Delete();
  c = vtkOrderedCompositeDistributor::New(); c->Print(cout); c->Delete();
  c = vtkPExtractHistogram::New(); c->Print(cout); c->Delete();
  c = vtkPhastaReader::New(); c->Print(cout); c->Delete();
  c = vtkPPhastaReader::New(); c->Print(cout); c->Delete();
  c = vtkPointHandleRepresentationSphere::New(); c->Print(cout); c->Delete();
  c = vtkPolyLineToRectilinearGridFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVAnimationScene::New(); c->Print(cout); c->Delete();
  c = vtkPVArrowSource::New(); c->Print(cout); c->Delete();
  c = vtkPVClipDataSet::New(); c->Print(cout); c->Delete();
  c = vtkPVConnectivityFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVDReader::New(); c->Print(cout); c->Delete();
  c = vtkPVEnSightMasterServerReader::New(); c->Print(cout); c->Delete();
  c = vtkPVEnSightMasterServerTranslator::New(); c->Print(cout); c->Delete();
  c = vtkPVExtentTranslator::New(); c->Print(cout); c->Delete();
  c = vtkPVExtractVOI::New(); c->Print(cout); c->Delete();
  c = vtkPVGenericRenderWindowInteractor::New(); c->Print(cout); c->Delete();
  c = vtkPVGeometryFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVGlyphFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVGeometryInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVInteractorStyle::New(); c->Print(cout); c->Delete();
  c = vtkPVJoystickFlyIn::New(); c->Print(cout); c->Delete();
  c = vtkPVJoystickFlyOut::New(); c->Print(cout); c->Delete();
  c = vtkPVLinearExtrusionFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVLODActor::New(); c->Print(cout); c->Delete();
  c = vtkPVLODVolume::New(); c->Print(cout); c->Delete();
  c = vtkPVMain::New(); c->Print(cout); c->Delete();
  c = vtkPVRenderViewProxy::New(); c->Print(cout); c->Delete();
  c = vtkPVServerArrayHelper::New(); c->Print(cout); c->Delete();
  c = vtkPVServerArraySelection::New(); c->Print(cout); c->Delete();
  c = vtkPVServerFileListing::New(); c->Print(cout); c->Delete();
  c = vtkPVServerObject::New(); c->Print(cout); c->Delete();
  c = vtkPVServerSelectTimeSet::New(); c->Print(cout); c->Delete();
  c = vtkPVServerTimeSteps::New(); c->Print(cout); c->Delete();
  c = vtkPVTextSource::New(); c->Print(cout); c->Delete();
  c = vtkPVTrackballMoveActor::New(); c->Print(cout); c->Delete();
  c = vtkPVTrackballPan::New(); c->Print(cout); c->Delete();
  c = vtkPVTrackballRoll::New(); c->Print(cout); c->Delete();
  c = vtkPVTrackballRotate::New(); c->Print(cout); c->Delete();
  c = vtkPVTrackballZoom::New(); c->Print(cout); c->Delete();
  c = vtkPVUpdateSuppressor::New(); c->Print(cout); c->Delete();
  c = vtkPVHardwareSelector::New(); c->Print(cout); c->Delete();
  c = vtkPVXMLElement::New(); c->Print(cout); c->Delete();
  c = vtkPVXMLParser::New(); c->Print(cout); c->Delete();
  c = vtkReductionFilter::New(); c->Print(cout); c->Delete();
  c = vtkSpyPlotReader::New(); c->Print(cout); c->Delete();
  c = vtkSpyPlotUniReader::New(); c->Print(cout); c->Delete();
  c = vtkSquirtCompressor::New(); c->Print(cout); c->Delete();
  c = vtkSurfaceVectors::New(); c->Print(cout); c->Delete();
  c = vtkTimeToTextConvertor::New(); c->Print(cout); c->Delete();
  c = vtkTransferFunctionEditorRepresentationShapes1D::New(); c->Print(cout); c->Delete();
  c = vtkTransferFunctionEditorRepresentationShapes2D::New(); c->Print(cout); c->Delete();
  c = vtkTransferFunctionEditorRepresentationSimple1D::New(); c->Print(cout); c->Delete();
  c = vtkTransferFunctionEditorWidgetShapes1D::New(); c->Print(cout); c->Delete();
  c = vtkTransferFunctionEditorWidgetShapes2D::New(); c->Print(cout); c->Delete();
  c = vtkTransferFunctionEditorWidgetSimple1D::New(); c->Print(cout); c->Delete();
  c = vtkTransferFunctionViewer::New(); c->Print(cout); c->Delete();
  c = vtkUpdateSuppressorPipeline::New(); c->Print(cout); c->Delete();
  c = vtkVRMLSource::New(); c->Print(cout); c->Delete();
  c = vtkXMLCollectionReader::New(); c->Print(cout); c->Delete();
  c = vtkXMLPVAnimationWriter::New(); c->Print(cout); c->Delete();
  c = vtkXMLPVDWriter::New(); c->Print(cout); c->Delete();

#ifdef VTK_USE_MPI
  c = vtkAllToNRedistributePolyData::New(); c->Print(cout); c->Delete();
  c = vtkBalancedRedistributePolyData::New(); c->Print(cout); c->Delete();
  c = vtkRedistributePolyData::New(); c->Print(cout); c->Delete();
# ifdef PARAVIEW_USE_ICE_T
  c = vtkDesktopDeliveryClient::New(); c->Print(cout); c->Delete();
  c = vtkDesktopDeliveryServer::New(); c->Print(cout); c->Delete();
//  c = vtkExtractCTHPart2::New(); c->Print(cout); c->Delete();
  c = vtkIceTContext::New(); c->Print(cout); c->Delete();
# endif
  c = vtkRedistributePolyData::New(); c->Print(cout); c->Delete();
//  c = vtkStructuredCacheFilter::New(); c->Print(cout); c->Delete();
  c = vtkWeightedRedistributePolyData::New(); c->Print(cout); c->Delete();
#endif //VTK_USE_MPI

#ifdef PARAVIEW_ENABLE_PYTHON
 c = vtkPythonProgrammableFilter::New(); c->Print(cout); c->Delete();
#endif //PARAVIEW_ENABLE_PYTHON

  return 0;
}
