/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSourceProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPVAlgorithmPortsInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMMessage.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <assert.h>
#include <sstream>
#include <string>
#include <vector>

#define OUTPUT_PORTNAME_PREFIX "Output-"
#define MAX_NUMBER_OF_PORTS 10

//---------------------------------------------------------------------------
class vtkSelectionForwarderCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkSelectionForwarderCommand, vtkCommand);

  static vtkSelectionForwarderCommand* New() { return new vtkSelectionForwarderCommand; }

  vtkSelectionForwarderCommand()
    : PortIndex(0)
    , Proxy(nullptr)
  {
  }

  void Execute(vtkObject*, unsigned long, void*) override
  {
    if (this->Proxy)
    {
      this->Proxy->InvokeEvent(vtkCommand::SelectionChangedEvent, &this->PortIndex);
    }
  }

  int PortIndex;
  vtkSMSourceProxy* Proxy;
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSourceProxy);

// This struct will keep all information associated with the output port.
struct vtkSMSourceProxyOutputPort
{
  vtkSmartPointer<vtkSMOutputPort> Port;
  vtkSmartPointer<vtkSMDocumentation> Documentation;
  std::string Name;
};

struct vtkSMSourceProxyInternals
{
  typedef std::vector<vtkSMSourceProxyOutputPort> VectorOfPorts;
  VectorOfPorts OutputPorts;
  std::vector<vtkSmartPointer<vtkSMSourceProxy> > SelectionProxies;
  std::vector<unsigned long> SelectionObservers;

  // Resizes output ports and ensures that Name for each port is initialized to
  // the default.
  void ResizeOutputPorts(unsigned int newsize)
  {
    this->OutputPorts.resize(newsize);

    VectorOfPorts::iterator it = this->OutputPorts.begin();
    for (unsigned int idx = 0; it != this->OutputPorts.end(); it++, idx++)
    {
      if (it->Name.empty())
      {
        std::ostringstream nameStream;
        nameStream << OUTPUT_PORTNAME_PREFIX << idx;
        it->Name = nameStream.str();
      }
    }
  }

  void EnsureOutputPortsSize(unsigned int size)
  {
    if (this->OutputPorts.size() < size)
    {
      this->ResizeOutputPorts(size);
    }
  }
};

//---------------------------------------------------------------------------
vtkSMSourceProxy::vtkSMSourceProxy()
{
  this->SetSIClassName("vtkSISourceProxy");

  this->PInternals = new vtkSMSourceProxyInternals;
  this->OutputPortsCreated = 0;

  this->ExecutiveName = nullptr;
  this->SetExecutiveName("vtkCompositeDataPipeline");

  this->DisableSelectionProxies = false;
  this->SelectionProxiesCreated = false;

  this->NumberOfAlgorithmOutputPorts = VTK_UNSIGNED_INT_MAX;
  this->NumberOfAlgorithmRequiredInputPorts = VTK_UNSIGNED_INT_MAX;
  this->ProcessSupport = vtkSMSourceProxy::BOTH;
  this->MPIRequired = false;
}

//---------------------------------------------------------------------------
vtkSMSourceProxy::~vtkSMSourceProxy()
{
  delete this->PInternals;

  this->SetExecutiveName(nullptr);
}

