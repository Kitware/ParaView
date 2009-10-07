/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingUpdateSuppressor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamingUpdateSuppressor.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkUpdateSuppressorPipeline.h"
#include "vtkPieceCacheFilter.h"
#include "vtkStreamingOptions.h"

#include "vtkPiece.h"
#include "vtkPieceList.h"
#include "vtkBoundingBox.h"
#include "vtkDoubleArray.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIMoveData.h"

vtkCxxRevisionMacro(vtkStreamingUpdateSuppressor, "1.2");
vtkStandardNewMacro(vtkStreamingUpdateSuppressor);

#define DEBUGPRINT_EXECUTION(arg)\
  if (vtkStreamingOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }
  
//----------------------------------------------------------------------------
vtkStreamingUpdateSuppressor::vtkStreamingUpdateSuppressor()
{
  this->Pass = 0;
  this->NumberOfPasses = 1;
  this->PieceList = NULL;
  this->MaxPass = 0;
  this->SerializedPriorities = NULL;
  this->MPIMoveData = NULL;
}

//----------------------------------------------------------------------------
vtkStreamingUpdateSuppressor::~vtkStreamingUpdateSuppressor()
{
  if (this->PieceList)
    {
    this->PieceList->Delete();
    }
  if (this->SerializedPriorities)
    {
    this->SerializedPriorities->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::ForceUpdate()
{    

  int gPiece = this->UpdatePiece*this->NumberOfPasses + this->GetPiece();
  int gPieces = this->UpdateNumberOfPieces*this->NumberOfPasses;

  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") ForceUpdate " << gPiece << "/" << gPieces << endl;
                       );

  // Make sure that output type matches input type
  this->UpdateInformation();

  vtkDataObject *input = this->GetInput();
  if (input == 0)
    {
    vtkErrorMacro("No valid input.");
    return;
    }
  vtkDataObject *output = this->GetOutput();
  vtkPiece *cachedPiece = NULL;
  if (this->PieceList)
    {
    cachedPiece = this->PieceList->GetPiece(this->Pass);
    }

  // int fixme; // I do not like this hack.  How can we get rid of it?
  // Assume the input is the collection filter.
  // Client needs to modify the collection filter because it is not
  // connected to a pipeline.
  vtkAlgorithm *source = input->GetProducerPort()->GetProducer();
  if (source &&
      (source->IsA("vtkMPIMoveData") ||
       source->IsA("vtkCollectPolyData") ||
       source->IsA("vtkM2NDuplicate") ||
       source->IsA("vtkM2NCollect") ||
       source->IsA("vtkOrderedCompositeDistributor") || 
       source->IsA("vtkClientServerMoveData")))
    {
    source->Modified();
    }

  vtkInformation* info = input->GetPipelineInformation();
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      vtkExecutive::PRODUCER()->GetExecutive(info));

  if (sddp)
    {
    sddp->SetUpdateExtent(info,
                          gPiece, 
                          gPieces, 
                          0);
    }
  else
    {
    vtkErrorMacro("Uh oh");
    return;
    }

  if (this->UpdateTimeInitialized)
    {
    info->Set(vtkCompositeDataPipeline::UPDATE_TIME_STEPS(), &this->UpdateTime, 1);
    }

  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") Update " << this->Pass << "->" << gPiece << endl;
  );
  
  input->Update();
  output->ShallowCopy(input);

  this->PipelineUpdateTime.Modified();
}

//----------------------------------------------------------------------------
int vtkStreamingUpdateSuppressor::GetPiece(int p)
{
  int piece;
  int pass = p;

  //check validity  
  if (pass < 0 || pass >= this->NumberOfPasses)
    {
    pass = this->Pass;
    }

  //lookup piece corresponding to pass
  vtkPiece *pStruct = NULL;
  if (!this->PieceList)
    {
    piece = pass;
    }
  else
    {
    pStruct = this->PieceList->GetPiece(pass);
    if (!pStruct)
      {
      piece = pass;
      }
    else
      {
      piece = pStruct->GetPiece();
      }
    }
  return piece;
}

