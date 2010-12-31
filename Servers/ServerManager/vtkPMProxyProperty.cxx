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
#include "vtkPMProxyProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPMProxy.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkSMRemoteObject.h"

#include <assert.h>
#include <vtkstd/set>

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
class vtkPMProxyProperty::InternalCache
{
public:
  //--------------------------------------------------------------------------
  void SetVariant(const Variant *variant)
    {
    this->VariantSet.clear();
    for (int cc=0; cc < variant->proxy_global_id_size(); cc++)
      {
      this->VariantSet.insert( variant->proxy_global_id(cc) );
      }
    }

  //--------------------------------------------------------------------------
  void CleanCommand()
    {
    this->RegisteredProxy.clear();
    }
  //--------------------------------------------------------------------------
  void GetProxyToRemove( vtkstd::vector<vtkTypeUInt32> &proxyToRemove )
    {
    proxyToRemove.clear();
    vtkstd::set<vtkTypeUInt32>::iterator iter = this->RegisteredProxy.begin();
    while(iter != this->RegisteredProxy.end())
      {
      if(this->VariantSet.find(*iter) == this->VariantSet.end())
        {
        proxyToRemove.push_back(*iter);
        }
      // Go to next item
      iter++;
      }
    }
  //--------------------------------------------------------------------------
  void GetProxyToAdd( vtkstd::vector<vtkTypeUInt32> &proxyToAdd )
    {
    proxyToAdd.clear();
    vtkstd::set<vtkTypeUInt32>::iterator iter = this->VariantSet.begin();
    while(iter != this->VariantSet.end())
      {
      if(this->RegisteredProxy.find(*iter) == this->RegisteredProxy.end())
        {
        proxyToAdd.push_back(*iter);
        }
      // Go to next item
      iter++;
      }
    }
  //--------------------------------------------------------------------------
  void UpdateRegisteredProxy()
    {
    this->RegisteredProxy = VariantSet;
    this->VariantSet.clear();
    }

private:
  vtkstd::set<vtkTypeUInt32> RegisteredProxy;
  vtkstd::set<vtkTypeUInt32> VariantSet;
};
//****************************************************************************/
vtkStandardNewMacro(vtkPMProxyProperty);
//----------------------------------------------------------------------------
vtkPMProxyProperty::vtkPMProxyProperty()
{
  this->Cache = new InternalCache();
  this->CleanCommand = 0;
  this->RemoveCommand = 0;
  this->ArgumentType = VTK;
  this->NullOnEmpty = false;
}

//----------------------------------------------------------------------------
vtkPMProxyProperty::~vtkPMProxyProperty()
{
  this->SetCleanCommand(0);
  this->SetRemoveCommand(0);
  delete this->Cache;
}

//----------------------------------------------------------------------------
bool vtkPMProxyProperty::ReadXMLAttributes(
  vtkPMProxy* proxyhelper, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxyhelper, element))
    {
    return false;
    }

  const char* clean_command = element->GetAttribute("clean_command");
  this->SetCleanCommand(clean_command);

  const char* remove_command = element->GetAttribute("remove_command");
  this->SetRemoveCommand(remove_command);

  // Allow to choose the kind of object to pass as argument based on
  // its global id.
  const char* arg_type = element->GetAttribute("argument_type");
  if(arg_type != NULL && arg_type[0] != 0)
    {
    if(strcmp(arg_type, "VTK") == 0)
      {
      this->ArgumentType = VTK;
      }
    else if(strcmp(arg_type, "SMProxy") == 0)
      {
      this->ArgumentType = SMProxy;
      }
    else if(strcmp(arg_type, "Kernel") == 0)
      {
      this->ArgumentType = Kernel;
      }
    }
  else
    {
    // If not set, DEFAULT value
    this->ArgumentType = VTK;
    }

  int null_on_empty;
  if (element->GetScalarAttribute("null_on_empty", &null_on_empty))
    {
    this->SetNullOnEmpty(null_on_empty);
    }

  if (this->InformationOnly)
    {
    vtkErrorMacro("InformationOnly proxy properties are not supported!");
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPMProxyProperty::Push(vtkSMMessage* message, int offset)
{
  assert(message->ExtensionSize(ProxyState::property) > offset);
  const ProxyState_Property *prop = &message->GetExtension(ProxyState::property,
                                                           offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  this->Cache->SetVariant(&prop->value());
  vtkstd::vector<vtkTypeUInt32> proxy_ids;

  vtkClientServerStream stream;
  vtkClientServerID objectId = this->GetVTKObjectID();

  // Deal with previous values to remove
  if (this->CleanCommand)
    {
    this->Cache->CleanCommand();
    stream << vtkClientServerStream::Invoke
      << objectId
      << this->CleanCommand
      << vtkClientServerStream::End;
    }
  else if(this->RemoveCommand)
    {
    // in all honesty, do we really need this anymore? It isn't a huge
    // performance bottleneck if all inputs to a filter are re-set every time
    // one changes.
    this->Cache->GetProxyToRemove(proxy_ids);
    for (size_t cc=0; cc < proxy_ids.size(); cc++)
      {
      vtkObjectBase* arg = this->GetObject(proxy_ids[cc]);
      if(arg != NULL)
        {
        stream << vtkClientServerStream::Invoke
               << objectId
               << this->GetRemoveCommand()
               << arg
               << vtkClientServerStream::End;
        }
      else
        {
        vtkWarningMacro("Try to REMOVE a Proxy to a ProxyProperty but the proxy was not found");
        }
      }
    }

  // Deal with proxy to add
  this->Cache->GetProxyToAdd(proxy_ids);
  for (size_t cc=0; cc < proxy_ids.size(); cc++)
    {
    vtkObjectBase* arg = this->GetObject(proxy_ids[cc]);
    if(arg != NULL)
      {
      stream << vtkClientServerStream::Invoke
             << objectId
             << this->GetCommand()
             << arg
             << vtkClientServerStream::End;
      }
    else
      {
      vtkWarningMacro("Try to ADD a Proxy to a ProxyProperty but the proxy was not found");
      }
    }

  // Take care of the Empty case
  if (this->NullOnEmpty && this->CleanCommand == NULL && proxy_ids.size() == 0)
    {
    stream << vtkClientServerStream::Invoke
      << objectId
      << this->GetCommand()
      << vtkClientServerID(0)
      << vtkClientServerStream::End;
    }

  this->Cache->UpdateRegisteredProxy();

  return this->ProcessMessage(stream);
}

//----------------------------------------------------------------------------
bool vtkPMProxyProperty::Pull(vtkSMMessage*)
{
  // since proxy-properties cannot be InformationOnly, return false, so that the
  // Proxy can simply return the cached property value, if any.
  return false;
}

//----------------------------------------------------------------------------
void vtkPMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
vtkObjectBase* vtkPMProxyProperty::GetObject(vtkTypeUInt32 globalId)
{
  vtkPMProxy* pmProxy = NULL;
  switch(this->ArgumentType)
    {
    case VTK:
      pmProxy = vtkPMProxy::SafeDownCast(this->GetPMObject(globalId));
      return (pmProxy == NULL) ? NULL : pmProxy->GetVTKObject();
    case SMProxy:
      return this->ProxyHelper->GetRemoteObject(globalId);
    case Kernel:
      return this->ProxyHelper->GetPMObject(globalId);
    }
  return NULL;
}
