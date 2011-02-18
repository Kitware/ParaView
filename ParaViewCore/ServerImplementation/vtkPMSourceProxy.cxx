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
#include "vtkPMSourceProxy.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInstantiator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPMPVRepresentationProxy.h"
#include "vtkPriorityHelper.h"
#include "vtkPVExtentTranslator.h"
#include "vtkPVExtractPieces.h"
#include "vtkPVPostFilter.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#include <assert.h>

//*****************************************************************************
class vtkPMSourceProxy::vtkInternals
{
public:
  vtkstd::vector<vtkSmartPointer<vtkAlgorithmOutput> > OutputPorts;
  vtkstd::vector<vtkSmartPointer<vtkPVExtractPieces> > ExtractPieces;
  vtkstd::vector<vtkSmartPointer<vtkPVPostFilter> > PostFilters;
};

//*****************************************************************************
vtkStandardNewMacro(vtkPMSourceProxy);
//----------------------------------------------------------------------------
vtkPMSourceProxy::vtkPMSourceProxy()
{
  this->ExecutiveName = 0;
  this->SetExecutiveName("vtkPVCompositeDataPipeline");
  this->Internals = new vtkInternals();
  this->PortsCreated = false;
}

//----------------------------------------------------------------------------
vtkPMSourceProxy::~vtkPMSourceProxy()
{
  this->SetExecutiveName(0);
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPMSourceProxy::GetOutputPort(int port)
{
  this->CreateOutputPorts();
  if (static_cast<int>(this->Internals->OutputPorts.size()) > port)
    {
    return this->Internals->OutputPorts[port];
    }

  return NULL;
}

//----------------------------------------------------------------------------
bool vtkPMSourceProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }

  if (!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  vtkAlgorithm* algorithm = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (algorithm == NULL)
    {
    return true;
    }

  if (this->ExecutiveName &&
    !this->GetVTKObject()->IsA("vtkPVDataRepresentation"))
    {
    vtkExecutive* executive = vtkExecutive::SafeDownCast(
      vtkInstantiator::CreateInstance(this->ExecutiveName));
    if (executive)
      {
      algorithm->SetExecutive(executive);
      executive->FastDelete();
      }
    }

  // Register observer to record the execution time for each algorithm in the
  // local timer-log.
  algorithm->AddObserver(
    vtkCommand::StartEvent, this, &vtkPMSourceProxy::MarkStartEvent);
  algorithm->AddObserver(
    vtkCommand::EndEvent, this, &vtkPMSourceProxy::MarkEndEvent);
  return true;
}

