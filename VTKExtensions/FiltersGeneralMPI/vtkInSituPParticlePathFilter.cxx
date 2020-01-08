/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInSituPParticlePathFilter.h"

#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTemporalInterpolatedVelocityField.h"

using namespace vtkParticleTracerBaseNamespace;

vtkStandardNewMacro(vtkInSituPParticlePathFilter);

vtkInSituPParticlePathFilter::vtkInSituPParticlePathFilter()
{
  this->SetNumberOfInputPorts(3);
  this->UseArrays = false;
  this->RestartedSimulation = false;
  this->FirstTimeStep = 0;
}

vtkInSituPParticlePathFilter::~vtkInSituPParticlePathFilter()
{
}

//----------------------------------------------------------------------------
void vtkInSituPParticlePathFilter::SetClearCache(bool clearCache)
{
  this->It.SetClearCache(clearCache);
}

//----------------------------------------------------------------------------
void vtkInSituPParticlePathFilter::AddRestartConnection(vtkAlgorithmOutput* input)
{
  this->AddInputConnection(2, input);
}

//----------------------------------------------------------------------------
void vtkInSituPParticlePathFilter::RemoveAllRestarts()
{
  this->SetInputConnection(2, 0);
}

//---------------------------------------------------------------------------
int vtkInSituPParticlePathFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  int retval = this->Superclass::FillInputPortInformation(port, info);
  if (port == 2)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  return retval;
}

//---------------------------------------------------------------------------
int vtkInSituPParticlePathFilter::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->GetNumberOfInputConnections(2) != 0)
  { // restart seeds
    if (vtkInformation* sourceInfo = inputVector[2]->GetInformationObject(0))
    {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    }
  }

  return Superclass::RequestUpdateExtent(request, inputVector, outputVector);
}

//---------------------------------------------------------------------------
void vtkInSituPParticlePathFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "RestartedSimulation: " << this->RestartedSimulation << endl;
  os << indent << "FirstTimeStep: " << this->FirstTimeStep << endl;
}

//---------------------------------------------------------------------------
void vtkInSituPParticlePathFilter::AddRestartSeeds(vtkInformationVector** inputVector)
{
  if (this->GetNumberOfInputConnections(2) == 0)
  { // no restart seeds
    return;
  }
  vtkInformation* inInfo = inputVector[2]->GetInformationObject(0);
  vtkDataSet* restartSeeds = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  ParticleVector localRestartSeeds;
  int localAssignedCount = 0;
  // temporarily mark that arrays should be used for particle history
  this->UseArrays = true;
  this->AssignSeedsToProcessors(
    this->GetCurrentTimeValue(), restartSeeds, 0, 0, localRestartSeeds, localAssignedCount);
  this->UseArrays = false;

  this->UpdateParticleList(localRestartSeeds);
}

