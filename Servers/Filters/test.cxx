/*=========================================================================

  Program:   ParaView
  Module:    test.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCleanUnstructuredGrid.h"
#include "vtkColorByPart.h"
#include "vtkGroup.h"
#include "vtkHDF5RawImageReader.h"
#include "vtkMergeArrays.h"
#include "vtkPVArrowSource.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVConnectivityFilter.h"
#include "vtkPVContourFilter.h"
#include "vtkPVEnSightMasterServerReader.h"
#include "vtkPVEnSightMasterServerTranslator.h"
#include "vtkPVGlyphFilter.h"
#include "vtkPVImageContinuousDilate3D.h"
#include "vtkPVImageContinuousErode3D.h"
#include "vtkPVImageGradient.h"
#include "vtkPVImageGradientMagnitude.h"
#include "vtkPVImageMedian3D.h"
#include "vtkPVRibbonFilter.h"
#include "vtkPVThresholdFilter.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkPVWarpScalar.h"
#include "vtkPVWarpVector.h"
#include "vtkVRMLSource.h"
#include "vtkXMLPVDWriter.h"
#include "vtkDataSetSubdivisionAlgorithm.h"
#include "vtkStreamingTessellator.h"

#ifdef VTK_USE_PATENTED
# include "vtkPVKitwareContourFilter.h"
#endif

#ifdef VTK_USE_MPI
# include "vtkDistributedDataFilter.h"
# include "vtkExtractCells.h"
# include "vtkKdTree.h"
# include "vtkMergeCells.h"
# include "vtkPKdTree.h"
# include "vtkPlanesIntersection.h"
# include "vtkPointsProjectedHull.h"
#endif

int main()
{
  vtkObject *c;
  c = vtkDataSetSubdivisionAlgorithm::New(); c->Print(cout); c->Delete();
  c = vtkStreamingTessellator::New(); c->Print(cout); c->Delete();
  c = vtkCleanUnstructuredGrid::New(); c->Print(cout); c->Delete();
  c = vtkColorByPart::New(); c->Print(cout); c->Delete();
  c = vtkGroup::New(); c->Print(cout); c->Delete();
  c = vtkHDF5RawImageReader::New(); c->Print(cout); c->Delete();
  c = vtkMergeArrays::New(); c->Print(cout); c->Delete();
  c = vtkPVArrowSource::New(); c->Print(cout); c->Delete();
  c = vtkPVClipDataSet::New(); c->Print(cout); c->Delete();
  c = vtkPVConnectivityFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVContourFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVEnSightMasterServerReader::New(); c->Print(cout); c->Delete();
  c = vtkPVEnSightMasterServerTranslator::New(); c->Print(cout); c->Delete();
  c = vtkPVGlyphFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVImageContinuousDilate3D::New(); c->Print(cout); c->Delete();;
  c = vtkPVImageContinuousErode3D::New(); c->Print(cout); c->Delete();
  c = vtkPVImageGradient::New(); c->Print(cout); c->Delete();
  c = vtkPVImageGradientMagnitude::New(); c->Print(cout); c->Delete();
  c = vtkPVImageMedian3D::New(); c->Print(cout); c->Delete();
#ifdef VTK_USE_PATENTED
  c = vtkPVKitwareContourFilter::New(); c->Print(cout); c->Delete();
#endif
  c = vtkPVRibbonFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVThresholdFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVUpdateSuppressor::New(); c->Print(cout); c->Delete();
  c = vtkPVWarpScalar::New(); c->Print(cout); c->Delete();
  c = vtkPVWarpVector::New(); c->Print(cout); c->Delete();
  c = vtkVRMLSource::New(); c->Print(cout); c->Delete();
  c = vtkXMLPVDWriter::New(); c->Print(cout); c->Delete();
#ifdef VTK_USE_MPI
  c = vtkDistributedDataFilter::New(); c->Print(cout); c->Delete();
  c = vtkExtractCells::New(); c->Print(cout); c->Delete();
  c = vtkKdTree::New(); c->Print(cout); c->Delete();
  c = vtkMergeCells::New(); c->Print(cout); c->Delete();
  c = vtkPKdTree::New(); c->Print(cout); c->Delete();
  c = vtkPlanesIntersection::New(); c->Print(cout); c->Delete();
  c = vtkPointsProjectedHull::New(); c->Print(cout); c->Delete();
#endif
  return 0;
}
