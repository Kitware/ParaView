/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQDistributedStreamTracer.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQDistributedStreamTracer.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkInterpolatedVelocityField.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRungeKutta2.h"
#include "vtkSmartPointer.h"

vtkCxxRevisionMacro(vtkSQDistributedStreamTracer, "$Revision: 1.11 $");
vtkStandardNewMacro(vtkSQDistributedStreamTracer);

//-----------------------------------------------------------------------------
vtkSQDistributedStreamTracer::vtkSQDistributedStreamTracer()
{
}

//-----------------------------------------------------------------------------
vtkSQDistributedStreamTracer::~vtkSQDistributedStreamTracer()
{
}

//-----------------------------------------------------------------------------
void vtkSQDistributedStreamTracer::ForwardTask(double seed[3],
                                             int direction,
                                             int taskType,
                                             int originatingProcId,
                                             int originatingStreamId,
                                             int currentLine,
                                             double* firstNormal,
                                             double propagation,
                                             vtkIdType numSteps)
{
  int myid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  int nextid;
  if (myid == numProcs-1)
    {
    nextid = 0;
    }
  else
    {
    nextid = myid+1;
    }

  this->Controller->Send(&taskType, 1, nextid, 311); // code indicates 0=,1=,2=stop
  this->Controller->Send(&originatingProcId, 1, nextid, 322);   //

  if (taskType!=TASK_CANCEL)
    {
    // not stopping.
    this->Controller->Send(&originatingStreamId, 1, nextid, 322); //
    this->Controller->Send(seed, 3, nextid, 333);        // seed point
    this->Controller->Send(&direction, 1, nextid, 344);
    this->Controller->Send(&currentLine, 1, nextid, 355);
    double tmpNormal[4];
    if (firstNormal)
      {
      tmpNormal[0] = 1;
      memcpy(tmpNormal+1, firstNormal, 3*sizeof(double));
      }
    else
      {
      tmpNormal[0] = 0;
      }
    this->Controller->Send(tmpNormal, 4, nextid, 366);
    this->Controller->Send(&propagation, 1, nextid, 367);
    this->Controller->Send(&numSteps, 1, nextid, 368);
    }
}

//-----------------------------------------------------------------------------
int vtkSQDistributedStreamTracer::ReceiveAndProcessTask()
{
  int taskType = 0;
  int originatingProcId = 0;
  int originatingStreamId = 0;
  int currentLine = 0;
  int direction=FORWARD;
  double seed[3] = {0.0, 0.0, 0.0};
  int myid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();

  this->Controller->Receive(&taskType, 
                            1,
                            vtkMultiProcessController::ANY_SOURCE,
                            311);
  this->Controller->Receive(&originatingProcId, 
                            1,
                            vtkMultiProcessController::ANY_SOURCE,
                            322);

  // We have a recv. a cancel task message, all seeds have been processed.
  // Stop here.
  if (taskType==TASK_CANCEL)
    {
    if ( (( myid==numProcs-1 && originatingProcId==0 ) ||
          ( myid!=numProcs-1 && originatingProcId==myid+1) ))
      {
      // All processes have been already told to stop. No need to tell
      // the next one.
      return 0;
      }
    // Pass on the cancel message to the next processor in the ring.
    this->ForwardTask(seed, direction, TASK_CANCEL, originatingProcId, 0, 0, 0, 0.0, 0);
    return 0;
    }

  this->Controller->Receive(&originatingStreamId, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            322);
  this->Controller->Receive(seed, 
                            3, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            333);
  this->Controller->Receive(&direction, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            344);
  this->Controller->Receive(&currentLine, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            355);
  double tmpNormal[4];
  this->Controller->Receive(tmpNormal, 
                            4, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            366);
  double propagation;
  this->Controller->Receive(&propagation, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            367);
  vtkIdType numSteps;
  this->Controller->Receive(&numSteps, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            368);

  double* firstNormal=0;
  if (tmpNormal[0] != 0)
    {
    firstNormal = &(tmpNormal[1]);
    }
  return this->ProcessTask(seed, 
                           direction, 
                           taskType, 
                           originatingProcId, 
                           originatingStreamId, 
                           currentLine, 
                           firstNormal,
                           propagation,
                           numSteps);
}

