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
#include "vtkMultiResolutionStreamer.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkPieceCacheFilter.h"
#include "vtkPieceList.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDriver.h"
#include "vtkStreamingHarness.h"

vtkStandardNewMacro(vtkMultiResolutionStreamer);

class Internals
{
public:
  Internals(vtkMultiResolutionStreamer *owner)
  {
    this->Owner = owner;
    this->WendDone = true;
  }
  ~Internals()
  {
  }
  vtkMultiResolutionStreamer *Owner;
  bool WendDone;
};

//----------------------------------------------------------------------------
vtkMultiResolutionStreamer::vtkMultiResolutionStreamer()
{
  this->Internal = new Internals(this);
}

//----------------------------------------------------------------------------
vtkMultiResolutionStreamer::~vtkMultiResolutionStreamer()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkMultiResolutionStreamer::IsFirstPass()
{
  if (this->Internal->WendDone)
    {
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkMultiResolutionStreamer::IsWendDone()
{
  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return true;
    }

  bool everyone_finished_wend = true;
  vtkCollectionIterator *iter = harnesses->NewIterator();
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
    {
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();

    //check if anyone hasn't reached the end of the visible domain
    vtkPieceList *ToDo = harness->GetPieceList1();
    if (ToDo->GetNumberNonZeroPriority() > 0)
      {
      everyone_finished_wend = false;
      break;
      }
    }
  iter->Delete();

  return everyone_finished_wend;
}

//----------------------------------------------------------------------------
bool vtkMultiResolutionStreamer::IsEveryoneDone()
{
  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return true;
    }

  bool everyone_completely_done = true;
  vtkCollectionIterator *iter = harnesses->NewIterator();
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
    {
    vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
      (iter->GetCurrentObject());
    iter->GoToNextItem();

    vtkPieceList *ToDo = harness->GetPieceList1();
    if (ToDo->GetNumberNonZeroPriority() > 0)
      {
      everyone_completely_done = false;
      break;
      }

    if (harness->GetNoneToRefine() == false)
      {
      everyone_completely_done = false;
      break;
      }
    }
  iter->Delete();

  return everyone_completely_done;
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::PrepareFirstPass(bool forCamera)
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

    vtkPieceList *ToDo = harness->GetPieceList1();
    if (!ToDo)
      {
      //very first pass, start off with one piece at lowest res
      vtkPieceList *pl = vtkPieceList::New();
      vtkPiece p;
      p.SetResolution(0.0);
      p.SetPiece(0);
      p.SetNumPieces(1);
      pl->AddPiece(p);
      harness->SetPieceList1(pl);
      pl->Delete();
      ToDo = pl;

      //and initialize the other two queues
      pl = vtkPieceList::New();
      harness->SetPieceList2(pl);
      pl->Delete();
      }

    //combine empties that no longer matter
    this->Reap(harness);

    harness->SetNoneToRefine(false); //assume it isn't all done
    //split and refine some of the pieces in nextframe
    int numRefined = this->Refine(harness);
    if (numRefined==0)
      {
      harness->SetNoneToRefine(true);
      }

    //compute priorities for everything we are going to draw this wend
    for (int i = 0; i < ToDo->GetNumberOfPieces(); i++)
      {
      vtkPiece piece = ToDo->GetPiece(i);
      int p = piece.GetPiece();
      int np = piece.GetNumPieces();
      double res = piece.GetResolution();
      double priority = harness->ComputePriority(p, np, res);
      /*
      if (!priority)
        {
        cerr << "CHECKED PPRI OF " << p << "/" << np << "@" << res
             << " = " << priority << endl;
        }
      */
      piece.SetPipelinePriority(priority);

      double pbbox[6];
      double gConf = 1.0;
      double aMin = 1.0;
      double aMax = -1.0;
      double aConf = 1.0;
      harness->ComputeMetaInformation
        (p, np, res,
         pbbox, gConf, aMin, aMax, aConf);
      double gPri = this->CalculateViewPriority(pbbox);
      //cerr << "CHECKED VPRI OF " << p << "/" << np << "@" << res
      //<< " = " << gPri << endl;
      piece.SetViewPriority(gPri);

      if (forCamera)
        {
        //TODO: this whole forCamera and reapedflag business is a workaround
        //for the way refining/reaping can by cyclic and not terminate.
        //I would like to find a better termination condition and remove this.
        piece.SetReapedFlag(false);
        }

      ToDo->SetPiece(i, piece);
      }

    //sort them
    ToDo->SortPriorities();
    //cerr << "NUM PIECES " << ToDo->GetNumberOfPieces() << endl;

   }

  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::ChooseNextPieces()
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

    vtkPieceList *ToDo = harness->GetPieceList1();
    vtkPieceList *NextFrame = harness->GetPieceList2();
    if (ToDo->GetNumberNonZeroPriority() > 0)
      {
      vtkPiece p = ToDo->PopPiece();
      NextFrame->AddPiece(p);
      //adjust pipeline to draw the chosen piece
      /*
      cerr << "CHOSE "
           << p.GetPiece() << "/" << p.GetNumPieces()
           << "@" << p.GetResolution() << endl;
      */
      harness->SetPiece(p.GetPiece());
      harness->SetNumberOfPieces(p.GetNumPieces());
      harness->SetResolution(p.GetResolution());

      //TODO:
      //This should not be necessary, but the PieceCacheFilter is silently
      //producing the stale (lower res?) results without it.
      harness->ComputePriority(p.GetPiece(), p.GetNumPieces(), p.GetResolution());
      }
    }

  iter->Delete();
}

