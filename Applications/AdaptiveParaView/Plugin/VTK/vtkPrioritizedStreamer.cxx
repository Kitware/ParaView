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
#include "vtkPieceList.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDriver.h"
#include "vtkStreamingHarness.h"

vtkStandardNewMacro(vtkPrioritizedStreamer);

class Internals
{
public:
  Internals(vtkPrioritizedStreamer *owner)
  {
    this->Owner = owner;
    this->FirstPass = true;
  }
  ~Internals()
  {
  }

  vtkPrioritizedStreamer *Owner;
  bool FirstPass;
};

//----------------------------------------------------------------------------
vtkPrioritizedStreamer::vtkPrioritizedStreamer()
{
  this->Internal = new Internals(this);
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
bool vtkPrioritizedStreamer::IsFirstPass()
{
  if (this->Internal->FirstPass)
    {
    return true;
    }
  return false;
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
    vtkStreamingHarness *next = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();

    //check if anyone hasn't reached the last necessary and possible pass
    int passNow = next->GetPass();
    int maxPiece = next->GetNumberOfPieces();
    if (passNow < maxPiece)
      {
      vtkPieceList *pl = next->GetPieceList1();
      if (pl)
        {
        double priority = pl->GetPiece(passNow).GetPriority();
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
void vtkPrioritizedStreamer::ResetEveryone()
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
      p.SetPipelinePriority(harness->ComputePriority(i,max,1.0));

      double pbbox[6];
      double gConf = 1.0;
      double aMin = 1.0;
      double aMax = -1.0;
      double aConf = 1.0;
      harness->ComputeMetaInformation
        (i, max, 1.0,
         pbbox, gConf, aMin, aMax, aConf);
      double gPri = this->CalculateViewPriority(pbbox);
      p.SetViewPriority(gPri);

      pl->AddPiece(p);
      }
    pl->SortPriorities();

    //convert pass number to absolute piece number
    int pieceNext = pl->GetPiece(0).GetPiece();
    //cerr << "PASS " << 0 << " PIECE " << pieceNext << endl;
    harness->SetPiece(pieceNext);
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::AdvanceEveryone()
{
  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
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

    //increment to the next pass
    int maxPiece = harness->GetNumberOfPieces();
    int passNow = harness->GetPass();
    int passNext = passNow;
    if (passNow < maxPiece)
      {
      passNext++;
      }
    harness->SetPass(passNext);

    //map that to an absolute piece number
    int pieceNext = passNext;
    vtkPieceList *pl = harness->GetPieceList1();
    if (pl)
      {
      pieceNext = pl->GetPiece(passNext).GetPiece();
      }
    //cerr << "PASS " << passNext << " PIECE " << pieceNext << endl;
    harness->SetPiece(pieceNext);
   }

  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::FinalizeEveryone()
{
  return;
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::StartRenderEvent()
{
  vtkCollection *harnesses = this->GetHarnesses();
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!harnesses || !ren || !rw)
    {
    return;
    }

  if (this->IsRestart() || this->IsFirstPass())
    {
    //start off by clearing the screen
    ren->EraseOn();
    rw->EraseOn();

    //and telling all the harnesses to get ready
    this->ResetEveryone();

    //don't swap back to front automatically, only show what we've
    //drawn when the entire domain is covered
    rw->SwapBuffersOff();
    //TODO:Except for diagnostic mode where it is helpful to see
    //everything as it is drawn
    }

  this->Internal->FirstPass = false;
}

//----------------------------------------------------------------------------
void vtkPrioritizedStreamer::EndRenderEvent()
{
  vtkCollection *harnesses = this->GetHarnesses();
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!harnesses || !ren || !rw)
    {
    return;
    }

  //after first pass all subsequent renders can not clear
  //otherwise we erase the partial results we drew before
  ren->EraseOff();
  rw->EraseOff();

  this->AdvanceEveryone();

  if (this->IsEveryoneDone())
    {
    //we just drew the last frame everyone has to start over next time
    this->FinalizeEveryone();

    //bring back buffer forward to show what we drew
    rw->SwapBuffersOn();
    rw->Frame();

    //and mark that the next render should start over
    this->Internal->FirstPass = true;
    }
  else
    {
    //we haven't finished yet so schedule the next pass
    this->RenderEventually();
    }
}
