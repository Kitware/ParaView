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

#include "vtkCaveRenderManager.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkClientCompositeManager.h"
#include "vtkColorByPart.h"
#include "vtkCompleteArrays.h"
#include "vtkCTHData.h"
#include "vtkCTHDataToPolyDataFilter.h"
#include "vtkCTHExtractAMRPart.h"
#include "vtkCTHFractal.h"
#include "vtkCTHOutlineFilter.h"
#include "vtkCTHSource.h"
#include "vtkDataSetSubdivisionAlgorithm.h"
#include "vtkGroup.h"
#include "vtkHDF5RawImageReader.h"
#include "vtkMazeSource.h"
#include "vtkMergeArrays.h"
#include "vtkMPIDuplicatePolyData.h"
#include "vtkMPIDuplicateUnstructuredGrid.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiDisplayManager.h"
#include "vtkPickFilter.h"
#include "vtkPickPointWidget.h"
#include "vtkPVArrowSource.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVCompositeBuffer.h"
#include "vtkPVCompositeUtilities.h"
#include "vtkPVConnectivityFilter.h"
#include "vtkPVContourFilter.h"
#include "vtkPVDReader.h"
#include "vtkPVEnSightMasterServerReader.h"
#include "vtkPVEnSightMasterServerTranslator.h"
#include "vtkPVExtentTranslator.h"
#include "vtkPVExtractVOI.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVGlyphFilter.h"
#include "vtkPVImageContinuousDilate3D.h"
#include "vtkPVImageContinuousErode3D.h"
#include "vtkPVImageGradient.h"
#include "vtkPVImageGradientMagnitude.h"
#include "vtkPVImageMedian3D.h"
#include "vtkPVLinearExtrusionFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVLODPartDisplayInformation.h"
//#include "vtkPVRenderModuleProxy.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkPVRibbonFilter.h"
#include "vtkPVServerArraySelection.h"
#include "vtkPVServerFileListing.h"
#include "vtkPVServerObject.h"
#include "vtkPVServerSelectTimeSet.h"
#include "vtkPVServerXDMFParameters.h"
#include "vtkPVSummaryHelper.h"
#include "vtkPVThresholdFilter.h"
#include "vtkPVTreeComposite.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkPVWarpScalar.h"
#include "vtkPVWarpVector.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSelectInputs.h"
#include "vtkStreamingTessellator.h"
#include "vtkSubdivisionAlgorithm.h"
#include "vtkTempTessellatorFilter.h"
#include "vtkTiledDisplaySchedule.h"
#include "vtkVRMLSource.h"
#include "vtkXMLCollectionReader.h"
#include "vtkXMLPVAnimationWriter.h"
#include "vtkXMLPVDWriter.h"

#ifdef VTK_USE_MPI
# include "vtkAllToNRedistributePolyData.h"
# include "vtkBalancedRedistributePolyData.h"
# ifdef PARAVIEW_USE_ICE_T
#  include "vtkDesktopDeliveryClient.h"
#  include "vtkDesktopDeliveryServer.h"
//#include "vtkExtractCTHPart2.h"
#  include "vtkIceTClientCompositeManager.h"
#  include "vtkIceTFactory.h"
#  include "vtkIceTRenderer.h"
#  include "vtkIceTRenderManager.h"
# endif
# include "vtkPVDuplicatePolyData.h"
# include "vtkRedistributePolyData.h"
//#include "vtkStructuredCacheFilter.h"
# include "vtkWeightedRedistributePolyData.h"
#endif //VTK_USE_MPI

#ifdef VTK_USE_PATENTED
# include "vtkPVKitwareContourFilter.h"
#endif //VTK_USE_PATENTED

