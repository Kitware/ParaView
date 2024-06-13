// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIProxyProperty.h"

#include "vtkClientServerStream.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSIProxy.h"
#include "vtkSMMessage.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>
#include <vector>

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
class vtkSIProxyProperty::InternalCache : public std::vector<vtkTypeUInt32>
{
};

class vtkSIProxyProperty::vtkObjectCache
  : public std::vector<std::pair<vtkTypeUInt32, vtkSmartPointer<vtkObjectBase>>>
{
};

//****************************************************************************/
vtkStandardNewMacro(vtkSIProxyProperty);
//----------------------------------------------------------------------------
vtkSIProxyProperty::vtkSIProxyProperty()
{
  this->Cache = new InternalCache();
  this->ObjectCache = new vtkObjectCache();

  this->CleanCommand = nullptr;
  this->RemoveCommand = nullptr;
  this->ArgumentType = vtkSIProxyProperty::VTK;
  this->NullOnEmpty = false;
  this->SkipValidCheck = false;
}

//----------------------------------------------------------------------------
vtkSIProxyProperty::~vtkSIProxyProperty()
{
  this->SetCleanCommand(nullptr);
  this->SetRemoveCommand(nullptr);
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
  if (arg_type != nullptr && arg_type[0] != 0)
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

  int skip_valid_check;
  if (element->GetScalarAttribute("skip_valid_check", &skip_valid_check))
  {
    this->SkipValidCheck = skip_valid_check != 0;
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

  std::vector<vtkTypeUInt32> new_value;
  for (int cc = 0; cc < prop->value().proxy_global_id_size(); cc++)
  {
    new_value.push_back(prop->value().proxy_global_id(cc));
  }

  std::vector<vtkTypeUInt32> to_add = new_value;

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
    std::vector<vtkTypeUInt32> to_remove;
    for (const vtkTypeUInt32& cache_id : *this->Cache)
    {
      if (std::find(new_value.begin(), new_value.end(), cache_id) == new_value.end())
      {
        to_remove.push_back(cache_id);
      }
    }

    for (const vtkTypeUInt32& id : to_remove)
    {
      vtkObjectBase* arg = this->GetObjectBase(id);
      if (arg == nullptr)
      {
        arg = std::find_if(this->ObjectCache->begin(), this->ObjectCache->end(),
          [id](std::pair<vtkTypeUInt32, vtkSmartPointer<vtkObjectBase>>& pair) {
            return pair.first == id;
          })->second;
      }
      if (arg != nullptr)
      {
        stream << vtkClientServerStream::Invoke << object << this->GetRemoveCommand() << arg
               << vtkClientServerStream::End;

        this->ObjectCache->erase(std::find_if(this->ObjectCache->begin(), this->ObjectCache->end(),
          [id](std::pair<vtkTypeUInt32, vtkSmartPointer<vtkObjectBase>>& pair) {
            return pair.first == id;
          }));
      }
      else
      {
        vtkWarningMacro("Failed to locate vtkObjectBase for id : " << id);
      }
    }

    to_add.clear();
    for (const vtkTypeUInt32& id : new_value)
    {
      if (std::find(this->Cache->begin(), this->Cache->end(), id) == this->Cache->end())
      {
        to_add.push_back(id);
      }
    }
  }

  // Deal with proxy to add
  for (const vtkTypeUInt32& id : to_add)
  {
    vtkObjectBase* arg = this->GetObjectBase(id);
    if (arg != nullptr || this->IsValidNull(id))
    {
      stream << vtkClientServerStream::Invoke << object << this->GetCommand() << arg
             << vtkClientServerStream::End;

      // we keep an object cache so that even if the object gets unregistered
      // before the property is pushed, we have a reference to it.
      this->ObjectCache->emplace_back(id, arg);
    }
    else
    {
      vtkWarningMacro("Try to ADD a Proxy to a ProxyProperty but the proxy was not found");
    }
  }

  // Take care of the Empty case
  if (this->NullOnEmpty && this->CleanCommand == nullptr && new_value.empty())
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
  vtkSIProxy* siProxy = nullptr;
  switch (this->ArgumentType)
  {
    case VTK:
      siProxy = vtkSIProxy::SafeDownCast(this->GetSIObject(globalId));
      return (siProxy == nullptr) ? nullptr : siProxy->GetVTKObject();
    case SMProxy:
      return this->SIProxyObject->GetRemoteObject(globalId);
    case SIProxy:
      return this->SIProxyObject->GetSIObject(globalId);
  }
  return nullptr;
}
//----------------------------------------------------------------------------
bool vtkSIProxyProperty::IsValidNull(vtkTypeUInt32 globalId)
{
  if (globalId == 0 || this->SkipValidCheck)
  {
    return true;
  }

  vtkSIProxy* siProxy = vtkSIProxy::SafeDownCast(this->GetSIObject(globalId));
  vtkLogIfF(ERROR, siProxy == nullptr,
    "Property '%s' on ['%s', '%s'] has a value that is not available on this process. "
    "That typically indicates that the Location for the Proxy may not be correct. "
    "Aborting for debugging purposes.",
    this->XMLName, this->SIProxyObject->GetXMLGroup(), this->SIProxyObject->GetXMLName());
  assert("SIProxy shouldn't be nullptr otherwise it's a Proxy location issue in the XML" &&
    siProxy != 0);
  return siProxy->IsNullProxy();
}
