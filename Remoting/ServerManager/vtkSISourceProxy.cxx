// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSISourceProxy.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerStreamInstantiator.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataPipeline.h"
#include "vtkPVLogger.h"
#include "vtkPVPostFilter.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif

#include <cassert>
#include <optional>
#include <sstream>
#include <vector>

//*****************************************************************************
class vtkSISourceProxy::vtkInternals
{
public:
  std::vector<vtkSmartPointer<vtkAlgorithmOutput>> OutputPorts;
  std::vector<vtkSmartPointer<vtkPVPostFilter>> PostFilters;

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  // std::nullopt means there is no request to Receive/Test/Cancel
  std::vector<std::optional<vtkMPICommunicator::Request>> AbortRequests;
#endif
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

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  auto controller = vtkMultiProcessController::GetGlobalController();
  const int processId = controller->GetLocalProcessId();
  const int numProcs = controller->GetNumberOfProcesses();
  if (numProcs > 1)
  {
    if (processId == 0)
    {
      // On rank 0, use request 1 to N
      this->Internals->AbortRequests.resize(numProcs);
    }
    else
    {
      // On rank N, use request 0
      this->Internals->AbortRequests.resize(1);
    }
  }
#endif
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

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  // Register abort check event to trigger abort reduction
  algorithm->AddObserver(vtkCommand::AbortCheckEvent, this, &vtkSISourceProxy::ReduceAbort);
  algorithm->AddObserver(
    vtkCommand::CleanupAbortCheckEvent, this, &vtkSISourceProxy::CleanupAbortRequests);
#endif

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
void vtkSISourceProxy::EnableAbortCheck()
{
  if (this->DisablePipelineExecution)
  {
    return;
  }

  // Enable CheckAbort in progress handler for underlying VTK object on first process only
  auto controller = vtkMultiProcessController::GetGlobalController();
  const int processid = controller->GetLocalProcessId();
  if (processid == 0 && this->GetVTKObject())
  {
    vtkVLogScopeF(
      PARAVIEW_LOG_PIPELINE_VERBOSITY(), "%s: enable CheckAbort", this->GetLogNameOrDefault());

    vtkClientServerStream substream;
    substream << vtkClientServerStream::Invoke << vtkClientServerID(1) << "GetActiveProgressHandler"
              << vtkClientServerStream::End;
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << substream << "EnableAbortCheck"
           << static_cast<int>(this->GetGlobalID()) << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::ClearAbortFlags()
{
  if (this->DisablePipelineExecution)
  {
    return;
  }

  vtkVLogScopeF(
    PARAVIEW_LOG_PIPELINE_VERBOSITY(), "%s: clear  abort flags", this->GetLogNameOrDefault());

  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (algo)
  {
    algo->AbortExecuteOff();
    algo->AbortOutputOff();
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

  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (algo)
  {
    algo->UpdateInformation();
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
void vtkSISourceProxy::ReduceAbort()
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (!algo)
  {
    return;
  }

  unsigned char abort = algo->GetAbortExecute();
  vtkMPIController* controller =
    vtkMPIController::SafeDownCast(vtkMultiProcessController::GetGlobalController());
  if (!controller)
  {
    return;
  }

  const int numProcs = controller->GetNumberOfProcesses();
  if (numProcs <= 1)
  {
    return;
  }

  // NOLINTBEGIN(bugprone-unchecked-optional-access)
  const int processId = controller->GetLocalProcessId();
  if (processId == 0)
  {
    // On rank 0, we send the abort flag to all other ranks
    // However they are three cases
    // 1. Nothing was ever sent, so we need to create the request first and then NoBlockSend
    // 2. There is an ongoing NoBlockSend, check if completed and if so, send again
    // 3. Abort was triggered, then cancel the request and send
    for (int i = 1; i < numProcs; i++)
    {
      if (!this->Internals->AbortRequests[i].has_value())
      {
        // First request to send
        this->Internals->AbortRequests[i] = vtkMPICommunicator::Request();
        controller->NoBlockSend(
          &abort, 1, i, vtkPVProgressHandler::ABORT_TAG, this->Internals->AbortRequests[i].value());
      }
      else if (this->Internals->AbortRequests[i].value().Test())
      {
        // Only send a new request if previous one completed
        controller->NoBlockSend(
          &abort, 1, i, vtkPVProgressHandler::ABORT_TAG, this->Internals->AbortRequests[i].value());
      }
      else if (abort)
      {
        // In case of abort, always send a new request
        this->Internals->AbortRequests[i].value().Cancel();
        this->Internals->AbortRequests[i] = vtkMPICommunicator::Request();
        controller->NoBlockSend(
          &abort, 1, i, vtkPVProgressHandler::ABORT_TAG, this->Internals->AbortRequests[i].value());
      }
    }
  }
  else
  {
    // On rank N, we receive the abort flag from rank 0
    // They are two cases:
    // 1. Nothing was ever received, create a request and then NoBlockReceive
    // 2. There is an ongoing NoBlockReceive, if it completes, check it, if not aborted
    // then start receiving again (and check right away just in case)
    if (!this->Internals->AbortRequests[0].has_value())
    {
      // First request to receive
      this->Internals->AbortRequests[0] = vtkMPICommunicator::Request();
      controller->NoBlockReceive(
        &abort, 1, 0, vtkPVProgressHandler::ABORT_TAG, this->Internals->AbortRequests[0].value());
    }
    else if (this->Internals->AbortRequests[0].value().Test())
    {
      this->Internals->AbortRequests[0].reset();

      // Check previous receive operation result
      if (abort)
      {
        algo->SetAbortExecuteAndUpdateTime();
        return;
      }

      // Only receive if not already receiving
      this->Internals->AbortRequests[0] = vtkMPICommunicator::Request();
      controller->NoBlockReceive(
        &abort, 1, 0, vtkPVProgressHandler::ABORT_TAG, this->Internals->AbortRequests[0].value());

      // Check if receive was immediately done
      if (this->Internals->AbortRequests[0].value().Test())
      {
        this->Internals->AbortRequests[0].reset();
        if (abort)
        {
          algo->SetAbortExecuteAndUpdateTime();
        }
      }
    }
    // NOLINTEND(bugprone-unchecked-optional-access)
  }
#endif
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::CleanupAbortRequests()
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (!algo)
  {
    return;
  }

  auto controller =
    vtkMPIController::SafeDownCast(vtkMultiProcessController::GetGlobalController());
  const int numProcs = controller->GetNumberOfProcesses();
  const int processId = controller->GetLocalProcessId();
  if (numProcs <= 1)
  {
    return;
  }

  unsigned char cleanup = 0;
  unsigned char abort = algo->GetAbortExecute();

  // NOLINTBEGIN(bugprone-unchecked-optional-access)
  // Cancel any remaining requests
  if (processId == 0)
  {
    for (int i = 1; i < numProcs; i++)
    {
      // First wait for satellite rank to reach cleanup
      controller->Receive(&cleanup, 1, i, vtkPVProgressHandler::CLEANUP_TAG);

      // NoBlockSend request may be pending completion, test first
      if (this->Internals->AbortRequests[i].has_value() &&
        !this->Internals->AbortRequests[i].value().Test())
      {
        this->Internals->AbortRequests[i].value().Cancel();
      }
      this->Internals->AbortRequests[i].reset();

      // Send a last abort flag so that all ranks are aware if abort happened
      controller->Send(&abort, 1, i, vtkPVProgressHandler::CLEANUP_TAG);
    }
  }
  else
  {
    // Send cleanup to main rank and synchronize
    vtkMPICommunicator::Request req;
    controller->NoBlockSend(&cleanup, 1, 0, vtkPVProgressHandler::CLEANUP_TAG, req);
    req.Wait();

    // Remaining NoBlockReceive request just needs canceling
    if (this->Internals->AbortRequests[0].has_value())
    {
      this->Internals->AbortRequests[0].value().Cancel();
    }
    this->Internals->AbortRequests[0].reset();

    // Cleanup any send already in the pipe and that cannot be cancelled properly
    do
    {
      controller->NoBlockReceive(&cleanup, 1, 0, vtkPVProgressHandler::ABORT_TAG, req);
    } while (req.Test());
    req.Cancel();

    // Receive the last abort flag so that we know for sure if abort happened
    controller->Receive(&abort, 1, 0, vtkPVProgressHandler::CLEANUP_TAG);
  }
  // NOLINTEND(bugprone-unchecked-optional-access)

  if (abort)
  {
    algo->SetAbortExecuteAndUpdateTime();
  }
#endif
}

//----------------------------------------------------------------------------
void vtkSISourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
