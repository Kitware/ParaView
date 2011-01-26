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
#include "vtkPMProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkPMProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionCore.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/string>
#include <vtksys/ios/sstream>

class vtkPMProxy::vtkInternals
{
public:
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPMProperty> >
    PropertyHelpersMapType;
  PropertyHelpersMapType PropertyHelpers;

  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPMProxy> >
    SubProxyHelpersMapType;
  SubProxyHelpersMapType SubProxyHelpers;
};

vtkStandardNewMacro(vtkPMProxy);
//----------------------------------------------------------------------------
vtkPMProxy::vtkPMProxy()
{
  this->Internals = new vtkInternals();
  this->VTKObject = NULL;
  this->ObjectsCreated = false;

  this->XMLGroup = 0;
  this->XMLName = 0;
  this->VTKClassName = 0;
  this->PostPush = 0;
  this->PostCreation = 0;
}

//----------------------------------------------------------------------------
vtkPMProxy::~vtkPMProxy()
{
  this->DeleteVTKObjects();

  delete this->Internals;
  this->Internals = 0;

  this->SetXMLGroup(0);
  this->SetXMLName(0);
  this->SetVTKClassName(0);
  this->SetPostPush(0);
  this->SetPostCreation(0);
}

//----------------------------------------------------------------------------
void vtkPMProxy::Push(vtkSMMessage* message)
{
  if (!this->CreateVTKObjects(message))
    {
    return;
    }

  for (int cc=0; cc<message->ExtensionSize(ProxyState::property); cc++)
    {
    const ProxyState_Property &propMsg =
      message->GetExtension(ProxyState::property, cc);

    // Convert state to interpretor stream
    vtkPMProperty* prop = this->GetPropertyHelper(propMsg.name().c_str());
    if (prop)
      {
      if (prop->Push(message, cc) == false)
        {
        vtkErrorMacro("Error pushing property state: " << propMsg.name());
        message->PrintDebugString();
        return;
        }
      }
    }

  // Execute post_push if any
  if(this->PostPush != NULL)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << this->PostPush
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }
}

//----------------------------------------------------------------------------
void vtkPMProxy::Invoke(vtkSMMessage* message)
{
  if (!this->CreateVTKObjects(message))
    {
    return;
    }

  vtkstd::string command;
  const VariantList* arguments;
  command = message->GetExtension(InvokeRequest::method);
  arguments = &message->GetExtension(InvokeRequest::arguments);
  bool disableError = message->GetExtension(InvokeRequest::no_error);
  vtkPMProxy* proxy = NULL;

  // Manage warning message
  int previousWarningValue = this->Interpreter->GetGlobalWarningDisplay();
  this->Interpreter->SetGlobalWarningDisplay( disableError ? 0 : 1);

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << this->GetVTKObject()
         << command.c_str();
  for (int cc=0; cc < arguments->variant_size(); cc++)
    {
    const Variant *v = &arguments->variant(cc);
    switch (v->type())
      {
      case Variant::PROXY:
        proxy =
            vtkPMProxy::SafeDownCast(
                this->SessionCore->GetPMObject(v->proxy_global_id(0)));
        if(proxy == NULL)
          {
          vtkErrorMacro("Did not find a PMProxy with id " << v->proxy_global_id(0));
          }
        else
          {
          stream << proxy->GetVTKObject();
          }
        break;
      default:
        stream << *v;
        break;
      }
    }
  stream << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);

  // Manage warning message (put back the previous value)
  this->Interpreter->SetGlobalWarningDisplay( previousWarningValue );

  // send back the result
  (*message) << pvstream::InvokeResponse()
             << this->Interpreter->GetLastResult();
}

//----------------------------------------------------------------------------
void vtkPMProxy::Pull(vtkSMMessage* message)
{
  if (!this->ObjectsCreated)
    {
    return;
    }

  // Return a set of Pull only property (information_only props)
  // In fact Pushed Property can not be fetch at the same time as Pull
  // property with the current implementation
  vtkSMMessage response = *message;
  response.ClearExtension(PullRequest::arguments);

  vtkstd::set<vtkstd::string> prop_names;
  if (message->ExtensionSize(PullRequest::arguments) > 0)
    {
    const Variant *propList = &message->GetExtension(PullRequest::arguments, 0);
    for(int i=0; i < propList->txt_size(); ++i)
      {
      const char* propertyName = propList->txt(i).c_str();
      prop_names.insert(propertyName);
      }
    }

  vtkInternals::PropertyHelpersMapType::iterator iter;
  for (iter = this->Internals->PropertyHelpers.begin(); iter !=
    this->Internals->PropertyHelpers.end(); ++iter)
    {
    if (prop_names.size() == 0 ||
      prop_names.find(iter->first) != prop_names.end())
      {
      if (!iter->second->GetIsInternal() && !iter->second->Pull(&response))
        {
        vtkErrorMacro("Error pulling property state: " << iter->first);
        return;
        }
      }
    }

  message->CopyFrom(response);
}

