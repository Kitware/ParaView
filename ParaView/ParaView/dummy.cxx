#include "vtkHDF5RawImageReader.h"
#include "vtkColorByPart.h"
#include "vtkGroup.h"
#include "vtkKWExtractGeometryByScalar.h"
#include "vtkMergeArrays.h"
#include "vtkMultiOut.h"
#include "vtkMultiOut2.h"
#include "vtkMultiOut3.h"
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
#include "vtkStructuredCacheFilter.h"
#include "vtkVRMLSource.h"
#include "vtkXMLPVDWriter.h"


int main()
{
  vtkObject* o;
  o = vtkHDF5RawImageReader::New(); o->Print(cout); o->Delete();
  o = vtkColorByPart::New(); o->Print(cout); o->Delete();
  o = vtkGroup::New(); o->Print(cout); o->Delete();
  o = vtkKWExtractGeometryByScalar::New(); o->Print(cout); o->Delete();
  o = vtkMergeArrays::New(); o->Print(cout); o->Delete();
  o = vtkMultiOut::New(); o->Print(cout); o->Delete();
  o = vtkMultiOut2::New(); o->Print(cout); o->Delete();
  o = vtkMultiOut3::New(); o->Print(cout); o->Delete();
  o = vtkPVArrowSource::New(); o->Print(cout); o->Delete();
  o = vtkPVClipDataSet::New(); o->Print(cout); o->Delete();
  o = vtkPVConnectivityFilter::New(); o->Print(cout); o->Delete();
  o = vtkPVContourFilter::New(); o->Print(cout); o->Delete();
  o = vtkPVEnSightMasterServerReader::New(); o->Print(cout); o->Delete();
  o = vtkPVEnSightMasterServerTranslator::New(); o->Print(cout); o->Delete();
  o = vtkPVGlyphFilter::New(); o->Print(cout); o->Delete();
  o = vtkPVImageContinuousDilate3D::New(); o->Print(cout); o->Delete();
  o = vtkPVImageContinuousErode3D::New(); o->Print(cout); o->Delete();
  o = vtkPVImageGradient::New(); o->Print(cout); o->Delete();
  o = vtkPVImageGradientMagnitude::New(); o->Print(cout); o->Delete();
  o = vtkPVImageMedian3D::New(); o->Print(cout); o->Delete();
  o = vtkPVRibbonFilter::New(); o->Print(cout); o->Delete();
  o = vtkPVThresholdFilter::New(); o->Print(cout); o->Delete();
  o = vtkPVUpdateSuppressor::New(); o->Print(cout); o->Delete();
  o = vtkPVWarpScalar::New(); o->Print(cout); o->Delete();
  o = vtkPVWarpVector::New(); o->Print(cout); o->Delete();
  o = vtkStructuredCacheFilter::New(); o->Print(cout); o->Delete();
  o = vtkVRMLSource::New(); o->Print(cout); o->Delete();
  o = vtkXMLPVDWriter::New(); o->Print(cout); o->Delete();
  return 0;
}