//----------------------------------------------------------------------------
int vtkMultiResolutionStreamer::Refine(vtkStreamingHarness *harness)
{
  //TODO: maxHeight must be common to both reap and refine and should be setable
  int maxHeight = 5;
  double res_delta = (1.0/maxHeight);

  vtkPieceList *ToDo = harness->GetPieceList1();
  vtkPieceList *NextFrame = harness->GetPieceList2();
  vtkPieceList *ToSplit = vtkPieceList::New();

  int numSplittable = 0;
  while (NextFrame->GetNumberOfPieces() != 0)
    {
    vtkPiece piece = NextFrame->PopPiece();
    double res = piece.GetResolution();
    double priority = piece.GetPriority();
    double mdr = 1.0;
    bool reaped = piece.GetReapedFlag();
    if (!reaped &&
        priority > 0.0 &&
        (res+res_delta <= 1.0))
      {
      numSplittable++;
      ToSplit->AddPiece(piece);
      }
    else
      {
      ToDo->AddPiece(piece);
      }
    }

  //loop through the splittable pieces, and split some of them
  int numSplit = 0;
  //TODO: maxSplits should be setable
  int maxSplits = 2;
  //TODO: Degree could be variable
  int degree = 2;
  for (;
       numSplit < maxSplits && ToSplit->GetNumberOfPieces() != 0;
       numSplit++)
  {
    //get the next piece
    vtkPiece piece = ToSplit->PopPiece();
    int p = piece.GetPiece();
    int np = piece.GetNumPieces();
    double res = piece.GetResolution();

    /*
    cerr << "SPLIT "
         << p << "/" << np
         << " -> ";
    */

    //compute next resolution to request for it
    double resolution = res + res_delta;
    if (resolution > 1.0)
      {
      resolution = 1.0;
      }

    //split it N times
    int nrNP = np*degree;
    for (int child=0; child < degree; child++)
      {
      int nrA = p * degree + child;

      vtkPiece pA;
      pA.SetPiece(nrA);
      pA.SetNumPieces(nrNP);
      pA.SetResolution(resolution);

      //cerr << nrA << "/" << nrNP << "@" << resolution << " & ";
      ToDo->AddPiece(pA);
      }

    //cerr << endl;
    }

  //put any pieces we did not split back into next wend
  ToDo->MergePieceList(ToSplit);
  ToSplit->Delete();

  return numSplit;
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::Reap(vtkStreamingHarness *harness)
{
  vtkPieceList *ToDo = harness->GetPieceList1();
  int important = ToDo->GetNumberNonZeroPriority();
  int total = ToDo->GetNumberOfPieces();

  //cerr << "ToDo:" << endl;
  //ToDo->Print();

  if (important == total)
    {
    return;
    }

  //TODO: maxHeight must be common to both reap and refine and should be setable
  int maxHeight = 5;
  double res_delta = (1.0/maxHeight);

  vtkPieceList *toMerge = vtkPieceList::New();
  for (int i = total-1; i>=important; --i)
    {
    vtkPiece piece = ToDo->PopPiece(i);
    toMerge->AddPiece(piece);
    }

 vtkPieceList *merged = vtkPieceList::New();

  bool done = false;
  while (!done)
    {
    int mcount = 0;
    //pick a piece
    while (toMerge->GetNumberOfPieces()>0)
      {
      vtkPiece piece = toMerge->PopPiece();
      int p = piece.GetPiece();
      int np = piece.GetNumPieces();
      bool found = false;

      //look for a piece that can be merged with it
      for (int j = 0; j < toMerge->GetNumberOfPieces(); j++)
        {
        vtkPiece other = toMerge->GetPiece(j);
        int p2 = other.GetPiece();
        int np2 = other.GetNumPieces();
        if ((np==np2) &&
            (p/2==p2/2) ) //TODO, when Degree==N!=2, have to round up all N sibs
          {
          piece.SetPiece(p/2);
          piece.SetNumPieces(np/2);
          double res = piece.GetResolution()-res_delta;
          if (res < 0.0)
            {
            res = 0.0;
            }
          piece.SetResolution(res);
          piece.SetPipelinePriority(0.0);
          piece.SetReapedFlag(true);

          /*
          cerr << "REAP "
               << p << "&" << p2 << "/" << np
               << " -> "
               << p/2 << "/" << np/2 << "@" << res << endl;
          */

          merged->AddPiece(piece);
          toMerge->RemovePiece(j);
          found = true;
          mcount++;

          vtkPieceCacheFilter *pcf = harness->GetCacheFilter();
          if (pcf)
            {
            int index;
            index = pcf->ComputeIndex(p,np);
            pcf->DeletePiece(index);
            index = pcf->ComputeIndex(p2,np);
            pcf->DeletePiece(index);
            }
          break;
          }
        }
      if (!found)
        {
        //cerr << "no match";
        merged->AddPiece(piece);
        }
      //cerr << endl;
      }
    if (mcount==0)
      {
      done = true;
      }

    toMerge->MergePieceList(merged);
    }

  //add whatever remains back in
  ToDo->MergePieceList(toMerge);
  toMerge->Delete();
  merged->Delete();
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::StartRenderEvent()
{
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!ren || !rw)
    {
    return;
    }

  bool forCamera = this->IsRestart();
  if (forCamera || this->IsFirstPass())
    {
    //start off by clearing the screen
    ren->EraseOn();
    rw->EraseOn();

    //and telling all the harnesses to get ready
    this->PrepareFirstPass(forCamera);

    //don't swap back to front automatically, only show what we've
    //drawn when the entire domain is covered
    rw->SwapBuffersOff();
    //TODO:Except for diagnostic mode where it is helpful to see
    //everything as it is drawn
    }

  //ask everyone to choose the next piece to draw
  this->ChooseNextPieces();

  //assume that we are not done covering all the domains
  this->Internal->WendDone = false;
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::EndRenderEvent()
{
  vtkRenderer *ren = this->GetRenderer();
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!ren || !rw)
    {
    return;
    }

  //after first pass all subsequent renders can not clear
  //otherwise we erase the partial results we drew before
  ren->EraseOff();
  rw->EraseOff();

 if (this->IsEveryoneDone())
    {
    //cerr << "EVERYONE DONE" << endl;
    //next pass should start with clear screen
    this->Internal->WendDone = true;

    //bring back buffer forward to show what we drew
    rw->SwapBuffersOn();
    rw->Frame();
    }
  else
    {
    if (this->IsWendDone())
      {
      //cerr << "START NEXT WEND" << endl;
      //next pass should start with clear screen
      this->Internal->WendDone = true;

      //everyone has covered their domain
      //bring back buffer forward to show what we drew
      rw->SwapBuffersOn();
      rw->Frame();
      }

    //there is more to draw, so keep going
    this->RenderEventually();
    }
}
