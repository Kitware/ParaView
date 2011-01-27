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
#include "vtkParallelStreamHelper.h"
#include "vtkPieceCacheFilter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDriver.h"
#include "vtkStreamingHarness.h"

#define DEBUGPRINT_PASSES( arg ) ;

vtkStandardNewMacro(vtkIterativeStreamer);

//----------------------------------------------------------------------------
vtkIterativeStreamer::vtkIterativeStreamer()
{
  this->NumberOfPasses = 32;
  this->LastPass = 32;
  this->StopNow = false;
  this->StartOver = true;
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
void vtkIterativeStreamer::AddHarnessInternal(vtkStreamingHarness *harness)
{
  vtkPieceCacheFilter *pcf = harness->GetCacheFilter();
  if (pcf)
    {
    pcf->SetCacheSize(this->CacheSize);
    }
  harness->SetNumberOfPieces(this->NumberOfPasses);
}

//----------------------------------------------------------------------------
bool vtkIterativeStreamer::IsFirstPass()
{
  if (this->HasCameraMoved())
    {
    DEBUGPRINT_PASSES(cerr << "CAMMOVED" << endl;);
    return true;
    }

  if (this->StartOver)
    {
    DEBUGPRINT_PASSES(cerr << "STARTOVER" << endl;);
    return true;
    }

  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkIterativeStreamer::PrepareFirstPass()
{
  DEBUGPRINT_PASSES(cerr << "PFP" << endl;);
  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return;
    }

  vtkCollectionIterator *iter = harnesses->NewIterator();
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
    {
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();
    if (!harness->GetEnabled())
      {
      continue;
      }
    harness->SetPiece(0);
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkIterativeStreamer::PrepareNextPass()
{
  DEBUGPRINT_PASSES(cerr <<"PNP"<< endl;);
  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return;
    }

  vtkCollectionIterator *iter = harnesses->NewIterator();
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
    {
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();
    if (!harness->GetEnabled())
      {
      continue;
      }
    int maxPiece = harness->GetNumberOfPieces();
    if (this->LastPass < maxPiece)
      {
      maxPiece = this->LastPass;
      }
    int pieceNow = harness->GetPiece();
    int pieceNext = pieceNow;
    if (pieceNow < maxPiece)
      {
      pieceNext++;
      }
    DEBUGPRINT_PASSES(cerr << pieceNext << endl;);

    harness->SetPiece(pieceNext);
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkIterativeStreamer::StartRenderEvent()
{
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();

  DEBUGPRINT_PASSES(cerr << "SRE" << endl;);
  bool firstPass = this->IsFirstPass();
  if (this->GetParallelHelper())
    {
    this->GetParallelHelper()->Reduce(firstPass);
    }
  if (firstPass)
    {
    //show whatever we partially drew before the camera moved
    this->CopyBackBufferToFront();

    //start off initial pass by clearing the screen
    if (ren && rw)
      {
      ren->EraseOn();
      rw->EraseOn();
      if (!rw->GetNeverRendered())
        {
        rw->Frame();
        }
      }

    this->PrepareFirstPass();
    }
  else
    {
    this->PrepareNextPass();
    }

  //we manage back to front swapping
  if (rw)
    {
    rw->SwapBuffersOff();
    }

  //assume that we are not done covering all the domains
  this->StartOver = false;
}

//----------------------------------------------------------------------------
bool vtkIterativeStreamer::IsEveryoneDone()
{
  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return true;
    }

  bool everyone_done = true;
  vtkCollectionIterator *iter = harnesses->NewIterator();
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
    {
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();
    if (!harness->GetEnabled())
      {
      continue;
      }
    int maxPiece = harness->GetNumberOfPieces();
    if (this->LastPass < maxPiece)
      {
      maxPiece = this->LastPass;
      }
    int pieceNow = harness->GetPiece();
    if (pieceNow <= maxPiece-2)
      {
      everyone_done = false;
      break;
      }
    }
  iter->Delete();
  return everyone_done;
}


//----------------------------------------------------------------------------
void vtkIterativeStreamer::EndRenderEvent()
{
  DEBUGPRINT_PASSES(cerr << "ERE" << endl;);
  //subsequent renders can not clear or they will erase what we just drew
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!ren || !rw)
    {
    return;
    }

  //don't clear the screen, keep adding to what we already drew
  ren->EraseOff();
  rw->EraseOff();

  bool allDone = this->IsEveryoneDone() || this->StopNow;
  if (this->GetParallelHelper())
    {
    this->GetParallelHelper()->Reduce(allDone);
    }
  if (allDone)
    {
    DEBUGPRINT_PASSES(cerr << "ALLDONE SHOW" << endl;);
    this->StopNow = false;
    //we just drew the last frame, everyone has to start over next time
    this->StartOver = true;
    //we also need to bring back buffer forward to show what we drew
    this->CopyBackBufferToFront();
    }
  else
    {
    if (this->DisplayFrequency == 1)
      {
      DEBUGPRINT_PASSES(cerr << "SHOW ALL" << endl;);
      this->CopyBackBufferToFront();
      }

    //we haven't finished yet so schedule the next pass
    DEBUGPRINT_PASSES(cerr << "RENDER EVENTUALLY" << endl;);
    this->RenderEventually();
    }
}

//------------------------------------------------------------------------------
void vtkIterativeStreamer::RestartStreaming()
{
  this->StartOver = true;
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
      if (!harness->GetEnabled())
        {
        continue;
        }
      harness->SetNumberOfPieces(nv);
      }
    iter->Delete();
    }
  this->Modified();
}
