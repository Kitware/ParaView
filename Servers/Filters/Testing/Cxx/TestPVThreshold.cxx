
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
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkPVThreshold.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

int main(int argc, char* argv[])
{
  // Easy to use variables.
  typedef vtkSmartPointer<vtkRTAnalyticSource>      vtkRTAnalyticSourceRefPtr;
  typedef vtkSmartPointer<vtkPVThreshold>           vtkPVThresholdRefPtr;
  typedef vtkSmartPointer<vtkDataSetSurfaceFilter>  vtkDataSetSurfaceFilterRefPtr;
  typedef vtkSmartPointer<vtkPolyDataMapper>        vtkPolyDataMapperRefPtr;
  typedef vtkSmartPointer<vtkPVLODActor>            vtkPVLODActorRefPtr;
  typedef vtkSmartPointer<vtkRenderer>              vtkRendererRefPtr;
  typedef vtkSmartPointer<vtkRenderWindow>          vtkRenderWindowRefPtr;
  typedef vtkSmartPointer<vtkRenderWindowInteractor>
                                                    vtkRenderWindowInteractorRefPtr;

  vtkRTAnalyticSourceRefPtr source1  (vtkRTAnalyticSourceRefPtr::New());
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

  vtkRTAnalyticSourceRefPtr source2  (vtkRTAnalyticSourceRefPtr::New());
  source2->SetWholeExtent(20, 40, -10, 10, -10, 10);
  source2->SetCenter(30.0, 0.0, 0.0);
  source2->SetXFreq(60.0);
  source2->SetYFreq(30.0);
  source2->SetZFreq(40.0);
  source2->SetMaximum(255.0);
  source2->SetXMag(10.0);
  source2->SetYMag(18.0);
  source2->SetZMag(5.0);
  source2->SetStandardDeviation(0.5);
  source2->SetSubsampleRate(1);

  vtkRTAnalyticSourceRefPtr source3  (vtkRTAnalyticSourceRefPtr::New());
  source3->SetWholeExtent(50, 70, -10, 10, -10, 10);
  source3->SetCenter(60.0, 0.0, 0.0);
  source3->SetXFreq(60.0);
  source3->SetYFreq(30.0);
  source3->SetZFreq(40.0);
  source3->SetMaximum(255.0);
  source3->SetXMag(10.0);
  source3->SetYMag(18.0);
  source3->SetZMag(5.0);
  source3->SetStandardDeviation(0.5);
  source3->SetSubsampleRate(1);

  // First case
  vtkPVThresholdRefPtr pvt1 (vtkPVThresholdRefPtr::New());
  pvt1->SetInputConnection(source1->GetOutputPort());
  pvt1->SetSelectionMode(VTK_SELECTION_MODE_CLIP_CELL);
  pvt1->ThresholdBetween(100, 150);
  pvt1->Update();

  vtkDataSetSurfaceFilterRefPtr sf1 (vtkDataSetSurfaceFilterRefPtr::New());
  sf1->SetInputConnection(pvt1->GetOutputPort(0));

  vtkPolyDataMapperRefPtr mapper1 (vtkPolyDataMapperRefPtr::New());
  mapper1->SetInputConnection(sf1->GetOutputPort());

  vtkPVLODActorRefPtr actor1 (vtkPVLODActorRefPtr::New());
  actor1->SetMapper( mapper1 );
  //--


  // Second case.
  vtkPVThresholdRefPtr pvt2 (vtkPVThresholdRefPtr::New());
  pvt2->SetInputConnection(source2->GetOutputPort());
  pvt2->SetSelectionMode(VTK_SELECTION_MODE_CLIP_CELL);
  pvt2->ThresholdBetween(50.0, 276.829);
  pvt2->Update();

  vtkDataSetSurfaceFilterRefPtr sf2 (vtkDataSetSurfaceFilterRefPtr::New());
  sf2->SetInputConnection(pvt2->GetOutputPort(0));

  vtkPolyDataMapperRefPtr mapper2 (vtkPolyDataMapperRefPtr::New());
  mapper2->SetInputConnection(sf2->GetOutputPort());

  vtkPVLODActorRefPtr actor2 (vtkPVLODActorRefPtr::New());
  actor2->SetMapper( mapper2 );
  //--


  // Third case.
  vtkPVThresholdRefPtr pvt3 (vtkPVThresholdRefPtr::New());
  pvt3->SetInputConnection(source3->GetOutputPort());
  pvt3->SetSelectionMode(VTK_SELECTION_MODE_ALL_POINTS_MATCH);
  pvt3->ThresholdBetween(100, 150);
  pvt3->Update();

  vtkDataSetSurfaceFilterRefPtr sf3 (vtkDataSetSurfaceFilterRefPtr::New());
  sf3->SetInputConnection(pvt3->GetOutputPort(0));

  vtkPolyDataMapperRefPtr mapper3 (vtkPolyDataMapperRefPtr::New());
  mapper3->SetInputConnection(sf3->GetOutputPort());

  vtkPVLODActorRefPtr actor3 (vtkPVLODActorRefPtr::New());
  actor3->SetMapper( mapper3 );
  //--

  vtkRendererRefPtr ren (vtkRendererRefPtr::New());
  ren->AddActor(actor1);
  ren->AddActor(actor2);
  ren->AddActor(actor3);
  ren->ResetCamera();

  vtkRenderWindowRefPtr renWin (vtkRenderWindowRefPtr::New());
  renWin->AddRenderer( ren );

  vtkRenderWindowInteractorRefPtr iren (vtkRenderWindowInteractorRefPtr::New());
  iren->SetRenderWindow(renWin);
  renWin->SetSize( 300, 300 );
  iren->SetRenderWindow( renWin );
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  return !retVal;
}