//----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::MarkMoveDataModified()
{
  if (this->MPIMoveData)
    {
    //We have to ensure that communication isn't halted when a piece is reused.
    //This is called whenever we set the pass, because any processor might
    //(because of view dependent reordering) rerender the same piece in two
    //subsequent frames in which case the pipeline would not update and 
    //communications will hang.
    this->MPIMoveData->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::SetPassNumber(int pass, int NPasses)
{
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") SetPassNumber " << Pass << "/" << NPasses << endl;
                       );
  this->SetPass(pass);
  this->SetNumberOfPasses(NPasses);
  this->MarkMoveDataModified();
}

//-----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::SetPieceList(vtkPieceList *other)
{
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") SET PIECE LIST" << endl;
  );
  if (this->PieceList)
    {
    this->PieceList->Delete();
    }
  this->PieceList = other;
  if (other)
    {
    other->Register(this);
    }
  this->MaxPass = this->NumberOfPasses;
  if (this->PieceList)
    {
    this->MaxPass = this->PieceList->GetNumberNonZeroPriority();
    }
}

//-----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::SerializePriorities()
{
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") SERIALIZE PRIORITIES" << endl;
  );
  this->PieceList->Serialize();
  DEBUGPRINT_EXECUTION(
  this->PieceList->Print();
  );
}

//-----------------------------------------------------------------------------
vtkDoubleArray *vtkStreamingUpdateSuppressor::GetSerializedPriorities()
{
  if (this->SerializedPriorities)
    {
    this->SerializedPriorities->Delete();
    }
  this->SerializedPriorities = vtkDoubleArray::New();
  double *buffer;
  int len=0;
  this->PieceList->GetSerializedList(&buffer, &len);
  this->SerializedPriorities->SetNumberOfComponents(1);
  this->SerializedPriorities->SetNumberOfTuples(len);
  this->SerializedPriorities->SetArray(buffer, len, 1);
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") My list was " << len << ":";
  for (int i = 0; i < len; i++)
    {
    cerr << this->SerializedPriorities->GetValue(i) << " ";
    }
  cerr << endl;
  );
  return this->SerializedPriorities;
}

//-----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::UnSerializePriorities(double *buffer)
{
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") UNSERIALIZE PRIORITIES" << endl;
  );

  if (!this->PieceList)
    {
    this->PieceList = vtkPieceList::New();
    }
  this->PieceList->UnSerialize(buffer);

  DEBUGPRINT_EXECUTION(
  int len = (int)*buffer * 6 + 1;
  for (int i = 0; i < len; i++)
    {
    cerr << buffer[i] << " ";
    };
  cerr << endl;
  this->PieceList->Print();
  );
}

//-----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::ClearPriorities()
{
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") CLEAR PRIORITIES" << endl;
  );

  if (this->PieceList)
    {
    this->PieceList->Delete();
    this->PieceList = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::ComputePriorities()
{
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") COMPUTE PRIORITIES ";
  this->PrintPipe(this);
  cerr << endl;
  );
  vtkDataObject *input = this->GetInput();
  if (input == 0)
    {
    cerr << "NO INPUT" << endl;
    return;
    }
  if (this->PieceList)
    {
    this->PieceList->Delete();
    }
  this->PieceList = vtkPieceList::New();
  this->PieceList->Clear();
  vtkInformation* info = input->GetPipelineInformation();
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      vtkExecutive::PRODUCER()->GetExecutive(info));
  if (sddp)
    {
    for (int i = 0; i < this->NumberOfPasses; i++)
      {
      double priority = 1.0;
      vtkPiece *piece = vtkPiece::New();    
      int gPiece = this->UpdatePiece*this->NumberOfPasses + i;
      int gPieces = this->UpdateNumberOfPieces*this->NumberOfPasses;
      if (vtkStreamingOptions::GetUsePrioritization())
        {
        DEBUGPRINT_EXECUTION(
        cerr << "US(" << this << ") COMPUTE PRIORITY ON " << gPiece << endl;
        );
        sddp->SetUpdateExtent(info, gPiece, gPieces, 0); 
        priority = sddp->ComputePriority();
        DEBUGPRINT_EXECUTION(
        cerr << "US(" << this << ") result was " << priority << endl;
        );
        }
      piece->SetPiece(i);
      piece->SetNumPieces(this->NumberOfPasses);
      piece->SetPriority(priority);
      this->PieceList->AddPiece(piece);
      piece->Delete();
      }
    }
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") PRESORT:" << endl;
  this->PieceList->Print();
  );

  //sorts pieces from priority 1.0 down to 0.0
  this->PieceList->SortPriorities();    

  //All nodes in server now have to agree on which pieces to process in order
  //to avoid deadlock.
  this->MergePriorities();

  //The client can use this to know when it can stop.
  this->MaxPass = this->PieceList->GetNumberNonZeroPriority();
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") " << this->MaxPass << " pieces that matter" << endl;
  );
}

