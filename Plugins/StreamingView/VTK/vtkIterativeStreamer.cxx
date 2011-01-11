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
  this->CameraMoved = true;
  this->NumberOfPasses = 32;
  this->LastPass = 32;
  this->StopNow = false;
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
  //watch for camera movement and restart when that happens so we don't
  //"streak"
  this->CameraMoved = this->HasCameraMoved();

  while(!iter->IsDoneWithTraversal())
    {
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();
    int pieceNow = harness->GetPiece();
    if (pieceNow <= 0 || this->CameraMoved)
      {
      harness->SetPiece(0);

      //first pass has to clear to start off
      ren->EraseOn();
      rw->EraseOn();
      rw->Frame();

      //and none but the last pass should swap back to front automatically
      rw->SwapBuffersOff();
      }
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
    if (this->LastPass < maxPiece)
      {
      maxPiece = this->LastPass;
      }
    int pieceNow = harness->GetPiece();
    int pieceNext = pieceNow;
    if (pieceNow+1 < maxPiece && !this->CameraMoved)
      //CameraMoved is here because we detect motion too late to reset pipeline
      //and we end up missing the first pass's piece
      {
      everyone_done = false;
      pieceNext++;
      }
    harness->SetPiece(pieceNext);
    }
  this->CameraMoved = false;
  if (everyone_done || this->StopNow)
    {
    this->StopNow = false;

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
    this->CopyBackBufferToFront();
    }
  else
    {
    if (this->DisplayFrequency == 1)
      {
      this->CopyBackBufferToFront();
      }

    //we haven't finished yet so schedule the next pass
    this->RenderEventually();
    }

  iter->Delete();
}

//------------------------------------------------------------------------------
void vtkIterativeStreamer::StopStreaming()
{
  this->StopNow = true;
}

//------------------------------------------------------------------------------
void vtkIterativeStreamer::SetNumberOfPasses(int nv)
{
  if (this->NumberOfPasses == nv)
  {
    return;
  }
  this->NumberOfPasses = nv;
  vtkCollection *harnesses = this->GetHarnesses();
  if (harnesses)
    {
    vtkCollectionIterator *iter = harnesses->NewIterator();
    iter->InitTraversal();
    while(!iter->IsDoneWithTraversal())
      {
      vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
        (iter->GetCurrentObject());
      iter->GoToNextItem();
      harness->SetNumberOfPieces(nv);
      }
    iter->Delete();
    }
  this->Modified();
}
