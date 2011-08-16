/*=========================================================================

  Program:   ParaView
  Module:    vtkSIProxyProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIProxyProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSIProxy.h"
#include "vtkSIObject.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"

#include <assert.h>
#include <vtkstd/set>
#include <vtkSmartPointer.h>

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
class vtkSIProxyProperty::InternalCache
{
public:
  InternalCache(vtkSIProxyProperty* parent)
    {
    this->Parent = parent;
    }

  //--------------------------------------------------------------------------
  void SetVariant(const Variant *variant)
    {
    this->NumberOfDependancyToDelete = this->Dependancy.size();
    this->VariantSet.clear();
    for (int cc=0; cc < variant->proxy_global_id_size(); cc++)
      {
      this->VariantSet.insert( variant->proxy_global_id(cc) );
      vtkSIObject* obj = this->Parent->GetSIObject(variant->proxy_global_id(cc));
      if(obj)
        {
        this->Dependancy.push_back(obj);
        }
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
    vtkstd::vector<vtkSmartPointer<vtkSIObject> >::iterator iterEnd = this->Dependancy.begin();
    iterEnd += this->NumberOfDependancyToDelete;
    this->Dependancy.erase(this->Dependancy.begin(), iterEnd);
    this->NumberOfDependancyToDelete = 0;
    this->RegisteredProxy = VariantSet;
    this->VariantSet.clear();
    }
  //--------------------------------------------------------------------------
private:
  vtkstd::set<vtkTypeUInt32> RegisteredProxy;
  vtkstd::set<vtkTypeUInt32> VariantSet;
  vtkstd::vector<vtkSmartPointer<vtkSIObject> > Dependancy;
  vtkSIProxyProperty* Parent;
  size_t NumberOfDependancyToDelete;
};
//****************************************************************************/
vtkStandardNewMacro(vtkSIProxyProperty);
//----------------------------------------------------------------------------
vtkSIProxyProperty::vtkSIProxyProperty()
{
  this->Cache = new InternalCache(this);
  this->CleanCommand = NULL;
  this->RemoveCommand = NULL;
  this->ArgumentType = vtkSIProxyProperty::VTK;
  this->NullOnEmpty = false;
}

//----------------------------------------------------------------------------
vtkSIProxyProperty::~vtkSIProxyProperty()
{
  this->SetCleanCommand(NULL);
  this->SetRemoveCommand(NULL);
  delete this->Cache;
}

//----------------------------------------------------------------------------
bool vtkSIProxyProperty::ReadXMLAttributes(
  vtkSIProxy* proxyhelper, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxyhelper, element))
    {
    return false;
    }

  this->SetCleanCommand(element->GetAttribute("clean_command"));

  this->SetRemoveCommand(element->GetAttribute("remove_command"));

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
    else if(strcmp(arg_type, "SIProxy") == 0)
      {
      this->ArgumentType = SIProxy;
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
    this->SetNullOnEmpty(null_on_empty != 0);
    }

  if (this->InformationOnly)
    {
    vtkErrorMacro("InformationOnly proxy properties are not supported!");
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSIProxyProperty::Push(vtkSMMessage* message, int offset)
{
  assert(message->ExtensionSize(ProxyState::property) > offset);
  const ProxyState_Property *prop = &message->GetExtension(ProxyState::property,
                                                           offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  this->Cache->SetVariant(&prop->value());
  vtkstd::vector<vtkTypeUInt32> proxy_ids;

  vtkClientServerStream stream;
  vtkObjectBase* object = this->GetVTKObject();

  // Deal with previous values to remove
  if (this->CleanCommand)
    {
    this->Cache->CleanCommand();
    stream << vtkClientServerStream::Invoke
           << object
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
      vtkObjectBase* arg = this->GetObjectBase(proxy_ids[cc]);
      if(arg != NULL)
        {
        stream << vtkClientServerStream::Invoke
               << object
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
    vtkObjectBase* arg = this->GetObjectBase(proxy_ids[cc]);

    if(arg != NULL || this->IsValidNull(proxy_ids[cc]))
      {
      stream << vtkClientServerStream::Invoke
             << object
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
           << object
           << this->GetCommand()
           << vtkClientServerID(0)
           << vtkClientServerStream::End;
    }

  this->Cache->UpdateRegisteredProxy();

  return this->ProcessMessage(stream);
}

//----------------------------------------------------------------------------
bool vtkSIProxyProperty::Pull(vtkSMMessage*)
{
  // since proxy-properties cannot be InformationOnly, return false, so that the
  // Proxy can simply return the cached property value, if any.
  return false;
}

//----------------------------------------------------------------------------
void vtkSIProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
vtkObjectBase* vtkSIProxyProperty::GetObjectBase(vtkTypeUInt32 globalId)
{
  vtkSIProxy* siProxy = NULL;
  switch(this->ArgumentType)
    {
    case VTK:
      siProxy = vtkSIProxy::SafeDownCast(this->GetSIObject(globalId));
      return (siProxy == NULL) ? NULL : siProxy->GetVTKObject();
    case SMProxy:
      return this->SIProxyObject->GetRemoteObject(globalId);
    case SIProxy:
      return this->SIProxyObject->GetSIObject(globalId);
    }
  return NULL;
}
//----------------------------------------------------------------------------
bool vtkSIProxyProperty::IsValidNull(vtkTypeUInt32 globalId)
{
  if(globalId == 0)
    {
    return true;
    }

  vtkSIProxy* siProxy = vtkSIProxy::SafeDownCast(this->GetSIObject(globalId));
  assert("SIProxy shouldn't be null otherwise it's a Proxy location issue in the XML" && siProxy != 0);
  return siProxy->IsNullProxy();
}
