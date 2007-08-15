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
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPart.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include <vtkstd/vector>
#include <vtksys/ios/sstream>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSourceProxy);
vtkCxxRevisionMacro(vtkSMSourceProxy, "1.55");

struct vtkSMSourceProxyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMPart> > Parts;
  vtkstd::vector<vtkSmartPointer<vtkSMSourceProxy> > SelectionProxies;
};

//---------------------------------------------------------------------------
vtkSMSourceProxy::vtkSMSourceProxy()
{
  this->PInternals = new  vtkSMSourceProxyInternals;
  this->PartsCreated = 0;

  this->DataInformationValid = false;
  this->ExecutiveName = 0;
  this->SetExecutiveName("vtkCompositeDataPipeline");

  this->DoInsertExtractPieces = 1;
  this->SelectionProxiesCreated = 0;
}

//---------------------------------------------------------------------------
vtkSMSourceProxy::~vtkSMSourceProxy()
{
  delete this->PInternals;

  this->SetExecutiveName(0);
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfParts()
{
  return this->PInternals->Parts.size();
}

//---------------------------------------------------------------------------
vtkSMPart* vtkSMSourceProxy::GetPart(unsigned int idx)
{
  return this->PInternals->Parts[idx].GetPointer();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UpdatePipelineInformation()
{
  if (this->GetID().IsNull())
    {
    return;
    }
  
  vtkClientServerStream command;
  command << vtkClientServerStream::Invoke 
          << this->GetID() << "UpdateInformation" 
          << vtkClientServerStream::End;
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->ConnectionID, this->Servers, command);

  // This simply iterates over subproxies and calls UpdatePropertyInformation();
  this->Superclass::UpdatePipelineInformation();

  this->InvokeEvent(vtkCommand::UpdateInformationEvent);
  // this->MarkModified(this);  
}
//---------------------------------------------------------------------------
int vtkSMSourceProxy::ReadXMLAttributes(vtkSMProxyManager* pm, 
                                        vtkPVXMLElement* element)
{
  const char* executiveName = element->GetAttribute("executive");
  if (executiveName)
    {
    this->SetExecutiveName(executiveName);
    }
  return this->Superclass::ReadXMLAttributes(pm, element);
}

//---------------------------------------------------------------------------
// Call Update() on all sources
// TODO this should update information properties.
void vtkSMSourceProxy::UpdatePipeline()
{
  int i;
  if (this->GetID().IsNull())
    {
    return;
    }

  if (strcmp(this->GetVTKClassName(), "vtkPVEnSightMasterServerReader") == 0)
    { 
    // Cannot set the update extent until we get the output.  Need to call
    // update before we can get the output.  Cannot not update whole extent
    // of every source.  Multiblock should fix this.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream command;
    command << vtkClientServerStream::Invoke 
            << this->GetID() << "Update" 
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, command);
    return;
    }
    
  this->CreateParts();
  int num = this->GetNumberOfParts();
  for (i=0; i < num; ++i)
    {
    this->GetPart(i)->UpdatePipeline();
    }

  this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
// Call Update() on all sources with given time
// TODO this should update information properties.
void vtkSMSourceProxy::UpdatePipeline(double time)
{
  int i;
  if (this->GetID().IsNull())
    {
    return;
    }

  if (strcmp(this->GetVTKClassName(), "vtkPVEnSightMasterServerReader") == 0)
    { 
    // Cannot set the update extent until we get the output.  Need to call
    // update before we can get the output.  Cannot not update whole extent
    // of every source.  Multiblock should fix this.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream command;
    command << vtkClientServerStream::Invoke 
            << this->GetID() << "Update" 
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, command);
    return;
    }
    
  this->CreateParts();
  int num = this->GetNumberOfParts();
  for (i=0; i < num; ++i)
    {
    this->GetPart(i)->UpdatePipeline(time);
    }

  this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerID sourceID = this->GetID();
  vtkClientServerStream stream;

  if (this->ExecutiveName)
    {
    vtkClientServerID execId = pm->NewStreamObject(
      this->ExecutiveName, stream);
    stream << vtkClientServerStream::Invoke 
           << sourceID << "SetExecutive" << execId 
           << vtkClientServerStream::End;
    pm->DeleteStreamObject(execId, stream);
    }

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
  
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}