//----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::PrintPipe(vtkAlgorithm *alg)
{
  //for debugging, this helps me understand which US is which in the messages
  vtkAlgorithm *algptr = alg;
  if (!algptr) return;
  if (algptr->GetNumberOfInputPorts() && 
      algptr->GetNumberOfInputConnections(0))
    {
    vtkAlgorithmOutput *ao = algptr->GetInputConnection(0,0);
    if (ao)
      {
      algptr = ao->GetProducer();
      this->PrintPipe(algptr);
      }
    cerr << "->";
    }   
  cerr << alg->GetClassName();
}

//----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::MergePriorities()
{
  //each processor (client and all nodes of the server) have to agree on 
  //piece list. No processor can skip a pipeline update in ForceUpdate 
  //(either because of 0.0 priority or because of reuse of cached results) 
  //unless all of the processors do. Otherwise 1 sided communication across 
  //processors will cause ParaView to hang.
  if (!this->PieceList)
    {
    //will someone ever not have a piecelist?
    return;
    }

  vtkMultiProcessController *controller =
    vtkMultiProcessController::GetGlobalController();
  int procid = 0;
  int numProcs = 1;

  //serialize locally computed priorities
  int np = this->PieceList->GetNumberOfPieces();
  double *mine = new double[np];
  for (int i = 0; i < np; i++)
    {
    mine[i] = this->PieceList->GetPiece(i)->GetPriority(); 
    }
  if (controller)
    {
    procid = controller->GetLocalProcessId();
    numProcs = controller->GetNumberOfProcesses();
    DEBUGPRINT_EXECUTION(
    cerr << "US(" << this << ") PREGATHER:" << endl;
    this->PieceList->Print();
    );
    }
  if (procid)
    {
    //send locally computed priorities to root
    controller->Send(mine, np, 0, PRIORITY_COMMUNICATION_TAG);
    //receive merged results from root
    controller->Receive(mine, np, 0, PRIORITY_COMMUNICATION_TAG);
    for (int i = 0; i < np; i++)
      {
      this->PieceList->GetPiece(i)->SetPriority(mine[i]);
      }
    }
  else if (numProcs > 1)
    {
    double *remotes = new double[np];
    for (int j = 1; j < numProcs; j++)
      {
      controller->Receive(remotes, np, j, PRIORITY_COMMUNICATION_TAG);
      for (int i = 0; i < np; i++)
        {
        if (remotes[i] > mine[i])
          {
          mine[i] = remotes[i];
          }
        }
      }
    delete[] remotes;
    for (int j = 1; j < numProcs; j++)
      {
      controller->Send(mine, np, j, PRIORITY_COMMUNICATION_TAG);
      }
    for (int i = 0; i < np; i++)
      {
      this->PieceList->GetPiece(i)->SetPriority(mine[i]);
      }
    }
  DEBUGPRINT_EXECUTION(
  cerr << "US(" << this << ") POSTGATHER" << endl;
  this->PieceList->Print();
  );

  delete[] mine;
}

//----------------------------------------------------------------------------
void vtkStreamingUpdateSuppressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