int main(int , char *[])
{
  vtkObject *c;
  c = vtkCaveRenderManager::New(); c->Print(cout); c->Delete();
  c = vtkCleanUnstructuredGrid::New(); c->Print(cout); c->Delete();
  c = vtkClientCompositeManager::New(); c->Print(cout); c->Delete();
  c = vtkColorByPart::New(); c->Print(cout); c->Delete();
  c = vtkCompleteArrays::New(); c->Print(cout); c->Delete();
  c = vtkCTHData::New(); c->Print(cout); c->Delete();
  c = vtkCTHDataToPolyDataFilter::New(); c->Print(cout); c->Delete();
  c = vtkCTHExtractAMRPart::New(); c->Print(cout); c->Delete();
  c = vtkCTHFractal::New(); c->Print(cout); c->Delete();
  c = vtkCTHOutlineFilter::New(); c->Print(cout); c->Delete();
  c = vtkCTHSource::New(); c->Print(cout); c->Delete();
  c = vtkDataSetSubdivisionAlgorithm::New(); c->Print(cout); c->Delete();
  c = vtkGroup::New(); c->Print(cout); c->Delete();
  c = vtkHDF5RawImageReader::New(); c->Print(cout); c->Delete();
  c = vtkMazeSource::New(); c->Print(cout); c->Delete();
  c = vtkMergeArrays::New(); c->Print(cout); c->Delete();
  c = vtkMPIDuplicatePolyData::New(); c->Print(cout); c->Delete();
  c = vtkMPIDuplicateUnstructuredGrid::New(); c->Print(cout); c->Delete();
  c = vtkMPIMoveData::New(); c->Print(cout); c->Delete();
  c = vtkMultiDisplayManager::New(); c->Print(cout); c->Delete();
  c = vtkPickFilter::New(); c->Print(cout); c->Delete();
  c = vtkPickPointWidget::New(); c->Print(cout); c->Delete();
  c = vtkPVArrowSource::New(); c->Print(cout); c->Delete();
  c = vtkPVClipDataSet::New(); c->Print(cout); c->Delete();
  c = vtkPVCompositeBuffer::New(); c->Print(cout); c->Delete();
  c = vtkPVCompositeUtilities::New(); c->Print(cout); c->Delete();
  c = vtkPVConnectivityFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVContourFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVDReader::New(); c->Print(cout); c->Delete();
  c = vtkPVEnSightMasterServerReader::New(); c->Print(cout); c->Delete();
  c = vtkPVEnSightMasterServerTranslator::New(); c->Print(cout); c->Delete();
  c = vtkPVExtentTranslator::New(); c->Print(cout); c->Delete();
  c = vtkPVExtractVOI::New(); c->Print(cout); c->Delete();
  c = vtkPVGenericRenderWindowInteractor::New(); c->Print(cout); c->Delete();
  c = vtkPVGeometryFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVGlyphFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVImageContinuousDilate3D::New(); c->Print(cout); c->Delete();
  c = vtkPVImageContinuousErode3D::New(); c->Print(cout); c->Delete();
  c = vtkPVImageGradient::New(); c->Print(cout); c->Delete();
  c = vtkPVImageGradientMagnitude::New(); c->Print(cout); c->Delete();
  c = vtkPVImageMedian3D::New(); c->Print(cout); c->Delete();
  c = vtkPVLinearExtrusionFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVLODActor::New(); c->Print(cout); c->Delete();
  c = vtkPVLODPartDisplayInformation::New(); c->Print(cout); c->Delete();
//  c = vtkPVRenderModuleProxy::New(); c->Print(cout); c->Delete();
  c = vtkPVRenderViewProxy::New(); c->Print(cout); c->Delete();
  c = vtkPVRibbonFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVServerArraySelection::New(); c->Print(cout); c->Delete();
  c = vtkPVServerFileListing::New(); c->Print(cout); c->Delete();
  c = vtkPVServerObject::New(); c->Print(cout); c->Delete();
  c = vtkPVServerSelectTimeSet::New(); c->Print(cout); c->Delete();
  c = vtkPVServerXDMFParameters::New(); c->Print(cout); c->Delete();
  c = vtkPVSummaryHelper::New(); c->Print(cout); c->Delete();
  c = vtkPVThresholdFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVTreeComposite::New(); c->Print(cout); c->Delete();
  c = vtkPVUpdateSuppressor::New(); c->Print(cout); c->Delete();
  c = vtkPVWarpScalar::New(); c->Print(cout); c->Delete();
  c = vtkPVWarpVector::New(); c->Print(cout); c->Delete();
  c = vtkPVXMLElement::New(); c->Print(cout); c->Delete();
  c = vtkPVXMLParser::New(); c->Print(cout); c->Delete();
  c = vtkSelectInputs::New(); c->Print(cout); c->Delete();
  c = vtkStreamingTessellator::New(); c->Print(cout); c->Delete();
  c = vtkSubdivisionAlgorithm::New(); c->Print(cout); c->Delete();
  c = vtkTempTessellatorFilter::New(); c->Print(cout); c->Delete();
  c = vtkTiledDisplaySchedule::New(); c->Print(cout); c->Delete();
  c = vtkVRMLSource::New(); c->Print(cout); c->Delete();
  c = vtkXMLCollectionReader::New(); c->Print(cout); c->Delete();
  c = vtkXMLPVAnimationWriter::New(); c->Print(cout); c->Delete();
  c = vtkXMLPVDWriter::New(); c->Print(cout); c->Delete();

#ifdef VTK_USE_MPI
  c = vtkAllToNRedistributePolyData::New(); c->Print(cout); c->Delete();
  c = vtkBalancedRedistributePolyData::New(); c->Print(cout); c->Delete();
# ifdef PARAVIEW_USE_ICE_T
  c = vtkDesktopDeliveryClient::New(); c->Print(cout); c->Delete();
  c = vtkDesktopDeliveryServer::New(); c->Print(cout); c->Delete();
//  c = vtkExtractCTHPart2::New(); c->Print(cout); c->Delete();
  c = vtkIceTClientCompositeManager::New(); c->Print(cout); c->Delete();
  c = vtkIceTFactory::New(); c->Print(cout); c->Delete();
  c = vtkIceTRenderer::New(); c->Print(cout); c->Delete();
  c = vtkIceTRenderManager::New(); c->Print(cout); c->Delete();
# endif
  c = vtkPVDuplicatePolyData::New(); c->Print(cout); c->Delete();
  c = vtkRedistributePolyData::New(); c->Print(cout); c->Delete();
//  c = vtkStructuredCacheFilter::New(); c->Print(cout); c->Delete();
  c = vtkWeightedRedistributePolyData::New(); c->Print(cout); c->Delete();
#endif //VTK_USE_MPI

#ifdef VTK_USE_PATENTED
  c = vtkPVKitwareContourFilter::New(); c->Print(cout); c->Delete(); 
#endif //VTK_USE_PATENTED

  return 0;
}
