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
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSIObject.h"
#include "vtkSIProxy.h"
#include "vtkSMMessage.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <assert.h>
#include <iterator>
#include <set>

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
class vtkSIProxyProperty::InternalCache : public std::set<vtkTypeUInt32>
{
};

class vtkSIProxyProperty::vtkObjectCache
  : public std::map<vtkTypeUInt32, vtkSmartPointer<vtkObjectBase> >
{
};

//****************************************************************************/
vtkStandardNewMacro(vtkSIProxyProperty);
//----------------------------------------------------------------------------
vtkSIProxyProperty::vtkSIProxyProperty()
{
  this->Cache = new InternalCache();
  this->ObjectCache = new vtkObjectCache();

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
  delete this->ObjectCache;
}

//----------------------------------------------------------------------------
bool vtkSIProxyProperty::ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element)
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
  if (arg_type != NULL && arg_type[0] != 0)
  {
    if (strcmp(arg_type, "VTK") == 0)
    {
      this->ArgumentType = VTK;
    }
    else if (strcmp(arg_type, "SMProxy") == 0)
    {
      this->ArgumentType = SMProxy;
    }
    else if (strcmp(arg_type, "SIProxy") == 0)
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
  const ProxyState_Property* prop = &message->GetExtension(ProxyState::property, offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  std::set<vtkTypeUInt32> new_value;
  for (int cc = 0; cc < prop->value().proxy_global_id_size(); cc++)
  {
    new_value.insert(prop->value().proxy_global_id(cc));
  }

  std::set<vtkTypeUInt32> to_add = new_value;

  vtkClientServerStream stream;
  vtkObjectBase* object = this->GetVTKObject();

  // Deal with previous values to remove
  if (this->CleanCommand)
  {
    stream << vtkClientServerStream::Invoke << object << this->CleanCommand
           << vtkClientServerStream::End;

    this->ObjectCache->clear();
  }
  else if (this->RemoveCommand)
  {
    std::set<vtkTypeUInt32> to_remove;
    std::set_difference(this->Cache->begin(), this->Cache->end(), new_value.begin(),
      new_value.end(), std::inserter(to_remove, to_remove.begin()));

    for (std::set<vtkTypeUInt32>::iterator iter = to_remove.begin(); iter != to_remove.end();
         ++iter)
    {
      vtkObjectBase* arg = this->GetObjectBase(*iter);
      if (arg == NULL)
      {
        arg = (*this->ObjectCache)[*iter].GetPointer();
      }
      if (arg != NULL)
      {
        stream << vtkClientServerStream::Invoke << object << this->GetRemoveCommand() << arg
               << vtkClientServerStream::End;

        this->ObjectCache->erase(*iter);
      }
      else
      {
        vtkWarningMacro("Failed to locate vtkObjectBase for id : " << *iter);
      }
    }

    to_add.clear();
    std::set_difference(new_value.begin(), new_value.end(), this->Cache->begin(),
      this->Cache->end(), std::inserter(to_add, to_add.begin()));
  }

  // Deal with proxy to add
  for (std::set<vtkTypeUInt32>::iterator iter = to_add.begin(); iter != to_add.end(); ++iter)
  {
    vtkObjectBase* arg = this->GetObjectBase(*iter);
    if (arg != NULL || this->IsValidNull(*iter))
    {
      stream << vtkClientServerStream::Invoke << object << this->GetCommand() << arg
             << vtkClientServerStream::End;

      // we keep an object cache so that even if the object gets unregistered
      // before the property is pushed, we have a reference to it.
      (*this->ObjectCache)[*iter] = arg;
    }
    else
    {
      vtkWarningMacro("Try to ADD a Proxy to a ProxyProperty but the proxy was not found");
    }
  }

  // Take care of the Empty case
  if (this->NullOnEmpty && this->CleanCommand == NULL && new_value.size() == 0)
  {
    stream << vtkClientServerStream::Invoke << object << this->GetCommand() << vtkClientServerID(0)
           << vtkClientServerStream::End;

    this->ObjectCache->clear();
  }

  this->Cache->clear();
  std::copy(new_value.begin(), new_value.end(), std::inserter(*this->Cache, this->Cache->begin()));

  // Save to cache when pulled for collaboration
  this->SaveValueToCache(message, offset);

  return this->ProcessMessage(stream);
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
  switch (this->ArgumentType)
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
  if (globalId == 0)
  {
    return true;
  }

  vtkSIProxy* siProxy = vtkSIProxy::SafeDownCast(this->GetSIObject(globalId));
  vtkLogIfF(ERROR, siProxy == nullptr,
    "Property '%s' on ['%s', '%s'] has a value that is not available on this process. "
    "That typically indicates that the Location for the Proxy may not be correct. "
    "Aborting for debugging purposes.",
    this->XMLName, this->SIProxyObject->GetXMLGroup(), this->SIProxyObject->GetXMLName());
  assert(
    "SIProxy shouldn't be null otherwise it's a Proxy location issue in the XML" && siProxy != 0);
  return siProxy->IsNullProxy();
}
