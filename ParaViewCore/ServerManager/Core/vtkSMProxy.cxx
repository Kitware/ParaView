/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxy.h"
#include "vtkSMProxyInternals.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVInformation.h"
#include "vtkPVInstantiator.h"
#include "vtkPVLogger.h"
#include "vtkPVOptions.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSIProxy.h"
#include "vtkSIProxyDefinitionManager.h"
#include "vtkSMDocumentation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStateLocator.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <assert.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

//---------------------------------------------------------------------------
// Observer for modified event of the property
class vtkSMProxyObserver : public vtkCommand
{
public:
  static vtkSMProxyObserver* New() { return new vtkSMProxyObserver; }

  void Execute(vtkObject* obj, unsigned long event, void* data) override
  {
    if (this->Proxy)
    {
      if (!this->PropertyName.empty())
      {
        // This is observing a property.
        this->Proxy->SetPropertyModifiedFlag(this->PropertyName.c_str(), 1);
      }
      else
      {
        this->Proxy->ExecuteSubProxyEvent(vtkSMProxy::SafeDownCast(obj), event, data);
      }
    }
  }

  void SetPropertyName(const char* name) { this->PropertyName = (name ? name : ""); }

  // Note that Proxy is not reference counted. Since the Proxy has a reference
  // to the Property and the Property has a reference to the Observer, making
  // Proxy reference counted would cause a loop.
  void SetProxy(vtkSMProxy* proxy) { this->Proxy = proxy; }

protected:
  std::string PropertyName;
  vtkSMProxy* Proxy;
};

vtkStandardNewMacro(vtkSMProxy);

vtkCxxSetObjectMacro(vtkSMProxy, XMLElement, vtkPVXMLElement);
vtkCxxSetObjectMacro(vtkSMProxy, Hints, vtkPVXMLElement);
vtkCxxSetObjectMacro(vtkSMProxy, Deprecated, vtkPVXMLElement);

//---------------------------------------------------------------------------
vtkSMProxy::vtkSMProxy()
{
  this->Internals = new vtkSMProxyInternals;
  this->SIClassName = 0;
  this->SetSIClassName("vtkSIProxy");

  // By default, all objects are created on data server.
  this->Location = vtkProcessModule::DATA_SERVER;
  this->VTKClassName = 0;
  this->XMLGroup = 0;
  this->XMLName = 0;
  this->XMLLabel = 0;
  this->XMLSubProxyName = 0;
  this->ObjectsCreated = 0;

  this->XMLElement = 0;
  this->DoNotUpdateImmediately = 0;
  this->DoNotModifyProperty = 0;
  this->InUpdateVTKObjects = 0;
  this->PropertiesModified = 0;

  this->SubProxyObserver = vtkSMProxyObserver::New();
  this->SubProxyObserver->SetProxy(this);

  this->Documentation = vtkSMDocumentation::New();
  this->InMarkModified = 0;

  this->NeedsUpdate = true;

  this->Hints = 0;
  this->Deprecated = 0;

  this->State = new vtkSMMessage();

  this->LogName = nullptr;
}

