/*=========================================================================

  Program:   ParaView
  Module:    TestContinuousClose3D.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageData.h"
#include "vtkImageReader.h"
#include "vtkPVImageContinuousDilate3D.h"
#include "vtkPVImageContinuousErode3D.h"
#include "vtkPVImageMedian3D.h"
#include "vtkPVImageGradient.h"
#include "vtkImageViewer.h"
#include "vtkPVImageGradientMagnitude.h"
#include "vtkTesting.h"

#ifdef VTK_USE_PATENTED
#include "vtkPVKitwareContourFilter.h"
#endif

int main(int argc, char* argv[])
{
  char* fname = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  // Image pipeline
  vtkImageReader *reader = vtkImageReader::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent (0, 63, 0, 63, 1, 93);
  reader->SetFilePrefix ( fname );
  reader->SetDataMask (0x7fff);

  delete [] fname;

  vtkPVImageContinuousDilate3D *dilate = vtkPVImageContinuousDilate3D::New();
  dilate->SetInput(reader->GetOutput());
  dilate->SetKernelSize( 11, 11, 1);

  vtkPVImageContinuousErode3D *erode = vtkPVImageContinuousErode3D::New();
  erode->SetInput (dilate->GetOutput());
  erode->SetKernelSize(11,11,1);
  
  vtkPVImageMedian3D *median = vtkPVImageMedian3D::New();
  median->SetInput( erode->GetOutput() );
  
  vtkPVImageGradient *gradient = vtkPVImageGradient::New();
  gradient->SetInput( median->GetOutput() );
  gradient->SetDimensionality (3);
  gradient->Update(); //discard gradient

  vtkPVImageGradientMagnitude *magnitude = vtkPVImageGradientMagnitude::New();
  magnitude->SetInput( erode->GetOutput() );
  magnitude->SetDimensionality (3);

#ifdef VTK_USE_PATENTED
  vtkPVKitwareContourFilter* contour = vtkPVKitwareContourFilter::New();
  contour->SetInput( magnitude->GetOutput() );
  contour->SetValue(0, 0.5);
  contour->Update();  //discard
  contour->Delete();
#endif

  vtkImageViewer *viewer = vtkImageViewer::New();
  viewer->SetInput ( magnitude->GetOutput() );
  viewer->SetColorWindow(2000);
  viewer->SetColorLevel(1000);
  viewer->Render();

  reader->Delete();
  dilate->Delete();
  erode->Delete();
  median->Delete();
  gradient->Delete();
  magnitude->Delete();
  viewer->Delete();

  return 0;
}


