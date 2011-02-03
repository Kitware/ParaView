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
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStreamedMandelbrot.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTesting.h"
#include "vtkXMLImageDataWriter.h"

#include "vtksys/SystemTools.hxx"

//---------------------------------------------------------------------------
int main(int , char **)
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
  vtkSmartPointer<vtkStreamedMandelbrot> sms =
    vtkSmartPointer<vtkStreamedMandelbrot>::New();
  sms->SetWholeExtent(0,127,0,127,0,127);
  sms->SetOriginCX(-1.75,-1.25,0,0);

  vtkDataObject *input = sms->GetOutput();
  vtkInformation* info = input->GetPipelineInformation();
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast
    (vtkExecutive::PRODUCER()->GetExecutive(info));
  sddp->SetUpdateResolution(info, 0.3);
  sddp->SetUpdateExtent(info, 4, 8, 0);
/*
  //why isn't this equivalent to above?
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(sms->GetExecutive());
  sddp->SetUpdateResolution(0, 0.0);
  sddp->SetUpdateExtent(0, 0, 1, 0);
*/
  double priority = sddp->ComputePriority();
  cerr << "PRIORITY IS " << priority << endl;
  //TODO: why does this need to be respecified?
  sddp->SetUpdateExtent(info, 4, 8, 0);
  input->Update();

  vtkSmartPointer<vtkImageData> id = vtkSmartPointer<vtkImageData>::New();
  id->ShallowCopy(input);
//  id->PrintSelf(cerr, vtkIndent(0));

  vtkSmartPointer<vtkXMLImageDataWriter> writer =
    vtkSmartPointer<vtkXMLImageDataWriter>::New();
  writer->SetFileName("/tmp/foo.vti");
  writer->SetInput(id);
  //writer->SetInputConnection(sms->GetOutputPort());
  //writer->Write();

  // Set up a sample pipeline containing filters that pass meta info downstream
  vtkSmartPointer<vtkContourFilter> contour =
    vtkSmartPointer<vtkContourFilter>::New();
  contour->SetInput(id);
  contour->SetValue(0,50.0);

  vtkSmartPointer<vtkDataSetMapper> map1 =
    vtkSmartPointer<vtkDataSetMapper>::New();
  map1->SetInputConnection(contour->GetOutputPort());

  vtkSmartPointer<vtkActor> act1 = vtkSmartPointer<vtkActor>::New();
  act1->SetMapper(map1);
  renderer->AddActor(act1);

  renWin->Render();
  iren->Start();

  return 0;
}
