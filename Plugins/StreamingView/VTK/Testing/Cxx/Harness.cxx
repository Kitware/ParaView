/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Test_StreamHarness.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests that the stream harness class can control the upstream pipeline
// and gather meta info from it.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStreamedMandelbrot.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStreamingHarness.h"
#include "vtkTesting.h"

#include "vtksys/SystemTools.hxx"

struct pieceRecord //used to store piece specifier, query result for the test
{
  int p;
  int n;
  double r;
  double bounds[6];
  double gconf;
  double range[2];
  double aconf;
};

//---------------------------------------------------------------------------
int Harness(int argc, char *argv[])
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

  // An access point to inject resolution into the pipeline
  vtkSmartPointer<vtkStreamingHarness> harness=
    vtkSmartPointer<vtkStreamingHarness>::New();
  harness->SetInputConnection(contour->GetOutputPort());
  harness->SetNumberOfPieces(16);
  harness->SetResolution(0.8);

  for (int p = 0; p < 16; p++)
    {
    if (harness->ComputePiecePriority(p, 16, 0.8) != 1.0)
      {
      cerr
        << "Without precomputed meta information, nothing can be "
        << "rejected by value before first update." << endl;
      return -1;
      }
    }

  for (int p = 0; p < 16; p++)
    {
    harness->SetPiece(p);
    harness->Update();
    }

  for (int p = 0; p < 16; p++)
    {
    double priority = harness->ComputePiecePriority(p, 16, 0.8);
    if ((p == 11 || p == 15) && priority > 0.5)
      {
      cerr << "This piece should have been rejected " << priority << endl;
      return -1;
      }
    if ((p != 11 && p != 15) && priority != 1.0)
      {
      cerr << "This piece should not have been rejected" << endl;
      return -1;
      }
    }

  double bounds[6];
  double gconf;
  double min;
  double max;
  double aconf;

  const pieceRecord pRecs[6] =
  {
    {0,16,0.0, {-1.75,-0.470472,-1.25,0.0295276,0,0.551181}, 1, {1.86486,100}, 1},
    {0,16,1.0, {-1.75,-0.490157,-1.25,0.00984252,0,0.503937}, 1, {1.86486,100}, 1},
    {0,4,0.5, {-1.75,0.67126,-1.25,-0.00984252,0,0.992126}, 1, {0,-1}, 0},
    {1,4,0.5, {-1.75,0.67126,-0.127953,1.17126,0,0.992126}, 1, {0,-1}, 0},
    {0,4,0.8, {-1.75,0.67126,-1.25,-0.00984252,0,1.00787}, 1, {0,-1}, 0},
    {1,4,0.8, {-1.75,0.67126,-0.127953,1.17126,0,1.00787}, 1, {0,-1}, 0}
  };

  for (int p = 0; p < 6; p++)
    {
    harness->ComputePieceMetaInformation(pRecs[p].p, pRecs[p].n, pRecs[p].r,
                                         bounds, gconf,
                                         min, max, aconf);
    if (gconf != pRecs[p].gconf)
      {
      cerr << "Got wrong geometric confidence ";
      cerr << p << " " << gconf << "!=" << pRecs[p].gconf << endl;
      return -1;
      }
    for (int i = 0; i < 6; i++)
      {
      if (fabs(bounds[i]-pRecs[p].bounds[i]) > 0.001)
        {
        cerr << "Got wrong bounds ";
        cerr << p << " " << i << " "
             << bounds[i] << "!= " << pRecs[p].bounds[i] << endl;
        return -1;
        }
      }
    if (aconf != pRecs[p].aconf)
      {
      cerr << "Got wrong attribute confidence ";
      cerr << p << " " << aconf << "!=" << pRecs[p].aconf << endl;
      return -1;
      }
    if ((fabs(min - pRecs[p].range[0]) > 0.001) ||
        (fabs(max - pRecs[p].range[1]) > 0.001))
      {
      cerr << "Got wrong data range ";
      cerr << p << " "
           << min << "!=" << pRecs[p].range[0] << " or "
           << max << "!=" << pRecs[p].range[1] << " or " << endl;
      return -1;
      }
    }

  //test simply using harness to specify a piece and showing that
  harness->SetNumberOfPieces(4);
  harness->SetPiece(1);
  harness->SetResolution(0.5);
  harness->Update();

  vtkSmartPointer<vtkDataSetMapper> map1 =
    vtkSmartPointer<vtkDataSetMapper>::New();
  map1->SetInputConnection(harness->GetOutputPort());

  vtkSmartPointer<vtkActor> act1 = vtkSmartPointer<vtkActor>::New();
  act1->SetMapper(map1);
  renderer->AddActor(act1);

  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }
  return !retVal;
}
