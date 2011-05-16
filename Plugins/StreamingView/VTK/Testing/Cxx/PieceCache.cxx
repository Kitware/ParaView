/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Test_StreamPieceCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests that the piece cache filter does what it needs to do.
// It must prevent pipeline updates when pieces are re-requested
// It must not prevent updates when the data is changed
// It has to be able to append all of the cached pieces into a single large
// piece for single pass rendering if and once everything visible is cached.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkPieceCacheFilter.h"
#include "vtkPrioritizedStreamer.h"
#include "vtkProgrammableFilter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStreamedMandelbrot.h"
#include "vtkStreamingHarness.h"
#include "vtkTesting.h"

#include "vtksys/SystemTools.hxx"

int execount = 0;

void WatchPipeline(void *)
{
  cerr << "Pipeline is executing for the "<< execount << "'th time" << endl;
  execount ++;
}

//---------------------------------------------------------------------------
int PieceCache(int , char *[])
{
  bool failed = false;

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
  // possibly scalar ranges) for any requested piece at any requested
  // resolution
  vtkSmartPointer<vtkStreamedMandelbrot> sms =
    vtkSmartPointer<vtkStreamedMandelbrot>::New();
  sms->SetWholeExtent(0,127,0,127,0,127);
  sms->SetOriginCX(-1.75,-1.25,0,0);

  // Set up a sample pipeline containing filters that pass meta info
  // downstream
  vtkSmartPointer<vtkContourFilter> contour =
    vtkSmartPointer<vtkContourFilter>::New();
  contour->SetInputConnection(sms->GetOutputPort());
  contour->SetValue(0,50.0);

  // Use a programmable filter to detect when the pipeline flows
  vtkSmartPointer<vtkProgrammableFilter> pfilter =
    vtkSmartPointer<vtkProgrammableFilter>::New();
  pfilter->SetInputConnection(contour->GetOutputPort());
  pfilter->CopyArraysOn();
  pfilter->SetExecuteMethod(WatchPipeline, NULL);

  // A cache in the pipeline is essential for decent performance
  vtkSmartPointer<vtkPieceCacheFilter> pcf =
    vtkSmartPointer<vtkPieceCacheFilter>::New();
  pcf->SetInputConnection(pfilter->GetOutputPort());

  // An access point to inject resolution into the pipeline
  vtkSmartPointer<vtkStreamingHarness> harness=
    vtkSmartPointer<vtkStreamingHarness>::New();
#if 0
  //remove pcf from pipeline to see how exe happens for all pieces on
  //each render
  harness->SetInputConnection(pfilter->GetOutputPort());
#else
  harness->SetInputConnection(pcf->GetOutputPort());
#endif
  harness->SetNumberOfPieces(16);
  harness->SetPiece(0);
  harness->SetResolution(1.0);

  cerr << "VERIFY EMPTY CACHE" << endl;
  int p0c,p1c,p2c;
  p0c = pcf->InCache(0, 16, 1.0);
  p1c = pcf->InCache(1, 16, 1.0);
  p2c = pcf->InCache(2, 16, 1.0);
  if ((p0c != 0) || (p1c != 0) || (p2c != 0))
    {
    cerr << p0c << " " << p1c << " " << p2c;
    cerr << " test failed cache should be empty" << endl;
    failed = true;
    }

  cerr << "ASK FOR 0/16" << endl;
  harness->Update();
  cerr << "VERIFY IN CACHE" << endl;
  p0c = pcf->InCache(0, 16, 1.0);
  p1c = pcf->InCache(1, 16, 1.0);
  p2c = pcf->InCache(2, 16, 1.0);
  if ((p0c != 1) || (p1c != 0) || (p2c != 0))
    {
    cerr << p0c << " " << p1c << " " << p2c;
    cerr << " test failed cache content wrong" << endl;
    failed = true;
    }
  if (execount != 1)
    {
    cerr << execount
         << " test failed because pipeline isn't prevented from updating" << endl;
    failed = true;
    }

  cerr << "ASK FOR 1/16" << endl;
  harness->SetPiece(1);
  harness->Update();
  cerr << "VERIFY IN CACHE" << endl;
  p0c = pcf->InCache(0, 16, 1.0);
  p1c = pcf->InCache(1, 16, 1.0);
  p2c = pcf->InCache(2, 16, 1.0);
  if ((p0c != 1) || (p1c != 1) || (p2c != 0))
    {
    cerr << p0c << " " << p1c << " " << p2c;
    cerr << " test failed cache content wrong" << endl;
    failed = true;
    }
  if (execount != 2)
    {
    cerr << execount
         << " test failed because pipeline isn't prevented from updating" << endl;
    failed = true;
    }

  cerr << "VERIFY CACHE PREVENTS UPDATE" << endl;
  cerr << "ASK FOR 0/16 AGAIN" << endl;
  harness->SetPiece(0);
  harness->Update();
  p0c = pcf->InCache(0, 16, 1.0);
  p1c = pcf->InCache(1, 16, 1.0);
  p2c = pcf->InCache(2, 16, 1.0);
  if ((p0c != 1) || (p1c != 1) || (p2c != 0))
    {
    cerr << p0c << " " << p1c << " " << p2c;
    cerr << " test failed cache content wrong" << endl;
    failed = true;
    }
  if (execount != 3) //TODO, not sure why this first update isn't prevented
    {
    cerr << execount
         << " test failed because pipeline isn't prevented from updating" << endl;
    failed = true;
    }

  cerr << "ASK FOR 1/16 AGAIN" << endl;
  harness->SetPiece(1);
  harness->Update();
  p0c = pcf->InCache(0, 16, 1.0);
  p1c = pcf->InCache(1, 16, 1.0);
  p2c = pcf->InCache(2, 16, 1.0);
  if ((p0c != 1) || (p1c != 1) || (p2c != 0))
    {
    cerr << p0c << " " << p1c << " " << p2c;
    cerr << " test failed cache content wrong" << endl;
    failed = true;
    }
  if (execount != 3)
    {
    cerr << execount
         << " test failed because pipeline isn't prevented from updating" << endl;
    failed = true;
    }

  cerr << "ASK FOR 0/16 AGAIN" << endl;
  harness->SetPiece(0);
  harness->Update();
  p0c = pcf->InCache(0, 16, 1.0);
  p1c = pcf->InCache(1, 16, 1.0);
  p2c = pcf->InCache(2, 16, 1.0);
  if ((p0c != 1) || (p1c != 1) || (p2c != 0))
    {
    cerr << p0c << " " << p1c << " " << p2c;
    cerr << " test failed cache content wrong" << endl;
    failed = true;
    }
  if (execount != 3)
    {
    cerr << execount
         << " test failed because pipeline isn't prevented from updating" << endl;
    failed = true;
    }

  cerr << "CHANGE UPSTREAM PIPELINE" << endl;
  contour->SetValue(0,10.0);
  harness->Update();
  if (execount != 4)
    {
    cerr << execount
         << " test failed because pipeline prevented update it shouldn't have" << endl;
    failed = true;
    }
  int npts_p0 = vtkPolyData::SafeDownCast(harness->GetOutput())->GetNumberOfPoints();
  cerr << "0/16 has " << npts_p0 << " points" << endl;
  cerr << "ASK FOR 1/16" << endl;
  harness->SetPiece(1);
  harness->Update();
  int npts_p1 = vtkPolyData::SafeDownCast(harness->GetOutput())->GetNumberOfPoints();
  cerr << "1/16 has " << npts_p1 << " points" << endl;
  if (execount != 5)
    {
    cerr << execount
         << " test failed because pipeline prevented update it shouldn't have" << endl;
    failed = true;
    }

  cerr << "TEST APPEND" << endl;
  cerr << "VERIFY NOTHING IN APPEND" << endl;
  p0c = pcf->InAppend(0, 16, 1.0);
  p1c = pcf->InAppend(1, 16, 1.0);
  p2c = pcf->InAppend(2, 16, 1.0);
  if ((p0c != 0) || (p1c != 0) || (p2c != 0))
    {
    cerr << p0c << " " << p1c << " " << p2c;
    cerr << " test failed appended cache should be empty" << endl;
    failed = true;
    }
  cerr << "APPEND" << endl;
  pcf->AppendPieces();
  cerr << "VERIFY CONTENTS OF APPEND" << endl;
  p0c = pcf->InAppend(0, 16, 1.0);
  p1c = pcf->InAppend(1, 16, 1.0);
  p2c = pcf->InAppend(2, 16, 1.0);
  if ((p0c != 1) || (p1c != 1) || (p2c != 0))
    {
    cerr << p0c << " " << p1c << " " << p2c;
    cerr << " test failed appended cache content wrong" << endl;
    failed = true;
    }
  vtkPolyData *pd = pcf->GetAppendedData();
  int npts_app = 0;
  if (pd)
    {
    npts_app = pd->GetNumberOfPoints();
    }
  if (npts_app != (npts_p0+npts_p1))
    {
    cerr << "test failed, appended output is wrong "
         << npts_app << "!=" <<  (npts_p0+npts_p1) << endl;
    failed = true;
    }

  vtkSmartPointer<vtkDataSetMapper> map1 =
    vtkSmartPointer<vtkDataSetMapper>::New();
  map1->SetInputConnection(harness->GetOutputPort());

  vtkSmartPointer<vtkActor> act1 = vtkSmartPointer<vtkActor>::New();
  act1->SetMapper(map1);
  renderer->AddActor(act1);

  vtkSmartPointer<vtkPrioritizedStreamer> sd =
    vtkSmartPointer<vtkPrioritizedStreamer>::New();
  sd->SetRenderWindow(renWin);
  sd->SetRenderer(renderer);
  sd->AddHarness(harness);

  renWin->Render();
  int lexecount = execount;
  renWin->Render();
  renWin->Render();
  renWin->Render();
  if (execount != lexecount)
    {
    cerr << execount << "!= " << lexecount
         << " test failed because pipeline isn't prevented from updating" << endl;
    failed = true;
    }

  return failed;
}