//----------------------------------------------------------------------------
bool vtkPMSourceProxy::CreateOutputPorts()
{
  if (this->PortsCreated)
    {
    return true;
    }
  this->PortsCreated = true;
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (!algo)
    {
    return true;
    }

  int ports = algo->GetNumberOfOutputPorts();
  this->Internals->OutputPorts.resize(ports);
  this->Internals->ExtractPieces.resize(ports);
  this->Internals->PostFilters.resize(ports);

  for (int cc=0; cc < ports; cc++)
    {
    if (!this->InitializeOutputPort(algo, cc))
      {
      return false;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkPMSourceProxy::InitializeOutputPort(vtkAlgorithm* algo, int port)
{
  // Save the output port in internal data-structure.
  this->Internals->OutputPorts[port] = algo->GetOutputPort(port);
  this->CreateTranslatorIfNecessary(algo, port);

  int num_of_required_inputs = 0;
  int numInputs = algo->GetNumberOfInputPorts();
  for (int cc=0; cc < numInputs; cc++)
    {
    vtkInformation* info = algo->GetInputPortInformation(cc);
    if (info && !info->Has(vtkAlgorithm::INPUT_IS_OPTIONAL()))
      {
      num_of_required_inputs++;
      }
    }

  if (algo->IsA("vtkPVEnSightMasterServerReader") == 0 &&
    algo->IsA("vtkPVUpdateSuppressor") == 0 &&
    algo->IsA("vtkMPIMoveData") == 0 &&
    num_of_required_inputs == 0)
    {
    this->InsertExtractPiecesIfNecessary(algo, port);
    }

  if (strcmp("vtkPVCompositeDataPipeline", this->ExecutiveName) == 0)
    {
    //add the post filters to the source proxy
    //so that we can do automatic conversion of properties.
    this->InsertPostFilterIfNecessary(algo, port);
    }
  return true;
}

//----------------------------------------------------------------------------
// Create the extent translator (sources with no inputs only).
// Needs to be before "ExtractPieces" because translator propagates.
bool vtkPMSourceProxy::CreateTranslatorIfNecessary(vtkAlgorithm* algo, int port)
{
  // Do not overwrite custom extent translators.
  // PVExtent translator should really be the default,
  // Then we would not need to do this.
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(algo->GetExecutive());
  assert(sddp != NULL);
  vtkExtentTranslator* translator = sddp->GetExtentTranslator(port);
  if (strcmp(translator->GetClassName(), "vtkExtentTranslator") == 0)
    {
    vtkPVExtentTranslator* pvtranslator = vtkPVExtentTranslator::New();
    pvtranslator->SetOriginalSource(algo);
    pvtranslator->SetPortIndex(port);
    sddp->SetExtentTranslator(port, pvtranslator);
    pvtranslator->Delete();
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::InsertExtractPiecesIfNecessary(vtkAlgorithm*, int port)
{

  vtkPVExtractPieces* extractPieces = vtkPVExtractPieces::New();
  this->Internals->ExtractPieces[port] = extractPieces;
  extractPieces->FastDelete();

  extractPieces->SetInputConnection(this->Internals->OutputPorts[port]);

  // update the OutputPorts so that the output port from vtkPVExtractPieces is
  // now used as the output port from this proxy.
  this->Internals->OutputPorts[port] = extractPieces->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::InsertPostFilterIfNecessary(vtkAlgorithm*, int port)
{
  vtkPVPostFilter* postFilter = vtkPVPostFilter::New();
  this->Internals->PostFilters[port] = postFilter;
  postFilter->FastDelete();

  postFilter->SetInputConnection(this->Internals->OutputPorts[port]);

  // Now substitute port to point to the post-filter.
  this->Internals->OutputPorts[port] = postFilter->GetOutputPort(0);
}

//----------------------------------------------------------------------------
bool vtkPMSourceProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(element))
    {
    return false;
    }

  const char* executiveName = element->GetAttribute("executive");
  if (executiveName)
    {
    this->SetExecutiveName(executiveName);
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::UpdatePipeline(int port, double time, bool doTime)
{
  int processid =
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  int numprocs =
    vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses();

  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  assert(algo);

  algo->UpdateInformation();
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      algo->GetExecutive());

  sddp->SetUpdateExtent(port, processid, numprocs, /*ghost level*/0);
  if (doTime)
    {
    sddp->SetUpdateTimeStep(port, time);
    }
  sddp->Update(port);
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::UpdateStreamingPipeline(
  int pass, int num_of_passes, double resolution,
  int port, double time, bool doTime)
{
  int processid =
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  int numprocs =
    vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses();

  vtkPriorityHelper* helper = vtkPriorityHelper::New();
  helper->SetInputConnection(this->GetOutputPort(port));
  helper->SetSplitUpdateExtent(
    port,//algorithm's output port
    processid, //processor
    numprocs, //numprocessors
    pass, //pass
    num_of_passes, //number of passes
    resolution //resolution
  );

  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  assert(algo);
  algo->UpdateInformation();
  if (doTime)
    {
    vtkStreamingDemandDrivenPipeline* sddp =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(algo->GetExecutive());
    sddp->SetUpdateTimeStep(port, time);
    }
  helper->Update();
  helper->Delete();
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::SetupSelectionProxy(int port, vtkPMProxy* extractSelection)
{
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(
    extractSelection->GetVTKObject());
  algo->SetInputConnection(this->GetOutputPort(port));
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::MarkStartEvent()
{
  vtksys_ios::ostringstream filterName;
  filterName
    << "Execute "
    << (this->GetVTKClassName()?  this->GetVTKClassName() : this->GetClassName())
    << " id: " << this->GetGlobalID();
  vtkTimerLog::MarkStartEvent(filterName.str().c_str());
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::MarkEndEvent()
{
  vtksys_ios::ostringstream filterName;
  filterName
    << "Execute "
    << (this->GetVTKClassName()?  this->GetVTKClassName() : this->GetClassName())
    << " id: " << this->GetGlobalID();
  vtkTimerLog::MarkEndEvent(filterName.str().c_str());
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