//-----------------------------------------------------------------------------
int vtkSQDistributedStreamTracer::ProcessNextLine(int currentLine)
{
  int myid = this->Controller->GetLocalProcessId();

  vtkIdType numLines = this->SeedIds->GetNumberOfIds();
  currentLine++;
  if ( currentLine < numLines )
    {
    vtkIdType seedId=this->SeedIds->GetId(currentLine);
    double *seedPt=this->Seeds->GetTuple(seedId);
    int direction=this->IntegrationDirections->GetValue(currentLine);
    return
      this->ProcessTask(
        seedPt,
        direction,
        /*taskType=*/TASK_INTEGRATE,
        /*originatingProcId=*/myid,
        /*originatingStreamId=*/-1,
        currentLine,
        /*firstNormal=*/0,
        /*propagation=*/0.0,
        /*numSteps=*/0);
    }

  // All done. Tell everybody to stop.
  double seed[3] = {0.0, 0.0, 0.0};
  this->ForwardTask(seed, 0, TASK_CANCEL, myid, 0, 0, 0, 0.0, 0);
  return 0;

}

// Integrate a streamline
//-----------------------------------------------------------------------------
int vtkSQDistributedStreamTracer::ProcessTask(double seed[3],
                                            int direction,
                                            int taskType,
                                            int originatingProcId,
                                            int originatingStreamId,
                                            int currentLine,
                                            double* firstNormal,
                                            double propagation,
                                            vtkIdType numSteps)
{
  int myid = this->Controller->GetLocalProcessId();

  // This seed was visited by everybody and nobody had it.
  // Must be out of domain.
  if (taskType==TASK_SEEK && originatingProcId==myid)
    {
    return this->ProcessNextLine(currentLine);
    }

  // Send progress.
  this->UpdateProgress((double)currentLine/this->SeedIds->GetNumberOfIds());

  // Check to see if the seed lies within our data.
  int retVal = 1;
  if (!this->EmptyData)
    {
    double velocity[3];
    this->Interpolator->ClearLastCellId();
    retVal = this->Interpolator->FunctionValues(seed, velocity);
    }

  // We don't have this seed, let's forward it to the next process, who will 
  // either intergate it if he has it or pass it on again
  if (!retVal || this->EmptyData)
    {
    this->ForwardTask(seed,
                      direction,
                      TASK_SEEK,
                      originatingProcId,
                      originatingStreamId,
                      currentLine,
                      firstNormal,
                      propagation,
                      numSteps);
    return 1;
    }
  // This seed point is local. Prepare to pass the intergation
  // of this single seed point to the superclass. Our input
  // is a multiblock one block, intergate the block with result
  // stored as a single polydata.
  vtkFloatArray* seeds = vtkFloatArray::New();
  seeds->SetNumberOfComponents(3);
  seeds->InsertNextTuple(seed);
  //
  vtkIdList* seedIds = vtkIdList::New();
  seedIds->InsertNextId(0);
  //
  vtkIntArray* integrationDirections = vtkIntArray::New();
  integrationDirections->InsertNextValue(direction);
  // Get the interpolator and max cell size from superclass.
  vtkInterpolatedVelocityField* func;
  int maxCellSize = 0;
  this->CheckInputs(func, &maxCellSize);
  // Get our input dataset, it's in the first non-empty block.
  vtkCompositeDataIterator* iter = this->InputData->NewIterator();
  iter->GoToFirstItem();
  vtkDataSet* input=0;
  if (!iter->IsDoneWithTraversal())
    {
    input=vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    }
  iter->Delete();
  // Pick up the active vector field.
  vtkDataArray *vectors=this->GetInputArrayToProcess(0,input);
  const char *vecName=vectors->GetName();
  // Supercalss Integrates.
  vtkPolyData* streamLine = vtkPolyData::New();
  this->TmpOutputs.push_back(streamLine);
  streamLine->Delete();
  double lastPoint[3];
  this->Integrate(input,
                  streamLine,
                  seeds,
                  seedIds,
                  integrationDirections,
                  lastPoint,
                  func,
                  maxCellSize,
                  vecName,
                  propagation,
                  numSteps);
  this->GenerateNormals(streamLine, firstNormal, vecName);

  seeds->Delete();
  seedIds->Delete();
  integrationDirections->Delete();

  // These are used to keep track of where the seed came from
  // and where it will go. Used later to fill the gaps between
  // streamlines.
  vtkIntArray* strOrigin = vtkIntArray::New();
  strOrigin->SetNumberOfComponents(2);
  strOrigin->SetNumberOfTuples(1);
  strOrigin->SetName("Streamline Origin");
  strOrigin->SetValue(0, originatingProcId);     // originating proc id
  strOrigin->SetValue(1, originatingStreamId);   // originating cell id
  streamLine->GetCellData()->AddArray(strOrigin);
  strOrigin->Delete();

  vtkIntArray* streamIds = vtkIntArray::New();
  streamIds->SetNumberOfTuples(1);
  streamIds->SetName("Streamline Ids");
  originatingStreamId = static_cast<int>(this->TmpOutputs.size()) - 1;
  streamIds->SetComponent(0, 0, originatingStreamId);
  streamLine->GetCellData()->AddArray(streamIds);
  streamIds->Delete();

  int nPoints=streamLine->GetNumberOfPoints();
  // Add a scalar to identify which seed point the segemnt
  // coresponds to.
  vtkIntArray *seedPointIds=vtkIntArray::New();
  seedPointIds->SetName("SeedPointIds");
  seedPointIds->SetNumberOfTuples(nPoints);
  seedPointIds->FillComponent(0,this->SeedIds->GetId(currentLine));
  streamLine->GetPointData()->AddArray(seedPointIds);
  seedPointIds->Delete();

  // We have to know why the integration terminated
  vtkIntArray* termArray
  =vtkIntArray::SafeDownCast(streamLine->GetCellData()->GetArray("ReasonForTermination"));
  int term
    = termArray?termArray->GetValue(0):vtkSQStreamTracer::OUT_OF_DOMAIN;
  // If the interation terminated due to something other than
  // moving outside the domain, move to the next seed.
  if (nPoints==0 || term!=vtkSQStreamTracer::OUT_OF_DOMAIN)
    {
    func->Delete();
    return this->ProcessNextLine(currentLine);
    }

  // Continue the integration a bit further to obtain a point
  // outside. The main integration step can not always be used
  // for this, specially if the integration is not 2nd order.
  streamLine->GetPoint(nPoints-1, lastPoint);

  vtkInitialValueProblemSolver* ivp = this->Integrator;
  ivp->Register(this);

  vtkRungeKutta2* tmpSolver = vtkRungeKutta2::New();
  this->SetIntegrator(tmpSolver);
  tmpSolver->Delete();

  double tmpseed[3];
  memcpy(tmpseed, lastPoint, 3*sizeof(double));
  this->SimpleIntegrate(tmpseed, lastPoint, this->LastUsedStepSize, func);
  func->Delete();

  this->SetIntegrator(ivp);
  ivp->UnRegister(this);

  double *lastNormal = 0;
  vtkDataArray* normals=streamLine->GetPointData()->GetArray("Normals");
  if (normals)
    {
    lastNormal = new double[3];
    vtkIdType nNormals=normals->GetNumberOfTuples();
    normals->GetTuple(nNormals-1, lastNormal);
    }

  streamLine->GetPoints()->SetPoint(nPoints-1, lastPoint);

  // Now we have traced the streamline as far as possible on this process,
  // and since we have not left the domain of the dataset, the streamline
  // continues on another process. Pass the end point of this streamline
  // on to the next process.
  this->ForwardTask(/*seed=*/lastPoint,
                    direction,
                    /*taskType=*/TASK_INTEGRATE,
                    /*originatingProcId=*/myid, // originates from me
                    originatingStreamId,        // index of the stream in our tmp container
                    /*seedId=*/currentLine,
                    lastNormal,
                    propagation,
                    numSteps);

  delete [] lastNormal;

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQDistributedStreamTracer::ParallelIntegrate()
{
  int myid = this->Controller->GetLocalProcessId();
  if (this->Seeds)
    {
    int doLoop = 1;
    // First process starts by integrating the first point
    if (myid==0)
      {
      double seed[3]={0.0};
      vtkIdType seedId=this->SeedIds->GetId(0);
      this->Seeds->GetTuple(seedId,seed);
      int direction=this->IntegrationDirections->GetValue(0);
      doLoop
      =this->ProcessTask(seed,direction,TASK_INTEGRATE,myid,-1,0,0,0.0,0);
      }
    // Wait for someone to send us a seed to start from.
    while(doLoop) 
      {
      if (!this->ReceiveAndProcessTask()) { break; }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSQDistributedStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
