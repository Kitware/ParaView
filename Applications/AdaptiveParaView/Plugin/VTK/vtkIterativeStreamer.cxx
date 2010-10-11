/*=========================================================================

  Program:   ParaView
  Module:    vtkIterativeStreamer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIterativeStreamer.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDriver.h"
#include "vtkStreamingHarness.h"

vtkStandardNewMacro(vtkIterativeStreamer);

//----------------------------------------------------------------------------
vtkIterativeStreamer::vtkIterativeStreamer()
{
}

//----------------------------------------------------------------------------
vtkIterativeStreamer::~vtkIterativeStreamer()
{
}

//----------------------------------------------------------------------------
void vtkIterativeStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkIterativeStreamer::StartRenderEvent()
{
  vtkCollection *harnesses = this->GetHarnesses();
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!harnesses || !ren || !rw)
    {
    return;
    }

  vtkCollectionIterator *iter = harnesses->NewIterator();
  iter->InitTraversal();
  bool everyone_done = true;
  while(!iter->IsDoneWithTraversal())
    {
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();
    int pieceNow = harness->GetPiece();
    if (pieceNow == 0)
      {
      //first pass has to clear to start off
      ren->EraseOn();
      rw->EraseOn();

      //and none but the last pass should swap back to front automatically
      rw->SwapBuffersOff();
      }

    int maxPiece = harness->GetNumberOfPieces();
    if (pieceNow < maxPiece-1)
      {
      everyone_done = false;
      }
    }

  if (everyone_done)
    {
    //cerr << "This is the last pass" << endl;
    }

  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkIterativeStreamer::EndRenderEvent()
{
  vtkCollection *harnesses = this->GetHarnesses();
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!harnesses || !ren || !rw)
    {
    return;
    }

  //subsequent renders can not clear or they will erase what we drew before
  ren->EraseOff();
  rw->EraseOff();

  vtkCollectionIterator *iter = harnesses->NewIterator();
  iter->InitTraversal();
  bool everyone_done = true;
  while(!iter->IsDoneWithTraversal())
    {
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();
    int maxPiece = harness->GetNumberOfPieces();
    int pieceNow = harness->GetPiece();
    int pieceNext = pieceNow;
    if (pieceNow+1 < maxPiece)
      {
      everyone_done = false;
      pieceNext++;
      }
    harness->SetPiece(pieceNext);
    }

  if (everyone_done)
    {
    //we just drew the last frame everyone has to start over next time
    iter->InitTraversal();
    while(!iter->IsDoneWithTraversal())
      {
      vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
        (iter->GetCurrentObject());
      harness->SetPiece(0);
      iter->GoToNextItem();
      }

    //we also need to bring back buffer forward to show what we drew
    rw->SwapBuffersOn();
    rw->Frame();
    }
  else
    {
    //we haven't finished yet so schedule the next pass
    this->RenderEventually();
    }

  iter->Delete();
}