//---------------------------------------------------------------------------
vtkTypeUInt32 vtkSMSourceProxy::GetGlobalID()
{
  bool has_gid = this->HasGlobalID();

  if (!has_gid && this->Session != nullptr)
  {
    // reserve 1+MAX_NUMBER_OF_PORTS contiguous IDs for the source proxies and possible extract
    // selection proxies.
    this->SetGlobalID(
      this->GetSession()->GetNextChunkGlobalUniqueIdentifier(1 + MAX_NUMBER_OF_PORTS));
  }
  return this->GlobalID;
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfOutputPorts()
{
  return static_cast<int>(this->PInternals->OutputPorts.size());
}

//---------------------------------------------------------------------------
vtkSMOutputPort* vtkSMSourceProxy::GetOutputPort(unsigned int idx)
{
  return idx == VTK_UNSIGNED_INT_MAX ? nullptr
                                     : this->PInternals->OutputPorts[idx].Port.GetPointer();
}

//---------------------------------------------------------------------------
vtkSMOutputPort* vtkSMSourceProxy::GetOutputPort(const char* portname)
{
  return this->GetOutputPort(this->GetOutputPortIndex(portname));
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetOutputPortIndex(const char* portname)
{
  // Since there are not really going to be hundreds of output ports, the is not
  // going to be any noticeable difference in accessing output ports by index or
  // by name.
  vtkSMSourceProxyInternals::VectorOfPorts::iterator it = this->PInternals->OutputPorts.begin();
  for (unsigned int idx = 0; it != this->PInternals->OutputPorts.end(); it++, idx++)
  {
    if (it->Name == portname)
    {
      return idx;
    }
  }

  return VTK_UNSIGNED_INT_MAX;
}

//---------------------------------------------------------------------------
const char* vtkSMSourceProxy::GetOutputPortName(unsigned int index)
{
  if (index >= this->PInternals->OutputPorts.size())
  {
    return nullptr;
  }

  return this->PInternals->OutputPorts[index].Name.c_str();
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSourceProxy::GetOutputPortDocumentation(unsigned int index)
{
  if (index >= this->PInternals->OutputPorts.size())
  {
    return nullptr;
  }

  return this->PInternals->OutputPorts[index].Documentation;
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSourceProxy::GetOutputPortDocumentation(const char* portname)
{
  return this->GetOutputPortDocumentation(this->GetOutputPortIndex(portname));
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UpdatePipelineInformation()
{
  if (this->ObjectsCreated)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << SIPROXY(this) << "UpdatePipelineInformation"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
  }

  // This simply iterates over subproxies and calls UpdatePropertyInformation();
  this->Superclass::UpdatePipelineInformation();

  this->InvokeEvent(vtkCommand::UpdateInformationEvent);
  // this->MarkModified(this);
}
//---------------------------------------------------------------------------
int vtkSMSourceProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  const char* executiveName = element->GetAttribute("executive");
  if (executiveName)
  {
    this->SetExecutiveName(executiveName);
  }
  if (const char* mp = element->GetAttribute("multiprocess_support"))
  {
    // For ParaView we don't mark any sources as multiple_processes only
    // since for things like tracing or Catalyst script generation we
    // want to make them always available. Other tools that are built on
    // top of ParaView though may want to be able to set multiple_processes
    // only.
    if (strcmp(mp, "multiple_processes") == 0)
    {
      this->ProcessSupport = vtkSMSourceProxy::MULTIPLE_PROCESSES;
    }
    else if (strcmp(mp, "single_process") == 0)
    {
      this->ProcessSupport = vtkSMSourceProxy::SINGLE_PROCESS;
    }
    else
    {
      this->ProcessSupport = vtkSMSourceProxy::BOTH;
    }
  }

  if (const char* mpi = element->GetAttribute("mpi_required"))
  {
    if (strcmp(mpi, "true") == 0 || strcmp(mpi, "1") == 0)
    {
      this->MPIRequired = true;
    }
    else
    {
      this->MPIRequired = false;
    }
  }

  int port_count = 0;
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "OutputPort") == 0)
    {
      // Load output port configuration.
      int index = 0;
      if (!child->GetScalarAttribute("index", &index))
      {
        index = port_count;
      }
      const char* portname = child->GetAttribute("name");
      if (!portname)
      {
        vtkErrorMacro("Missing OutputPort attribute 'name'.");
        return 0;
      }
      port_count++;
      this->PInternals->EnsureOutputPortsSize(index + 1);
      this->PInternals->OutputPorts[index].Name = portname;

      // Load output port documentation.
      for (unsigned int kk = 0; kk < child->GetNumberOfNestedElements(); ++kk)
      {
        vtkPVXMLElement* subElem = child->GetNestedElement(kk);
        if (strcmp(subElem->GetName(), "Documentation") == 0)
        {
          this->Documentation->SetDocumentationElement(subElem);
          vtkSMDocumentation* doc = vtkSMDocumentation::New();
          doc->SetDocumentationElement(subElem);
          this->PInternals->OutputPorts[index].Documentation = doc;
          doc->Delete();
        }
      }
    }
  }

  return this->Superclass::ReadXMLAttributes(pm, element);
}

