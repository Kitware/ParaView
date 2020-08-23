/*=========================================================================

  Program:   ParaView
  Module:    vtkCPProcessor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPProcessor.h"

#include "vtkPVConfig.h" // need ParaView defines before MPI stuff

#include "vtkCPCxxHelper.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPPipeline.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPassArrays.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTemporalDataSetCache.h"

#include <list>
#include <map>
#include <string>
#include <vtksys/SystemTools.hxx>

struct vtkCPProcessorInternals
{
  typedef std::list<vtkSmartPointer<vtkCPPipeline> > PipelineList;
  typedef PipelineList::iterator PipelineListIterator;
  PipelineList Pipelines;

  typedef std::map<std::string, vtkSmartPointer<vtkSMSourceProxy> > CacheList;
  typedef CacheList::iterator CacheListIterator;
  CacheList TemporalCaches;
};

vtkStandardNewMacro(vtkCPProcessor);
vtkMultiProcessController* vtkCPProcessor::Controller = nullptr;
//----------------------------------------------------------------------------
vtkCPProcessor::vtkCPProcessor()
{
  this->Internal = new vtkCPProcessorInternals;
  this->InitializationHelper = nullptr;
  this->WorkingDirectory = nullptr;
  this->TemporalCacheSize = 0;
}

//----------------------------------------------------------------------------
vtkCPProcessor::~vtkCPProcessor()
{
  // in case the adaptor failed to call `vtkCPProcessor::Finalize()`
  // see paraview/paraview#20154
  this->FinalizeAndRemovePipelines();
  if (this->Internal)
  {
    delete this->Internal;
    this->Internal = nullptr;
  }

  if (this->InitializationHelper)
  {
    this->InitializationHelper->Delete();
    this->InitializationHelper = nullptr;
  }
  this->SetWorkingDirectory(nullptr);
}

//----------------------------------------------------------------------------
int vtkCPProcessor::AddPipeline(vtkCPPipeline* pipeline)
{
  if (!pipeline)
  {
    vtkErrorMacro("Pipeline is NULL.");
    return 0;
  }

  this->Internal->Pipelines.push_back(pipeline);
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::GetNumberOfPipelines()
{
  return static_cast<int>(this->Internal->Pipelines.size());
}

//----------------------------------------------------------------------------
vtkCPPipeline* vtkCPProcessor::GetPipeline(int which)
{
  if (which < 0 || which >= this->GetNumberOfPipelines())
  {
    return nullptr;
  }
  int counter = 0;
  vtkCPProcessorInternals::PipelineListIterator iter = this->Internal->Pipelines.begin();
  while (counter <= which)
  {
    if (counter == which)
    {
      return *iter;
    }
    counter++;
    iter++;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkCPProcessor::RemovePipeline(vtkCPPipeline* pipeline)
{
  this->Internal->Pipelines.remove(pipeline);
}

//----------------------------------------------------------------------------
void vtkCPProcessor::RemoveAllPipelines()
{
  this->Internal->Pipelines.clear();
}

//----------------------------------------------------------------------------
vtkObject* vtkCPProcessor::NewInitializationHelper()
{
  return vtkCPCxxHelper::New();
}

//----------------------------------------------------------------------------
int vtkCPProcessor::Initialize(const char* workingDirectory)
{
  if (this->InitializationHelper == nullptr)
  {
    this->InitializationHelper = this->NewInitializationHelper();
  }
  // make sure the directory exists here so that we only do it once
  if (workingDirectory)
  {
    vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
    int success = 1;
    if (controller == nullptr || controller->GetLocalProcessId() == 0)
    {
      success = vtksys::SystemTools::MakeDirectory(workingDirectory) == true ? 1 : 0;
      if (success == 0)
      {
        vtkWarningMacro("Could not make "
          << workingDirectory << " directory. "
          << "Results will be generated in current working directory instead.");
      }
    }
    if (controller)
    {
      controller->Broadcast(&success, 1, 0);
    }
    if (success)
    {
      this->SetWorkingDirectory(workingDirectory);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::Initialize(vtkMPICommunicatorOpaqueComm& comm, const char* workingDirectory)
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  if (vtkCPProcessor::Controller)
  {
    vtkErrorMacro("Can only initialize with a communicator once per process.");
    return 0;
  }
  if (this->InitializationHelper == nullptr)
  {
    vtkMPICommunicator* communicator = vtkMPICommunicator::New();
    communicator->InitializeExternal(&comm);
    vtkMPIController* controller = vtkMPIController::New();
    controller->SetCommunicator(communicator);
    this->Controller = controller;
    this->Controller->SetGlobalController(controller);
    communicator->Delete();
    return this->Initialize(workingDirectory);
  }
  return 1;
#else
  static_cast<void>(&comm); // get rid of variable not used warning
  return this->Initialize(workingDirectory);
#endif
}

//----------------------------------------------------------------------------
int vtkCPProcessor::RequestDataDescription(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
  }

  // first set all inputs to be off and set to on as needed.
  // we don't use vtkCPInputDataDescription::Reset() because
  // that will reset any field names that were added in.
  for (unsigned int i = 0; i < dataDescription->GetNumberOfInputDescriptions(); i++)
  {
    dataDescription->GetInputDescription(i)->GenerateMeshOff();
    dataDescription->GetInputDescription(i)->AllFieldsOff();
  }

  dataDescription->ResetInputDescriptions();
  int doCoProcessing = 0;
  for (vtkCPProcessorInternals::PipelineListIterator iter = this->Internal->Pipelines.begin();
       iter != this->Internal->Pipelines.end(); iter++)
  {
    if (iter->GetPointer()->RequestDataDescription(dataDescription))
    {
      doCoProcessing = 1;
    }
  }
  return doCoProcessing;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::CoProcess(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
  }
  int success = 1;
  // We need to add in information like channel name and time value here to the
  // field data. The channel name is used to automatically keep track of which
  // channel things are happening with so we can hide that complexity from the user.
  // The time value needs to be added here since the XML writers will add that
  // information in and if the user tries to create a Catalyst Python script
  // pipeline that uses that field data (e.g. Annotate Field Data filter) from those
  // files they'll get failures.
  for (unsigned int i = 0; i < dataDescription->GetNumberOfInputDescriptions(); i++)
  {
    if (vtkDataObject* input = dataDescription->GetInputDescription(i)->GetGrid())
    {
      vtkNew<vtkStringArray> catalystChannel;
      catalystChannel->SetName(this->GetInputArrayName());
      catalystChannel->InsertNextValue(dataDescription->GetInputDescriptionName(i));
      input->GetFieldData()->AddArray(catalystChannel);

      vtkNew<vtkDoubleArray> time;
      time->SetNumberOfTuples(1);
      time->SetTypedComponent(0, 0, dataDescription->GetTime());
      time->SetName("TimeValue");
      input->GetFieldData()->AddArray(time);

      input->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dataDescription->GetTime());
      if (this->GetTemporalCacheSize() > 0)
      {
        vtkSMSourceProxy* cacheForInput =
          this->GetTemporalCache(dataDescription->GetInputDescriptionName(i));
        if (cacheForInput)
        {
          vtkTemporalDataSetCache* tc =
            vtkTemporalDataSetCache::SafeDownCast(cacheForInput->GetClientSideObject());
          tc->SetInputDataObject(input);

          tc->UpdateTimeStep(dataDescription->GetTime());
        }
      }
    }
  }

  std::string originalWorkingDirectory;
  if (this->WorkingDirectory)
  {
    originalWorkingDirectory = vtksys::SystemTools::GetCurrentWorkingDirectory();
    vtksys::SystemTools::ChangeDirectory(this->WorkingDirectory);
  }
  for (vtkCPProcessorInternals::PipelineListIterator iter = this->Internal->Pipelines.begin();
       iter != this->Internal->Pipelines.end(); iter++)
  {
    // Reset dataDescription so that we can check each pipeline again
    // before calling CoProcess to make sure which pipelines should
    // be executing.
    for (unsigned int i = 0; i < dataDescription->GetNumberOfInputDescriptions(); i++)
    {
      dataDescription->GetInputDescription(i)->Reset();
    }
    if (iter->GetPointer()->RequestDataDescription(dataDescription))
    {
      // now we need to filter out arrays that are not needed by this pipeline
      // but were requested by other pipelines at this time step
      vtkSmartPointer<vtkCPDataDescription> dataDescriptionCopy = dataDescription;
      if (this->Internal->Pipelines.size() > 1)
      {
        // if there's only one pipeline we don't have to worry about getting
        // more arrays than we requesting arrays
        dataDescriptionCopy = vtkSmartPointer<vtkCPDataDescription>::New();
        dataDescriptionCopy->Copy(dataDescription);
        for (unsigned int i = 0; i < dataDescription->GetNumberOfInputDescriptions(); i++)
        {
          vtkCPInputDataDescription* idd = dataDescriptionCopy->GetInputDescription(i);
          if (idd->GetIfGridIsNecessary() == true && idd->GetAllFields() == false)
          {
            vtkNew<vtkPassArrays> passArrays;
            passArrays->UseFieldTypesOn();
            passArrays->AddFieldType(vtkDataObject::FIELD);
            passArrays->AddFieldType(vtkDataObject::POINT);
            passArrays->AddFieldType(vtkDataObject::CELL);
            passArrays->SetInputData(idd->GetGrid());
            for (unsigned int j = 0; j < idd->GetNumberOfFields(); j++)
            {
              int type = idd->GetFieldType(j);
              passArrays->AddArray(type, idd->GetFieldName(j));
            }
            passArrays->Update();
            idd->SetGrid(passArrays->GetOutput());
          }
        }
      }
      if (!iter->GetPointer()->CoProcess(dataDescriptionCopy))
      {
        success = 0;
      }
    }
  }
  if (originalWorkingDirectory.empty() == false)
  {
    vtksys::SystemTools::ChangeDirectory(originalWorkingDirectory);
  }
  // we want to reset everything here to make sure that new information
  // is properly passed in the next time.
  dataDescription->ResetAll();
  return success;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::Finalize()
{
  if (this->Controller)
  {
    this->Controller->SetGlobalController(nullptr);
    this->Controller->Finalize(1);
    this->Controller->Delete();
    this->Controller = nullptr;
  }
  this->FinalizeAndRemovePipelines();
  return 1;
}

//----------------------------------------------------------------------------
void vtkCPProcessor::FinalizeAndRemovePipelines()
{
  for (vtkCPProcessorInternals::PipelineListIterator it = this->Internal->Pipelines.begin();
       it != this->Internal->Pipelines.end(); it++)
  {
    if (!it->GetPointer()->Finalize())
    {
      vtkWarningMacro("Problems finalizing a Catalyst pipeline.");
    }
  }

  this->RemoveAllPipelines();
}

//----------------------------------------------------------------------------
void vtkCPProcessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkCPProcessor::SetTemporalCacheSize(int nv)
{
  if (this->TemporalCacheSize == nv)
  {
    return;
  }
  this->TemporalCacheSize = nv;
  for (vtkCPProcessorInternals::CacheListIterator it = this->Internal->TemporalCaches.begin();
       it != this->Internal->TemporalCaches.end(); it++)
  {
    vtkTemporalDataSetCache* tc =
      vtkTemporalDataSetCache::SafeDownCast(it->second.GetPointer()->GetClientSideObject());
    tc->SetCacheSize(this->TemporalCacheSize);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCPProcessor::MakeTemporalCache(const char* name)
{
  if (this->Internal->TemporalCaches.find(name) != this->Internal->TemporalCaches.end())
  {
    // Its ah, very nice but tell him we already got one!
    return;
  }

  // have to make a ParaView level object so that python can grab and work
  // with it. Unfortunately this has to wait until paraview is running or
  // we crash with no sessionProxyManager.
  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  vtkSMSessionProxyManager* sessionProxyManager = proxyManager->GetActiveSessionProxyManager();
  if (!sessionProxyManager)
  {
    return;
  }
  vtkSmartPointer<vtkSMSourceProxy> producer;
  producer.TakeReference(vtkSMSourceProxy::SafeDownCast(
    sessionProxyManager->NewProxy("sources", "TemporalCache"))); // note: source
  producer->UpdateVTKObjects();
  vtkTemporalDataSetCache* tc =
    vtkTemporalDataSetCache::SafeDownCast(producer->GetClientSideObject());
  tc->SetCacheSize(this->TemporalCacheSize);
  tc->CacheInMemkindOn();
  this->Internal->TemporalCaches[name] = producer;
}

//----------------------------------------------------------------------------
vtkSMSourceProxy* vtkCPProcessor::GetTemporalCache(const char* name)
{
  if (this->Internal->TemporalCaches.find(name) == this->Internal->TemporalCaches.end())
  {
    return nullptr;
  }
  return this->Internal->TemporalCaches[name];
}
