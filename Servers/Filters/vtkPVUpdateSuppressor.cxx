/*=========================================================================

  Program:   ParaView
  Module:    vtkPVUpdateSuppressor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVUpdateSuppressor.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCacheSizeKeeper.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUpdateSuppressorPipeline.h"

#include <vtkstd/map>

#ifdef MYDEBUG
# define vtkMyDebug(x)\
    cout << x;
#else
# define vtkMyDebug(x)
#endif

class vtkPVUpdateSuppressorCacheMap : 
  public vtkstd::map<double, vtkSmartPointer<vtkDataObject> >
{
public:
  unsigned long GetActualMemorySize() 
    {
    unsigned long actual_size = 0;
    vtkPVUpdateSuppressorCacheMap::iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter)
      {
      actual_size += iter->second.GetPointer()->GetActualMemorySize();
      }
    return actual_size;
    }
};

vtkCxxRevisionMacro(vtkPVUpdateSuppressor, "1.62");
vtkStandardNewMacro(vtkPVUpdateSuppressor);
vtkCxxSetObjectMacro(vtkPVUpdateSuppressor, CacheSizeKeeper, vtkCacheSizeKeeper);
//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::vtkPVUpdateSuppressor()
{
  this->UpdatePiece = 0;
  this->UpdateNumberOfPieces = 1;

  this->UpdateTime = 0.0;
  this->UpdateTimeInitialized = false;

  this->Cache = new vtkPVUpdateSuppressorCacheMap();

  this->Enabled = 1;

  this->CacheSizeKeeper = 0;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (pm)
    {
    this->SetCacheSizeKeeper(
      vtkProcessModule::GetProcessModule()->GetCacheSizeKeeper());

    this->UpdateNumberOfPieces = pm->GetNumberOfLocalPartitions();
    this->UpdatePiece = pm->GetPartitionId();
    }
}

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::~vtkPVUpdateSuppressor()
{
  this->RemoveAllCaches();

  // Unset cache keeper only after having cleared the cache.
  this->SetCacheSizeKeeper(0);

  delete this->Cache;
  this->Cache = 0;
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::SetUpdateTime(double utime)
{
  this->UpdateTimeInitialized = true;
  if (this->UpdateTime != utime)
    {
    this->Modified();
    this->UpdateTime = utime;
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::SetEnabled(int enable)
{
  if (this->Enabled == enable)
    {
    return;
    }
  this->Enabled = enable;
  this->Modified();
  vtkUpdateSuppressorPipeline* executive = 
    vtkUpdateSuppressorPipeline::SafeDownCast(this->GetExecutive());
  if (executive)
    {
    executive->SetEnabled(enable);
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::ForceUpdate()
{    
  // Make sure that output type matches input type
  this->UpdateInformation();

  vtkDataObject *input = this->GetInput();
  if (input == 0)
    {
    vtkErrorMacro("No valid input.");
    return;
    }
  vtkDataObject *output = this->GetOutput();

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
                          this->UpdatePiece, 
                          this->UpdateNumberOfPieces, 
                          0);
    }
  else
    {
    input->SetUpdatePiece(this->UpdatePiece);
    input->SetUpdateNumberOfPieces(this->UpdateNumberOfPieces);
    input->SetUpdateGhostLevel(0);
    }
  vtkMyDebug("ForceUpdate ");
  if (this->UpdateTimeInitialized)
    {
    info->Set(vtkCompositeDataPipeline::UPDATE_TIME_STEPS(), &this->UpdateTime, 1);
    vtkMyDebug(this->UpdateTime);
    }
  vtkMyDebug(endl);

  input->Update();
  // Input may have changed, we obtain the pointer again.
  input = this->GetInput();

  output->ShallowCopy(input);
  this->PipelineUpdateTime.Modified();
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::RemoveAllCaches()
{
  unsigned long freed_size = this->Cache->GetActualMemorySize();
  this->Cache->clear();
  if (freed_size > 0 && this->CacheSizeKeeper)
    {
    // Tell the cache size keeper about the newly freed memory size.
    this->CacheSizeKeeper->FreeCacheSize(freed_size);
    }
}

//----------------------------------------------------------------------------
int vtkPVUpdateSuppressor::IsCached(double cacheTime)
{
  vtkPVUpdateSuppressorCacheMap::iterator iter = this->Cache->find(cacheTime);
  return  (iter == this->Cache->end())? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::CacheUpdate(double cacheTime)
{
  vtkPVUpdateSuppressorCacheMap::iterator iter = this->Cache->find(cacheTime);
  vtkDataObject* output = this->GetOutput();
  if (iter == this->Cache->end())
    {
    // No cache present, force update.
    this->ForceUpdate();
  
    if (!this->CacheSizeKeeper  || !this->CacheSizeKeeper->GetCacheFull())
      {
      vtkSmartPointer<vtkDataObject> cache;
      cache.TakeReference(output->NewInstance());
      cache->ShallowCopy(output);

      // FIXME: I am commenting this code as I don't know what it's doing.
      // // Compositing seems to update the input properly.
      // // But this update is needed when doing animation without compositing.
      cache->Update();
      (*this->Cache)[cacheTime] = cache;

      if (this->CacheSizeKeeper)
        {
        // Register used cache size.
        this->CacheSizeKeeper->AddCacheSize(cache->GetActualMemorySize());
        }

      vtkMyDebug(this->UpdatePiece << " "
        << "Cached data: " << cacheTime 
        << " " << vtkDataSet::SafeDownCast(cache)->GetNumberOfPoints() << endl);
      }
    else
      {
      vtkMyDebug(this->UpdatePiece << " "
        << "Cached data: " << cacheTime 
        << " -- not caching since cache full" << endl);
      }
    }
  else
    {
    // Using the cached data.
    output->ShallowCopy(iter->second.GetPointer());
    vtkMyDebug( 
      this->UpdatePiece << " "
      << "Using cache: " << cacheTime 
      << " " << vtkDataSet::SafeDownCast(output)->GetNumberOfPoints() << endl)
    }
  this->PipelineUpdateTime.Modified();
  this->Modified();
  output->Modified();
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVUpdateSuppressor::CreateDefaultExecutive()
{
  vtkUpdateSuppressorPipeline* executive = vtkUpdateSuppressorPipeline::New();
  executive->SetEnabled(this->Enabled);
  return executive;
}

//----------------------------------------------------------------------------
int vtkPVUpdateSuppressor::RequestDataObject(
  vtkInformation* vtkNotUsed(reqInfo), 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* outInfo = outputVector->GetInformationObject(i);
      vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
    
      if (!output || !output->IsA(input->GetClassName())) 
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(outInfo);
        newOutput->Delete();
        this->GetOutputPortInformation(i)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
        }
      }
    return 1;
    }
  return 0;

}

//----------------------------------------------------------------------------
int vtkPVUpdateSuppressor::RequestData(vtkInformation* vtkNotUsed(reqInfo),
                                       vtkInformationVector** inputVector,
                                       vtkInformationVector* outputVector)
{
  // RequestData is only called by its executive when 
  // (Enabled==off) and thus acting as a passthrough filter
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "UpdatePiece: " << this->UpdatePiece << endl;
  os << indent << "UpdateNumberOfPieces: " << this->UpdateNumberOfPieces << endl;
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "CacheSizeKeeper: " << this->CacheSizeKeeper << endl;
  os << indent << "UpdateTime: " << this->UpdateTime << endl;
}