//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateParts()
{
  this->CreatePartsInternal(this);
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreatePartsInternal(vtkSMProxy* op)
{
  if (this->PartsCreated && this->GetNumberOfParts())
    {
    return;
    }
  this->PartsCreated = 1;

  // This will only create objects if they are not already created.
  // This happens when connecting a filter to a source which is not
  // initialized. In other situations, SetInput() creates the VTK
  // objects before this gets called.
  op->CreateVTKObjects();

  this->PInternals->Parts.clear();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkPVNumberOfOutputsInformation* info = 
    vtkPVNumberOfOutputsInformation::New();

  // Create one part each output of each filter
  vtkClientServerStream stream;
  vtkClientServerID sourceID = op->GetID();
  // TODO replace this with UpdateInformation and OutputInformation
  // property.
  pm->GatherInformation(
    this->ConnectionID, this->Servers, info, sourceID);
  int numOutputs = info->GetNumberOfOutputs();
  for (int j=0; j<numOutputs; j++)
    {
    stream << vtkClientServerStream::Invoke << sourceID
           << "GetOutputPort" << j <<  vtkClientServerStream::End;
    vtkClientServerID portID = pm->GetUniqueID();
    stream << vtkClientServerStream::Assign << portID
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;

    vtkClientServerID producerID = pm->GetUniqueID();
    stream << vtkClientServerStream::Assign << producerID
           << sourceID
           << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke << sourceID
           << "GetExecutive" <<  vtkClientServerStream::End;
    vtkClientServerID execID = pm->GetUniqueID();
    stream << vtkClientServerStream::Assign << execID
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;

    vtkSMPart* part = vtkSMPart::New();
    part->SetConnectionID(this->ConnectionID);
    part->SetServers(this->Servers);
    part->InitializeWithIDs(portID, producerID, execID);
    part->SetPortIndex(j);
    this->PInternals->Parts.push_back(part);
    part->Delete();
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, op->GetServers(), stream);
    }
  info->Delete();

  vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
     this->PInternals->Parts.begin();

  if (this->DoInsertExtractPieces)
    {
    for(; it != this->PInternals->Parts.end(); it++)
      {
      it->GetPointer()->CreateTranslatorIfNecessary();
      if (strcmp(this->GetVTKClassName(), "vtkPVEnSightMasterServerReader"))
        {
        it->GetPointer()->InsertExtractPiecesIfNecessary();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::CleanInputs(const char* method)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  vtkClientServerID sourceID = this->GetID();
  stream << vtkClientServerStream::Invoke 
         << sourceID << method 
         << vtkClientServerStream::End;
  
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::AddInput(unsigned int inputPort,
                                vtkSMSourceProxy *input, 
                                unsigned int outputPort,
                                const char* method)
{

  if (!input)
    {
    return;
    }

  input->CreateParts();
  unsigned int numPorts = input->GetNumberOfParts();
  if (outputPort >= numPorts)
    {
    vtkErrorMacro("Specified output port (" << outputPort << ") does "
                  "not exist. Cannot make connection");
    return;
    }

  this->CreateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID sourceID = this->GetID();
  vtkSMPart* part = input->GetPart(outputPort);
  stream << vtkClientServerStream::Invoke;
  if (inputPort > 0)
    {
    stream << sourceID << method << inputPort << part->GetID();
    }
  else
    {
    stream << sourceID << method << part->GetID();
    }
  stream << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
                 this->Servers & input->GetServers(), 
                 stream);
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  if (this->PartsCreated && !this->GetNumberOfParts())
    {
    this->UpdatePipeline();
    }

  this->Superclass::MarkModified(modifiedProxy);
  this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UpdateSelfAndAllInputs()
{
  this->Superclass::UpdateSelfAndAllInputs();
  this->UpdatePipelineInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetDataInformation(
  unsigned int idx, bool update/*=true*/)
{
  this->CreateParts();
  if (idx >= this->GetNumberOfParts())
    {
    return 0;
    }

  if (!this->DataInformationValid)
    {
    if (update)
      {
      // Make sure the output filter is up-to-date before
      // getting information.
      this->UpdatePipeline();
      this->DataInformationValid = true;
      }
    }
  return this->GetPart(idx)->GetDataInformation();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::InvalidateDataInformation(int invalidateConsumers)
{
  if (invalidateConsumers)
    {
    unsigned int numConsumers = this->GetNumberOfConsumers();
    for (unsigned int i=0; i<numConsumers; i++)
      {
      vtkSMSourceProxy* cons = vtkSMSourceProxy::SafeDownCast(
        this->GetConsumerProxy(i));
      if (cons)
        {
        cons->InvalidateDataInformation(invalidateConsumers);
        }
      }
    }
  this->InvalidateDataInformation();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::InvalidateDataInformation()
{
  this->DataInformationValid = false;
  vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
    this->PInternals->Parts.begin();
  for (; it != this->PInternals->Parts.end(); it++)
    {
    it->GetPointer()->InvalidateDataInformation();
    }
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSourceProxy::SaveRevivalState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* revivalElem = this->Superclass::SaveRevivalState(root);
  if (revivalElem)
    {
    vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
      this->PInternals->Parts.begin();
    for(; it != this->PInternals->Parts.end(); ++it)
      {
      vtkPVXMLElement* partsElement = vtkPVXMLElement::New();
      partsElement->SetName("Part");
      revivalElem->AddNestedElement(partsElement);
      partsElement->Delete();
      it->GetPointer()->SaveRevivalState(partsElement);
      }
    }
  return revivalElem;
}

//---------------------------------------------------------------------------
int vtkSMSourceProxy::LoadRevivalState(vtkPVXMLElement* revivalElem,
  vtkSMStateLoaderBase* loader)
{
  if (!this->Superclass::LoadRevivalState(revivalElem, loader))
    {
    return 0;
    }

  unsigned int num_elems = revivalElem->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc <num_elems; cc++)
    {
    vtkPVXMLElement* curElement = revivalElem->GetNestedElement(cc);
    if (curElement->GetName() && strcmp(curElement->GetName(), "Part") == 0)
      {
      vtkSmartPointer<vtkSMPart> part = vtkSmartPointer<vtkSMPart>::New();
      part->SetConnectionID(this->ConnectionID);
      part->SetServers(this->Servers);
      if (part->LoadRevivalState(curElement->GetNestedElement(0), loader))
        {
        this->PInternals->Parts.push_back(part);
        }
      else
        {
        vtkErrorMacro("Failed to revive part");
        return 0;
        }
      }
    }
  this->PartsCreated = 1;
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateSelectionProxies()
{
  if (this->SelectionProxiesCreated)
    {
    return;
    }
  this->CreateParts();

  vtkClientServerStream stream;
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  unsigned int numParts = this->GetNumberOfParts(); 
  for (unsigned int cc=0; cc < numParts; cc++)
    {
    vtkSmartPointer<vtkSMSourceProxy> esProxy;
    esProxy.TakeReference(vtkSMSourceProxy::SafeDownCast(
        pxm->NewProxy("filters", "ExtractSelection")));
    if (esProxy)
      {
      esProxy->SetServers(this->Servers);
      esProxy->SetConnectionID(this->ConnectionID);
      esProxy->SelectionProxiesCreated = 1;
      esProxy->UpdateVTKObjects();

      // We don't use input property since that leads to reference loop cycles
      // and I don't feel like doing the garbage collection thing right now. 
      stream << vtkClientServerStream::Invoke
             << this->GetID()
             << "GetOutputPort"
             << cc
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke             
             << esProxy->GetID()
             << "SetInputConnection"
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      }

    this->PInternals->SelectionProxies.push_back(esProxy);
    }

  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->Servers, stream);

  this->SelectionProxiesCreated = 1;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::SetSelectionInput(unsigned int portIndex,
  vtkSMSourceProxy* input, unsigned int outputport)
{
  if (this->PInternals->SelectionProxies.size() <= portIndex)
    {
    return;
    }
  vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
  if (esProxy)
    {
    vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
      esProxy->GetProperty("Selection"));
    pp->RemoveAllProxies();
    pp->AddInputConnection(input, outputport);
    esProxy->UpdateVTKObjects();
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
      vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
        esProxy->GetProperty("Selection"));
      if (pp->GetNumberOfProxies() == 1)
        {
        return vtkSMSourceProxy::SafeDownCast(pp->GetProxy(0));
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetSelectionInputPort(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() > portIndex)
    {
    vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
    if (esProxy)
      {
      vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
        esProxy->GetProperty("Selection"));
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
    vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
      esProxy->GetProperty("Selection"));
    pp->RemoveAllProxies();
    esProxy->UpdateVTKObjects();
    }
}

//---------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMSourceProxy::GetSelectionOutput(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() > portIndex)
    {
    return this->PInternals->SelectionProxies[portIndex];
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataInformationValid: " << this->DataInformationValid << endl;
}