//---------------------------------------------------------------------------
// Call Update() on all sources
// TODO this should update information properties.
void vtkSMSourceProxy::UpdatePipeline()
{
  if (!this->NeedsUpdate)
  {
    return;
  }

  this->CreateOutputPorts();
  int num = this->GetNumberOfOutputPorts();
  for (int i = 0; i < num; ++i)
  {
    this->GetOutputPort(i)->UpdatePipeline();
  }

  this->PostUpdateData(false);
  // this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
// Call Update() on all sources with given time
// TODO this should update information properties.
void vtkSMSourceProxy::UpdatePipeline(double time)
{
  int i;

  this->CreateOutputPorts();
  int num = this->GetNumberOfOutputPorts();
  for (i = 0; i < num; ++i)
  {
    this->GetOutputPort(i)->UpdatePipeline(time);
  }

  // When calling UpdatePipeline() we check if this->NeedsUpdate is true and
  // call the real update only if that's the case. We don't do that when one
  // uses UpdatePipeline(time) since we can never be too sure what time was
  // used. In that case, we assume the pipeline needs update and hence we should
  // set the NeedsUpdate ivar to true as well. Otherwise PostUpdateData()
  // doesn't fire the necessary events and that can cause problems (BUG
  // #12571).
  this->NeedsUpdate = true;

  this->PostUpdateData(false);
  // this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();

  // We are going to fix the ports such that we don't have to update the
  // pipeline or even UpdateInformation() to create the ports.
  this->CreateOutputPorts();
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfAlgorithmOutputPorts()
{
  if (this->NumberOfAlgorithmOutputPorts != VTK_UNSIGNED_INT_MAX)
  {
    // avoids unnecessary information gathers.
    return this->NumberOfAlgorithmOutputPorts;
  }

  if (this->ObjectsCreated)
  {
    vtkSmartPointer<vtkPVAlgorithmPortsInformation> info =
      vtkSmartPointer<vtkPVAlgorithmPortsInformation>::New();
    this->GatherInformation(info);
    this->NumberOfAlgorithmOutputPorts = info->GetNumberOfOutputs();
    this->NumberOfAlgorithmRequiredInputPorts = info->GetNumberOfRequiredInputs();
    return this->NumberOfAlgorithmOutputPorts;
  }

  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfAlgorithmRequiredInputPorts()
{
  this->GetNumberOfAlgorithmOutputPorts();

  if (this->NumberOfAlgorithmRequiredInputPorts != VTK_UNSIGNED_INT_MAX)
  {
    // avoid unnecessary information gathers.
    return this->NumberOfAlgorithmRequiredInputPorts;
  }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateOutputPorts()
{
  if (this->OutputPortsCreated && this->GetNumberOfOutputPorts())
  {
    return;
  }
  this->OutputPortsCreated = 1;

  // This will only create objects if they are not already created.
  // This happens when connecting a filter to a source which is not
  // initialized. In other situations, SetInput() creates the VTK
  // objects before this gets called.
  this->CreateVTKObjects();

  // We simply set/replace the Port pointers in the
  // this->PInternals->OutputPorts. This ensures that the port names,
  // port documentation is preserved.

  // Create one output port proxy for each output of the filter
  int numOutputs = this->GetNumberOfAlgorithmOutputPorts();

  // Ensure that output ports size matches the number of output ports provided
  // by the algorithm.
  this->PInternals->ResizeOutputPorts(numOutputs);
  for (int j = 0; j < numOutputs; j++)
  {
    vtkSMOutputPort* opPort = vtkSMOutputPort::New();
    opPort->SetPortIndex(j);
    opPort->SetSourceProxy(this);
    this->PInternals->OutputPorts[j].Port = opPort;
    opPort->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::SetOutputPort(
  unsigned int index, const char* name, vtkSMOutputPort* port, vtkSMDocumentation* doc)
{
  this->PInternals->EnsureOutputPortsSize(index + 1);
  this->PInternals->OutputPorts[index].Name = name;
  this->PInternals->OutputPorts[index].Port = port;
  this->PInternals->OutputPorts[index].Documentation = doc;
  if (port && port->GetSourceProxy() == nullptr)
  {
    port->SetSourceProxy(this);
  }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::RemoveAllOutputPorts()
{
  this->PInternals->OutputPorts.clear();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::SetExtractSelectionProxy(unsigned int index, vtkSMSourceProxy* proxy)
{
  if (this->PInternals->SelectionProxies.size() <= index + 1)
  {
    this->PInternals->SelectionProxies.resize(index + 1);
    this->PInternals->SelectionObservers.resize(index + 1);
  }

  this->PInternals->SelectionProxies[index] = proxy;

  // Set up observer on the "Selection" property's ModifiedEvent and invoke
  // a SelectionChangedEvent from this proxy.
  vtkNew<vtkSelectionForwarderCommand> selectionCommand;
  selectionCommand->PortIndex = index;
  selectionCommand->Proxy = this;
  this->PInternals->SelectionObservers[index] =
    proxy->AddObserver(vtkCommand::ModifiedEvent, selectionCommand);
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::RemoveAllExtractSelectionProxies()
{
  for (size_t i = 0; i < this->PInternals->SelectionObservers.size(); ++i)
  {
    this->PInternals->SelectionProxies[i]->RemoveObserver(this->PInternals->SelectionObservers[i]);
  }
  this->PInternals->SelectionProxies.clear();
  this->PInternals->SelectionObservers.clear();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::PostUpdateData(bool using_cache)
{
  if (!using_cache)
  {
    this->InvalidateDataInformation();
  }
  this->Superclass::PostUpdateData(using_cache);
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  // Mark the extract selection proxies modified as well.
  // This is needed to be done explicitly since we don't use vtkSMInputProperty
  // to connect this proxy to the input of the extract selection filter.
  std::vector<vtkSmartPointer<vtkSMSourceProxy> >::iterator iter;
  for (iter = this->PInternals->SelectionProxies.begin();
       iter != this->PInternals->SelectionProxies.end(); ++iter)
  {
    iter->GetPointer()->MarkDirty(modifiedProxy);
  }

  this->Superclass::MarkDirty(modifiedProxy);
  // this->InvalidateDataInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetDataInformation(unsigned int idx)
{
  this->CreateOutputPorts();
  if (idx >= this->GetNumberOfOutputPorts())
  {
    return nullptr;
  }

  return this->GetOutputPort(idx)->GetDataInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetSubsetDataInformation(
  unsigned int idx, const char* selector, const char* assemblyName)
{
  this->CreateOutputPorts();
  if (idx >= this->GetNumberOfOutputPorts())
  {
    return nullptr;
  }

  return this->GetOutputPort(idx)->GetSubsetDataInformation(selector, assemblyName);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetSubsetDataInformation(
  unsigned int idx, unsigned int compositeIndex)
{
  this->CreateOutputPorts();
  if (idx >= this->GetNumberOfOutputPorts())
  {
    return nullptr;
  }

  return this->GetOutputPort(idx)->GetSubsetDataInformation(compositeIndex);
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::InvalidateDataInformation()
{
  if (this->OutputPortsCreated)
  {
    vtkSMSourceProxyInternals::VectorOfPorts::iterator it = this->PInternals->OutputPorts.begin();
    for (; it != this->PInternals->OutputPorts.end(); it++)
    {
      it->Port.GetPointer()->InvalidateDataInformation();
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateSelectionProxies()
{
  if (this->DisableSelectionProxies || this->SelectionProxiesCreated)
  {
    return;
  }

  this->CreateOutputPorts();
  this->SelectionProxiesCreated = true;

  int numOutputs = this->GetNumberOfAlgorithmOutputPorts();

  // Setup selection proxies
  if (numOutputs > MAX_NUMBER_OF_PORTS)
  {
    vtkErrorMacro("vtkSMSourceProxy was not designed to handle more than "
      << MAX_NUMBER_OF_PORTS
      << " output ports. "
         "In general, that's not a good practice. Try  reducing the number of "
         "output ports. Aborting for debugging purposes.");
    abort();
  }
  this->PInternals->SelectionProxies.resize(numOutputs);
  this->PInternals->SelectionObservers.resize(numOutputs);

  vtkClientServerStream stream;
  assert("Session should be valid" && this->Session);
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  for (int j = 0; j < numOutputs; j++)
  {
    vtkSmartPointer<vtkSMSourceProxy> esProxy;
    // If the selection proxy has been created previously on the server,
    // lets register it...
    // This happen in collaboration mode when the SelectionProxy
    // became alive because of the SelectionRepresentation before the
    // CreateSelectionProxies() get called on the original source proxy...
    if ((esProxy = vtkSMSourceProxy::SafeDownCast(
           this->Session->GetRemoteObject(this->GetGlobalID() + j + 1))) != nullptr)
    {
      esProxy->DisableSelectionProxies = true;
      this->PInternals->SelectionProxies[j] = esProxy;
      continue;
    }

    // Otherwise create it properly
    esProxy.TakeReference(
      vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "PVExtractSelection")));
    if (esProxy)
    {
      esProxy->DisableSelectionProxies = true;
      esProxy->SetLocation(this->Location);
      esProxy->SetGlobalID(this->GetGlobalID() + j + 1);
      esProxy->UpdateVTKObjects();

      std::ostringstream sstream;
      sstream << this->GetLogNameOrDefault() << "(ExtractSelection:" << j << ")";
      esProxy->SetLogName(sstream.str().c_str());

      this->SetExtractSelectionProxy(j, esProxy);

      // We don't use input property since that leads to reference loop cycles
      // and I don't feel like doing the garbage collection thing right now.
      stream << vtkClientServerStream::Invoke << SIPROXY(this) << "SetupSelectionProxy" << j
             << SIPROXY(esProxy) << vtkClientServerStream::End;
    }
  }
  this->ExecuteStream(stream);
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::SetSelectionInput(
  unsigned int portIndex, vtkSMSourceProxy* input, unsigned int outputport)
{
  this->CreateSelectionProxies();

  if (this->PInternals->SelectionProxies.size() <= portIndex)
  {
    return;
  }
  vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
  if (esProxy)
  {
    vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(esProxy->GetProperty("Selection"));
    pp->RemoveAllProxies();
    pp->AddInputConnection(input, outputport);
    esProxy->UpdateVTKObjects();
    this->InvokeEvent(vtkCommand::SelectionChangedEvent, &portIndex);
  }
}

//---------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMSourceProxy::GetSelectionInput(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() > portIndex)
  {
    vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
    if (esProxy)
    {
      vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(esProxy->GetProperty("Selection"));
      if (pp->GetNumberOfProxies() == 1)
      {
        return vtkSMSourceProxy::SafeDownCast(pp->GetProxy(0));
      }
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetSelectionInputPort(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() > portIndex)
  {
    vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
    if (esProxy)
    {
      vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(esProxy->GetProperty("Selection"));
      if (pp->GetNumberOfProxies() == 1)
      {
        return pp->GetOutputPortForConnection(portIndex);
      }
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CleanSelectionInputs(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() <= portIndex)
  {
    return;
  }
  vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
  if (esProxy)
  {
    esProxy->RemoveObserver(this->PInternals->SelectionObservers[portIndex]);
    vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(esProxy->GetProperty("Selection"));
    pp->RemoveAllProxies();
    esProxy->UpdateVTKObjects();
    this->InvokeEvent(vtkCommand::SelectionChangedEvent, &portIndex);
  }
}

//---------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMSourceProxy::GetSelectionOutput(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() > portIndex)
  {
    return this->PInternals->SelectionProxies[portIndex];
  }

  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::SetLogNameInternal(
  const char* name, bool propagate_to_subproxies, bool propagate_to_proxylistdomains)
{
  this->Superclass::SetLogNameInternal(
    name, propagate_to_subproxies, propagate_to_proxylistdomains);
  auto& internals = *this->PInternals;
  for (size_t port = 0, max = internals.SelectionProxies.size(); port < max; ++port)
  {
    if (auto esProxy = internals.SelectionProxies[port])
    {
      std::ostringstream stream;
      stream << name << "(ExtractSelection:" << port << ")";
      esProxy->SetLogName(stream.str().c_str());
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputPortsCreated: " << this->OutputPortsCreated << endl;
  os << indent << "ProcessSupport: " << this->ProcessSupport << endl;
}
