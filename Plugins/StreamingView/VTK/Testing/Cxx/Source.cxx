/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Test_StreamSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests that the stream source can generate data for different pieces at
// different resolutions.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkImageData.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStreamedMandelbrot.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTesting.h"

#include "vtksys/SystemTools.hxx"

//---------------------------------------------------------------------------
int Source(int argc, char *argv[])
{
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  renderer->GetActiveCamera()->SetPosition( 0, 0, 10);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->SetBackground(0.0,0.0,0.0);
  renWin->SetSize(300,300);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  renWin->Render();

  // create a streaming capable source.
  // it provides data and meta data (including world space bounds and
  // possibly scalar ranges) for any requested piece at any requested resolution
#if 0
  vtkSmartPointer<vtkImageMandelbrotSource> sms =
    vtkSmartPointer<vtkImageMandelbrotSource>::New();
#else
  vtkSmartPointer<vtkStreamedMandelbrot> sms =
    vtkSmartPointer<vtkStreamedMandelbrot>::New();
#endif
  sms->SetWholeExtent(0,127,0,127,0,127);
  sms->SetOriginCX(-1.75,-1.25,0,0);

  vtkDataObject *input = sms->GetOutput();
  vtkInformation* info = sms->GetOutputInformation(0);
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      sms->GetExecutive());

  //test source by asking it to produce different pieces...
  const int numers[5] = {0, 0, 1, 1, 3};
  const int denoms[5] = {1, 2, 2, 4, 4};
  int i = -1, j = -1;
  for (int pieceChoice = 0; pieceChoice < 5; pieceChoice++)
    {
    i++;
    j = -1;

    //...at different resolutions
    for (double res = 0.0; res < 1.0; res+=0.4)
      {
      j++;

      //TODO: Fix the pipeline bug or pipeline cache mode that causes the need
      //to update twice. It is needed even without res
      sms->Modified();
      sddp->SetUpdateResolution(info, res);
      sddp->SetUpdateExtent(info, numers[pieceChoice], denoms[pieceChoice], 0);
      sms->Update();
      input = sms->GetOutput();
      sms->Modified();
      sddp->SetUpdateResolution(info, res);
      sddp->SetUpdateExtent(info, numers[pieceChoice], denoms[pieceChoice], 0);
      sms->Update();
      input = sms->GetOutput();

      //don't let contour request entire extent
      vtkSmartPointer<vtkImageData> id = vtkSmartPointer<vtkImageData>::New();
      id->ShallowCopy(input);

      vtkSmartPointer<vtkContourFilter> contour =
        vtkSmartPointer<vtkContourFilter>::New();
      contour->SetInputData(id);
      contour->SetValue(0,50.0);
      contour->Update();

      vtkSmartPointer<vtkDataSetMapper> map1 =
        vtkSmartPointer<vtkDataSetMapper>::New();
      map1->SetInputConnection(contour->GetOutputPort());

      vtkSmartPointer<vtkActor> act1 = vtkSmartPointer<vtkActor>::New();
      act1->SetMapper(map1);
      act1->SetPosition(i*2.0, j*2.0, 0.0);
      renderer->AddActor(act1);
      }
    }

  renWin->Render();
  renderer->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }
  return !retVal;
}
