#include "vtkImageReader.h"
#include "vtkPVImageContinuousDilate3D.h"
#include "vtkPVImageContinuousErode3D.h"
#include "vtkImageViewer.h"

int main()
{
  // Image pipeline
  vtkImageReader *reader = vtkImageReader::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent (0, 63, 0, 63, 1, 93);
  reader->SetFilePrefix ("$VTK_DATA_ROOT/Data/headsq/quarter");
  reader->SetDataMask (0x7fff);

  vtkPVImageContinuousDilate3D *dilate = vtkPVImageContinuousDilate3D::New();
  dilate->SetInput(reader->GetOutput());
  dilate->SetKernelSize( 11, 11, 1);

  vtkPVImageContinuousErode3D *erode = vtkPVImageContinuousErode3D::New();
  erode->SetInput (dilate->GetOutput());
  erode->SetKernelSize(11,11,1);

  vtkImageViewer *viewer = vtkImageViewer::New();
  viewer->SetInput (erode->GetOutput());
  viewer->SetColorWindow(2000);
  viewer->SetColorLevel(1000);

  viewer->Render();

  return 0;
}


