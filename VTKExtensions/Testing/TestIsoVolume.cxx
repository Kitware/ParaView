
/*=========================================================================

  Program:   ParaView
  Module:    TestPVThreshold.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVThreshold.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

int main(int argc, char* argv[])
{
  // Easy to use variables.
  typedef vtkSmartPointer<vtkRTAnalyticSource> vtkRTAnalyticSourceRefPtr;
  typedef vtkSmartPointer<vtkPVThreshold> vtkPVThresholdRefPtr;
  typedef vtkSmartPointer<vtkDataSetSurfaceFilter> vtkDataSetSurfaceFilterRefPtr;
  typedef vtkSmartPointer<vtkPolyDataMapper> vtkPolyDataMapperRefPtr;
  typedef vtkSmartPointer<vtkPVLODActor> vtkPVLODActorRefPtr;
  typedef vtkSmartPointer<vtkRenderer> vtkRendererRefPtr;
  typedef vtkSmartPointer<vtkRenderWindow> vtkRenderWindowRefPtr;
  typedef vtkSmartPointer<vtkRenderWindowInteractor> vtkRenderWindowInteractorRefPtr;

  vtkRTAnalyticSourceRefPtr source1(vtkRTAnalyticSourceRefPtr::New());
  source1->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  source1->SetCenter(0.0, 0.0, 0.0);
  source1->SetXFreq(60.0);
  source1->SetYFreq(30.0);
  source1->SetZFreq(40.0);
  source1->SetMaximum(255.0);
  source1->SetXMag(10.0);
  source1->SetYMag(18.0);
  source1->SetZMag(5.0);
  source1->SetStandardDeviation(0.5);
  source1->SetSubsampleRate(1);

  vtkPVThresholdRefPtr pvt1(vtkPVThresholdRefPtr::New());
  pvt1->SetInputConnection(source1->GetOutputPort());
  pvt1->ThresholdBetween(100, 150);
  pvt1->Update();

  vtkDataSetSurfaceFilterRefPtr sf1(vtkDataSetSurfaceFilterRefPtr::New());
  sf1->SetInputConnection(pvt1->GetOutputPort(0));

  vtkPolyDataMapperRefPtr mapper1(vtkPolyDataMapperRefPtr::New());
  mapper1->SetInputConnection(sf1->GetOutputPort());

  vtkPVLODActorRefPtr actor1(vtkPVLODActorRefPtr::New());
  actor1->SetMapper(mapper1);

  vtkRendererRefPtr ren(vtkRendererRefPtr::New());
  ren->AddActor(actor1);
  ren->ResetCamera();

  vtkRenderWindowRefPtr renWin(vtkRenderWindowRefPtr::New());
  renWin->AddRenderer(ren);

  vtkRenderWindowInteractorRefPtr iren(vtkRenderWindowInteractorRefPtr::New());
  iren->SetRenderWindow(renWin);
  renWin->SetSize(300, 300);
  iren->SetRenderWindow(renWin);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