//---------------------------------------------------------------------------
void vtkInSituPParticlePathFilter::AssignSeedsToProcessors(double t, vtkDataSet* source,
  int sourceID, int ptId, ParticleVector& localSeedPoints, int& localAssignedCount)
{
  if (!this->Controller)
  {
    return Superclass::AssignSeedsToProcessors(
      t, source, sourceID, ptId, localSeedPoints, localAssignedCount);
  }

  ParticleVector candidates;
  //
  // take points from the source object and create a particle list
  //
  int numSeeds = source->GetNumberOfPoints();
  candidates.resize(numSeeds);
  //
  for (int i = 0; i < numSeeds; i++)
  {
    ParticleInformation& info = candidates[i];
    memcpy(&(info.CurrentPosition.x[0]), source->GetPoint(i), sizeof(double) * 3);
    info.CurrentPosition.x[3] = t;
    info.LocationState = 0;
    info.CachedCellId[0] = -1;
    info.CachedCellId[1] = -1;
    info.CachedDataSetId[0] = 0;
    info.CachedDataSetId[1] = 0;
    if (this->UseArrays)
    {
      vtkPointData* pd = source->GetPointData();
      vtkCharArray* sourceId = vtkCharArray::SafeDownCast(pd->GetArray("ParticleSourceId"));
      info.SourceID = sourceId->GetValue(i);
      vtkIntArray* injectedPointId = vtkIntArray::SafeDownCast(pd->GetArray("InjectedPointId"));
      info.InjectedPointId = injectedPointId->GetValue(i);
      vtkIntArray* injectedStepId = vtkIntArray::SafeDownCast(pd->GetArray("InjectionStepId"));
      info.InjectedStepId = injectedStepId->GetValue(i);
      vtkFloatArray* age = vtkFloatArray::SafeDownCast(pd->GetArray("ParticleAge"));
      info.age = age->GetValue(i);
      vtkIntArray* uniqueParticleId = vtkIntArray::SafeDownCast(pd->GetArray("ParticleId"));
      info.UniqueParticleId = uniqueParticleId->GetValue(i);
      if (static_cast<vtkIdType>(info.UniqueParticleId) >= this->UniqueIdCounter)
      {
        this->UniqueIdCounter = info.UniqueParticleId + 1;
      }
      info.TimeStepAge = this->FirstTimeStep - info.InjectedStepId;
    }
    else
    {
      info.SourceID = sourceID;
      info.InjectedPointId = i + ptId;
      // this->GetReinjectionCounter() really gets the reinjection time step
      info.InjectedStepId = this->GetReinjectionCounter() + this->FirstTimeStep;
      info.UniqueParticleId = -1;
      info.age = 0.0;
      info.TimeStepAge = 0;
    }
    info.rotation = 0.0;
    info.angularVel = 0.0;
    info.time = 0.0;
    info.speed = 0.0;
    info.SimulationTime = this->GetCurrentTimeValue();
    info.ErrorCode = 0;
    info.PointId = -1;
    info.TailPointId = -1;
  }
  //
  // Check all Seeds on all processors for classification
  //
  std::vector<int> owningProcess(numSeeds, -1);
  int myRank = this->Controller->GetLocalProcessId();
  ParticleIterator it = candidates.begin();
  for (int i = 0; it != candidates.end(); ++it, ++i)
  {
    ParticleInformation& info = (*it);
    double* pos = &info.CurrentPosition.x[0];
    // if outside bounds, reject instantly
    if (this->InsideBounds(pos))
    {
      // since this is first test, avoid bad cache tests
      this->GetInterpolator()->ClearCache();
      int searchResult = this->GetInterpolator()->TestPoint(pos);
      if (searchResult == ID_INSIDE_ALL || searchResult == ID_OUTSIDE_T0)
      {
        // this particle is in this process's domain for the latest time step
        owningProcess[i] = myRank;
      }
    }
  }
  std::vector<int> realOwningProcess(numSeeds);
  this->Controller->AllReduce(
    &owningProcess[0], &realOwningProcess[0], numSeeds, vtkCommunicator::MAX_OP);

  for (size_t i = 0; i < realOwningProcess.size(); i++)
  {
    if (realOwningProcess[i] == myRank)
    {
      localSeedPoints.push_back(candidates[i]);
    }
  }

  if (!this->UseArrays)
  {
    // Assign unique identifiers taking into account uneven distribution
    // across processes and seeds which were rejected. Only when
    // non-restarted particles are added do we have to assign
    // unique ids.
    this->AssignUniqueIds(localSeedPoints);
  }
}

//---------------------------------------------------------------------------
std::vector<vtkDataSet*> vtkInSituPParticlePathFilter::GetSeedSources(
  vtkInformationVector* inputVector, int timeStep)
{
  int numSources = inputVector->GetNumberOfInformationObjects();
  std::vector<vtkDataSet*> seedSources;
  if (this->RestartedSimulation == false || timeStep != 0)
  {
    for (int idx = 0; idx < numSources; ++idx)
    {
      if (vtkInformation* inInfo = inputVector->GetInformationObject(idx))
      {
        vtkDataObject* dobj = inInfo->Get(vtkDataObject::DATA_OBJECT());
        seedSources.push_back(vtkDataSet::SafeDownCast(dobj));
      }
    }
  }

  return seedSources;
}
