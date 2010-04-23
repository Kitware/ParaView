/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMInputProperty);

int vtkSMInputProperty::InputsUpdateImmediately = 1;

struct vtkSMInputPropertyInternals
{
  vtkstd::vector<unsigned int> OutputPorts;
  vtkstd::vector<unsigned int> UncheckedOutputPorts;
  vtkstd::vector<unsigned int> PreviousOutputPorts;
};

//---------------------------------------------------------------------------
vtkSMInputProperty::vtkSMInputProperty()
{
  this->ImmediateUpdate = vtkSMInputProperty::InputsUpdateImmediately;
  this->UpdateSelf = 1;
  this->MultipleInput = 0;
  this->PortIndex = 0;

  this->IPInternals = new vtkSMInputPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMInputProperty::~vtkSMInputProperty()
{
  delete this->IPInternals;
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::GetInputsUpdateImmediately()
{
  return vtkSMInputProperty::InputsUpdateImmediately;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetInputsUpdateImmediately(int up)
{
  vtkSMInputProperty::InputsUpdateImmediately = up;

  vtkSMPropertyIterator* piter = vtkSMPropertyIterator::New();
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  while(!iter->IsAtEnd())
    {
    piter->SetProxy(iter->GetProxy());
    while(!piter->IsAtEnd())
      {
      vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
        piter->GetProperty());
      if (ip)
        {
        ip->SetImmediateUpdate(up);
        }
      piter->Next();
      }
    iter->Next();
    }
  iter->Delete();
  piter->Delete();
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AppendCommandToStream(
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->InformationOnly)
    {
    return;
    }

  this->RemoveConsumerFromPreviousProxies(cons);
  this->RemoveAllPreviousProxies();
  this->IPInternals->PreviousOutputPorts.clear();

  if (this->CleanCommand)
    {
    *str << vtkClientServerStream::Invoke 
         << objectId << "CleanInputs" << this->CleanCommand
         << vtkClientServerStream::End;
    }
  unsigned int numInputs = this->GetNumberOfProxies();
  for (unsigned int i=0; i<numInputs; i++)
    {
    vtkSMProxy* proxy = this->GetProxy(i) ;
    if (proxy)
      {
      this->AddPreviousProxy(proxy);
      this->IPInternals->PreviousOutputPorts.push_back(
        this->GetOutputPortForConnection(i));
      proxy->AddConsumer(this, cons);
      cons->AddProducer(this, proxy);

      *str << vtkClientServerStream::Invoke 
           << objectId 
           << "AddInput" 
           << this->PortIndex
           << proxy
           << this->GetOutputPortForConnection(i)
           << this->Command;
      *str << vtkClientServerStream::End;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                          vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(parent, element);

  int multiple_input;
  int retVal = element->GetScalarAttribute("multiple_input", &multiple_input);
  if(retVal) 
    { 
    this->SetMultipleInput(multiple_input); 
    this->Repeatable = multiple_input;
    }

  int port_index;
  retVal = element->GetScalarAttribute("port_index", &port_index);
  if(retVal) 
    { 
    this->SetPortIndex(port_index); 
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultipleInput: " << this->MultipleInput << endl;
  os << indent << "PortIndex: " << this->PortIndex << endl;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetNumberOfProxies(unsigned int num)
{
  if (num != 0)
    {
    this->IPInternals->OutputPorts.resize(num);
    }
  else
    {
    this->IPInternals->OutputPorts.clear();
    }

  this->Superclass::SetNumberOfProxies(num);
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetProxies(unsigned int numProxies,
  vtkSMProxy* proxies[], unsigned int outputports[])
{
  this->IPInternals->OutputPorts.clear();
  for (unsigned int cc=0; cc < numProxies; cc++)
    {
    this->IPInternals->OutputPorts.push_back(outputports[cc]);
    }

  this->Superclass::SetProxies(numProxies, proxies);
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::AddInputConnection(vtkSMProxy* proxy, 
                                           unsigned int outputPort,
                                           int modify)
{
  if (this->IPInternals->OutputPorts.size() != 
      this->GetNumberOfProxies())
    {
    this->IPInternals->OutputPorts.resize(
      this->GetNumberOfProxies());
    }
  this->IPInternals->OutputPorts.push_back(outputPort);

  int retval = this->AddProxy(proxy, modify);
  if (retval)
    {
    if (modify)
      {
      this->Modified();
      }
    return retval;
    }
  else
    {
    return 0;
    }
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::SetInputConnection(unsigned int idx, 
                                           vtkSMProxy* proxy, 
                                           unsigned int outputPort)
{
  if (idx >= this->IPInternals->OutputPorts.size())
    {
    this->IPInternals->OutputPorts.resize(idx+1);
    }
  this->IPInternals->OutputPorts[idx] = outputPort;

  int retval = this->SetProxy(idx, proxy);
  if (retval)
    {
    return retval;
    }
  else
    {
    return 0;
    }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AddUncheckedInputConnection(vtkSMProxy* proxy, 
                                                     unsigned int outputPort)
{
  if (this->IPInternals->UncheckedOutputPorts.size() != 
      this->GetNumberOfUncheckedProxies())
    {
    this->IPInternals->UncheckedOutputPorts.resize(
      this->GetNumberOfUncheckedProxies());
    }
  this->IPInternals->UncheckedOutputPorts.push_back(outputPort);

  this->AddUncheckedProxy(proxy);
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetUncheckedInputConnection(unsigned int idx, 
                                                    vtkSMProxy* proxy, 
                                                     unsigned int outputPort)
{
  if (idx >= this->IPInternals->UncheckedOutputPorts.size())
    {
    this->IPInternals->UncheckedOutputPorts.resize(idx+1);
    }
  this->IPInternals->UncheckedOutputPorts[idx] = outputPort;

  this->SetUncheckedProxy(idx, proxy);
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::RemoveProxy(vtkSMProxy* proxy, int modify)
{
  unsigned int idx =
    this->Superclass::RemoveProxy(proxy, modify);
  if (idx < this->IPInternals->OutputPorts.size())
    {
    this->IPInternals->OutputPorts.erase(
      this->IPInternals->OutputPorts.begin() + idx);
    }
  return idx;
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::RemoveUncheckedProxy(vtkSMProxy* proxy)
{
  unsigned int idx =
    this->Superclass::RemoveUncheckedProxy(proxy);
  if (idx < this->IPInternals->UncheckedOutputPorts.size())
    {
    this->IPInternals->UncheckedOutputPorts.erase(
      this->IPInternals->UncheckedOutputPorts.begin() + idx);
    }
  return idx;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::RemoveAllUncheckedProxies()
{
  this->IPInternals->UncheckedOutputPorts.clear();

  this->Superclass::RemoveAllUncheckedProxies();
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::RemoveAllProxies(int modify)
{
  this->IPInternals->OutputPorts.clear();

  this->Superclass::RemoveAllProxies(modify);
  if (modify)
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetOutputPortForConnection(unsigned int idx)
{
  if (idx >= this->IPInternals->OutputPorts.size())
    {
    return 0;
    }
  return this->IPInternals->OutputPorts[idx];
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetPreviousOutputPortForConnection(
  unsigned int idx)
{
  if (idx >= this->IPInternals->PreviousOutputPorts.size())
    {
    return 0;
    }
  return this->IPInternals->PreviousOutputPorts[idx];
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetUncheckedOutputPortForConnection(
  unsigned int idx)
{
  if (idx >= this->IPInternals->UncheckedOutputPorts.size())
    {
    return 0;
    }
  return this->IPInternals->UncheckedOutputPorts[idx];
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::LoadState(vtkPVXMLElement* element,
                                  vtkSMProxyLocator* loader, 
                                  int loadLastPushedValues/*=0*/)
{
  if (!loader)
    {
    // If no loader, leave state unchanged.
    return 1;
    }

  // NOTE: This method by-passes LoadState() of vtkSMProxyProperty and
  // re-implements a lot of it's functionality to add output ports. 
  // Therefore, care must be taken to keep the two in sync.

  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  this->vtkSMProperty::LoadState(element, loader, loadLastPushedValues);

  // If "clear" is present and is 0, it implies that the proxy elements
  // currently in the property should not be cleared before loading 
  // the new state.
  int clear=1;
  element->GetScalarAttribute("clear", &clear);
  if (clear)
    {
    this->RemoveAllProxies(0);
    }

  if (loadLastPushedValues)
    {
    element = element->FindNestedElementByName("LastPushedValues");
    if (!element)
      {
      vtkErrorMacro("Failed to locate LastPushedValues.");
      this->ImmediateUpdate = prevImUpdate;
      return 0;
      }
    }

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() &&
        (strcmp(currentElement->GetName(), "Element") == 0 ||
         strcmp(currentElement->GetName(), "Proxy") == 0) )
      {
      int id;
      if (currentElement->GetScalarAttribute("value", &id))
        {
        int outputPort = 0;
        currentElement->GetScalarAttribute("output_port", &outputPort);
        if (id)
          {
          vtkSMProxy* proxy = loader->LocateProxy(id);
          if (proxy)
            {
            this->AddInputConnection(proxy, outputPort, 0);
            }
          else
            {
            // It is not an error to have missing proxies in a proxy property.
            // We simply ignore such proxies.
            //vtkErrorMacro("Could not create proxy of id: " << id);
            //return 0;
            }
          }
        else
          {
          this->AddProxy(0, 0);
          }
        }
      }
    }

  // Do not immediately update. Leave it to the loader.
  this->Modified();
  this->ImmediateUpdate = prevImUpdate;
  return 1;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMInputProperty::SaveProxyElementState(
  unsigned int idx, bool use_previous_proxies)
{
  vtkPVXMLElement* proxyElement = this->Superclass::SaveProxyElementState(idx,
    use_previous_proxies);
  if (proxyElement)
    {
    proxyElement->AddAttribute("output_port",
      (use_previous_proxies?
       this->GetPreviousOutputPortForConnection(idx) :
       this->GetOutputPortForConnection(idx)));
    }
  return proxyElement;
}

