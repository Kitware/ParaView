/*=========================================================================

  Program:   ParaView
  Module:    vtkPrioritizedStreamer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPrioritizedStreamer.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkParallelStreamHelper.h"
#include "vtkPieceCacheFilter.h"
#include "vtkPieceList.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDriver.h"
#include "vtkStreamingHarness.h"

#define DEBUGPRINT_PASSES( arg ) ;
#define DEBUGPRINT_PRIORITY( arg ) ;

vtkStandardNewMacro(vtkPrioritizedStreamer);

class vtkPrioritizedStreamer::Internals
{
public:
  Internals(vtkPrioritizedStreamer *owner)
  {
    this->Owner = owner;
    this->StartOver = true;
    this->DebugPass = 0;
    this->StopNow = false;
  }
  ~Internals()
  {
  }

  vtkPrioritizedStreamer *Owner;
  bool StartOver;
  bool StopNow;
  int DebugPass;  //used solely for debug messages
};

//----------------------------------------------------------------------------
vtkPrioritizedStreamer::vtkPrioritizedStreamer()
{
  this->Internal = new Internals(this);
  this->NumberOfPasses = 32;
  this->LastPass = 32;
  this->PipelinePrioritization = 1;
  this->ViewPrioritization = 1;
}

//----------------------------------------------------------------------------
vtkPrioritizedStreamer::~vtkPrioritizedStreamer()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::AddHarnessInternal(vtkStreamingHarness *harness)
{
  vtkPieceCacheFilter *pcf = harness->GetCacheFilter();
  if (pcf)
    {
    pcf->SetCacheSize(this->CacheSize);
    }
  harness->SetNumberOfPieces(this->NumberOfPasses);
}

//----------------------------------------------------------------------------
bool vtkPrioritizedStreamer::IsFirstPass()
{
  if (this->HasCameraMoved())
    {
    DEBUGPRINT_PASSES(cerr << "CAMMOVED" << endl;);
    return true;
    }

  if (this->Internal->StartOver)
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
void vtkPrioritizedStreamer::PrepareFirstPass()
{
  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return;
    }

  vtkCollectionIterator *iter = harnesses->NewIterator();
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
    {
    //get a hold of the pipeline for this object
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();
    if (!harness->GetEnabled())
      {
      continue;
      }

    //make it start over
    harness->SetPass(0);

    //compute pipeline priority to render in most to least important order
    //get a hold of the priority list
    vtkPieceList *pl = harness->GetPieceList1();
    if (!pl)
      {
      //make one if never created before
      pl = vtkPieceList::New();
      harness->SetPieceList1(pl);
      pl->Delete();
      }
    //start off cleanly
    pl->Clear();

    //compute a priority for each piece and sort them
    int max = harness->GetNumberOfPieces();
    for (int i = 0; i < max; i++)
      {
      vtkPiece p;
      p.SetPiece(i);
      p.SetNumPieces(max);
      p.SetResolution(1.0);
      double priority = 1.0;
      if (this->PipelinePrioritization)
        {
        priority = harness->ComputePiecePriority(i,max,1.0);
        }
      DEBUGPRINT_PRIORITY
        (
         if (!priority)
           {
           cerr << "CHECKED PPRI OF " << i << "/" << max << "@" << 1.0
                << " = " << priority << endl;
           }
         );
      p.SetPipelinePriority(priority);

      double pbbox[6];
      double gConf = 1.0;
      double aMin = 1.0;
      double aMax = -1.0;
      double aConf = 1.0;
      harness->ComputePieceMetaInformation
        (i, max, 1.0,
         pbbox, gConf, aMin, aMax, aConf);
      double gPri = 1.0;
      if (this->ViewPrioritization)
        {
        gPri = this->CalculateViewPriority(pbbox);
        }
      DEBUGPRINT_PRIORITY
        (
         if (!gPri)
           {
           cerr << "CHECKED VPRI OF " << i << "/" << max << "@" << 1.0
                << " [" << pbbox[0] << "," << pbbox[1] << " "
                << pbbox[2] << "," << pbbox[3] << " "
                << pbbox[4] << "," << pbbox[5] << "]"
                << " = " << gPri << endl;
           }
         );
      p.SetViewPriority(gPri);

      pl->AddPiece(p);
      }
    pl->SortPriorities();
    //pl->Print();

    //convert pass number to absolute piece number
    int pieceNext = pl->GetPiece(0).GetPiece();
    DEBUGPRINT_PRIORITY
      (
       cerr << harness << " P0 PASS " << 0 << " PIECE " << pieceNext << endl;
       );
    harness->SetPiece(pieceNext);
    //this is a hack
    harness->SetPass(-1);
    //but I can't get the first piece to show consistently without it
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::PrepareNextPass()
{
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

    //increment to the next pass
    int maxPass = harness->GetNumberOfPieces();
    int passNow = harness->GetPass();
    int passNext = passNow;
    if (passNow < maxPass)
      {
      passNext++;
      }
    harness->SetPass(passNext);

    //map that to an absolute piece number, but don't got beyond end
    vtkPieceList *pl = harness->GetPieceList1();
    double priority = pl->GetPiece(passNext).GetPriority();
    if (priority)
      {
      int pieceNext = pl->GetPiece(passNext).GetPiece();
      DEBUGPRINT_PRIORITY
        (
         cerr <<harness<<" NP PASS "<<passNext<<" PIECE "<<pieceNext<<endl;
         );
      harness->SetPiece(pieceNext);
      }
   }

  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::StartRenderEvent()
{
  DEBUGPRINT_PASSES
    (
     cerr << "SR " << this->Internal->DebugPass << endl;
     );
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();

  bool firstPass = this->IsFirstPass();
  if (this->GetParallelHelper())
    {
    this->GetParallelHelper()->Reduce(firstPass);
    }
  if (firstPass)
    {
    DEBUGPRINT_PASSES
      (
       cerr << "RESTART" << endl;
       this->Internal->DebugPass = 0;
       );

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

    //compute priority of subsequent passes
    //set pipeline to show the most important one this pass
    this->PrepareFirstPass();
    }
  else
    {
    //subsequent passes pick next less important piece each pass
    this->PrepareNextPass();
    }

  //don't swap back to front automatically
  //only update the screen once the last piece is drawn
  rw->SwapBuffersOff();

  //assume that we are not done covering all the domains
  this->Internal->StartOver = false;
}

//----------------------------------------------------------------------------
bool vtkPrioritizedStreamer::IsEveryoneDone()
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

    //check if anyone hasn't drawn the last necessary pass
    int passNow = harness->GetPass();
    int maxPass = harness->GetNumberOfPieces();
    if (this->LastPass < maxPass)
      {
      maxPass = this->LastPass;
      }
    if (passNow <= maxPass-2)
      {
      vtkPieceList *pl = harness->GetPieceList1();
      if (pl)
        {
        double priority = pl->GetPiece(passNow+1).GetPriority();
        if (priority == 0.0)
          {
          //if this object finished early, don't keep going just for it's sake
          continue;
          }
        }
      everyone_done = false;
      break;
      }
    }
  iter->Delete();

  return everyone_done;
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::EndRenderEvent()
{
  DEBUGPRINT_PASSES
    (
     cerr << "ER " << this->Internal->DebugPass << endl;
     this->Internal->DebugPass++;
     );

  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!ren || !rw)
    {
    return;
    }

  //don't clear the screen, keep adding to what we already drew
  ren->EraseOff();
  rw->EraseOff();

  bool allDone = this->IsEveryoneDone()||this->Internal->StopNow;
  if (this->GetParallelHelper())
    {
    this->GetParallelHelper()->Reduce(allDone);
    }
  if (allDone)
    {
    DEBUGPRINT_PASSES(cerr << "ALLDONE SHOW" << endl;);
    this->Internal->StopNow = false;
    //we just drew the last frame, everyone has to start over next time
    this->Internal->StartOver = true;
    //bring back buffer forward to show what we drew
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
void vtkPrioritizedStreamer::RestartStreaming()
{
  this->Internal->StartOver = true;
}

//------------------------------------------------------------------------------
void vtkPrioritizedStreamer::StopStreaming()
{
  this->Internal->StopNow = true;
}

//------------------------------------------------------------------------------
void vtkPrioritizedStreamer::SetNumberOfPasses(int nv)
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
