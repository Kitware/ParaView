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
#include "vtkParallelStreamHelper.h"
#include "vtkPieceCacheFilter.h"
#include "vtkPieceList.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDriver.h"
#include "vtkStreamingHarness.h"
#include "vtkVisibilityPrioritizer.h"

#define DEBUGPRINT_PASSES( arg ) ;
#define DEBUGPRINT_PRIORITY( arg ) ;
#define DEBUGPRINT_REFINE( arg ) ;

vtkStandardNewMacro(vtkMultiResolutionStreamer);

class vtkMultiResolutionStreamer::Internals
{
public:
  Internals(vtkMultiResolutionStreamer *owner)
  {
    this->Owner = owner;
    this->StartOver = true;
    this->StopNow = false;
    this->RefineNow = false;
    this->CoarsenNow = false;
  }
  ~Internals()
  {
  }
  vtkMultiResolutionStreamer *Owner;
  bool StartOver;
  bool StopNow;
  bool RefineNow;
  bool CoarsenNow;
};

//----------------------------------------------------------------------------
vtkMultiResolutionStreamer::vtkMultiResolutionStreamer()
{
  this->Internal = new Internals(this);
  this->PipelinePrioritization = 1;
  this->ViewPrioritization = 1;
  this->ProgressionMode = 0;
  this->RefinementDepth = 5;
  this->DepthLimit = 15;
  this->MaxSplits = 8;
  this->Interacting = false;

  //0.666 = assume i,j,k about same, then 3rd root to get edge, and square to get
  //numcells on near side
  this->CellPixelFactor = 0.666;
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
void vtkMultiResolutionStreamer::AddHarnessInternal(vtkStreamingHarness *harness)
{
  vtkPieceCacheFilter *pcf = harness->GetCacheFilter();
  if (pcf)
    {
    pcf->SetCacheSize(this->CacheSize);
    }
  harness->SetPass(0);
  harness->SetNumberOfPieces(1);
  harness->SetPiece(0);
  harness->SetResolution(0.0);
}

//----------------------------------------------------------------------------
bool vtkMultiResolutionStreamer::IsFirstPass()
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
void vtkMultiResolutionStreamer::PrepareFirstPass()
{
  vtkCollection *harnesses = this->GetHarnesses();
  if (!harnesses)
    {
    return;
    }

  int manualCommand = STAY;
  if (this->Internal->RefineNow)
    {
    this->Internal->RefineNow = false;
    manualCommand = ADVANCE;
    }
  if (this->Internal->CoarsenNow)
    {
    this->Internal->CoarsenNow = false;
    manualCommand = COARSEN;
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

    vtkPieceList *ToDo = harness->GetPieceList1();
    if (!ToDo)
      {
      //very first pass, start off with one piece at lowest res
      vtkPiece p;
      p.SetResolution(0.0);
      p.SetPiece(0);
      p.SetNumPieces(1);

      //initialize the "this set of passes" and "next set passes" queues
      vtkPieceList *pl = vtkPieceList::New(); //ToDo == "this set of passes"
      harness->SetPieceList1(pl);
      pl->Delete();
      ToDo = pl;
      pl = vtkPieceList::New(); //NextFrame == "next set of passes"
      pl->AddPiece(p);
      harness->SetPieceList2(pl);
      pl->Delete();
      }

    //at this point TD is either empty or has only unimportant pieces in it
    //and NF has all of the pieces we drew last time

    //don't prevent refinement of pieces that are 'unimportant' just because
    //they are in append slot
    vtkPieceList *NextFrame = harness->GetPieceList2();
    vtkPieceList *tmp = vtkPieceList::New();
    vtkPieceCacheFilter *pcf = harness->GetCacheFilter();
    while (ToDo->GetNumberOfPieces() != 0)
      {
      vtkPiece p = ToDo->PopPiece();
      if (p.GetCachedPriority() == 0.0)
        {
        p.SetCachedPriority(1.0);
        NextFrame->AddPiece(p);
        }
      else
        {
        tmp->AddPiece(p);
        }
      }
    ToDo->MergePieceList(tmp);
    tmp->Delete();

    //combine empties that no longer matter
    this->Reap(harness); //merges unimportant pieces left in TD
    this->PixelBackoff(harness); //coarsens pieces that are over pixel resolution

    //either refine, coarsen or leave alone the pieces depending on mode
    if ((this->ProgressionMode == MANUAL && manualCommand == COARSEN))
      {
      this->Coarsen(harness); //merges pieces in NF and empties NF onto TD
      }
    if (!this->Interacting &&
        (this->ProgressionMode == AUTOMATIC ||
         (this->ProgressionMode == MANUAL && manualCommand == ADVANCE))
        )
      {
      this->Refine(harness); //splits pieces in NF and empties NF onto TD
      }
    if (this->Interacting ||
        (this->ProgressionMode != AUTOMATIC && manualCommand == STAY))
      {
      //we weren't told to refine or coarsen anything
      //simply dump NF onto TD to keep redrawing it
      ToDo->MergePieceList(harness->GetPieceList2());
      }

    //at this point NF is empty and TD has all of the pieces we are going to
    //draw in this set of passes, including the unimportant ones that we will
    //skip

    //compute priorities for everything we are going to draw this wend
    for (int i = 0; i < ToDo->GetNumberOfPieces(); i++)
      {
      vtkPiece piece = ToDo->GetPiece(i);
      int p = piece.GetPiece();
      int np = piece.GetNumPieces();
      double res = piece.GetResolution();
      double priority = 1.0;
      if (this->PipelinePrioritization)
        {
        priority = harness->ComputePiecePriority(p, np, res);
        }
      DEBUGPRINT_PRIORITY
        (
         if (!priority)
           {
           cerr << "CHECKED PPRI OF " << p << "/" << np << "@" << res
                << " = " << priority << endl;
           }
         );
      piece.SetPipelinePriority(priority);

      double pbbox[6];
      double gConf = 1.0;
      double aMin = 1.0;
      double aMax = -1.0;
      double aConf = 1.0;
      unsigned long numCells = 0;
      unsigned long int nPix = 0;
      double pNormal[3];
      double *normalresult = pNormal;
      harness->ComputePieceMetaInformation
        (p, np, res,
         pbbox, gConf, aMin, aMax, aConf,
         numCells, &normalresult);
      double gPri = 1.0;
      piece.SetReachedLimit(false);
      if (res >= 1.0)
        {
        piece.SetReachedLimit(true);
        }
      if (this->ViewPrioritization && res<1.0)
        {
        nPix = this->ComputePixelCount(pbbox);
        gPri = this->CalculateViewPriority(pbbox, normalresult);
        double nc = (double)numCells;
        double side = pow(nc, this->CellPixelFactor);
        numCells = (unsigned long)side;
        //cerr << p << "/" << np << "@" << res
        //       << " " << numCells << " vs " << nPix << endl;
        if (numCells > nPix)
          {
          //cerr << p << "/" << np << "@" << res
          //<< " reached limit " << numCells << ">" << nPix << endl;
          piece.SetReachedLimit(true);
          }
        }
      DEBUGPRINT_PRIORITY
        (
         if (!gPri)
           {
           cerr << "CHECKED VPRI OF " << p << "/" << np << "@" << res
                << " = " << gPri << endl;
           }
         );
      piece.SetViewPriority(gPri);

      //don't use cached priority calculated last pass
      piece.SetCachedPriority(1.0);

      if (!piece.GetPriority() && pcf)
        {
        //remove unimportant pieces from the cache
        int index = pcf->ComputeIndex(p,np);
        pcf->DeletePiece(index);
        }
      ToDo->SetPiece(i, piece);
      }

    //combine everything we have cached to render it all in the first pass
    harness->Append();
    //and don't waste passes re-rendering its content
    for (int i = 0; i < ToDo->GetNumberOfPieces(); i++)
      {
      vtkPiece piece = ToDo->GetPiece(i);
      int p = piece.GetPiece();
      int np = piece.GetNumPieces();
      double res = piece.GetResolution();
      if (harness->InAppend(p,np,res))
        {
        //cerr << "SKIP " << p << "/" << np << "@" << res << endl;
        piece.SetCachedPriority(0.0);
        }
      else
        {
        piece.SetCachedPriority(1.0);
        }

      ToDo->SetPiece(i, piece);
      }

    //sort list of pieces in most to least important order
    ToDo->SortPriorities();

    //setup pipeline to show the first one in the upcoming render
    vtkPiece p = ToDo->GetPiece(0);
    harness->SetPiece(p.GetPiece());
    harness->SetNumberOfPieces(p.GetNumPieces());
    harness->SetResolution(p.GetResolution());
    harness->ComputePiecePriority(p.GetPiece(), p.GetNumPieces(),
                                  p.GetResolution());
    DEBUGPRINT_PASSES
      (
       cerr << "FIRST PIECE " << p.GetPiece() << "/" << p.GetNumPieces()
       << "@" << p.GetResolution() << endl;
       );
    }

  iter->Delete();
}

//----------------------------------------------------------------------------
int vtkMultiResolutionStreamer::Refine(vtkStreamingHarness *harness)
{
  if (harness->GetLockRefinement())
    {
    return 0;
    }

  double res_delta = (1.0/this->RefinementDepth);

  vtkPieceList *ToDo = harness->GetPieceList1();
  vtkPieceList *NextFrame = harness->GetPieceList2();
  vtkPieceList *ToSplit = vtkPieceList::New();

  //cerr << "PRE REFINE:" << endl;
  //cerr << "TODO:" << endl;
  //ToDo->Print();
  //cerr << "NEXT:" << endl;
  //NextFrame->Print();

  //find candidates for refinement
  double maxRes = 1.0;
  if (this->DepthLimit>0.0)
    {
    maxRes = res_delta*this->DepthLimit;
    maxRes = (maxRes<1.0?maxRes:1.0);
    }
  int numSplittable = 0;
  while (NextFrame->GetNumberOfPieces() != 0)
    {
    vtkPiece piece = NextFrame->PopPiece();
    double res = piece.GetResolution();
    double priority = piece.GetPriority();
    if ((priority > 0.0) &&
        (res+res_delta <= maxRes) &&
        (!piece.GetReachedLimit()))
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
  //TODO:
  //Degree is potentially variable, but it would take some work
  int degree = 2;
  for (;
       numSplit < this->MaxSplits
         && ToSplit->GetNumberOfPieces() != 0;
       numSplit++)
    {
    //get the next piece
    vtkPiece piece = ToSplit->PopPiece();
    int p = piece.GetPiece();
    int np = piece.GetNumPieces();
    double res = piece.GetResolution();

    DEBUGPRINT_REFINE
      (
       cerr << "SPLIT "
       << p << "/" << np
       << " -> ";
       );

    //remove it from the cache filter
    vtkPieceCacheFilter *pcf = harness->GetCacheFilter();
    if (pcf)
      {
      int index = pcf->ComputeIndex(p,np);
      pcf->DeletePiece(index);
      }

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

      DEBUGPRINT_REFINE
        (
         cerr
         << nrA << "/" << nrNP << "@" << resolution
         << (child<degree-1?" & ":"");
         );

      //put the split piece back
      ToDo->AddPiece(pA);
      }

    DEBUGPRINT_REFINE
      (
       cerr << endl;
       );
    }

  //put any pieces we did not split back
  ToDo->MergePieceList(ToSplit);
  ToSplit->Delete();

  //cerr << "POST REFINE:" << endl;
  //cerr << "TODO:" << endl;
  //ToDo->Print();
  //cerr << "NEXT:" << endl;
  //NextFrame->Print();

  return numSplit;
}

//----------------------------------------------------------------------------
int vtkMultiResolutionStreamer::Coarsen(vtkStreamingHarness *harness)
{
  if (harness->GetLockRefinement())
    {
    return 0;
    }

  int cnt = 0;

  //find every pair of siblings and merge them
  std::map<int, vtkPieceList* > levels;
  std::map<int, vtkPieceList* >::iterator iter;

  vtkPieceList *ToDo = harness->GetPieceList1();
  vtkPieceList *NextFrame = harness->GetPieceList2();
  NextFrame->MergePieceList(ToDo);

  //sort pieces according to levels
  while (NextFrame->GetNumberOfPieces())
    {
    vtkPiece piece = NextFrame->PopPiece();
    int np = piece.GetNumPieces();
    iter = levels.find(np);
    vtkPieceList *npl = NULL;
    if (iter == levels.end())
      {
      npl = vtkPieceList::New();
      levels[np] = npl;
      }
    else
      {
      npl = iter->second;
      }
    //cerr << "adding " << piece.GetPiece() << " to level " << np << endl;
    npl->AddPiece(piece);
    }

  double res_delta = (1.0/this->RefinementDepth);

  for(iter = levels.begin(); iter != levels.end(); iter++)
    {
    //cerr << "looking at level " << iter->first << endl;
    vtkPieceList *npl = iter->second;
    while (npl->GetNumberOfPieces())
      {
      vtkPiece piece = npl->PopPiece();
      int p = piece.GetPiece();
      int np = piece.GetNumPieces();
      bool found = false;
      for (int i = 0; i < npl->GetNumberOfPieces(); i++)
        {
        vtkPiece other = npl->GetPiece(i);
        int p2 = other.GetPiece();
        if (p/2 == p2/2)
          {
          cnt++;
          //cerr << "joined " << p << " and " << p2 << " / " << np << endl;
          piece.SetPiece(p/2);
          piece.SetNumPieces(np/2);
          double opp;
          opp = piece.GetPipelinePriority();
          //you down with opp?
          if (opp > piece.GetPipelinePriority())
            {
            piece.SetPipelinePriority(opp);
            //yeah you know me.
            }
          opp = piece.GetViewPriority();
          if (opp > piece.GetViewPriority())
            {
            piece.SetViewPriority(opp);
            }
          double res = piece.GetResolution()-res_delta;
          piece.SetResolution(res);
          NextFrame->AddPiece(piece);
          npl->RemovePiece(i);
          found = true;

          //remove it from the cache
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
        //cerr << p << "/" << np << " no match found" << endl;
        NextFrame->AddPiece(piece);
        }
      }
    npl->Delete();
    }

  levels.clear();
  ToDo->MergePieceList(NextFrame);
  return cnt;
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::PixelBackoff(vtkStreamingHarness *harness)
{
  double res_delta = (1.0/this->RefinementDepth);

  //find pieces that are two levels too refined for this viewpoint
  vtkPieceList *tmp = vtkPieceList::New();
  vtkPieceList *tooHigh = vtkPieceList::New();
  vtkPieceList *NextFrame = harness->GetPieceList2();
  tmp->MergePieceList(NextFrame);
  while (tmp->GetNumberOfPieces())
    {
    vtkPiece piece = tmp->PopPiece();
    if (piece.GetReachedLimit())
      {
      double res = piece.GetResolution();
      res = res - res_delta;
      if (res < 0.0)
        {
        NextFrame->AddPiece(piece);
        continue;
        }
      int p = piece.GetPiece();
      int np = piece.GetNumPieces();
      //cerr << p << "/" << np << "@" << res+res_delta << " is over";
      p = p/2;
      np = np/2;
      double pbbox[6];
      double gConf = 1.0;
      double aMin = 1.0;
      double aMax = -1.0;
      double aConf = 1.0;
      unsigned long numCells = 0;
      double pNormal[3];
      double *normalresult = pNormal;
      harness->ComputePieceMetaInformation
        (p, np, res,
         pbbox, gConf, aMin, aMax, aConf,
         numCells, &normalresult);
      unsigned long int nPix = 0;
      nPix = this->ComputePixelCount(pbbox);
      double nc = (double)numCells;
      double side = pow(nc, this->CellPixelFactor);
      numCells = (unsigned long)side;
      if (numCells > nPix)
        {
        //cerr << p << "/" << np << "@" << res << " is too";
        tooHigh->AddPiece(piece);
        }
      else
        {
        NextFrame->AddPiece(piece);
        }
      //cerr << endl;
      }
    else
      {
      NextFrame->AddPiece(piece);
      }
    }
  //cerr << "CANDIDATES ARE:" << endl;
  //tooHigh->Print();

  //cerr << "PRE REAP:" << endl;
  //cerr << "TODO:" << endl;
  //tooHigh->Print();

  vtkPieceList *toMerge = vtkPieceList::New();
  toMerge->MergePieceList(tooHigh);

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
            (p/2==p2/2) )
          //TODO, when Degree==N!=2, have to round up all N sibs
          {
          //make the parent of the two pieces
          piece.SetPiece(p/2);
          piece.SetNumPieces(np/2);
          double res = piece.GetResolution()-res_delta;
          if (res < 0.0)
            {
            res = 0.0;
            }
          piece.SetResolution(res);
          //cerr << "JOIN "
          //     << p << "&" << p2 << "/" << np
          //     << " -> "
          //     << p/2 << "/" << np/2 << "@" << res << endl;
          //cerr << "-------------------------------------------------------------------------------" << endl;
          //save it
          tmp->AddPiece(piece);
          //get rid of the second half of the piece
          toMerge->RemovePiece(j);
          found = true;
          mcount++;

          //remove the cached data for the merged pieces
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
        //put the candidate back
        merged->AddPiece(piece);
        }
      }
    if (mcount==0)
      {
      done = true;
      }
    toMerge->MergePieceList(merged);
    }

  //add the merged and nonmergable pieces back
  NextFrame->MergePieceList(toMerge);
  NextFrame->MergePieceList(tmp);
  tooHigh->Delete();
  toMerge->Delete();
  merged->Delete();
  tmp->Delete();

}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::Reap(vtkStreamingHarness *harness)
{
  double res_delta = (1.0/this->RefinementDepth);
  vtkPieceList *ToDo = harness->GetPieceList1();
  int important = ToDo->GetNumberNonZeroPriority();
  int total = ToDo->GetNumberOfPieces();
  if (important == total)
    {
    return;
    }

  //cerr << "PRE REAP:" << endl;
  //cerr << "TODO:" << endl;
  //ToDo->Print();

  //double res_delta = (1.0/this->RefinementDepth);

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
            (p/2==p2/2) )
          //TODO, when Degree==N!=2, have to round up all N sibs
          {
          //make the parent of the two pieces
          piece.SetPiece(p/2);
          piece.SetNumPieces(np/2);
          double res = piece.GetResolution()-res_delta;
          if (res < 0.0)
            {
            res = 0.0;
            }
          piece.SetResolution(res);
          piece.SetPipelinePriority(0.0);
          DEBUGPRINT_REFINE
            (
             cerr << "REAP "
             << p << "&" << p2 << "/" << np
             << " -> "
             << p/2 << "/" << np/2 << "@" << res << endl;
             );

          //save it
          merged->AddPiece(piece);
          //get rid of the second half of the piece
          toMerge->RemovePiece(j);
          found = true;
          mcount++;

          //remove the cached data for the merged pieces
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
        //put the candidate back
        merged->AddPiece(piece);
        }
      }
    if (mcount==0)
      {
      done = true;
      }
    //else
    //{
    //  try to merge the ones we just merged
    //}

    toMerge->MergePieceList(merged);
    }

  //add the merged and nonmergable pieces back
  ToDo->MergePieceList(toMerge);
  toMerge->Delete();
  merged->Delete();

  //cerr << "POST REAP:" << endl;
  //cerr << "TODO:" << endl;
  //ToDo->Print();
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::PrepareNextPass()
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

    vtkPieceList *ToDo = harness->GetPieceList1();
    vtkPieceList *NextFrame = harness->GetPieceList2();
    if (ToDo && NextFrame && (ToDo->GetNumberNonZeroPriority() > 0))
      {
      vtkPiece p = ToDo->PopPiece();
      NextFrame->AddPiece(p);
      //adjust pipeline to draw the chosen piece
      DEBUGPRINT_PASSES
        (
         cerr << "CHOSE "
         << p.GetPiece() << "/" << p.GetNumPieces()
         << "@" << p.GetResolution() << endl;
         );
      harness->SetPiece(p.GetPiece());
      harness->SetNumberOfPieces(p.GetNumPieces());
      harness->SetResolution(p.GetResolution());

      //TODO:
      //This should not be necessary, but the PieceCacheFilter is silently
      //producing the stale (lower res?) results without it.
      harness->ComputePiecePriority(p.GetPiece(), p.GetNumPieces(),
                                    p.GetResolution());
      }
    }

  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::StartRenderEvent()
{
  DEBUGPRINT_PASSES
    (
     cerr << "SR " << endl;
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
       );

    //show whatever we partially drew before the camera moved
    //this->CopyBackBufferToFront();

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

    //compute priority of subsequent set of passes
    //set pipeline to show the most important one this pass
    this->PrepareFirstPass();

    //above figures out what the subsequent set of passes should do,
    //but doesn't actually update the pipeline to point to the first passes
    //work. do that like so here.
    //this->PrepareNextPass();
    }
  else
    {
    //ask everyone to choose the next piece to draw
    this->PrepareNextPass();
    }

  //don't swap back to front automatically
  //only update the screen once the last piece is drawn
  if (rw)
    {
    rw->SwapBuffersOff();
    }

  //assume that we are not done covering all the domains
  this->Internal->StartOver = false;
}

