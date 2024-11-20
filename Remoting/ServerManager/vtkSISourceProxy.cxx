// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSISourceProxy.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkClientServerStreamInstantiator.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataPipeline.h"
#include "vtkPVLogger.h"
#include "vtkPVPostFilter.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"

#include <cassert>
#include <sstream>
#include <vector>

//*****************************************************************************
class vtkSISourceProxy::vtkInternals
{
public:
  std::vector<vtkSmartPointer<vtkAlgorithmOutput>> OutputPorts;
  std::vector<vtkSmartPointer<vtkPVPostFilter>> PostFilters;
};

//*****************************************************************************
vtkStandardNewMacro(vtkSISourceProxy);
//----------------------------------------------------------------------------
vtkSISourceProxy::vtkSISourceProxy()
{
  this->ExecutiveName = nullptr;
  this->SetExecutiveName("vtkPVCompositeDataPipeline");
  this->Internals = new vtkInternals();
  this->PortsCreated = false;
  this->StartEventCounter = 0;
  this->DisablePipelineExecution = false;
}

//----------------------------------------------------------------------------
vtkSISourceProxy::~vtkSISourceProxy()
{
  this->SetExecutiveName(nullptr);
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

  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkSISourceProxy::CreateVTKObjects()
{
  assert(this->ObjectsCreated == false);
  if (!this->Superclass::CreateVTKObjects())
  {
    return false;
  }

  vtkAlgorithm* algorithm = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (algorithm == nullptr)
  {
    return true;
  }

  // Register observer to record the execution time for each algorithm in the
  // local timer-log.
  algorithm->AddObserver(vtkCommand::StartEvent, this, &vtkSISourceProxy::MarkStartEvent);
  algorithm->AddObserver(vtkCommand::EndEvent, this, &vtkSISourceProxy::MarkEndEvent);
  return true;
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::OnCreateVTKObjects()
{
  this->Superclass::OnCreateVTKObjects();

  vtkAlgorithm* algorithm = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (algorithm == nullptr)
  {
    return;
  }

  // Create the right kind of executive.
  if (this->ExecutiveName && !this->GetVTKObject()->IsA("vtkPVDataRepresentation"))
  {
    if (!algorithm->GetExecutive()->IsA(this->ExecutiveName))
    {
      vtkExecutive* executive = vtkExecutive::SafeDownCast(
        vtkClientServerStreamInstantiator::CreateInstance(this->ExecutiveName));
      if (executive)
      {
        algorithm->SetExecutive(executive);
        executive->FastDelete();
      }
    }
  }
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

  vtkInternals& internals = (*this->Internals);
  int ports = algo->GetNumberOfOutputPorts();
  internals.OutputPorts.resize(ports);
  internals.PostFilters.resize(ports);

  for (int cc = 0; cc < ports; cc++)
  {
    internals.OutputPorts[cc] = algo->GetOutputPort(cc);
    if (vtkPVCompositeDataPipeline::SafeDownCast(algo->GetExecutive()) != nullptr)
    {
      // add the post filters to the source proxy
      // so that we can do automatic conversion of properties.
      if (internals.PostFilters[cc] == nullptr)
      {
        internals.PostFilters[cc] = vtkSmartPointer<vtkPVPostFilter>::New();
      }
      internals.PostFilters[cc]->SetInputConnection(internals.OutputPorts[cc]);
      internals.OutputPorts[cc] = internals.PostFilters[cc]->GetOutputPort(0);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::RecreateVTKObjects()
{
  // If a proxy has no PostFilters, the original algorithm's vtkAlgorithmOutput
  // may be used in pipeline connections. Since we don't have access to those,
  // we cannot recreated those pipelines after the VTK object has been
  // recreated. Hence we warn and ignore the request.
  // I don't anticipate a whole lot of use-cases where we'll encounter those
  // since all public-facing ParaView proxies will employ a PostFilter.
  if (this->PortsCreated && this->Internals->PostFilters.empty())
  {
    vtkWarningMacro("You have encountered a proxy that currently does not support call to "
                    "RecreateVTKObjects() properly. Please contact the ParaView developers."
                    "This request will be ignored.");
    return;
  }

  this->Superclass::RecreateVTKObjects();
  if (this->PortsCreated)
  {
    assert(this->Internals->PostFilters.size() > 0);
    this->PortsCreated = false;
    this->CreateOutputPorts();
  }
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
  if (this->DisablePipelineExecution)
  {
    return;
  }

  auto controller = vtkMultiProcessController::GetGlobalController();
  const int processid = controller->GetLocalProcessId();
  const int numprocs = controller->GetNumberOfProcesses();

  // This will create the output ports if needed.
  vtkAlgorithmOutput* output_port = this->GetOutputPort(port);
  if (!output_port)
  {
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_PIPELINE_VERBOSITY(), "%s: update pipeline(%d, %f, %s) ",
    this->GetLogNameOrDefault(), port, time, (doTime ? "true" : "false"));

  vtkAlgorithm* algo = output_port->GetProducer();
  assert(algo);

  auto sddp = vtkStreamingDemandDrivenPipeline::SafeDownCast(algo->GetExecutive());

  sddp->UpdateInformation();

  const int real_port = output_port->GetIndex();

  // Refer to BUG #11811 and BUG #12546. vtkGeometryRepresentation needs
  // ghost-cells if available (11811), but not asking for ghost-cells earlier than the
  // representation results in multiple executes (12546). Hence, we request
  // ghost-cells in UpdatePipeline().
  vtkInformation* outInfo = sddp->GetOutputInformation(real_port);
  const int ghost_levels = vtkProcessModule::GetNumberOfGhostLevelsToRequest(outInfo);

  outInfo->Set(sddp->UPDATE_PIECE_NUMBER(), processid);
  outInfo->Set(sddp->UPDATE_NUMBER_OF_PIECES(), numprocs);
  outInfo->Set(sddp->UPDATE_NUMBER_OF_GHOST_LEVELS(), ghost_levels);
  if (doTime)
  {
    outInfo->Set(sddp->UPDATE_TIME_STEP(), time);
  }
  // get the pipeline result
  bool result = sddp->Update(real_port) == 1;
  if (auto actualAlgo = vtkAlgorithm::SafeDownCast(this->GetVTKObject()))
  {
    // also check if the algorithm set any error codes
    result &= actualAlgo->GetErrorCode() == vtkErrorCode::NoError;
  }
  if (numprocs > 1)
  {
    // Reduce the result to all processes.
    unsigned char sendResult = result, reducedResult;
    controller->Reduce(&sendResult, &reducedResult, 1, vtkCommunicator::LOGICAL_AND_OP, 0);
    // If it's the server root process and the result is not the same as the reduced result,
    // print an error message. If there was an error in all processes, the server root will
    // print the error message, so we don't need to print anything here.
    if (processid == 0 && reducedResult != sendResult)
    {
      auto actualAlgo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
      std::ostringstream message;
      message << (actualAlgo ? actualAlgo->GetObjectDescription() + ": " : "")
              << "There was an error on a server process. Please check the server console or log "
                 "output for details.";
      vtkErrorMacro(<< message.str());
    }
  }
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::UpdatePipelineInformation()
{
  if (this->DisablePipelineExecution)
  {
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_PIPELINE_VERBOSITY(), "%s: update pipeline information",
    this->GetLogNameOrDefault());

  if (this->GetVTKObject())
  {
    vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
    if (algo)
    {
      algo->UpdateInformation();
    }
  }

  // Call UpdatePipelineInformation() on all subproxies.
  for (unsigned int cc = 0; cc < this->GetNumberOfSubSIProxys(); cc++)
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
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(extractSelection->GetVTKObject());
  algo->SetInputConnection(this->GetOutputPort(port));
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::MarkStartEvent()
{
  if (this->StartEventCounter++ == 0)
  {
    std::ostringstream filterName;
    filterName << "Execute " << this->GetLogNameOrDefault() << " id: " << this->GetGlobalID();
    vtkTimerLog::MarkStartEvent(filterName.str().c_str());

    vtkVLogStartScopeF(PARAVIEW_LOG_EXECUTION_VERBOSITY(), vtkLogIdentifier(this), "%s: execute",
      this->GetLogNameOrDefault());
  }
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::MarkEndEvent()
{
  if (--this->StartEventCounter == 0)
  {
    vtkLogEndScope(vtkLogIdentifier(this));

    std::ostringstream filterName;
    filterName << "Execute " << this->GetLogNameOrDefault() << " id: " << this->GetGlobalID();
    vtkTimerLog::MarkEndEvent(filterName.str().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
