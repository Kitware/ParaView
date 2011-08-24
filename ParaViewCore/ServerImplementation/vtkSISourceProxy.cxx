/*=========================================================================

  Program:   ParaView
  Module:    vtkSISourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSISourceProxy.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkClientServerInterpreter.h"
#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkGeometryRepresentation.h"
#include "vtkInformation.h"
#include "vtkInstantiator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPriorityHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVExtentTranslator.h"
#include "vtkPVPostFilter.h"
#include "vtkPVXMLElement.h"
#include "vtkSIPVRepresentationProxy.h"
#include "vtkSMMessage.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#include <assert.h>

bool vtkSISourceProxy::DisableExtentsTranslator = false;

//*****************************************************************************
class vtkSISourceProxy::vtkInternals
{
public:
  vtkstd::vector<vtkSmartPointer<vtkAlgorithmOutput> > OutputPorts;
  vtkstd::vector<vtkSmartPointer<vtkAlgorithm> > ExtractPieces;
  vtkstd::vector<vtkSmartPointer<vtkPVPostFilter> > PostFilters;
};

//*****************************************************************************
vtkStandardNewMacro(vtkSISourceProxy);
//----------------------------------------------------------------------------
vtkSISourceProxy::vtkSISourceProxy()
{
  this->ExecutiveName = 0;
  this->SetExecutiveName("vtkPVCompositeDataPipeline");
  this->Internals = new vtkInternals();
  this->PortsCreated = false;
}

//----------------------------------------------------------------------------
vtkSISourceProxy::~vtkSISourceProxy()
{
  this->SetExecutiveName(0);
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkSISourceProxy::GetOutputPort(int port)
{
  this->CreateOutputPorts();
  if (static_cast<int>(this->Internals->OutputPorts.size()) > port)
    {
    return this->Internals->OutputPorts[port];
    }

  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSISourceProxy::CreateVTKObjects(vtkSMMessage* message)
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
    vtkCommand::StartEvent, this, &vtkSISourceProxy::MarkStartEvent);
  algorithm->AddObserver(
    vtkCommand::EndEvent, this, &vtkSISourceProxy::MarkEndEvent);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSISourceProxy::CreateOutputPorts()
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
bool vtkSISourceProxy::InitializeOutputPort(vtkAlgorithm* algo, int port)
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
bool vtkSISourceProxy::CreateTranslatorIfNecessary(vtkAlgorithm* algo, int port)
{
  if(this->DisableExtentsTranslator)
    {
    return false;
    }

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
void vtkSISourceProxy::InsertExtractPiecesIfNecessary(
  vtkAlgorithm*, int port)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkAlgorithmOutput* outputPort = this->Internals->OutputPorts[port];
  vtkAlgorithm* algorithm = outputPort->GetProducer();
  assert(algorithm != NULL);

  algorithm->UpdateInformation();
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      algorithm->GetExecutive());
  vtkDataObject* outputDO = algorithm->GetOutputDataObject(outputPort->GetIndex());
  if (outputDO == NULL || sddp == NULL)
    {
    vtkErrorMacro("Missing data information.");
    return;
    }

  if (pm->GetNumberOfLocalPartitions() == 1)
    {
    // Don't add anything if we are only using one processes.
    return;
    }
  if (sddp->GetMaximumNumberOfPieces(outputPort->GetIndex()) != 1)
    {
    // The source can already produce pieces.
    return;
    }

  const char* extractPiecesClass  = 0;
  if (outputDO->IsA("vtkPolyData"))
    {
    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      extractPiecesClass = "vtkExtractPolyDataPiece";
      }
    else
      {
      extractPiecesClass = "vtkTransmitPolyDataPiece";
      }
    }
  else if (outputDO->IsA("vtkUnstructuredGrid"))
    {
    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      extractPiecesClass = "vtkExtractUnstructuredGridPiece";
      }
    else
      {
      extractPiecesClass = "vtkTransmitUnstructuredGridPiece";
      }
    }
  else if (outputDO->IsA("vtkHierarchicalBoxDataSet") ||
           outputDO->IsA("vtkMultiBlockDataSet"))
    {
    extractPiecesClass = "vtkExtractPiece";
    }

  // If no filter is to be inserted, just return.
  if (extractPiecesClass == NULL)
    {
    return;
    }

  vtkAlgorithm* extractPieces = vtkAlgorithm::SafeDownCast(
    this->GetInterpreter()->NewInstance(extractPiecesClass));
  if (!extractPieces)
    {
    vtkErrorMacro("Failed to create " << extractPiecesClass);
    return;
    }

  // Set the right executive
  vtkCompositeDataPipeline* exec = vtkCompositeDataPipeline::New();
  extractPieces->SetExecutive(exec);
  exec->FastDelete();

  this->Internals->ExtractPieces[port] = extractPieces;
  extractPieces->FastDelete();
  extractPieces->SetInputConnection(outputPort);

  // update the OutputPorts so that the output port from extract-pieces is
  // now used as the output port from this proxy.
  this->Internals->OutputPorts[port] = extractPieces->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::InsertPostFilterIfNecessary(vtkAlgorithm*, int port)
{
  vtkPVPostFilter* postFilter = vtkPVPostFilter::New();
  this->Internals->PostFilters[port] = postFilter;
  postFilter->FastDelete();

  postFilter->SetInputConnection(this->Internals->OutputPorts[port]);

  // Now substitute port to point to the post-filter.
  this->Internals->OutputPorts[port] = postFilter->GetOutputPort(0);
}

//----------------------------------------------------------------------------
bool vtkSISourceProxy::ReadXMLAttributes(vtkPVXMLElement* element)
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
void vtkSISourceProxy::UpdatePipeline(int port, double time, bool doTime)
{
  int processid =
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  int numprocs =
    vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses();

  // This will create the output ports if needed.
  vtkAlgorithmOutput* output_port = this->GetOutputPort(port);
  if (!output_port)
    {
    return;
    }
  vtkAlgorithm* algo = output_port->GetProducer();
  assert(algo);


  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      algo->GetExecutive());

  sddp->UpdateInformation();
  sddp->UpdateDataObject();

  int real_port = output_port->GetIndex();

  // Refer to BUG #11811 and BUG #12546. vtkGeometryRepresentation needs
  // ghost-cells if available (11811), but not asking for ghost-cells earlier than the
  // representation results in multiple executes (12546). Hence, we request
  // ghost-cells in UpdatePipeline().
  bool req_ghost_cells = vtkGeometryRepresentation::DoRequestGhostCells(
    sddp->GetOutputInformation(real_port));

  sddp->SetUpdateExtent(real_port, processid, numprocs, /*ghost level*/
    req_ghost_cells?1 : 0);
  if (doTime)
    {
    sddp->SetUpdateTimeStep(real_port, time);
    }
  sddp->Update(real_port);
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::UpdateStreamingPipeline(
  int pass, int num_of_passes, double resolution,
  int port, double time, bool doTime)
{
  vtkAlgorithm* algo = this->GetOutputPort(port)->GetProducer();
  assert(algo);
  algo->UpdateInformation();

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
void vtkSISourceProxy::UpdatePipelineInformation()
{
  if (this->GetVTKObject())
    {
    vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
    if(algo)
      {
      algo->UpdateInformation();
      }
    }

  // Call UpdatePipelineInformation() on all subproxies.
  for (unsigned int cc=0; cc < this->GetNumberOfSubSIProxys(); cc++)
    {
    vtkSISourceProxy* src = vtkSISourceProxy::SafeDownCast(this->GetSubSIProxy(cc));
    if (src)
      {
      src->UpdatePipelineInformation();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::SetupSelectionProxy(int port, vtkSIProxy* extractSelection)
{
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(
    extractSelection->GetVTKObject());
  algo->SetInputConnection(this->GetOutputPort(port));
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::SetDisableExtentsTranslator(bool value)
{
  DisableExtentsTranslator = value;
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::MarkStartEvent()
{
  vtksys_ios::ostringstream filterName;
  filterName
    << "Execute "
    << (this->GetVTKClassName()?  this->GetVTKClassName() : this->GetClassName())
    << " id: " << this->GetGlobalID();
  vtkTimerLog::MarkStartEvent(filterName.str().c_str());
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::MarkEndEvent()
{
  vtksys_ios::ostringstream filterName;
  filterName
    << "Execute "
    << (this->GetVTKClassName()?  this->GetVTKClassName() : this->GetClassName())
    << " id: " << this->GetGlobalID();
  vtkTimerLog::MarkEndEvent(filterName.str().c_str());
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
