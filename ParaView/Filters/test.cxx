#include "vtkCleanUnstructuredGrid.h"
#include "vtkColorByPart.h"
#include "vtkDistributedDataFilter.h"
#include "vtkExtractCells.h"
#include "vtkGroup.h"
#include "vtkHDF5RawImageReader.h"
#include "vtkKWExtractGeometryByScalar.h"
#include "vtkKdTree.h"
#include "vtkMergeArrays.h"
#include "vtkMergeCells.h"
#include "vtkMultiOut.h"
#include "vtkMultiOut2.h"
#include "vtkMultiOut3.h"
#include "vtkPKdTree.h"
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
#include "vtkPVKitwareContourFilter.h"
#include "vtkPVRibbonFilter.h"
#include "vtkPVThresholdFilter.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkPVWarpScalar.h"
#include "vtkPVWarpVector.h"
#include "vtkPlanesIntersection.h"
#include "vtkPointsProjectedHull.h"
#include "vtkStructuredCacheFilter.h"
#include "vtkVRMLSource.h"
#include "vtkXMLPVDWriter.h"

int main()
{
  vtkObject *c;
  c = vtkCleanUnstructuredGrid::New(); c->Print(cout); c->Delete();
  c = vtkColorByPart::New(); c->Print(cout); c->Delete();
  c = vtkDistributedDataFilter::New(); c->Print(cout); c->Delete();
  c = vtkExtractCells::New(); c->Print(cout); c->Delete();
  c = vtkGroup::New(); c->Print(cout); c->Delete();
  c = vtkHDF5RawImageReader::New(); c->Print(cout); c->Delete();
  c = vtkKWExtractGeometryByScalar::New(); c->Print(cout); c->Delete();
  c = vtkKdTree::New(); c->Print(cout); c->Delete();
  c = vtkMergeArrays::New(); c->Print(cout); c->Delete();
  c = vtkMergeCells::New(); c->Print(cout); c->Delete();
  c = vtkMultiOut::New(); c->Print(cout); c->Delete();
  c = vtkMultiOut2::New(); c->Print(cout); c->Delete();
  c = vtkMultiOut3::New(); c->Print(cout); c->Delete();
  c = vtkPKdTree::New(); c->Print(cout); c->Delete();
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
  c = vtkPVKitwareContourFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVRibbonFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVThresholdFilter::New(); c->Print(cout); c->Delete();
  c = vtkPVUpdateSuppressor::New(); c->Print(cout); c->Delete();
  c = vtkPVWarpScalar::New(); c->Print(cout); c->Delete();
  c = vtkPVWarpVector::New(); c->Print(cout); c->Delete();
  c = vtkPlanesIntersection::New(); c->Print(cout); c->Delete();
  c = vtkPointsProjectedHull::New(); c->Print(cout); c->Delete();
  c = vtkStructuredCacheFilter::New(); c->Print(cout); c->Delete();
  c = vtkVRMLSource::New(); c->Print(cout); c->Delete();
  c = vtkXMLPVDWriter::New(); c->Print(cout); c->Delete();
  return 0;
}
