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
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVExtentTranslator.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#include <assert.h>

//*****************************************************************************
class vtkPMSourceProxy::vtkInternals
{
public:
  vtkstd::vector<vtkClientServerID> OutputPortIDs;
  vtkstd::vector<vtkClientServerID> ExtractPiecesIDs;
  vtkstd::vector<vtkClientServerID> PostFilterIDs;
};
//*****************************************************************************
vtkStandardNewMacro(vtkPMSourceProxy);
//----------------------------------------------------------------------------
vtkPMSourceProxy::vtkPMSourceProxy()
{
  this->ExecutiveName = 0;
  this->SetExecutiveName("vtkPVCompositeDataPipeline");

  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkPMSourceProxy::~vtkPMSourceProxy()
{
  if (this->Interpreter)
    {
    vtkClientServerStream stream;
    vtkstd::vector<vtkClientServerID>::iterator iter;
    for (iter = this->Internals->ExtractPiecesIDs.begin();
      iter != this->Internals->ExtractPiecesIDs.end(); ++iter)
      {
      if (!iter->IsNull())
        {
        stream << vtkClientServerStream::Delete << *iter
          << vtkClientServerStream::End;
        }
      }
    for (iter = this->Internals->PostFilterIDs.begin();
      iter != this->Internals->PostFilterIDs.end(); ++iter)
      {
      if (!iter->IsNull())
        {
        stream << vtkClientServerStream::Delete << *iter
          << vtkClientServerStream::End;
        }
      }

    this->Interpreter->ProcessStream(stream);
    }

  this->SetExecutiveName(0);
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPMSourceProxy::GetOutputPortID(int port)
{
  if (static_cast<int>(this->Internals->OutputPortIDs.size()) > port)
    {
    return this->Internals->OutputPortIDs[port];
    }

  return vtkClientServerID();
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

  vtkClientServerID sourceID = this->GetVTKObjectID();

  if (sourceID.IsNull())
    {
    return true;
    }

  vtkClientServerStream stream;
  if (this->ExecutiveName &&
    !this->GetVTKObject()->IsA("vtkPVDataRepresentation"))
    {
    vtkClientServerID execId = this->Interpreter->GetNextAvailableId();
    stream << vtkClientServerStream::New
           << this->ExecutiveName
           << execId
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << sourceID
           << "SetExecutive"
           << execId
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Delete
           << execId
           << vtkClientServerStream::End;
    }

#ifdef FIXME
  // Keep track of how long each filter takes to execute.
  vtksys_ios::ostringstream filterName_with_warning_C4701;
  filterName_with_warning_C4701 << "Execute " << this->VTKClassName
                                << " id: " << sourceID.ID << ends;
  vtkClientServerStream start;
  start << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "LogStartEvent" << filterName_with_warning_C4701.str().c_str()
        << vtkClientServerStream::End;
  vtkClientServerStream end;
  end << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
      << "LogEndEvent" << filterName_with_warning_C4701.str().c_str()
      << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke
         << sourceID << "AddObserver" << "StartEvent" << start
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << sourceID << "AddObserver" << "EndEvent" << end
         << vtkClientServerStream::End;
#endif
  if (!this->Interpreter->ProcessStream(stream))
    {
    return false;
    }

  return this->CreateOutputPorts();
}

//----------------------------------------------------------------------------
bool vtkPMSourceProxy::CreateOutputPorts()
{
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (!algo)
    {
    return true;
    }

  int ports = algo->GetNumberOfOutputPorts();
  this->Internals->OutputPortIDs.resize(ports, vtkClientServerID(0));
  this->Internals->ExtractPiecesIDs.resize(ports, vtkClientServerID(0));
  this->Internals->PostFilterIDs.resize(ports, vtkClientServerID(0));

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
  // Assign an ID to this output port.
  vtkClientServerID portID = this->Interpreter->GetNextAvailableId();
  this->Internals->OutputPortIDs[port] = portID;

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->GetVTKObjectID()
    << "GetOutputPort" << port
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Assign << portID
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);

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
  vtkClientServerID portID = this->Internals->OutputPortIDs[port];
  vtkClientServerID extractID = this->Interpreter->GetNextAvailableId();

  this->Internals->ExtractPiecesIDs[port] = extractID;

  vtkClientServerStream stream;
  stream << vtkClientServerStream::New
    << "vtkPVExtractPieces" << extractID
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
    << extractID
    << "SetInputConnection"
    << portID
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Delete
         << portID
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << extractID
         << "GetOutputPort"
         << 0
         << vtkClientServerStream::End
         << vtkClientServerStream::Assign
         << portID
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::InsertPostFilterIfNecessary(vtkAlgorithm*, int port)
{
  vtkClientServerID portID = this->Internals->OutputPortIDs[port];
  vtkClientServerID postFilterID = this->Interpreter->GetNextAvailableId();

  this->Internals->PostFilterIDs[port] = postFilterID;

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::New
          << "vtkPVPostFilter" << postFilterID
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << postFilterID << "SetInputConnection" << portID
          << vtkClientServerStream::End;

  // Now substitute portID to point to the post-filter.
  stream << vtkClientServerStream::Delete
         << portID
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << postFilterID
         << "GetOutputPort"
         << 0
         << vtkClientServerStream::End
         << vtkClientServerStream::Assign
         << portID
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
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
void vtkPMSourceProxy::Invoke(vtkSMMessage* message)
{
  vtkstd::string command;
  const VariantList* arguments;
  command = message->GetExtension(InvokeRequest::method);
  arguments = &message->GetExtension(InvokeRequest::arguments);

  if (command == "UpdatePipeline")
    {
    assert(arguments->variant_size() == 3);
    int port_index = arguments->variant(0).integer(0);
    double time = arguments->variant(1).float64(0);
    bool doTime = arguments->variant(2).integer(0) != 0;
    this->UpdatePipeline(port_index, time, doTime);
    message->Clear();
    return;
    }
  else if (command == "UpdateInformation")
    {
    this->UpdateInformation();
    message->Clear();
    return;
    }

  return this->Superclass::Invoke(message);
}

//----------------------------------------------------------------------------
void vtkPMSourceProxy::UpdateInformation()
{
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  assert(algo);
  algo->UpdateInformation();
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
void vtkPMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