//----------------------------------------------------------------------------
bool vtkMultiResolutionStreamer::AnyToRefine(vtkStreamingHarness *harness)
{
  if (harness->GetLockRefinement())
    {
    return false;
    }

  double res_delta = (1.0/this->RefinementDepth);
  vtkPieceList *NextFrame = harness->GetPieceList2();
  double maxRes = 1.0;
  if (this->DepthLimit>0.0)
    {
    maxRes = res_delta*this->DepthLimit;
    maxRes = (maxRes<1.0?maxRes:1.0);
    }
  for (int i = 0; i < NextFrame->GetNumberOfPieces(); i++)
    {
    vtkPiece piece = NextFrame->GetPiece(i);
    double res = piece.GetResolution();
    double priority = piece.GetPriority();
    if ((priority > 0.0) &&
        (res+res_delta <= maxRes) &&
        !piece.GetReachedLimit())
      {
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkMultiResolutionStreamer::IsCompletelyDone()
{
  if (this->Internal->StopNow)
    {
    return true;
    }

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
    if (!harness->GetEnabled())
      {
      continue;
      }

    //check if anyone hasn't reached the end of the visible domain
    vtkPieceList *ToDo = harness->GetPieceList1();
    if (ToDo && ToDo->GetNumberNonZeroPriority() > 0)
      {
      everyone_completely_done = false;
      break;
      }

    //look for any in nextframe that could be refined
    if (this->ProgressionMode == AUTOMATIC)
      {
      if (this->AnyToRefine(harness))
        {
        everyone_completely_done = false;
        break;
        }
      }
    }
  iter->Delete();

  return everyone_completely_done;
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
    if (!harness->GetEnabled())
      {
      continue;
      }

    //check if anyone hasn't reached the end of the visible domain
    vtkPieceList *ToDo = harness->GetPieceList1();
    if (ToDo && ToDo->GetNumberNonZeroPriority() > 0)
      {
      everyone_finished_wend = false;
      break;
      }
    }
  iter->Delete();

  return everyone_finished_wend;
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::EndRenderEvent()
{
  DEBUGPRINT_PASSES
    (
     cerr << "ER " << endl;
     );

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

  bool allDone = this->IsCompletelyDone();
  if (this->GetParallelHelper())
    {
    this->GetParallelHelper()->Reduce(allDone);
    }
  if (allDone)
    {
    DEBUGPRINT_PASSES(cerr << "ALL DONE" << endl;);
    this->Internal->StopNow = false;
    //next pass should start with clear screen
    this->Internal->StartOver = true;
    //bring back buffer forward to show what we drew
    this->CopyBackBufferToFront();
    }
  else
    {
    bool wendDone = this->IsWendDone();
    if (this->GetParallelHelper())
      {
      this->GetParallelHelper()->Reduce(wendDone);
      }
    if (wendDone)
      {
      DEBUGPRINT_PASSES(cerr << "WEND DONE" << endl;);
      //next pass should start with clear screen
      this->Internal->StartOver = true;
      }
    if (wendDone || this->DisplayFrequency==1)
      {
      this->CopyBackBufferToFront();
      }

    //there is more to draw, so keep going
    this->RenderEventually();
    }
}

//------------------------------------------------------------------------------
void vtkMultiResolutionStreamer::RestartStreaming()
{
  this->Internal->StartOver = true;
}

//------------------------------------------------------------------------------
void vtkMultiResolutionStreamer::StopStreaming()
{
  this->Internal->StopNow = true;
}

//------------------------------------------------------------------------------
void vtkMultiResolutionStreamer::Refine()
{
  this->Internal->RefineNow = true;
}

//------------------------------------------------------------------------------
void vtkMultiResolutionStreamer::Coarsen()
{
  this->Internal->CoarsenNow = true;
}

//----------------------------------------------------------------------------
void vtkMultiResolutionStreamer::SetBackFaceFactor(double value)
{
  vtkVisibilityPrioritizer *viewsorter = this->GetVisibilityPrioritizer();
  if (viewsorter)
    {
    viewsorter->SetBackFaceFactor(value);
    }
}

//----------------------------------------------------------------------------
double vtkMultiResolutionStreamer::GetBackFaceFactor()
{
  vtkVisibilityPrioritizer *viewsorter = this->GetVisibilityPrioritizer();
  if (viewsorter)
    {
    return viewsorter->GetBackFaceFactor();
    }
  return -1.0;
}
