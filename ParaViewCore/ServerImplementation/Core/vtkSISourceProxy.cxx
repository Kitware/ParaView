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
#include "vtkCompositeDataSet.h"
//#include "vtkGeometryRepresentation.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVInstantiator.h"
#include "vtkPVPostFilter.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"

#include <vector>
#include <vtksys/ios/sstream>

#include <assert.h>

//*****************************************************************************
class vtkSISourceProxy::vtkInternals
{
public:
  std::vector<vtkSmartPointer<vtkAlgorithmOutput> > OutputPorts;
  std::vector<vtkSmartPointer<vtkAlgorithm> > ExtractPieces;
  std::vector<vtkSmartPointer<vtkPVPostFilter> > PostFilters;
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
  this->DisablePipelineExecution = false;
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
    if(!algorithm->GetExecutive()->IsA(this->ExecutiveName))
      {
      vtkExecutive* executive = vtkExecutive::SafeDownCast(
        vtkPVInstantiator::CreateInstance(this->ExecutiveName));
      if (executive)
        {
        algorithm->SetExecutive(executive);
        executive->FastDelete();
        }
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

  if (strcmp("vtkPVCompositeDataPipeline", this->ExecutiveName) == 0)
    {
    //add the post filters to the source proxy
    //so that we can do automatic conversion of properties.
    this->InsertPostFilterIfNecessary(algo, port);
    }
  return true;
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
// FIXME: avoid code-duplication with vtkGeometryRepresentation. However I
// cannot add dependecy on vtkGeometryRepresentation here. Fix it!!!
namespace
{
  bool vtkGeometryRepresentationDoRequestGhostCells(vtkInformation* info)
    {
    vtkMultiProcessController* controller =
      vtkMultiProcessController::GetGlobalController();
    if (controller == NULL || controller->GetNumberOfProcesses() <= 1)
      {
      return false;
      }

    if (vtkUnstructuredGrid::GetData(info) != NULL ||
      vtkCompositeDataSet::GetData(info) != NULL)
      {
      // ensure that there's no WholeExtent to ensure
      // that this UG was never born out of a structured dataset.
      bool has_whole_extent = (info->Has(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) != 0);
      if (!has_whole_extent)
        {
        //cout << "Need ghosts" << endl;
        return true;
        }
      }
    return false;
    }
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::UpdatePipeline(int port, double time, bool doTime)
{
  if(this->DisablePipelineExecution)
    {
    return;
    }

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
  bool req_ghost_cells = vtkGeometryRepresentationDoRequestGhostCells(
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
void vtkSISourceProxy::UpdatePipelineInformation()
{
  if(this->DisablePipelineExecution)
    {
    return;
    }

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