//----------------------------------------------------------------------------
vtkSMProxyDefinitionManager* vtkPMProxy::GetProxyDefinitionManager()
{
  if (this->SessionCore)
    {
    return this->SessionCore->GetProxyDefinitionManager();
    }

  vtkWarningMacro("No valid session provided. "
    "This class may not have been initialized yet.");
  return NULL;
}

//----------------------------------------------------------------------------
vtkPMProperty* vtkPMProxy::GetPropertyHelper(const char* name)
{
  vtkInternals::PropertyHelpersMapType::iterator iter =
    this->Internals->PropertyHelpers.find(name);
  if (iter != this->Internals->PropertyHelpers.end())
    {
    return iter->second.GetPointer();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPMProxy::AddPropertyHelper(const char* name, vtkPMProperty* property)
{
  this->Internals->PropertyHelpers[name] = property;
}

//----------------------------------------------------------------------------
bool vtkPMProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }

  if (!message->HasExtension(ProxyState::xml_group) ||
    !message->HasExtension(ProxyState::xml_name))
    {
    vtkErrorMacro("Incorrect message received. "
                  << "Missing xml_group and xml_name information." << endl
                  << message->DebugString().c_str() << endl);
    return false;
    }

  vtkSMProxyDefinitionManager* pdm = this->GetProxyDefinitionManager();
  vtkPVXMLElement* element = pdm->GetCollapsedProxyDefinition(
    message->GetExtension(ProxyState::xml_group).c_str(),
    message->GetExtension(ProxyState::xml_name).c_str(),
    (message->HasExtension(ProxyState::xml_sub_proxy_name) ?
     message->GetExtension(ProxyState::xml_sub_proxy_name).c_str() :
     NULL));
  if (!element)
    {
    vtkErrorMacro("Definition not found for xml_group: "
                  << message->GetExtension(ProxyState::xml_group).c_str()
                  << " and xml_name: "
                  << message->GetExtension(ProxyState::xml_name).c_str()
                  << endl << message->DebugString().c_str() << endl );
    return false;
    }

  // Create and setup the VTK object, if needed before parsing the property
  // helpers. This is needed so that the property helpers can push their default
  // values as they are reading the xml-attributes.
  const char* className = element->GetAttribute("class");
  if (className && className[0])
    {
    this->SetVTKClassName(className);
    this->VTKObject.TakeReference(vtkInstantiator::CreateInstance(className));
    }

#ifdef FIXME_COLLABORATION
  // ensure that this is happening correctly in PMProxy
  if (this->VTKClassName && this->VTKClassName[0] != '\0')
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    this->VTKObjectID =
      pm->NewStreamObject(this->VTKClassName, stream);
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID()
           << "RegisterProgressEvent"
           << this->VTKObjectID
           << static_cast<int>(this->VTKObjectID.ID)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
#endif

  this->SetXMLGroup(message->GetExtension(ProxyState::xml_group).c_str());
  this->SetXMLName(message->GetExtension(ProxyState::xml_name).c_str());

  // Locate sub-proxies.
  for (int cc=0; cc < message->ExtensionSize(ProxyState::subproxy); cc++)
    {
    const ProxyState_SubProxy& subproxyMsg =
      message->GetExtension(ProxyState::subproxy, cc);
    vtkPMProxy* subproxy = vtkPMProxy::SafeDownCast(
      this->GetPMObject(subproxyMsg.global_id()));
    if (subproxy == NULL)
      {
      // This code has been commented to support ImplicitPlaneWidgetRepresentation
      // which as a widget as SubProxy which stay on the client side.
      // Therefore, when ParaView is running on Client/Server mode, that SubProxy
      // does NOT exist on the Server side. This case should not fail the current
      // proxy creation.

      // vtkErrorMacro("Failed to locate subproxy with global-id: "
      //                << subproxyMsg.global_id() << endl
      //                << message->DebugString().c_str());
      // return false;
      }
    else
      {
      this->Internals->SubProxyHelpers[subproxyMsg.name()] = subproxy;
      }
    }

  // Allow subclasses to do some initialization if needed. Note this is called
  // before properties are created.
  this->OnCreateVTKObjects();

  // Process the XML and update properties etc.
  if (!this->ReadXMLAttributes(element))
    {
    this->DeleteVTKObjects();
    return false;
    }

  this->ObjectsCreated = true;

  // Execute post-creation if any
  if(this->PostCreation != NULL)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << this->PostCreation
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkPMProxy::DeleteVTKObjects()
{
  this->VTKObject =  NULL;
}

//----------------------------------------------------------------------------
void vtkPMProxy::OnCreateVTKObjects()
{
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkPMProxy::GetVTKObject()
{
  return this->VTKObject.GetPointer();
}

//----------------------------------------------------------------------------
bool vtkPMProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  // Add hook for post_push and post_creation
  this->SetPostPush(element->GetAttribute("post_push"));
  this->SetPostCreation(element->GetAttribute("post_creation"));

  for(unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* propElement = element->GetNestedElement(i);

    if (strcmp(propElement->GetName(), "SubProxy") == 0)
      {
      // read subproxy xml.
      if (!this->ReadXMLSubProxy(propElement))
        {
        return false;
        }
      }
    else
      {
      // read property xml
      const char* name = propElement->GetAttribute("name");
      vtkstd::string tagName = propElement->GetName();
      if (name && tagName.find("Property") == (tagName.size()-8))
        {
        if (!this->ReadXMLProperty(propElement))
          {
          return false;
          }
        }
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPMProxy::ReadXMLSubProxy(vtkPVXMLElement* )
{
  // vtkErrorMacro("Not supported yet.");
  return true;
}
//----------------------------------------------------------------------------
bool vtkPMProxy::ReadXMLProperty(vtkPVXMLElement* propElement)
{
  // Since the XML is "cleaned" out, we are assured that there are no duplicate
  // properties.
  const char* name = propElement->GetAttribute("name");
  assert(name && this->GetPropertyHelper(name) == NULL);

  // Patch XML to remove InformationHelper and set right kernel_class
  vtkSMProxyDefinitionManager::PatchXMLProperty(propElement);

  vtkSmartPointer<vtkObject> object;
  vtkstd::string classname;
  if (propElement->GetAttribute("kernel_class"))
    {
    classname = propElement->GetAttribute("kernel_class");
    }
  else
    {
    vtksys_ios::ostringstream cname;
    cname << "vtkPM" << propElement->GetName() << ends;
    classname = cname.str();
    }

  object.TakeReference(vtkInstantiator::CreateInstance(classname.c_str()));
  if (!object)
    {
    vtkErrorMacro("Failed to create helper for property: " << classname);
    return false;
    }
  vtkPMProperty* property = vtkPMProperty::SafeDownCast(object);
  if (!property)
    {
    vtkErrorMacro(<< classname << " must be a vtkPMProperty subclass.");
    return false;
    }

  if (!property->ReadXMLAttributes(this, propElement))
    {
    vtkErrorMacro("Could not parse property: " << name);
    return false;
    }

  this->AddPropertyHelper(name, property);
  return true;
}

//----------------------------------------------------------------------------
vtkPMProxy* vtkPMProxy::GetSubProxyHelper(const char* name)
{
  return this->Internals->SubProxyHelpers[name];
}

//----------------------------------------------------------------------------
unsigned int vtkPMProxy::GetNumberOfSubProxyHelpers()
{
  return static_cast<unsigned int>(this->Internals->SubProxyHelpers.size());
}
//----------------------------------------------------------------------------
vtkPMProxy* vtkPMProxy::GetSubProxyHelper(unsigned int cc)
{
  if (cc >= this->GetNumberOfSubProxyHelpers())
    {
    return NULL;
    }

  unsigned int index=0;
  vtkInternals::SubProxyHelpersMapType::iterator iter;
  for (iter = this->Internals->SubProxyHelpers.begin();
    iter != this->Internals->SubProxyHelpers.end();
    ++iter, ++index)
    {
    if (index == cc)
      {
      return iter->second;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPMProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