//---------------------------------------------------------------------------
vtkSMProxy::~vtkSMProxy()
{
  this->RemoveAllObservers();

  // ensure that the properties are destroyed before we delete this->Internals.
  this->Internals->Properties.clear();

  delete this->Internals;
  this->SetVTKClassName(0);
  this->SetXMLGroup(0);
  this->SetXMLName(0);
  this->SetXMLLabel(0);
  this->SetXMLSubProxyName(0);
  this->SetXMLElement(0);
  if (this->SubProxyObserver)
  {
    this->SubProxyObserver->SetProxy(0);
    this->SubProxyObserver->Delete();
  }
  this->Documentation->Delete();
  this->SetHints(0);
  this->SetDeprecated(0);
  this->SetSIClassName(0);

  if (this->State)
  {
    delete this->State;
    this->State = 0;
  }

  delete[] this->LogName;
  this->LogName = nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMProxy::GetLogNameOrDefault()
{
  if (this->LogName && this->LogName[0] != '\0')
  {
    return this->LogName;
  }

  if (this->DefaultLogName.empty())
  {
    std::ostringstream stream;
    stream << vtkLogger::GetIdentifier(this);
    if (this->XMLName && this->XMLGroup)
    {
      stream << "[" << this->XMLGroup << ", " << this->XMLName << "]";
    }
    this->DefaultLogName = stream.str();
  }
  return this->DefaultLogName.c_str();
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetLocation(vtkTypeUInt32 location)
{
  this->Superclass::SetLocation(location);

  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    it2->second.GetPointer()->SetLocation(location);
  }
}

//---------------------------------------------------------------------------
vtkObjectBase* vtkSMProxy::GetClientSideObject()
{
  if (this->Session)
  {
    this->CreateVTKObjects();

    vtkTypeUInt32 gid = this->GetGlobalID();
    vtkSIProxy* siProxy = vtkSIProxy::SafeDownCast(this->Session->GetSIObject(gid));
    if (siProxy)
    {
      return siProxy->GetVTKObject();
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
const char* vtkSMProxy::GetPropertyName(vtkSMProperty* prop)
{
  const char* result = 0;
  vtkSMPropertyIterator* piter = this->NewPropertyIterator();
  for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
  {
    if (prop == piter->GetProperty())
    {
      result = piter->GetKey();
      break;
    }
  }
  piter->Delete();
  return result;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::GetProperty(const char* name, int selfOnly)
{
  if (!name)
  {
    return 0;
  }
  vtkSMProxyInternals::PropertyInfoMap::iterator it = this->Internals->Properties.find(name);
  if (it != this->Internals->Properties.end())
  {
    return it->second.Property.GetPointer();
  }
  if (!selfOnly)
  {
    vtkSMProxyInternals::ExposedPropertyInfoMap::iterator eiter =
      this->Internals->ExposedProperties.find(name);
    if (eiter == this->Internals->ExposedProperties.end())
    {
      // no such property is being exposed.
      return 0;
    }
    const char* subproxy_name = eiter->second.SubProxyName.c_str();
    const char* property_name = eiter->second.PropertyName.c_str();
    vtkSMProxy* sp = this->GetSubProxy(subproxy_name);
    if (sp)
    {
      return sp->GetProperty(property_name, 0);
    }
    // indicates that the internal dbase for exposed properties is
    // corrupt.. when a subproxy was removed, the exposed properties
    // for that proxy should also have been cleaned up.
    // Flag an error so that it can be debugged.
    vtkWarningMacro("Subproxy required for the exposed property is missing."
                    "No subproxy with name : "
      << subproxy_name);
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveAllObservers()
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it = this->Internals->Properties.begin(); it != this->Internals->Properties.end(); ++it)
  {
    vtkSMProperty* prop = it->second.Property.GetPointer();
    if (it->second.ObserverTag > 0)
    {
      prop->RemoveObserver(it->second.ObserverTag);
    }
  }

  vtkSMProxyInternals::ProxyMap::iterator it2;
  for (it2 = this->Internals->SubProxies.begin(); it2 != this->Internals->SubProxies.end(); ++it2)
  {
    it2->second.GetPointer()->RemoveObserver(this->SubProxyObserver);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddProperty(const char* name, vtkSMProperty* prop)
{
  if (!prop)
  {
    return;
  }
  if (!name)
  {
    vtkErrorMacro("Can not add a property without a name.");
    return;
  }

  // Check if the property already exists. If it does, we will
  // replace it (and remove the observer from it)
  vtkSMProxyInternals::PropertyInfoMap::iterator it = this->Internals->Properties.find(name);

  if (it != this->Internals->Properties.end())
  {
    vtkWarningMacro("Property " << name << " already exists. Replacing");
    vtkSMProperty* oldProp = it->second.Property.GetPointer();
    if (it->second.ObserverTag > 0)
    {
      oldProp->RemoveObserver(it->second.ObserverTag);
    }
    oldProp->SetParent(0);
  }

  unsigned int tag = 0;

  vtkSMProxyObserver* obs = vtkSMProxyObserver::New();
  obs->SetProxy(this);
  obs->SetPropertyName(name);
  // We have to store the tag in order to be able to remove
  // the observer later.
  tag = prop->AddObserver(vtkCommand::ModifiedEvent, obs);
  obs->Delete();

  prop->SetParent(this);

  vtkSMProxyInternals::PropertyInfo newEntry;
  newEntry.Property = prop;
  newEntry.ObserverTag = tag;
  this->Internals->Properties[name] = newEntry;

  // BUG: Hmm, if this replaces an existing property, are we ending up with that
  // name being pushed in twice in the PropertyNamesInOrder list?
  // => this vector is used by the OrderedProperty iterator which mean's
  //    that the iterator will go several time on the same property.

  // Add the property name to the vector of property names.
  // This vector keeps track of the order in which properties
  // were added.
  this->Internals->PropertyNamesInOrder.push_back(name);
}

//---------------------------------------------------------------------------
bool vtkSMProxy::UpdateProperty(const char* name, int force)
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it = this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
  {
    // Search exposed subproxy properties.
    vtkSMProxyInternals::ExposedPropertyInfoMap::iterator eiter =
      this->Internals->ExposedProperties.find(name);
    if (eiter == this->Internals->ExposedProperties.end())
    {
      return false;
    }
    const char* subproxy_name = eiter->second.SubProxyName.c_str();
    const char* property_name = eiter->second.PropertyName.c_str();
    vtkSMProxy* sp = this->GetSubProxy(subproxy_name);
    if (sp && sp->UpdateProperty(property_name, force))
    {
      this->MarkModified(this);
      return true;
    }

    return false;
  }

  if (!it->second.ModifiedFlag && !force)
  {
    return false;
  }

  if (it->second.Property->GetInformationOnly())
  {
    // cannot update information only properties.
    return false;
  }

  this->CreateVTKObjects();

  // In case this property is a self property and causes
  // another UpdateVTKObjects(), make sure that it does
  // not cause recursion. If this is not set, UpdateVTKObjects()
  // that is caused by UpdateProperty() can end up calling trying
  // to push the same property.
  it->second.ModifiedFlag = 0;

  vtkSMMessage message;

  // Make sure the local state is updated as well
  if (this->State)
  {
    vtkSMMessage oldState;
    oldState.CopyFrom(*this->State);
    this->State->ClearExtension(ProxyState::property);
    int nbProps = oldState.ExtensionSize(ProxyState::property);
    for (int cc = 0; cc < nbProps; cc++)
    {
      const ProxyState_Property* oldProperty = &oldState.GetExtension(ProxyState::property, cc);

      if (oldProperty->name() == it->second.Property->GetXMLName())
      {
        it->second.Property->WriteTo(this->State);
      }
      else
      {
        ProxyState_Property* newProperty = this->State->AddExtension(ProxyState::property);
        newProperty->CopyFrom(oldState.GetExtension(ProxyState::property, cc));
      }
    }
  }

  it->second.Property->WriteTo(&message);
  this->PushState(&message);

  // Fire event to let everyone know that a property has been updated.
  this->InvokeEvent(vtkCommand::UpdatePropertyEvent, const_cast<char*>(name));
  this->MarkModified(this);
  return true;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  if (this->DoNotModifyProperty)
  {
    return;
  }

  vtkSMProxyInternals::PropertyInfoMap::iterator it = this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
  {
    return;
  }

  this->InvokeEvent(vtkCommand::PropertyModifiedEvent, (void*)name);

  vtkSMProperty* prop = it->second.Property.GetPointer();
  if (prop->GetInformationOnly())
  {
    // Information only property is modified...nothing much to do.
    return;
  }

  it->second.ModifiedFlag = flag;

  if (flag && !this->DoNotUpdateImmediately && prop->GetImmediateUpdate())
  {
    this->UpdateProperty(it->first.c_str());
  }
  else
  {
    this->PropertiesModified = 1;
  }
}

//-----------------------------------------------------------------------------
void vtkSMProxy::MarkAllPropertiesAsModified()
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it = this->Internals->Properties.begin(); it != this->Internals->Properties.end(); it++)
  {
    // Not the most efficient way to set the flag, but probably the safest.
    this->SetPropertyModifiedFlag(it->first.c_str(), 1);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::ResetPropertiesToXMLDefaults()
{
  this->ResetPropertiesToDefault(vtkSMProxy::ONLY_XML);
}

//---------------------------------------------------------------------------
void vtkSMProxy::ResetPropertiesToDomainDefaults()
{
  this->ResetPropertiesToDefault(vtkSMProxy::ONLY_DOMAIN);
}

//---------------------------------------------------------------------------
void vtkSMProxy::ResetPropertiesToDefault(ResetPropertiesMode mode)
{
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(this->NewPropertyIterator());

  // iterate over properties and reset them to default.
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProperty* smproperty = iter->GetProperty();
    if (!smproperty->GetInformationOnly())
    {
      vtkPVXMLElement* propHints = iter->GetProperty()->GetHints();
      if (propHints && propHints->FindNestedElementByName("NoDefault"))
      {
        // Don't reset properties that request overriding of the default mechanism.
        continue;
      }
      switch (mode)
      {
        case vtkSMProxy::ONLY_XML:
        {
          iter->GetProperty()->ResetToXMLDefaults();
          break;
        }
        case vtkSMProxy::ONLY_DOMAIN:
        {
          iter->GetProperty()->ResetToDomainDefaults();
          break;
        }
        default:
        {
          iter->GetProperty()->ResetToDefault();
          break;
        }
      }
    }
  }
  this->UpdateVTKObjects();
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdatePropertyInformation(vtkSMProperty* prop)
{
  // If property does not belong to this proxy do nothing.
  int found = 0;
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it = this->Internals->Properties.begin(); it != this->Internals->Properties.end(); ++it)
  {
    if (prop == it->second.Property.GetPointer())
    {
      found = 1;
      break;
    }
  }

  if (!found)
  {
    // Check if the property is an exposed property
    const char* exposed_name = this->GetPropertyName(prop);
    if (exposed_name)
    {
      vtkSMProxyInternals::ExposedPropertyInfoMap::iterator eiter =
        this->Internals->ExposedProperties.find(exposed_name);
      if (eiter != this->Internals->ExposedProperties.end())
      {
        const char* subproxy_name = eiter->second.SubProxyName.c_str();
        const char* property_name = eiter->second.PropertyName.c_str();
        vtkSMProxy* sp = this->GetSubProxy(subproxy_name);
        if (sp)
        {
          sp->UpdatePropertyInformation(sp->GetProperty(property_name));
        }
      }
    }
    return;
  }

  this->UpdatePropertyInformationInternal(prop);
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdatePropertyInformation()
{
  this->UpdatePropertyInformationInternal(NULL);

  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    it2->second.GetPointer()->UpdatePropertyInformation();
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdatePropertyInformationInternal(vtkSMProperty* single_property /*=NULL*/)
{
  this->CreateVTKObjects();

  // If no location, it means no state...
  if (!this->ObjectsCreated || this->Location == 0)
  {
    return;
  }

  bool some_thing_to_fetch = false;
  vtkSMMessage message;
  Variant* var = message.AddExtension(PullRequest::arguments);
  var->set_type(Variant::STRING);

  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  if (single_property != NULL)
  {
    if (single_property->GetInformationOnly())
    {
      var->add_txt(single_property->GetXMLName());
      some_thing_to_fetch = true;
    }
  }
  else
  {
    // Update all information properties.
    for (it = this->Internals->Properties.begin(); it != this->Internals->Properties.end(); ++it)
    {
      vtkSMProperty* prop = it->second.Property.GetPointer();
      if (prop->GetInformationOnly())
      {
        var->add_txt(it->first.c_str());
        some_thing_to_fetch = true;
      }
    }
  }

  if (!some_thing_to_fetch)
  {
    return;
  }

  // Hmm, this changes message itself. Funky.
  this->PullState(&message);

  // Update internal values
  this->LoadState(&message, this->Session->GetProxyLocator());
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateVTKObjects()
{
  this->CreateVTKObjects();
  if (!this->ObjectsCreated || this->InUpdateVTKObjects || !this->ArePropertiesModified() ||
    this->Location == 0)
  {
    return;
  }

  if (this->PropertiesModified)
  {
    this->InUpdateVTKObjects = 1;

    // Save previous property values and clear the State properties
    vtkSMMessage oldState;
    oldState.CopyFrom(*this->State);
    this->State->ClearExtension(ProxyState::property);

    // iterate over all properties and push modified ones.
    vtkSMMessage message;
    vtkSMProxyInternals::PropertyInfoMap::iterator iter;
    int cc = 0;
    for (iter = this->Internals->Properties.begin(); iter != this->Internals->Properties.end();
         ++iter)
    {
      vtkSMProperty* property = iter->second.Property;
      if (property && !property->GetInformationOnly())
      {
        if (property->GetIsInternal() || property->IsStateIgnored() ||
          strcmp(property->GetClassName(), "vtkSMProperty") == 0)
        {
          // Push only modified properties
          if (iter->second.ModifiedFlag)
          {
            // Write to message because vtkSMProperty do not have state
            property->WriteTo(&message);

            // the property is no longer dirty.
            iter->second.ModifiedFlag = 0;

            // Fire event to let everyone know that a property has been updated.
            // This is currently used by vtkSMLink. Need to see if we can avoid this
            // as firing these events ain't inexpensive.
            this->InvokeEvent(
              vtkCommand::UpdatePropertyEvent, const_cast<char*>(iter->first.c_str()));
          }
        }
        else
        {
          // Push only modified properties
          if (iter->second.ModifiedFlag)
          {
            // Write to state
            property->WriteTo(this->State);

            // the property is no longer dirty.
            iter->second.ModifiedFlag = 0;

            // Write to Push message
            ProxyState_Property* prop = message.AddExtension(ProxyState::property);
            prop->CopyFrom(this->State->GetExtension(ProxyState::property, cc));

            // Fire event to let everyone know that a property has been updated.
            // This is currently used by vtkSMLink. Need to see if we can avoid this
            // as firing these events ain't inexpensive.
            this->InvokeEvent(
              vtkCommand::UpdatePropertyEvent, const_cast<char*>(iter->first.c_str()));
          }
          else
          {
            // Just copy the previous old value to the state
            ProxyState_Property* prop = this->State->AddExtension(ProxyState::property);
            prop->CopyFrom(oldState.GetExtension(ProxyState::property, cc));
          }

          // One more property
          ++cc;
        }
      }
    }
    this->InUpdateVTKObjects = 0;
    this->PropertiesModified = false;

    // Send the message
    this->PushState(&message);
  }

  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    it2->second.GetPointer()->UpdateVTKObjects();
  }

  this->MarkModified(this);
  this->InvokeEvent(vtkCommand::UpdateEvent, 0);
}

//---------------------------------------------------------------------------
bool vtkSMProxy::ArePropertiesModified()
{
  if (this->PropertiesModified)
  {
    return true;
  }

  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    if (it2->second.GetPointer()->ArePropertiesModified())
    {
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkSMProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated && this->State && this->Location == 0)
  {
    return;
  }
  this->WarnIfDeprecated();

  assert("Test Proxy definition" && this->GetClassName() && this->GetSIClassName() &&
    this->GetXMLGroup() && this->GetXMLName());

  vtkSMMessage message;
  message.SetExtension(DefinitionHeader::client_class, this->GetClassName());
  message.SetExtension(DefinitionHeader::server_class, this->GetSIClassName());
  message.SetExtension(ProxyState::xml_group, this->GetXMLGroup());
  message.SetExtension(ProxyState::xml_name, this->GetXMLName());
  if (this->XMLSubProxyName)
  {
    message.SetExtension(ProxyState::xml_sub_proxy_name, this->XMLSubProxyName);
  }
  if (this->LogName)
  {
    message.SetExtension(ProxyState::log_name, this->LogName);
  }

  // Create sub-proxies first.
  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    it2->second.GetPointer()->CreateVTKObjects();
    ProxyState_SubProxy* subproxy = message.AddExtension(ProxyState::subproxy);
    subproxy->set_name(it2->first.c_str());
    subproxy->set_global_id(it2->second.GetPointer()->GetGlobalID());
  }

  // Save to state
  this->State->CopyFrom(message);

  // Add Empty property into state to keep track of index later on
  this->RebuildStateForProperties();

  // Even if the Proxy was marked as Created, we went so far to build correctly
  // the state and this is the same case for prototype.
  if (this->ObjectsCreated)
  {
    return;
  }
  this->ObjectsCreated = 1;
  if (this->Location == 0)
  {
    return;
  }

  // Push the state
  this->PushState(&message);

  // Update assigned id/location while the push
  this->State->set_global_id(this->GetGlobalID());
  // Using the real location and not the filtered one allow us to store
  // the correct location in full state that is used in Undo/Redo.
  this->State->set_location(this->Location);

  bool oldPushState = this->Internals->EnableAnnotationPush;
  this->Internals->EnableAnnotationPush = false;
  this->UpdateAndPushAnnotationState();
  this->Internals->EnableAnnotationPush = oldPushState;
}

//---------------------------------------------------------------------------
void vtkSMProxy::RebuildStateForProperties()
{
  this->State->ClearExtension(ProxyState::property);

  vtkSMProxyInternals::PropertyInfoMap::iterator iter;
  for (iter = this->Internals->Properties.begin(); iter != this->Internals->Properties.end();
       ++iter)
  {
    vtkSMProperty* property = iter->second.Property;
    if (property && !property->GetInformationOnly())
    {
      if (property->GetIsInternal() || property->IsStateIgnored() ||
        strcmp(property->GetClassName(), "vtkSMProperty") == 0)
      {
        // No state for vtkSMProperty
      }
      else
      {
        // Write empty property inside state
        property->WriteTo(this->State);
      }
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSMProxy::GatherInformation(vtkPVInformation* information)
{
  assert(information);

  vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "%s: gather information %s",
    this->GetLogNameOrDefault(), information->GetClassName());

  if (this->GetSession() && this->Location != 0)
  {
    // ensure that the proxy is created.
    this->CreateVTKObjects();

    return this->GetSession()->GatherInformation(this->Location, information, this->GetGlobalID());
  }
  return false;
}

//---------------------------------------------------------------------------
bool vtkSMProxy::GatherInformation(vtkPVInformation* information, vtkTypeUInt32 location)
{
  assert(information);

  vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "%s: gather information %s",
    this->GetLogNameOrDefault(), information->GetClassName());

  vtkTypeUInt32 realLocation = (this->Location & location);
  if (this->GetSession() && realLocation != 0)
  {
    // ensure that the proxy is created.
    this->CreateVTKObjects();

    return this->GetSession()->GatherInformation(realLocation, information, this->GetGlobalID());
  }
  if ((this->Location != 0) && (realLocation == 0) && (location != 0))
  {
    vtkWarningMacro("GatherInformation was called with location "
                    "on which the proxy does not exist. Ignoring.");
  }
  return false;
}

//---------------------------------------------------------------------------
bool vtkSMProxy::WarnIfDeprecated()
{
  if (this->Deprecated)
  {
    vtkWarningMacro("Proxy ("
      << this->XMLGroup << ", " << this->XMLName << ")  has been deprecated in ParaView "
      << this->Deprecated->GetAttribute("deprecated_in") << " and will be removed by ParaView "
      << this->Deprecated->GetAttribute("to_remove_in") << ". "
      << (this->Deprecated->GetCharacterData() ? this->Deprecated->GetCharacterData() : ""));
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxy::GetNumberOfSubProxies()
{
  return static_cast<unsigned int>(this->Internals->SubProxies.size());
}

//---------------------------------------------------------------------------
const char* vtkSMProxy::GetSubProxyName(unsigned int index)
{
  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (unsigned int idx = 0; it2 != this->Internals->SubProxies.end(); it2++, idx++)
  {
    if (idx == index)
    {
      return it2->first.c_str();
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMProxy::GetSubProxyName(vtkSMProxy* proxy)
{
  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    if (it2->second.GetPointer() == proxy)
    {
      return it2->first.c_str();
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetSubProxy(unsigned int index)
{
  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (unsigned int idx = 0; it2 != this->Internals->SubProxies.end(); it2++, idx++)
  {
    if (idx == index)
    {
      return it2->second.GetPointer();
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetSubProxy(const char* name)
{
  vtkSMProxyInternals::ProxyMap::iterator it = this->Internals->SubProxies.find(name);

  if (it == this->Internals->SubProxies.end())
  {
    return 0;
  }

  return it->second.GetPointer();
}

//---------------------------------------------------------------------------
bool vtkSMProxy::GetIsSubProxy()
{
  return this->ParentProxy != NULL;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetParentProxy()
{
  return this->ParentProxy.GetPointer();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetTrueParentProxy()
{
  vtkSMProxy* self = this;
  while (self->GetParentProxy())
  {
    self = self->GetParentProxy();
  }
  return self;
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddSubProxy(const char* name, vtkSMProxy* proxy, int override)
{
  // Check if the proxy already exists. If it does, we will replace it
  vtkSMProxyInternals::ProxyMap::iterator it = this->Internals->SubProxies.find(name);

  if (it != this->Internals->SubProxies.end())
  {
    if (!override)
    {
      vtkWarningMacro("Proxy " << name << " already exists. Replacing");
    }
    // needed to remove any observers.
    this->RemoveSubProxy(name);
  }

  this->Internals->SubProxies[name] = proxy;
  proxy->ParentProxy = this;

  proxy->AddObserver(vtkCommand::PropertyModifiedEvent, this->SubProxyObserver);
  proxy->AddObserver(vtkCommand::UpdatePropertyEvent, this->SubProxyObserver);
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveSubProxy(const char* name)
{
  if (!name)
  {
    return;
  }

  vtkSMProxyInternals::ProxyMap::iterator it = this->Internals->SubProxies.find(name);

  vtkSmartPointer<vtkSMProxy> subProxy;
  if (it != this->Internals->SubProxies.end())
  {
    subProxy = it->second; // we keep the proxy since we need it to remove links.
    it->second.GetPointer()->RemoveObserver(this->SubProxyObserver);
    // Note, we are assuming here that a proxy cannot be added
    // twice as a subproxy to the same proxy.
    this->Internals->SubProxies.erase(it);
  }

  // Now, remove any exposed properties for this subproxy.
  vtkSMProxyInternals::ExposedPropertyInfoMap::iterator iter =
    this->Internals->ExposedProperties.begin();
  while (iter != this->Internals->ExposedProperties.end())
  {
    if (iter->second.SubProxyName == name)
    {
      this->Internals->ExposedProperties.erase(iter);
      // start again.
      iter = this->Internals->ExposedProperties.begin();
    }
    else
    {
      iter++;
    }
  }

  if (subProxy.GetPointer())
  {
    subProxy->ParentProxy = NULL;
    // Now, remove any shared property links for the subproxy.
    vtkSMProxyInternals::SubProxyLinksType::iterator iter2 = this->Internals->SubProxyLinks.begin();
    while (iter2 != this->Internals->SubProxyLinks.end())
    {
      iter2->GetPointer()->RemoveLinkedProxy(subProxy.GetPointer());
      if (iter2->GetPointer()->GetNumberOfLinkedProxies() <= 1)
      {
        // link is useless, remove it.
        this->Internals->SubProxyLinks.erase(iter2);
        iter2 = this->Internals->SubProxyLinks.begin();
      }
      else
      {
        iter2++;
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::ExecuteSubProxyEvent(vtkSMProxy* subproxy, unsigned long event, void* data)
{
  if (subproxy &&
    (event == vtkCommand::PropertyModifiedEvent || event == vtkCommand::UpdatePropertyEvent))
  {
    // A Subproxy has been modified.
    const char* name = reinterpret_cast<const char*>(data);
    const char* exposed_name = 0;
    if (name)
    {
      // Check if the property from the subproxy was exposed.
      // If so, we invoke this event with the exposed name.

      // First determine the name for this subproxy.
      vtkSMProxyInternals::ProxyMap::iterator proxy_iter = this->Internals->SubProxies.begin();
      const char* subproxy_name = 0;
      for (; proxy_iter != this->Internals->SubProxies.end(); ++proxy_iter)
      {
        if (proxy_iter->second.GetPointer() == subproxy)
        {
          subproxy_name = proxy_iter->first.c_str();
          break;
        }
      }
      if (subproxy_name)
      {
        // Now locate the exposed property name.
        vtkSMProxyInternals::ExposedPropertyInfoMap::iterator iter =
          this->Internals->ExposedProperties.begin();
        for (; iter != this->Internals->ExposedProperties.end(); ++iter)
        {
          if (iter->second.SubProxyName == subproxy_name && iter->second.PropertyName == name)
          {
            // This property is indeed exposed. Set the corrrect exposed name.
            exposed_name = iter->first.c_str();
            break;
          }
        }
      }
    }

    if (event == vtkCommand::PropertyModifiedEvent)
    {
      // Let the world know that one of the subproxies of this proxy has
      // been modified. If the subproxy exposed the modified property, we
      // provide the name of the property. Otherwise, 0, indicating
      // some internal property has changed.
      this->InvokeEvent(vtkCommand::PropertyModifiedEvent, (void*)exposed_name);
    }
    else if (exposed_name && event == vtkCommand::UpdatePropertyEvent)
    {
      // UpdatePropertyEvent is fired only for exposed properties.
      this->InvokeEvent(vtkCommand::UpdatePropertyEvent, (void*)exposed_name);
      this->MarkModified(subproxy);
    }
  }

  // Note we are not throwing vtkCommand::UpdateEvent fired by subproxies.
  // Since doing so would imply that this proxy (as well as all its subproxies)
  // are updated, which is not necessarily true when a subproxy fires
  // an UpdateEvent.
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  int found = 0;
  std::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = this->Internals->Consumers.begin();
  for (; i != this->Internals->Consumers.end(); i++)
  {
    if (i->Property == property && i->Proxy == proxy)
    {
      found = 1;
      break;
    }
  }

  if (!found)
  {
    vtkSMProxyInternals::ConnectionInfo info(property, proxy);
    this->Internals->Consumers.push_back(info);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveConsumer(vtkSMProperty* property, vtkSMProxy*)
{
  std::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = this->Internals->Consumers.begin();
  for (; i != this->Internals->Consumers.end(); i++)
  {
    if (i->Property == property)
    {
      this->Internals->Consumers.erase(i);
      break;
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveAllConsumers()
{
  this->Internals->Consumers.erase(
    this->Internals->Consumers.begin(), this->Internals->Consumers.end());
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxy::GetNumberOfConsumers()
{
  return static_cast<unsigned int>(this->Internals->Consumers.size());
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetConsumerProxy(unsigned int idx)
{
  return this->Internals->Consumers[idx].Proxy;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::GetConsumerProperty(unsigned int idx)
{
  return this->Internals->Consumers[idx].Property;
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddProducer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  int found = 0;
  std::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = this->Internals->Producers.begin();
  for (; i != this->Internals->Producers.end(); i++)
  {
    if (i->Property == property && i->Proxy == proxy)
    {
      found = 1;
      break;
    }
  }

  if (!found)
  {
    vtkSMProxyInternals::ConnectionInfo info(property, proxy);
    this->Internals->Producers.push_back(info);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveProducer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  std::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = this->Internals->Producers.begin();
  for (; i != this->Internals->Producers.end(); i++)
  {
    if (i->Property == property && i->Proxy == proxy)
    {
      this->Internals->Producers.erase(i);
      break;
    }
  }
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxy::GetNumberOfProducers()
{
  return static_cast<unsigned int>(this->Internals->Producers.size());
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetProducerProxy(unsigned int idx)
{
  return this->Internals->Producers[idx].Proxy;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::GetProducerProperty(unsigned int idx)
{
  return this->Internals->Producers[idx].Property;
}

//----------------------------------------------------------------------------
void vtkSMProxy::PostUpdateData(bool using_cache)
{
  unsigned int numProducers = this->GetNumberOfProducers();
  for (unsigned int i = 0; i < numProducers; i++)
  {
    if (this->GetProducerProxy(i)->NeedsUpdate)
    {
      this->GetProducerProxy(i)->PostUpdateData(using_cache);
    }
  }
  if (this->NeedsUpdate)
  {
    vtkLogF(TRACE, "PostUpdateData (%s)", this->GetLogNameOrDefault());
    // this->NeedsUpdate must be set to false before firing this event otherwise
    // if the event handler results in other view updates, we end up
    // unnecessarily thinking that this proxy needs update.
    this->NeedsUpdate = false;

    if (!using_cache)
    {
      this->InvokeEvent(vtkCommand::UpdateDataEvent, 0);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  /*
   * UpdatePropertyInformation() is now explicitly called in
   * UpdatePipelineInformation(). The calling on UpdatePropertyInformation()
   * was not really buying us much as far as keeping dependent domains updated
   * was concerned, for unless UpdatePipelineInformation was called on the
   * reader/filter, updating info properties was not going to yield any
   * changed values. Removing this also allows for linking for info properties
   * and properties using property links.
   * A side effect of this may be that the 3DWidgets information properties wont get
   * updated on setting "action" properties such as PlaceWidget.
  if (this->ObjectsCreated)
    {
    // If not created yet, don't worry syncing the info properties.
    this->UpdatePropertyInformation();
    }
  */
  if (!this->InMarkModified)
  {
    this->InMarkModified = 1;
    this->InvokeEvent(vtkCommand::ModifiedEvent, (void*)modifiedProxy);
    this->MarkDirty(modifiedProxy);
    this->InMarkModified = 0;
  }
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if (this->NeedsUpdate)
  {
    return;
  }

  vtkLogF(TRACE, "MarkDirty (%s)", this->GetLogNameOrDefault());
  this->MarkConsumersAsDirty(modifiedProxy);
  this->NeedsUpdate = true;
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkConsumersAsDirty(vtkSMProxy* modifiedProxy)
{
  for (const auto& cinfo : this->Internals->Consumers)
  {
    if (auto cons = cinfo.Proxy.GetPointer())
    {
      cons->MarkDirtyFromProducer(modifiedProxy, this, cinfo.Property);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkDirtyFromProducer(vtkSMProxy* modifiedProxy, vtkSMProxy* vtkNotUsed(producer),
  vtkSMProperty* vtkNotUsed(producerProperty))
{
  this->MarkDirty(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkInputsAsDirty()
{
  for (unsigned int cc = 0, max = this->GetNumberOfProducers(); cc < max; ++cc)
  {
    if (vtkSMInputProperty::SafeDownCast(this->GetProducerProperty(cc)))
    {
      auto producer = this->GetProducerProxy(cc);
      producer->NeedsUpdate = true;
      producer->MarkInputsAsDirty();
    }
  }
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::NewProperty(const char* name)
{
  vtkSMProperty* property = this->GetProperty(name);
  if (property)
  {
    return property;
  }

  vtkPVXMLElement* element = this->XMLElement;
  if (!element)
  {
    return 0;
  }

  vtkPVXMLElement* propElement = 0;
  for (unsigned int i = 0; i < element->GetNumberOfNestedElements(); ++i)
  {
    propElement = element->GetNestedElement(i);
    if (strcmp(propElement->GetName(), "SubProxy") != 0)
    {
      const char* pname = propElement->GetAttribute("name");
      if (pname && strcmp(name, pname) == 0)
      {
        break;
      }
    }
    propElement = 0;
  }
  if (!propElement)
  {
    return 0;
  }
  return this->NewProperty(name, propElement);
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::NewProperty(const char* name, vtkPVXMLElement* propElement)
{
  vtkSMProperty* property = this->GetProperty(name);
  if (property)
  {
    return property;
  }

  if (!propElement)
  {
    return 0;
  }

  // Patch XML to remove InformationHelper and set right si_class
  vtkSIProxyDefinitionManager::PatchXMLProperty(propElement);

  vtkObject* object = 0;
  std::ostringstream cname;
  cname << "vtkSM" << propElement->GetName() << ends;
  object = vtkPVInstantiator::CreateInstance(cname.str().c_str());

  property = vtkSMProperty::SafeDownCast(object);
  if (property)
  {
    int old_val = this->DoNotUpdateImmediately;
    int old_val2 = this->DoNotModifyProperty;
    this->DoNotUpdateImmediately = 1;

    // Internal properties should not be created as modified.
    // Otherwise, properties like ForceUpdate get pushed and
    // cause problems.
    int is_internal;
    if (property->GetIsInternal() || property->IsStateIgnored() ||
      strcmp(property->GetClassName(), "vtkSMProperty") == 0)
    {
      this->DoNotModifyProperty = 1;
    }
    if (propElement->GetScalarAttribute("is_internal", &is_internal))
    {
      if (is_internal)
      {
        this->DoNotModifyProperty = 1;
      }
    }
    this->AddProperty(name, property);
    if (!property->ReadXMLAttributes(this, propElement))
    {
      vtkErrorMacro("Could not parse property: " << propElement->GetName());
      this->DoNotUpdateImmediately = old_val;
      return 0;
    }
    this->DoNotUpdateImmediately = old_val;
    this->DoNotModifyProperty = old_val2;

    // Properties should be created as modified unless they
    // are internal.
    //     if (!property->GetIsInternal())
    //       {
    //       this->Internals->Properties[name].ModifiedFlag = 1;
    //       }
    property->Delete();
  }
  else
  {
    vtkErrorMacro("Could not instantiate property: " << propElement->GetName());
  }

  return property;
}

//---------------------------------------------------------------------------
class vtkSMProxyPropertyLinkObserver : public vtkCommand
{
public:
  vtkWeakPointer<vtkSMProperty> Output;
  typedef vtkCommand Superclass;
  const char* GetClassNameInternal() const override { return "vtkSMProxyPropertyLinkObserver"; }
  static vtkSMProxyPropertyLinkObserver* New() { return new vtkSMProxyPropertyLinkObserver(); }
  void Execute(vtkObject* caller, unsigned long event, void* calldata) override
  {
    (void)event;
    (void)calldata;
    vtkSMProperty* input = vtkSMProperty::SafeDownCast(caller);
    if (input && this->Output)
    {
      // this will copy both checked and unchecked property values.
      this->Output->Copy(input);
    }
  }
};

//---------------------------------------------------------------------------
void vtkSMProxy::LinkProperty(vtkSMProperty* input, vtkSMProperty* output)
{
  if (input == output || input == NULL || output == NULL)
  {
    vtkErrorMacro(
      "Invalid call to vtkSMProxy::LinkProperty. Check arguments." << this->GetXMLName());
    return;
  }

  vtkSMProxyPropertyLinkObserver* observer = vtkSMProxyPropertyLinkObserver::New();
  observer->Output = output;
  input->AddObserver(vtkCommand::PropertyModifiedEvent, observer);
  input->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, observer);
  observer->FastDelete();
}

//---------------------------------------------------------------------------
vtkSMPropertyGroup* vtkSMProxy::NewPropertyGroup(vtkPVXMLElement* groupElem)
{
  vtkSMPropertyGroup* group = vtkSMPropertyGroup::New();
  if (!group->ReadXMLAttributes(this, groupElem))
  {
    group->Delete();
    return NULL;
  }

  // FIXME: should we use group-name as the "key" for the property groups?
  this->Internals->PropertyGroups.push_back(group);
  group->Delete();

  return group;
}

//---------------------------------------------------------------------------
int vtkSMProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  this->SetXMLElement(element);

  // Read the common attributes.
  const char* className = element->GetAttribute("class");
  if (className)
  {
    this->SetVTKClassName(className);
  }

  const char* kernelClass = element->GetAttribute("si_class");
  if (kernelClass)
  {
    this->SetSIClassName(kernelClass);
  }

  const char* xmllabel = element->GetAttribute("label");
  if (xmllabel)
  {
    this->SetXMLLabel(xmllabel);
  }
  else
  {
    this->SetXMLLabel(this->GetXMLName());
  }

  const char* processes = element->GetAttribute("processes");
  if (processes)
  {
    vtkTypeUInt32 uiprocesses = 0;
    std::string strprocesses = processes;
    if (strprocesses.find("client") != std::string::npos)
    {
      uiprocesses |= vtkProcessModule::CLIENT;
    }
    if (strprocesses.find("renderserver") != std::string::npos)
    {
      uiprocesses |= vtkProcessModule::RENDER_SERVER;
    }
    if (strprocesses.find("dataserver") != std::string::npos)
    {
      uiprocesses |= vtkProcessModule::DATA_SERVER;
    }
    this->SetLocation(uiprocesses);
  }

  // Locate documentation.
  for (unsigned int cc = 0; cc < element->GetNumberOfNestedElements(); ++cc)
  {
    vtkPVXMLElement* subElem = element->GetNestedElement(cc);
    if (strcmp(subElem->GetName(), "Documentation") == 0)
    {
      this->Documentation->SetDocumentationElement(subElem);
    }
    else if (strcmp(subElem->GetName(), "Hints") == 0)
    {
      this->SetHints(subElem);
    }
    else if (strcmp(subElem->GetName(), "Deprecated") == 0)
    {
      this->SetDeprecated(subElem);
    }
  }

  // Create all properties
  int old_value = this->DoNotModifyProperty; // FIXME COLLAB: Prevent sending default values
  this->DoNotModifyProperty = 1;             // FIXME COLLAB: Prevent sending default values

  if (!this->CreateSubProxiesAndProperties(pm, element))
  {
    return 0;
  }

  // Setup subproxy links with parent proxy.
  for (unsigned int cc = 0, max_cc = element->GetNumberOfNestedElements(); cc < max_cc; ++cc)
  {
    vtkPVXMLElement* subElem = element->GetNestedElement(cc);
    if (strcmp(subElem->GetName(), "SubProxy") != 0)
    {
      continue;
    }
    std::string subproxyName;
    for (unsigned int kk = 0, max_kk = subElem->GetNumberOfNestedElements(); kk < max_kk; ++kk)
    {
      vtkPVXMLElement* kElem = subElem->GetNestedElement(kk);
      if (strcmp(kElem->GetName(), "Proxy") == 0)
      {
        subproxyName = kElem->GetAttributeOrDefault("name", "");
        break;
      }
    }
    vtkSMProxy* subProxy = subproxyName.empty() ? NULL : this->GetSubProxy(subproxyName.c_str());
    if (subProxy == NULL)
    {
      continue;
    }
    for (unsigned int kk = 0, max_kk = subElem->GetNumberOfNestedElements(); kk < max_kk; ++kk)
    {
      vtkPVXMLElement* kElem = subElem->GetNestedElement(kk);
      if (strcmp(kElem->GetName(), "LinkProperties") != 0)
      {
        continue;
      }
      for (unsigned int jj = 0, max_jj = kElem->GetNumberOfNestedElements(); jj < max_jj; ++jj)
      {
        vtkPVXMLElement* jElem = kElem->GetNestedElement(jj);
        if (strcmp(jElem->GetName(), "Property") == 0)
        {
          this->LinkProperty(this->GetProperty(jElem->GetAttribute("with_property")),
            subProxy->GetProperty(jElem->GetAttribute("name")));
        }
      }
    }
  }
  this->DoNotModifyProperty = old_value; // FIXME COLLAB: Prevent sending default values
  this->SetXMLElement(0);
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxy::CreateSubProxiesAndProperties(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!element)
  {
    return 0;
  }

  // Just build once
  static vtksys::RegularExpression END_WITH_PROPERTY(".*Property$");

  for (unsigned int i = 0; i < element->GetNumberOfNestedElements(); ++i)
  {
    vtkPVXMLElement* propElement = element->GetNestedElement(i);
    if (pm != NULL && strcmp(propElement->GetName(), "SubProxy") == 0)
    {
      vtkPVXMLElement* subElement = propElement->GetNestedElement(0);
      if (subElement)
      {
        const char* name = subElement->GetAttribute("name");
        const char* pname = subElement->GetAttribute("proxyname");
        const char* gname = subElement->GetAttribute("proxygroup");
        int override = 0;
        if (!subElement->GetScalarAttribute("override", & override))
        {
          override = 0;
        }
        if (pname && !gname)
        {
          vtkErrorMacro("proxygroup not specified. Subproxy cannot be created.");
          return 0;
        }
        if (gname && !pname)
        {
          vtkErrorMacro("proxyname not specified. Subproxy cannot be created.");
          return 0;
        }
        if (name)
        {
          vtkSMProxy* subproxy = 0;
          if (pname && gname)
          {
            subproxy = pm->NewProxy(gname, pname);
          }
          else
          {
            gname = this->XMLGroup;
            pname = this->XMLName;
            subproxy = pm->NewProxy(subElement, gname, pname, name);
          }
          if (!subproxy)
          {
            vtkErrorMacro("Failed to create subproxy: " << (pname ? pname : "(none"));
            return 0;
          }
          // Here, we turn on DoNotModifyProperty to ensure that we don't mark
          // the properties modified as we are processing them e.g. setting
          // panel-visibilities, etc.
          subproxy->DoNotModifyProperty = 1;
          this->AddSubProxy(name, subproxy, override);
          this->SetupSharedProperties(subproxy, propElement);
          this->SetupExposedProperties(name, propElement);
          subproxy->DoNotModifyProperty = 0;
          subproxy->Delete();
        }
      }
    }
    else if (END_WITH_PROPERTY.find(propElement->GetName()) && propElement->GetAttribute("name"))
    {
      // Make sure that attribute value won't get corrupted inside the coming call
      std::string propName = propElement->GetAttribute("name");
      this->NewProperty(propName.c_str(), propElement);
    }
    else if (strcmp(propElement->GetName(), "PropertyGroup") == 0)
    {
      // Create a property group.
      this->NewPropertyGroup(propElement);
    }
  }
  return 1;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::SetupExposedProperty(
  vtkPVXMLElement* propertyElement, const char* subproxy_name)
{
  const char* name = propertyElement->GetAttribute("name");
  if (!name || !name[0])
  {
    vtkErrorMacro("Attribute name is required!");
    return 0;
  }
  const char* exposed_name = propertyElement->GetAttribute("exposed_name");
  if (!exposed_name)
  {
    // use the property name as the exposed name.
    exposed_name = name;
  }
  int override = 0;
  if (!propertyElement->GetScalarAttribute("override", & override))
  {
    override = 0;
  }

  if (propertyElement->GetAttribute("default_values"))
  {
    vtkSMProxy* subproxy = this->GetSubProxy(subproxy_name);
    vtkSMProperty* prop = subproxy->GetProperty(name);
    if (!prop)
    {
      vtkWarningMacro(
        "Failed to locate property '" << name << "' on subproxy '" << subproxy_name << "'");
      return 0;
    }
    const std::string propertyName(prop->GetXMLName());
    if (!prop->ReadXMLAttributes(subproxy, propertyElement))
    {
      prop->SetXMLName(propertyName.c_str());
      return 0;
    }
    prop->SetXMLName(propertyName.c_str());

    // Since we are not processing ExposedProperties elements on the SIProxy
    // side, the SIProxy doesn't have the information about updated defaults for
    // the properties. Hence, we need to push those values in the next
    // UpdateVTKObjects(). To ensure that, we have to mark this Property
    // modified.
    int old_val = subproxy->DoNotModifyProperty;
    subproxy->DoNotModifyProperty = 0;
    prop->Modified();
    subproxy->DoNotModifyProperty = old_val;
  }
  this->ExposeSubProxyProperty(subproxy_name, name, exposed_name, override);

  vtkSMProxy* subproxy = this->GetSubProxy(subproxy_name);
  vtkSMProperty* prop = subproxy->GetProperty(name);

  if (!prop)
  {
    vtkWarningMacro("Failed to locate property '" << name << "' on subproxy '" << subproxy_name
                                                  << "': " << this->XMLName);
    return 0;
  }

  // override panel_visibility with that of the exposed property
  const char* panel_visibility = propertyElement->GetAttribute("panel_visibility");
  if (panel_visibility)
  {
    prop->SetPanelVisibility(panel_visibility);
  }

  // override panel_visibility_default_for_representation with that of the exposed property
  const char* panel_visibility_default_for_representation =
    propertyElement->GetAttribute("panel_visibility_default_for_representation");
  if (panel_visibility_default_for_representation)
  {
    prop->SetPanelVisibilityDefaultForRepresentation(panel_visibility_default_for_representation);
  }

  // override panel_widget with that of the exposed property
  const char* panel_widget = propertyElement->GetAttribute("panel_widget");
  if (panel_widget)
  {
    prop->SetPanelWidget(panel_widget);
  }

  // override label with that of the exposed property
  const char* label = propertyElement->GetAttribute("label");
  if (label)
  {
    prop->SetXMLLabel(label);
  }

  return prop;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetupExposedProperties(const char* subproxy_name, vtkPVXMLElement* element)
{
  if (!subproxy_name || !element)
  {
    return;
  }

  unsigned int i, j;
  for (i = 0; i < element->GetNumberOfNestedElements(); i++)
  {
    vtkPVXMLElement* exposedElement = element->GetNestedElement(i);
    if (!(strcmp(exposedElement->GetName(), "ExposedProperties") == 0 ||
          strcmp(exposedElement->GetName(), "PropertyGroup") == 0))
    {
      continue;
    }
    for (j = 0; j < exposedElement->GetNumberOfNestedElements(); j++)
    {
      vtkPVXMLElement* propertyElement = exposedElement->GetNestedElement(j);
      if (strcmp(propertyElement->GetName(), "Property") == 0)
      {
        this->SetupExposedProperty(propertyElement, subproxy_name);
      }
      else if (strcmp(propertyElement->GetName(), "PropertyGroup") == 0)
      {
        // Process properties exposed under this element first.
        vtkPVXMLElement* groupElement = propertyElement;
        for (unsigned int k = 0; k < groupElement->GetNumberOfNestedElements(); k++)
        {
          vtkPVXMLElement* subElem = groupElement->GetNestedElement(k);
          if (strcmp(subElem->GetName(), "Hints") == 0)
          {
            continue;
          }
          this->SetupExposedProperty(subElem, subproxy_name);
        }

        // Now create the group.
        this->NewPropertyGroup(groupElement);
      }
      else
      {
        vtkErrorMacro("<ExposedProperties> can contain <Property> or <PropertyGroup> elements.");
        continue;
      }
    }
  }
}
//---------------------------------------------------------------------------
void vtkSMProxy::SetupSharedProperties(vtkSMProxy* subproxy, vtkPVXMLElement* element)
{
  if (!subproxy || !element)
  {
    return;
  }

  for (unsigned int i = 0; i < element->GetNumberOfNestedElements(); i++)
  {
    vtkPVXMLElement* propElement = element->GetNestedElement(i);
    if (strcmp(propElement->GetName(), "ShareProperties") == 0)
    {
      const char* name = propElement->GetAttribute("subproxy");
      if (!name || !name[0])
      {
        continue;
      }
      vtkSMProxy* src_subproxy = this->GetSubProxy(name);
      if (!src_subproxy)
      {
        vtkErrorMacro("Subproxy " << name << " must be defined before "
                                             "its properties can be shared with another subproxy.");
        continue;
      }
      vtkSMProxyLink* sharingLink = vtkSMProxyLink::New();
      sharingLink->PropagateUpdateVTKObjectsOff();

      // Read the exceptions.
      for (unsigned int j = 0; j < propElement->GetNumberOfNestedElements(); j++)
      {
        vtkPVXMLElement* exceptionProp = propElement->GetNestedElement(j);
        if (strcmp(exceptionProp->GetName(), "Exception") != 0)
        {
          continue;
        }
        const char* exp_name = exceptionProp->GetAttribute("name");
        if (!exp_name)
        {
          vtkErrorMacro("Exception tag must have the attribute 'name'.");
          continue;
        }
        sharingLink->AddException(exp_name);
      }
      sharingLink->AddLinkedProxy(src_subproxy, vtkSMLink::INPUT);
      sharingLink->AddLinkedProxy(subproxy, vtkSMLink::OUTPUT);
      this->Internals->SubProxyLinks.push_back(sharingLink);
      sharingLink->Delete();
    }
  }
}

//---------------------------------------------------------------------------
vtkSMPropertyIterator* vtkSMProxy::NewPropertyIterator()
{
  vtkSMPropertyIterator* iter = vtkSMPropertyIterator::New();
  iter->SetProxy(this);
  return iter;
}

//---------------------------------------------------------------------------
void vtkSMProxy::Copy(vtkSMProxy* src)
{
  this->Copy(src, 0, vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE);
}

//---------------------------------------------------------------------------
void vtkSMProxy::Copy(vtkSMProxy* src, const char* exceptionClass)
{
  this->Copy(src, exceptionClass, vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE);
}

//---------------------------------------------------------------------------
void vtkSMProxy::Copy(vtkSMProxy* src, const char* exceptionClass, int proxyPropertyCopyFlag)
{
  if (!src)
  {
    return;
  }

  if (proxyPropertyCopyFlag != COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE)
  {
    vtkWarningMacro("COPY_PROXY_PROPERTY_VALUES_BY_CLONING is no longer supported."
                    " Using COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE instead.");
    proxyPropertyCopyFlag = COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE;
  }

  vtkSMPropertyIterator* iter = this->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    const char* key = iter->GetKey();
    vtkSMProperty* dest = iter->GetProperty();
    if (key && dest)
    {
      vtkSMProperty* source = src->GetProperty(key);
      if (source)
      {
        if (!exceptionClass || !dest->IsA(exceptionClass))
        {
          vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(dest);
          if (!pp || proxyPropertyCopyFlag == vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE)
          {
            dest->Copy(source);
          }
        }
      }
    }
  }

  iter->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxy::ExposeSubProxyProperty(
  const char* subproxy_name, const char* property_name, const char* exposed_name, int override)
{
  if (!subproxy_name || !property_name || !exposed_name)
  {
    vtkErrorMacro("Either subproxy name, property name, or exposed name is NULL.");
    return;
  }

  vtkSMProxyInternals::ExposedPropertyInfoMap::iterator iter =
    this->Internals->ExposedProperties.find(exposed_name);
  if (iter != this->Internals->ExposedProperties.end())
  {
    if (!override)
    {
      vtkWarningMacro("An exposed property with the name \""
        << exposed_name << "\" already exists. It will be replaced.");
    }
  }

  vtkSMProxyInternals::ExposedPropertyInfo info;
  info.SubProxyName = subproxy_name;
  info.PropertyName = property_name;
  this->Internals->ExposedProperties[exposed_name] = info;

  // Add the exposed property name to the vector of property names.
  // This vector keeps track of the order in which properties
  // were added.
  this->Internals->PropertyNamesInOrder.push_back(exposed_name);
}

//---------------------------------------------------------------------------
void vtkSMProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LogName: " << (this->LogName ? this->LogName : "(null)") << endl;
  os << indent << "VTKClassName: " << (this->VTKClassName ? this->VTKClassName : "(null)") << endl;
  os << indent << "XMLName: " << (this->XMLName ? this->XMLName : "(null)") << endl;
  os << indent << "XMLGroup: " << (this->XMLGroup ? this->XMLGroup : "(null)") << endl;
  os << indent << "XMLLabel: " << (this->XMLLabel ? this->XMLLabel : "(null)") << endl;
  os << indent << "Documentation: " << this->Documentation << endl;
  os << indent << "ObjectsCreated: " << this->ObjectsCreated << endl;
  os << indent << "Hints: ";
  if (this->Hints)
  {
    this->Hints->PrintSelf(os, indent);
  }
  else
  {
    os << "(null)" << endl;
  }

  vtkSMPropertyIterator* iter = this->NewPropertyIterator();
  if (iter)
  {
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      const char* key = iter->GetKey();
      vtkSMProperty* property = iter->GetProperty();
      if (key)
      {
        os << indent << "Property (" << key << "): ";
        if (property)
        {
          os << endl;
          property->PrintSelf(os, indent.GetNextIndent());
        }
        else
        {
          os << "(none)" << endl;
        }
      }
    }
    iter->Delete();
  }
}
//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxy::SaveXMLState(vtkPVXMLElement* root)
{
  vtkSMPropertyIterator* iter = this->NewPropertyIterator();
  vtkPVXMLElement* result = this->SaveXMLState(root, iter);
  iter->Delete();
  return result;
}
//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxy::SaveXMLState(vtkPVXMLElement* root, vtkSMPropertyIterator* iter)
{
  if (iter == NULL)
  {
    return this->SaveXMLState(root);
  }

  vtkPVXMLElement* proxyXml = vtkPVXMLElement::New();

  proxyXml->SetName("Proxy");
  proxyXml->AddAttribute("group", this->XMLGroup);
  proxyXml->AddAttribute("type", this->XMLName);
  proxyXml->AddAttribute("id", static_cast<unsigned int>(this->GetGlobalID()));
  proxyXml->AddAttribute("servers", static_cast<unsigned int>(this->GetLocation()));

  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    if (!iter->GetProperty())
    {
      vtkWarningMacro(
        "Missing property with name: " << iter->GetKey() << " on " << this->GetXMLName());
      continue;
    }
    if (!iter->GetProperty()->GetIsInternal())
    {
      std::ostringstream propID;
      propID << this->GetGlobalID() << "." << iter->GetKey() << ends;
      iter->GetProperty()->SaveState(proxyXml, iter->GetKey(), propID.str().c_str());
    }
  }

  // Add proxy annotation in XML state
  vtkSMProxyInternals::AnnotationMap::iterator annotationIterator =
    this->Internals->Annotations.begin();
  while (annotationIterator != this->Internals->Annotations.end())
  {
    vtkNew<vtkPVXMLElement> annotation;
    annotation->SetName("Annotation");
    annotation->AddAttribute("key", annotationIterator->first.c_str());
    annotation->AddAttribute("value", annotationIterator->second.c_str());
    proxyXml->AddNestedElement(annotation.GetPointer());

    // move forward
    annotationIterator++;
  }

  if (root)
  {
    root->AddNestedElement(proxyXml);
    proxyXml->FastDelete();
  }

  return proxyXml;
}
//---------------------------------------------------------------------------
int vtkSMProxy::LoadXMLState(vtkPVXMLElement* proxyElement, vtkSMProxyLocator* locator)
{
  unsigned int numElems = proxyElement->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (!name)
    {
      continue;
    }
    if (strcmp(name, "Property") == 0)
    {
      const char* prop_name = currentElement->GetAttribute("name");
      if (!prop_name)
      {
        vtkErrorMacro("Cannot load property without a name.");
        continue;
      }
      vtkSMProperty* property = this->GetProperty(prop_name);
      if (!property)
      {
        vtkDebugMacro("Property " << prop_name << " does not exist.");
        continue;
      }
      if (property->GetInformationOnly())
      {
        // don't load state for information only property.
        continue;
      }
      if (!property->LoadState(currentElement, locator))
      {
        return 0;
      }
    }
    if (strcmp(name, "Annotation") == 0)
    {
      this->SetAnnotation(
        currentElement->GetAttribute("key"), currentElement->GetAttribute("value"));
    }
  }
  return 1;
}
//---------------------------------------------------------------------------
const vtkSMMessage* vtkSMProxy::GetFullState()
{
  return this->State;
}
//---------------------------------------------------------------------------
void vtkSMProxy::LoadState(const vtkSMMessage* message, vtkSMProxyLocator* locator)
{
  // Update globalId. This will fails if that one is already set with a different value
  if (this->HasGlobalID() && this->GetGlobalID() != message->global_id())
  {
    vtkErrorMacro("Try to load a state on a proxy which has a different ID"
      << "(" << this->GetGlobalID() << " != " << message->global_id() << ")");
  }
  else
  {
    this->SetGlobalID(message->global_id());
  }

  // We try to extract some message information that we might not get from
  // proxy definition in the XML. This is specially true in collaboration.
  if (message->HasExtension(DefinitionHeader::server_class))
  {
    this->SetSIClassName(message->GetExtension(DefinitionHeader::server_class).c_str());
  }
  if (message->HasExtension(ProxyState::xml_group))
  {
    this->SetXMLGroup(message->GetExtension(ProxyState::xml_group).c_str());
  }
  if (message->HasExtension(ProxyState::xml_name))
  {
    this->SetXMLName(message->GetExtension(ProxyState::xml_name).c_str());
  }
  if (message->HasExtension(ProxyState::xml_sub_proxy_name))
  {
    this->SetXMLSubProxyName(message->GetExtension(ProxyState::xml_sub_proxy_name).c_str());
  }

  // Manage its sub-proxy state
  int nbSubProxy = message->ExtensionSize(ProxyState::subproxy);
  std::vector<vtkSMMessage> subProxyStateToLoad;
  for (int idx = 0; idx < nbSubProxy; idx++)
  {
    const ProxyState_SubProxy* subProxyMsg = &message->GetExtension(ProxyState::subproxy, idx);
    vtkSMProxy* subProxy = this->GetSubProxy(subProxyMsg->name().c_str());

    if (subProxy == NULL)
    {
      vtkWarningMacro("State provide a sub-proxy information although the proxy"
        << "does not find that sub-proxy."
        << " - Proxy: " << this->XMLGroup << " - " << this->XMLName << endl
        << " - Sub-Proxy: " << subProxyMsg->name().c_str() << " " << subProxyMsg->global_id());
      continue;
    }

    // Make sure we do not try to load a state to a proxy that has already
    // sub-proxy with IDs that differ from the message state
    if (subProxy->HasGlobalID() && (subProxy->GlobalID != subProxyMsg->global_id() ||
                                     !this->Session->GetRemoteObject(subProxyMsg->global_id())))
    {
      vtkErrorMacro("Invalid Proxy for message "
        << endl
        << "Parent Proxy : (" << this->XMLGroup << "," << this->XMLName << ")" << endl
        << "SubProxy - XMLName: " << subProxy->GetXMLName() << " - SubProxyName: "
        << subProxyMsg->name().c_str() << " - Id: " << subProxy->GlobalID << endl
        << message->DebugString().c_str() << endl);
    }

    // Update sub-proxy state if possible
    if (!subProxy->HasGlobalID())
    {
      vtkSMMessage subProxyState;
      subProxy->SetGlobalID(subProxyMsg->global_id());
      if (this->GetSession()->GetStateLocator()->FindState(subProxy->GetGlobalID(), &subProxyState))
      {
        subProxyStateToLoad.push_back(subProxyState);
      }
    }
  }
  // Load deferred sub-proxy state
  // Deferring sub-proxy loading IS VERY IMPORTANT, especially for compound proxy
  // that define pipeline connectivity.
  // If not done while loading the pipeline connection, this will failed because
  // the sub-proxy involved might not have a GlobalID yet !
  for (size_t i = 0; i < subProxyStateToLoad.size(); i++)
  {
    vtkSMProxy* proxy =
      vtkSMProxy::SafeDownCast(this->Session->GetRemoteObject(subProxyStateToLoad[i].global_id()));
    proxy->LoadState(&subProxyStateToLoad[i], locator);
  }

  // Manage properties
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (int i = 0; i < message->ExtensionSize(ProxyState::property); ++i)
  {
    const ProxyState_Property* prop_message = &message->GetExtension(ProxyState::property, i);
    const char* pname = prop_message->name().c_str();
    it = this->Internals->Properties.find(pname);
    if (it != this->Internals->Properties.end())
    {
      if (it->second.Property->GetIsInternal())
      {
        // skip internal properties. Their state is never updated.
        continue;
      }

      // Some view properties need some special treatment and some
      // of there properties MUST NOT be updated in case of collaborative
      // notification.
      if (this->Session->IsProcessingRemoteNotification() &&
        it->second.Property->GetIgnoreSynchronization())
      {
        continue;
      }

      it->second.Property->ReadFrom(message, i, locator);
    }
  }

  // We don't need to do anything special to update domains that may have
  // changed as a consequence of the information properties being updated since
  // the vtkSMProperty automatically called vtkSMProperty::UpdateDomains() when
  // the property changes.

  // Manage annotation
  if (message->GetExtension(ProxyState::has_annotation))
  {
    int nbAnnotation = message->ExtensionSize(ProxyState::annotation);
    bool previousAnnotationPush = this->Internals->EnableAnnotationPush;
    this->RemoveAllAnnotations();
    for (int idx = 0; idx < nbAnnotation; idx++)
    {
      const ProxyState_Annotation* annotation = &message->GetExtension(ProxyState::annotation, idx);
      this->SetAnnotation(annotation->key().c_str(), annotation->value().c_str());
    }
    this->Internals->EnableAnnotationPush = previousAnnotationPush;
  }
}

//---------------------------------------------------------------------------
vtkSMPropertyGroup* vtkSMProxy::GetPropertyGroup(size_t index) const
{
  assert(index < this->Internals->PropertyGroups.size());

  return this->Internals->PropertyGroups[index];
}

//---------------------------------------------------------------------------
size_t vtkSMProxy::GetNumberOfPropertyGroups() const
{
  return this->Internals->PropertyGroups.size();
}

//---------------------------------------------------------------------------
void vtkSMProxy::PrototypeOn()
{
  this->SetPrototype(true);
}

//---------------------------------------------------------------------------
void vtkSMProxy::PrototypeOff()
{
  this->SetPrototype(false);
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetPrototype(bool proto)
{
  this->Superclass::SetPrototype(proto);
  for (unsigned int cc = 0; cc < this->GetNumberOfSubProxies(); cc++)
  {
    this->GetSubProxy(cc)->SetPrototype(proto);
  }
}
//---------------------------------------------------------------------------
void vtkSMProxy::UpdateSelfAndAllInputs()
{
  vtkSMPropertyIterator* iter = this->NewPropertyIterator();

  while (!iter->IsAtEnd())
  {
    iter->GetProperty()->UpdateAllInputs();
    iter->Next();
  }
  iter->Delete();

  this->UpdateVTKObjects();
}

//---------------------------------------------------------------------------
void vtkSMProxy::InitializeAndCopyFromProxy(vtkSMProxy* fromP)
{
  if (this->ObjectsCreated)
  {
    vtkWarningMacro("Cannot Initialize since proxy already created.");
    return;
  }
  if (this->GetSession() != fromP->GetSession())
  {
    vtkErrorMacro("Proxies on different sessions.");
    return;
  }

  fromP->CreateVTKObjects();
  this->SetLocation(fromP->GetLocation());
  this->UpdateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << SIPROXY(this) << "SetVTKObject" << VTKOBJECT(fromP)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//---------------------------------------------------------------------------
void vtkSMProxy::ExecuteStream(
  const vtkClientServerStream& stream, bool ignore_errors /*=false*/, vtkTypeUInt32 location /*=0*/)
{
  if (location == 0)
  {
    location = this->Location;
  }
  if (location == 0 || stream.GetNumberOfMessages() == 0)
  {
    return;
  }

  if (this->GetSession())
  {
    this->GetSession()->ExecuteStream(location, stream, ignore_errors);
  }
  // if no session, nothing to do.
}

//---------------------------------------------------------------------------
const vtkClientServerStream& vtkSMProxy::GetLastResult()
{
  return this->GetLastResult(this->Location);
}

//---------------------------------------------------------------------------
const vtkClientServerStream& vtkSMProxy::GetLastResult(vtkTypeUInt32 location)
{
  return this->Session->GetLastResult(location);
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdatePipelineInformation()
{
  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    it2->second.GetPointer()->UpdatePipelineInformation();
  }

  this->UpdatePropertyInformation();
}

//---------------------------------------------------------------------------
void vtkSMProxy::EnableLocalPushOnly()
{
  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    it2->second.GetPointer()->EnableLocalPushOnly();
  }
  this->Superclass::EnableLocalPushOnly();
}

//---------------------------------------------------------------------------
void vtkSMProxy::DisableLocalPushOnly()
{
  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    it2->second.GetPointer()->DisableLocalPushOnly();
  }
  this->Superclass::DisableLocalPushOnly();
}

//---------------------------------------------------------------------------
vtkClientServerStream& operator<<(vtkClientServerStream& stream, const VTKOBJECT& manipulator)
{
  vtkClientServerStream substream;
  substream << vtkClientServerStream::Invoke
            << vtkClientServerID(1) // ID for the vtkSMSessionCore helper.
            << "GetVTKObject" << manipulator.Reference->GetGlobalID() << vtkClientServerStream::End;
  stream << substream;
  return stream;
}

//---------------------------------------------------------------------------
//                       Annotation management
//---------------------------------------------------------------------------

void vtkSMProxy::SetAnnotation(const char* key, const char* value)
{
  assert("We expect a valid key for proxy annotation." && key);
  if (value)
  {
    this->Internals->Annotations[key] = value;
    this->UpdateAndPushAnnotationState();
  }
  else
  {
    this->RemoveAnnotation(key);
  }
}
//---------------------------------------------------------------------------

const char* vtkSMProxy::GetAnnotation(const char* key)
{
  vtkSMProxyInternals::AnnotationMap::iterator iter = this->Internals->Annotations.find(key);
  if (iter != this->Internals->Annotations.end())
  {
    return iter->second.c_str();
  }
  return NULL;
}
//---------------------------------------------------------------------------
void vtkSMProxy::RemoveAnnotation(const char* key)
{
  this->Internals->Annotations.erase(key);
  this->UpdateAndPushAnnotationState();
}
//---------------------------------------------------------------------------

void vtkSMProxy::RemoveAllAnnotations()
{
  this->Internals->Annotations.clear();
  this->UpdateAndPushAnnotationState();
}
//---------------------------------------------------------------------------

bool vtkSMProxy::HasAnnotation(const char* key)
{
  return (this->Internals->Annotations.find(key) != this->Internals->Annotations.end());
}
//---------------------------------------------------------------------------

int vtkSMProxy::GetNumberOfAnnotations()
{
  return static_cast<int>(this->Internals->Annotations.size());
}
//---------------------------------------------------------------------------

const char* vtkSMProxy::GetAnnotationKeyAt(int index)
{
  vtkSMProxyInternals::AnnotationMap::iterator iter = this->Internals->Annotations.begin();
  int searchIndex = 0;
  while (searchIndex < index && iter != this->Internals->Annotations.end())
  {
    iter++;
    searchIndex++;
  }
  if (searchIndex == index && iter != this->Internals->Annotations.end())
  {
    return iter->first.c_str();
  }
  return NULL;
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateAndPushAnnotationState()
{
  if (!this->ObjectsCreated)
  {
    return;
  }

  // Update state
  vtkSMMessage localAnnotationState;
  localAnnotationState.SetExtension(ProxyState::has_annotation, true);

  this->State->ClearExtension(ProxyState::annotation);
  this->State->SetExtension(ProxyState::has_annotation, true);

  vtkSMProxyInternals::AnnotationMap::iterator iter = this->Internals->Annotations.begin();
  ProxyState_Annotation* annotation = NULL;
  while (iter != this->Internals->Annotations.end())
  {
    // Add in full state
    annotation = this->State->AddExtension(ProxyState::annotation);
    annotation->set_key(iter->first);
    annotation->set_value(iter->second);

    // Add in local tmp state
    annotation = localAnnotationState.AddExtension(ProxyState::annotation);
    annotation->set_key(iter->first);
    annotation->set_value(iter->second);

    // move forward
    iter++;
  }

  // Push annotation state to the session
  if (this->Internals->EnableAnnotationPush)
  {
    this->PushState(&localAnnotationState);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::RecreateVTKObjects()
{
  vtkSMProxyInternals::ProxyMap::iterator it2 = this->Internals->SubProxies.begin();
  for (; it2 != this->Internals->SubProxies.end(); it2++)
  {
    it2->second.GetPointer()->RecreateVTKObjects();
  }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << SIPROXY(this) << "RecreateVTKObjects"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  // Since VTK objects were recreated, we know that pipeline's dirty.
  this->MarkModified(this);

  // Push our full state over to the server. This is akin to loading the
  // state on the newly created VTK object.
  this->PushState(this->State);
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetLogName(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    vtkErrorMacro("Invalid name (nullptr or empty), ignoring.");
  }
  else
  {
    this->SetLogNameInternal(name, true, true);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetLogNameInternal(
  const char* name, bool propagate_to_subproxies, bool propagate_to_proxylistdomains)
{
  assert(name != nullptr && name[0] != '\0');
  delete[] this->LogName;
  this->LogName = vtksys::SystemTools::DuplicateString(name);

  if (propagate_to_subproxies)
  {
    // recursively label subproxies.
    for (const auto& apair : this->Internals->SubProxies)
    {
      if (apair.second != nullptr)
      {
        std::ostringstream str;
        str << this->LogName << "/" << apair.first;
        apair.second->SetLogName(str.str().c_str());
      }
    }
  }

  if (propagate_to_proxylistdomains)
  {
    // recursively label proxy-list domain proxies.
    for (const auto& apair : this->Internals->Properties)
    {
      if (auto plistdomain = apair.second.Property->FindDomain<vtkSMProxyListDomain>())
      {
        std::ostringstream str;
        str << this->LogName << "/" << apair.first;
        plistdomain->SetLogName(str.str().c_str());
      }
    }
  }

  if (this->ObjectsCreated)
  {
    // need to push state to update log name on the si objects.
    vtkSMMessage logname_state;
    logname_state.SetExtension(ProxyState::log_name, (this->LogName ? this->LogName : ""));

    // update local `full state`.
    this->State->SetExtension(ProxyState::log_name, (this->LogName ? this->LogName : ""));

    this->PushState(&logname_state);
  }
}
