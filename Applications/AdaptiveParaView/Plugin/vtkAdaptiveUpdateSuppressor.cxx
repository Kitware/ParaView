/*=========================================================================

  Program:   ParaView
  Module:    vtkAdaptiveUpdateSuppressor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdaptiveUpdateSuppressor.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkUpdateSuppressorPipeline.h"
#include "vtkPieceCacheFilter.h"
#include "vtkPolyData.h"
#include "vtkAdaptiveOptions.h"

#include "vtkPiece.h"
#include "vtkPieceList.h"
#include "vtkBoundingBox.h"
#include "vtkDoubleArray.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIMoveData.h"
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkAdaptiveUpdateSuppressor, "1.2");
vtkStandardNewMacro(vtkAdaptiveUpdateSuppressor);

#if 0
#define DEBUGPRINT_EXECUTION(arg)\
  arg;
#else
#define DEBUGPRINT_EXECUTION(arg)\
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }
#endif

#define PIECE this->PieceInfo[0]
#define NUMPIECES this->PieceInfo[1]
#define RESOLUTION this->PieceInfo[2]
#define PRIORITY this->PieceInfo[3]
#define HIT this->PieceInfo[4]
#define FROMAPPEND this->PieceInfo[5]

#define ALLDONE this->StateInfo[0]
#define WENDDONE this->StateInfo[1]
#define NEXTAPPEND this->StateInfo[2]

//----------------------------------------------------------------------------
vtkAdaptiveUpdateSuppressor::vtkAdaptiveUpdateSuppressor()
{
  this->PQ1 = vtkPieceList::New();
  this->PQ2 = vtkPieceList::New();
  this->ToSplit = vtkPieceList::New();
  this->ToDo = this->PQ1;
  this->NextFrame = this->PQ2;

  this->PieceCacheFilter = NULL;
  this->MPIMoveData = NULL;

  this->Height = 5;
  this->Degree = 2;
  this->MaxSplits = -1;
  this->MaxDepth = -1;

  ///
  PIECE = 0.0;
  NUMPIECES = 1.0;
  RESOLUTION = 0.0;
  PRIORITY = 1.0;
  HIT = 0.0;
  FROMAPPEND = 0.0;

  ///
  ALLDONE = 1;
  WENDDONE = 1;
  NEXTAPPEND = 1;
}

//----------------------------------------------------------------------------
vtkAdaptiveUpdateSuppressor::~vtkAdaptiveUpdateSuppressor()
{
  this->PQ1->Delete();
  this->PQ2->Delete();
  this->ToSplit->Delete();
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::ForceUpdate()
{    
  int gPiece = this->UpdatePiece;
  int gPieces = this->UpdateNumberOfPieces;

  if (NEXTAPPEND || FROMAPPEND)
    {
    NEXTAPPEND = 0;
    if (this->PieceCacheFilter->GetAppendedData())
      {
      DEBUGPRINT_EXECUTION(
                           cerr << "SUS(" << this << ") ForceUpdate FROM APPEND" << endl;
                           );

      vtkDataObject *output = this->GetOutput();
      output->ShallowCopy(this->PieceCacheFilter->GetAppendedData());
      this->PipelineUpdateTime.Modified();
      return;
      }
    }

  // Make sure that output type matches input type
  this->UpdateInformation();

  vtkDataObject *input = this->GetInput();
  if (input == 0)
    {
    vtkErrorMacro("No valid input.");
    return;
    }

  int p = PIECE;
  int np = NUMPIECES;
  double resolution = RESOLUTION;

  DEBUGPRINT_EXECUTION(
  cerr << "SUS(" << this << ") ForceUpdate " 
       << p << "/" << np << "@" << resolution << endl;
  );
  gPiece = this->UpdatePiece*np+p;
  gPieces = this->UpdateNumberOfPieces*np;

  vtkInformation* info = input->GetPipelineInformation();
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      vtkExecutive::PRODUCER()->GetExecutive(info));
  if (sddp)
    {
    sddp->SetUpdateResolution(info, resolution);
    sddp->SetUpdateExtent(info,
                          gPiece, 
                          gPieces, 
                          0);
    this->ComputePriority(p,np,resolution);
    }
  else
    {
    vtkErrorMacro("Input has invalid Executive");
    return;
    }

  if (this->UpdateTimeInitialized)
    {
    info->Set(vtkCompositeDataPipeline::UPDATE_TIME_STEPS(), &this->UpdateTime, 1);
    }
  
  input->Update();

  vtkDataObject *output = this->GetOutput();
  output->ShallowCopy(input);
  output->GetInformation()->Copy(input->GetInformation());

  this->PipelineUpdateTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::ClearPriorities()
{
  DEBUGPRINT_EXECUTION(
  cerr << "SUS(" << this << ") CLEAR PRIORITIES " << endl;
                       );

  this->ToDo->Clear();
  this->NextFrame->Clear();
  this->ToSplit->Clear();
  ALLDONE = WENDDONE = 0;
  NEXTAPPEND = 1;

  PIECE = 0.0;
  NUMPIECES = 1.0;
  RESOLUTION = 0.0;
  PRIORITY = 1.0;
  HIT = 0.0;
  FROMAPPEND = 0.0;
  vtkPiece *initialPiece = vtkPiece::New();
  initialPiece->SetPiece(PIECE);
  initialPiece->SetNumPieces(NUMPIECES);
  initialPiece->SetResolution(RESOLUTION);
  initialPiece->SetPriority(PRIORITY);
  this->ToDo->AddPiece(initialPiece);
  initialPiece->Delete();
}

//----------------------------------------------------------------------------
double vtkAdaptiveUpdateSuppressor::ComputePriority(int n, int m, double res)
{
  DEBUGPRINT_EXECUTION(
  cerr << "SUS(" << this << ") ComputePriority " 
       << n << "/" << m << "@" << res << endl;
  );
  vtkDataObject *input = this->GetInput();
  vtkInformation* info = input->GetPipelineInformation();
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      vtkExecutive::PRODUCER()->GetExecutive(info));
  sddp->SetUpdateResolution(info, res);
  sddp->SetUpdateExtent(info, n, m, 0); 

  double priority = 1.0;
  if (vtkAdaptiveOptions::GetUsePrioritization())
    {
    priority = sddp->ComputePriority();
    }
  return priority;
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::PrepareFirstPass()
{
  DEBUGPRINT_EXECUTION(
  cerr << "SUS(" << this << ") PrepareFirstPass " << endl;
  );

  this->Height = vtkAdaptiveOptions::GetHeight();
  this->Degree = vtkAdaptiveOptions::GetDegree();
  this->MaxSplits = vtkAdaptiveOptions::GetMaxSplits();

  //when camera causes us to start over of a wend, be sure to include the 
  //pieces we drew before and need to draw again
  this->ToDo->MergePieceList(this->NextFrame);

  //compute priorities for everything we are going to draw this wend
  for (int i = 0; i < this->ToDo->GetNumberOfPieces(); i++)
    {
    int p = this->ToDo->GetPiece(i)->GetPiece();
    int np = this->ToDo->GetPiece(i)->GetNumPieces();
    double res = this->ToDo->GetPiece(i)->GetResolution();
    int gPiece = this->UpdatePiece*np + p;
    int gPieces = this->UpdateNumberOfPieces*np;

    double priority = this->ComputePriority(gPiece, gPieces, res);

    this->ToDo->GetPiece(i)->SetPriority(priority);
    }
  //sort them
  this->ToDo->SortPriorities();
  
  //combine empties
  this->Reap();

  //point to the most important piece
  vtkPiece *currentPiece = this->ToDo->GetPiece(0);
  PIECE = (double)currentPiece->GetPiece();
  NUMPIECES = (double)currentPiece->GetNumPieces();
  RESOLUTION = currentPiece->GetResolution();
  PRIORITY = currentPiece->GetPriority();
  HIT = 0.0;
  FROMAPPEND = 0.0;

  ALLDONE = WENDDONE = 0;
  NEXTAPPEND = 1;
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::PrepareAnotherPass()
{
  DEBUGPRINT_EXECUTION(cerr << "SUS(" << this << ") PrepareAnotherPass " << endl;);

  //point to the next most important piece
  vtkPiece *currentPiece = this->ToDo->GetPiece(0);
  PIECE = (double)currentPiece->GetPiece();
  NUMPIECES = (double)currentPiece->GetNumPieces();
  RESOLUTION = currentPiece->GetResolution();
  PRIORITY = currentPiece->GetPriority();
  HIT = 0.0;
  FROMAPPEND = 0.0;

  ALLDONE = WENDDONE = 0;
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::ChooseNextPiece()
{
  //choose appended results first
  if (NEXTAPPEND && this->PieceCacheFilter->GetAppendedData())
    {
    DEBUGPRINT_EXECUTION(cerr << "SUS(" << this << ") Choose append content" << endl;);
    PIECE = 0.0;
    NUMPIECES = 1.0;
    RESOLUTION = 0.0;
    PRIORITY = 1.0;
    FROMAPPEND = 1.0;
    HIT = 1.0;

    ALLDONE = 0;
    WENDDONE = 0;

    this->ForceUpdate();
    return;
    }

  DEBUGPRINT_EXECUTION(cerr << "SUS(" << this << ") Choose a normal piece" << endl;);

  vtkTimerLog::MarkStartEvent("Choose");

  bool found = false;
  bool noneLeft = false;
  bool restWorthless = false;
  while (!found && !noneLeft && !restWorthless)
    {
    //exit once the wend is done
    if (this->ToDo->IsEmpty())
      {
      DEBUGPRINT_EXECUTION(cerr << "SUS(" << this << ") wend is done" << endl;);
      noneLeft = true;
      continue;
      } 

    //get topmost piece
    vtkPiece *piece = this->ToDo->PopPiece();
    int p = piece->GetPiece();
    int np = piece->GetNumPieces();
    double res = piece->GetResolution();
    double priority = piece->GetPriority();
    int gPiece = this->UpdatePiece*np + p;
    int gPieces = this->UpdateNumberOfPieces*np;

    //remember it for next wend
    this->NextFrame->AddPiece(piece);
    piece->Delete();
    
    //early exit when we hit pieces that will not contribute
    if (priority == 0.0)
      {
      DEBUGPRINT_EXECUTION(                          
      cerr << "SUS(" << this << ") nothing else contributes" << endl;
      );
      this->NextFrame->MergePieceList(this->ToDo);
      restWorthless = true;
      continue;
      }

    //make sure it wasn't already rendered as part of append slot
    bool fromAppend = this->PieceCacheFilter->InAppend(gPiece, gPieces, res);
    if (fromAppend)
      {
      DEBUGPRINT_EXECUTION(
        cerr << "SUS(" << this << ") skip appended " << gPiece << "/" << gPieces << "@" << res << endl;
      );
      continue;
      }

    //notice if it is part of cache for bounding box diagnostic display color
    bool fromCache = this->PieceCacheFilter->InCache(gPiece, gPieces, res);
    DEBUGPRINT_EXECUTION(
      if (fromCache)
        {
        cerr << "SUS(" << this << ") " << gPiece << "/" << gPieces << "@" << res << " was cached" << endl;
        }
    );

    found = true;
    PIECE = p;
    NUMPIECES = np;
    RESOLUTION = res;
    PRIORITY = priority;
    FROMAPPEND = 0.0;
    HIT = (fromCache?1.0:0.0);
    DEBUGPRINT_EXECUTION(
      cerr << "SUS(" << this << ") chose"
      << p << "/" << np << "@" << res << ":" << priority
      << endl;
      );
    }


  if (!found)
    {
    WENDDONE = 1;
    //there was nothing worth updating
    DEBUGPRINT_EXECUTION(
                         cerr << "SUS(" << this << ") wend is done, no piece chosen."
                         << (noneLeft?" Last piece was in append.":"")
                         << (restWorthless?" Last piece has zero priority.":"")
                         << endl;
                         );
    }
  else
    {
    //check if we just took the last piece in the wend
    //let the application know so it can skip a step
    //also use this opportunity to update the cache filter's append slot
    if (this->ToDo->IsEmpty())
      {
      DEBUGPRINT_EXECUTION(
      cerr << "SUS(" << this << ") that was last piece in the wend." << endl;
                           );
      this->PieceCacheFilter->AppendPieces();
      WENDDONE = 1;
      }
    this->ForceUpdate();
    }

  vtkTimerLog::MarkEndEvent("Choose");
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::FinishPass()
{
  DEBUGPRINT_EXECUTION(cerr << "SUS(" << this << ") FinishPass" << endl;);
  ALLDONE = 0;
  if (!WENDDONE)
    {
    return;
    }

  int max = this->NextFrame->GetNumberOfPieces();
  for (int i = 0; i < max; i++)
    {
    vtkPiece *piece = this->NextFrame->GetPiece(i);
    double res = piece->GetResolution();
    double priority = piece->GetPriority();
    double mdr = 1.0;
    if (this->MaxDepth != -1)
      {
      mdr = (double)this->MaxDepth/(double)this->Height;
      }
    if (priority > 0.0 &&
        res < 1 &&
        res < mdr)
      {
      return;
      }
    }
  //didn't find any pieces worth refining, we are all done
  ALLDONE = 1;
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::Refine()
{
  DEBUGPRINT_EXECUTION(
  cerr << "SUS(" << this << ") Refining " 
       << this->ToDo->GetNumberOfPieces() << " "
       << this->NextFrame->GetNumberOfPieces() << " "
       << this->ToSplit->GetNumberOfPieces() << " "
       << endl;
  );
  vtkTimerLog::MarkStartEvent("Refine");

  //loop through pieces, find ones that should be refined, dump all others onto next wend
  int numSplittable = 0;
  while (!this->NextFrame->IsEmpty())
    {
    vtkPiece *piece = this->NextFrame->PopPiece();
    //int p = piece->GetPiece();
    //int np = piece->GetNumPieces();
    double res = piece->GetResolution();
    double priority = piece->GetPriority();
    //int gPiece = this->UpdatePiece*np + p;
    //int gPieces = this->UpdateNumberOfPieces*np;
    double mdr = 1.0;
    if (this->MaxDepth > -1)
      {
      mdr = (double)this->MaxDepth/(double)this->Height;
      }
    if (priority > 0.0 &&
        res < 1.0 &&
        res < mdr)
      {
      numSplittable++;
      this->ToSplit->AddPiece(piece);
      }
    else
      {
      this->ToDo->AddPiece(piece);
      }
    piece->Delete();
    }
  DEBUGPRINT_EXECUTION(
  cerr << numSplittable << " are splittable" << endl;
  );

  //loop through the splittable pieces, and split some of them
  //remove those we split from the cache
  int numSplit;
  for (numSplit = 0; 
       (this->MaxSplits == -1 || numSplit < this->MaxSplits) &&
         !this->ToSplit->IsEmpty();
       numSplit++)
    {
    //get the next piece
    vtkPiece *piece = this->ToSplit->PopPiece();
    int p = piece->GetPiece();
    int np = piece->GetNumPieces();
    double res = piece->GetResolution();
    piece->Delete();
    
    DEBUGPRINT_EXECUTION(cerr << "SUS(" << this << ") Split " << p << "/" << np << "@" << res << endl;);
    
    //remove it from the cache
    int index = this->PieceCacheFilter->ComputeIndex(p,np);
    this->PieceCacheFilter->DeletePiece(index);
    
    //compute next resolution to request for it
    double resolution = res + (1.0/this->Height);
    
    //split it N times
    int nrNP = np*this->Degree;
    int gPieces = this->UpdateNumberOfPieces*nrNP;
    for (int child=0; child < this->Degree; child++)
      {
      int nrA = p * this->Degree + child;
      int gPieceA = this->UpdatePiece * nrNP + nrA;

      DEBUGPRINT_EXECUTION(cerr << "SUS(" << this << ") created " << nrA << "/" << nrNP << "@" << resolution << endl;);
      
      vtkPiece *pA = vtkPiece::New();
      pA->SetPiece(nrA);
      pA->SetNumPieces(nrNP);
      pA->SetResolution(resolution);
      
      double priority = this->ComputePriority(gPieceA, gPieces, resolution);        
      pA->SetPriority(priority);
      
      this->ToDo->AddPiece(pA);
      pA->Delete();
      }          
    }
  DEBUGPRINT_EXECUTION(
   cerr << numSplit << " actually split" << endl;
  );
  if (numSplit == 0)
    {
    ALLDONE = 1;
    }

  //put any pieces we did not split back into next wend
  this->ToDo->MergePieceList(this->ToSplit);

  //create the append slot from everything left in the cache
  //ie, everything we drew this wend, minus whatever we chose to refine
  this->PieceCacheFilter->AppendPieces();
  
  DEBUGPRINT_EXECUTION(cerr << "SUS(" << this << ") "
                       << this->ToDo->GetNumberOfPieces() << " "
                       << this->NextFrame->GetNumberOfPieces() << " "
                       << this->ToSplit->GetNumberOfPieces() << " "
                       << endl;);
                         
  vtkTimerLog::MarkEndEvent("Refine");
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::Coarsen()
{
  DEBUGPRINT_EXECUTION(
  cerr << "SUS(" << this << ") Coarsen " 
       << this->ToDo->GetNumberOfPieces() << " "
       << this->NextFrame->GetNumberOfPieces() << " "
       << this->ToSplit->GetNumberOfPieces() << " "
       << endl;
  );
  vtkTimerLog::MarkStartEvent("Coarsen");

  //find every pair of siblings and merge them

  vtkstd::map<int, vtkPieceList* > levels;
  vtkstd::map<int, vtkPieceList* >::iterator iter;

  //sort pieces according to levels
  while (!this->NextFrame->IsEmpty())
    {
    vtkPiece *piece = this->NextFrame->PopPiece();
    //int p = piece->GetPiece();
    int np = piece->GetNumPieces();
    iter = levels.find(np);
    vtkPieceList *npl = NULL;
    if (iter == levels.end())
      {
      //cerr << "making new list for " << np << endl;
      npl = vtkPieceList::New();
      levels[np] = npl;
      }
    else
      {
      //cerr << "adding to existing list for " << np << endl;
      npl = iter->second;
      }   
    //cerr << "adding " << p << endl;
    npl->AddPiece(piece);
    piece->Delete();
    }

  for(iter = levels.begin(); iter != levels.end(); iter++)
    {
    //cerr << "looking at level " << iter->first << endl;
    vtkPieceList *npl = iter->second;
    while (!npl->IsEmpty())
      {
      vtkPiece *piece = npl->PopPiece();
      int p = piece->GetPiece();
      int np = piece->GetNumPieces();
      
      for (int i = 0; i < npl->GetNumberOfPieces(); i++)
        {
        vtkPiece *other = npl->GetPiece(i);
        int p2 = other->GetPiece();        
        if (p/2 == p2/2) //TODO, when Degree==N!=2, have to round up all N sibs
          {
          //cerr << p << " and " << p2 << "/" << np << " match found" << endl;
          piece->SetPiece(p/2);
          piece->SetNumPieces(np/2);
          double opp = piece->GetPriority();
          //you down with opp?
          if (opp > piece->GetPriority())
            {
            piece->SetPriority(opp);
            //yeah you know me.
            }
          double res = piece->GetResolution()-(1.0/this->Height);
          piece->SetResolution(res);
          this->NextFrame->AddPiece(piece);
          npl->RemovePiece(i);
          piece->Delete();
          piece = NULL;

          //remove it from the cache
          int index;
          index = this->PieceCacheFilter->ComputeIndex(p,np);
          this->PieceCacheFilter->DeletePiece(index);
          index = this->PieceCacheFilter->ComputeIndex(p2,np);
          this->PieceCacheFilter->DeletePiece(index);

          break;
          }
        }
      if (piece != NULL)
        {
        //cerr << p << "/" << np << " no match found" << endl;
        this->NextFrame->AddPiece(piece);
        piece->Delete();
        }      
      }
    npl->Delete();
    }

  levels.clear();
  vtkTimerLog::MarkEndEvent("Coarsen");
}

//----------------------------------------------------------------------------
void vtkAdaptiveUpdateSuppressor::Reap()
{
  vtkTimerLog::MarkStartEvent("Reap");

  int important = this->ToDo->GetNumberNonZeroPriority();
  int total = this->ToDo->GetNumberOfPieces();

  //cerr << "REAP==================================================" << endl;
  //this->ToDo->Print();
  //cerr << "Important " << important << " total " << total << endl;
  if (important == total)
    {
    vtkTimerLog::MarkEndEvent("Reap");
    return;
    }

  vtkPieceList *toMerge = vtkPieceList::New();
  for (int i = total-1; i>=important; --i)
    {
    vtkPiece *p = this->ToDo->PopPiece(i);
    toMerge->AddPiece(p);
    p->Delete();
    }

  //cerr << "TOMERGE:" << endl;
  //toMerge->Print();

  vtkPieceList *merged = vtkPieceList::New();

  bool done = false;
  while (!done)
    {
    int mcount = 0;
    //pick a piece
    while (toMerge->GetNumberOfPieces()>0)
      {
      vtkPiece *piece = toMerge->PopPiece();
      int p = piece->GetPiece();
      int np = piece->GetNumPieces();
      //cerr << p << "/" << np << " vs ";
      bool found = false;

      //look for a piece that can be merged with it
      for (int j = 0; j < toMerge->GetNumberOfPieces(); j++)
        {
        vtkPiece *other = toMerge->GetPiece(j);
        int p2 = other->GetPiece();
        int np2 = other->GetNumPieces();
        //cerr << p2 << "/" << np2 << " ";
        if ((np==np2) &&
            (p/2==p2/2) ) //TODO, when Degree==N!=2, have to round up all N sibs
          {
          //cerr << "REAP";
          piece->SetPiece(p/2);
          piece->SetNumPieces(np/2);
          double res = piece->GetResolution()-(1.0/this->Height);
          piece->SetResolution(res);

          merged->AddPiece(piece);
          toMerge->RemovePiece(j);
          found = true;
          mcount++;

          int index;
          index = this->PieceCacheFilter->ComputeIndex(p,np);
          this->PieceCacheFilter->DeletePiece(index);
          index = this->PieceCacheFilter->ComputeIndex(p2,np);
          this->PieceCacheFilter->DeletePiece(index);

          break;
          }
        }
      if (!found)
        {
        //cerr << "no match";
        merged->AddPiece(piece);
        }
      piece->Delete();
      //cerr << endl;
      }
    if (mcount==0)
      {
      done = true;
      }

    toMerge->MergePieceList(merged);
    }
  //cerr << "after merge:"<< endl;
  //toMerge->Print();
  //cerr << "TM:" << toMerge->GetNumberOfPieces() << " "
  //     << "M: " << merged->GetNumberOfPieces() << endl;

  //add whatever remains back in
  this->ToDo->MergePieceList(toMerge);
  //this->ToDo->Print();
  toMerge->Delete();
  merged->Delete();
  vtkTimerLog::MarkEndEvent("Reap");
}
