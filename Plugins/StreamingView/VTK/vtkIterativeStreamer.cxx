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
  if (this->StartOver)
    {
    DEBUGPRINT_PASSES(cerr << "STARTOVER" << endl;);
    this->StartOver = false;
    return true;
    }

  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return true;
    }

  if (this->HasCameraMoved())
    {
    DEBUGPRINT_PASSES(cerr << "CAMMOVED" << endl;);
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkIterativeStreamer::PrepareFirstPass()
{
  DEBUGPRINT_PASSES(cerr << "PFP" << endl;);
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!ren || !rw)
    {
    return;
    }

  //first pass has to clear to start off
  ren->EraseOn();
  rw->EraseOn();
  if (!rw->GetNeverRendered())
    {
    rw->Frame();
    }

  //we manage back to front swapping
  rw->SwapBuffersOff();

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
    int maxPiece = harness->GetNumberOfPieces();
    if (this->LastPass < maxPiece)
      {
      maxPiece = this->LastPass;
      }
    int pieceNow = harness->GetPiece();
    int pieceNext = pieceNow;
    if (pieceNow+1 < maxPiece)
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
  DEBUGPRINT_PASSES(cerr << "SRE" << endl;);
  bool firstPass = this->IsFirstPass();
  if (this->GetParallelHelper())
    {
    this->GetParallelHelper()->Reduce(firstPass);
    }
  if (firstPass)
    {
    this->PrepareFirstPass();
    }
  else
    {
    this->PrepareNextPass();
    }
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
    int maxPiece = harness->GetNumberOfPieces();
    if (this->LastPass < maxPiece)
      {
      maxPiece = this->LastPass;
      }
    int pieceNow = harness->GetPiece();
    if (pieceNow+1 < maxPiece)
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
  if (ren && rw)
    {
    ren->EraseOff();
    rw->EraseOff();
    }

  bool everyoneDone = (this->IsEveryoneDone() || this->StopNow);
  if (this->GetParallelHelper())
    {
    this->GetParallelHelper()->Reduce(everyoneDone);
    }
  if (everyoneDone )
    {
    DEBUGPRINT_PASSES(cerr << "ALLDONE SHOW" << endl;);

    this->StopNow = false;
    this->StartOver = true;

    //we also need to bring back buffer forward to show what we drew
    this->CopyBackBufferToFront();
    }
  else
    {
    if (this->DisplayFrequency == 1)
      {
      DEBUGPRINT_PASSES(cerr << "ALLDONE SHOW ALL" << endl;);
      this->CopyBackBufferToFront();
      }

    //we haven't finished yet so schedule the next pass
    DEBUGPRINT_PASSES(cerr << "RENDER EVENTUALLY" << endl;);
    this->RenderEventually();
    }
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
