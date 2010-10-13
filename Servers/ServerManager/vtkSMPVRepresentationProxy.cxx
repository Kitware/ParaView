/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"

#include <vtkstd/string>
#include <vtkstd/map>
#include <vtkstd/set>

class vtkSMPVRepresentationProxy::vtkInternals
{
public:
  struct vtkValue
    {
    vtkSMRepresentationProxy* Representation;
    int Value;
    vtkstd::string Text;
    vtkValue(vtkSMRepresentationProxy* repr= 0, int value=-1, const char* text="")
      {
      this->Representation = repr;
      this->Value = value;
      this->Text = text? text : "";
      }
    };

  typedef vtkstd::map<int, vtkValue> RepresentationProxiesMap;
  RepresentationProxiesMap RepresentationProxies;

  // This is in some sense unnecessary, since the RepresentationProxies map
  // keeps this information, however, this makes it easy to iterate over unique
  // proxies.
  typedef vtkstd::set<vtkSMRepresentationProxy*> RepresentationProxiesSet;
  RepresentationProxiesSet UniqueRepresentationProxies;


  vtkstd::map<vtkstd::string, int> TraditionalValues;
  vtkInternals()
    {
    this->TraditionalValues["Points"] = 0;
    this->TraditionalValues["Wireframe"] = 1;
    this->TraditionalValues["Surface"] = 2;
    this->TraditionalValues["Outline"] = 3;
    this->TraditionalValues["Volume"] = 4;
    this->TraditionalValues["Surface With Edges"] = 5;
    this->TraditionalValues["Slice"] = 6;
    }
};

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->Representation = -1;//vtkSMPVRepresentationProxy::SURFACE;
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->GetID() << "SetCubeAxesRepresentation"
    << this->GetSubProxy("CubeAxesRepresentation")->GetID()
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
    << this->GetID() << "SetSelectionRepresentation"
    << this->GetSubProxy("SelectionRepresentation")->GetID()
    << vtkClientServerStream::End;

  vtkInternals::RepresentationProxiesMap::iterator iter =
    this->Internals->RepresentationProxies.begin();
  for (; iter != this->Internals->RepresentationProxies.end(); ++iter)
    {
    stream << vtkClientServerStream::Invoke
      << this->GetID() << "AddRepresentation"
      << iter->second.Text.c_str()
      << iter->second.Representation->GetID()
      << vtkClientServerStream::End;
    }
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->Servers, stream);

//  if (surfaceRepr)
//    {
//    this->LinkSelectionProp(surfaceRepr->GetProp3D());
//    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetRepresentation(int repr)
{
  if (this->Representation != repr)
    {
    vtkInternals::RepresentationProxiesMap::iterator iter =
      this->Internals->RepresentationProxies.find(repr);
    if (iter == this->Internals->RepresentationProxies.end())
      {
      vtkErrorMacro("Representation type " << repr << " not supported.");
      return;
      }

    this->Representation = repr;

    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetActiveRepresentation"
      << iter->second.Text.c_str()
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, this->Servers, stream);

    vtkSMProxy* subProxy = iter->second.Representation;
    if (subProxy && iter->second.Value != -1)
      {
      vtkSMPropertyHelper(subProxy, "Representation").Set(iter->second.Value);
      subProxy->UpdateVTKObjects();
      }
    this->Modified();
    }
  this->InvalidateDataInformation();
}

//----------------------------------------------------------------------------
int vtkSMPVRepresentationProxy::CreateSubProxiesAndProperties(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::CreateSubProxiesAndProperties(pm, element))
    {
    return 0;
    }

  // Locate the representation-type domain.
  vtkSMProperty* repProp = this->GetProperty("Representation");
  vtkSMEnumerationDomain* enumDomain = repProp?
    vtkSMEnumerationDomain::SafeDownCast(repProp->GetDomain("enum")): 0;

  // <RepresentationType subproxy="OutlineRepresentation" text="Outline"
  //    subtype="0"/>
  unsigned int numElements = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElements; cc++)
    {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(),
        "RepresentationType") == 0)
      {
      const char* name = child->GetAttribute("subproxy");
      vtkSMRepresentationProxy* subproxy =
        vtkSMRepresentationProxy::SafeDownCast(this->GetSubProxy(name));
      if (!subproxy)
        {
        vtkErrorMacro("Missing data representation subproxy '"
          << (name? name : "<null>")
          << "' when processing RepresentationType element.");
        return 0;
        }
      const char* text = child->GetAttribute("text");
      if (!text)
        {
        vtkErrorMacro(
          "Missing required 'text' attribute on RepresentationType element");
        return 0;
        }

      // read optional subtype.
      int subtype = -1;
      child->GetScalarAttribute("subtype", &subtype);

      // this ensures that when new representations are added from plugins
      // everyone gets a unique id.
      int representation = USER_DEFINED +
        this->Internals->RepresentationProxies.size();

      // Handle traditional values explicitly; this makes backwards
      // compatibility easier.
      if (this->Internals->TraditionalValues.find(text) !=
        this->Internals->TraditionalValues.end())
        {
        representation = this->Internals->TraditionalValues[text];
        }
      if (enumDomain)
        {
        enumDomain->AddEntry(text, representation);
        }
      this->Internals->RepresentationProxies[representation] =
        vtkInternals::vtkValue(subproxy, subtype, text);
      this->Internals->UniqueRepresentationProxies.insert(subproxy);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSMPVRepresentationProxy::LoadState(
  vtkPVXMLElement* element, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::LoadState(element, locator))
    {
    return 0;
    }

  // Representation types can be added through plugins. Hence the available
  // representation types and the (text==value) mapping may be different at time
  // of loading the state than at the time of saving the state. So we parse the
  // state to obtain the text for the state-selected representation type and set
  // the property to the value that corresponds to the state-indicated
  // representation type (using the text as the invariant).

  vtkSMIntVectorProperty* repProp = vtkSMIntVectorProperty::SafeDownCast(
   this->GetProperty("Representation"));
  vtkSMEnumerationDomain* enumDomain = repProp?
    vtkSMEnumerationDomain::SafeDownCast(repProp->GetDomain("enum")): 0;
  if (!enumDomain)
    {
    return 1;
    }
  unsigned int numElements = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElements; cc++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(cc);
    const char* name =  currentElement->GetName();
    if (name && strcmp(name, "Property") == 0 &&
      currentElement->GetAttribute("name") &&
      strcmp(currentElement->GetAttribute("name"), "Representation") == 0 &&
      currentElement->FindNestedElementByName("Domain"))
      {
      // Using the domain in the state file, determine what was the
      // representation type the user wanted and then set it on the property.
      const char* repr_text = 0;

      vtkSmartPointer<vtkCollection> entries = vtkSmartPointer<vtkCollection>::New();
      currentElement->FindNestedElementByName("Domain")->GetElementsByName(
        "Entry", entries);
      for (int kk=0; kk < entries->GetNumberOfItems() && !repr_text; kk++)
        {
        vtkPVXMLElement* entry = vtkPVXMLElement::SafeDownCast(
          entries->GetItemAsObject(kk));
        int value;
        if (entry->GetScalarAttribute("value", &value) &&
          value == repProp->GetElement(0))
          {
          repr_text = entry->GetAttribute("text");
          }
        }
      if (repr_text)
        {
        if (!enumDomain->HasEntryText(repr_text))
          {
          vtkWarningMacro("Cannot restore representation type to \'"
            << repr_text << "\' since possibly some plugins are missing.");
          return 1;
          }

        repProp->SetElement(0, enumDomain->GetEntryValueForText(repr_text));
        }
      break;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::AddInput(unsigned int inputPort,
  vtkSMSourceProxy* input, unsigned int outputPort, const char* method)
{
  if (inputPort == 0)
    {
    input->CreateSelectionProxies();

    vtkSMSourceProxy* esProxy = input->GetSelectionOutput(outputPort);
    if (!esProxy)
      {
      vtkErrorMacro("Input proxy does not support selection extraction.");
      return;
      }

    vtkSMPropertyHelper(
      this->GetSubProxy("SelectionRepresentation"),"Input").Set(esProxy);
    this->GetSubProxy("SelectionRepresentation")->UpdateVTKObjects();

    this->Superclass::AddInput(1, esProxy, 0, "SetInputConnection");

    // Next we ensure that input is set on all input properties for the
    // subproxies. This ensures that domains and such on subproxies can still
    // function correctly.

    // This piece of code is a disgrace :). We shouldn't have to do these kinds of
    // hacks. It implies that something's awry with the way domains and
    // updated/defined for sub-proxies.
    vtkInternals::RepresentationProxiesSet::iterator iter;
    for (iter = this->Internals->UniqueRepresentationProxies.begin();
      iter != this->Internals->UniqueRepresentationProxies.end(); ++iter)
      {
      if ((*iter)->GetProperty("Input"))
        {
        vtkSMPropertyHelper((*iter), "Input").Set(input, outputPort);
        (*iter)->UpdateProperty("Input");
        (*iter)->GetProperty("Input")->UpdateDependentDomains();
        }
      }
    }

  this->Superclass::AddInput(inputPort, input, outputPort, method);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
#define PRINT_REP_CASE(type) case type: os << #type << endl; break;
  os << indent << "Representation: " ;
  switch (this->Representation)
    {
    PRINT_REP_CASE(SURFACE);
    PRINT_REP_CASE(WIREFRAME);
    PRINT_REP_CASE(POINTS);
    PRINT_REP_CASE(OUTLINE);
    PRINT_REP_CASE(VOLUME);
    PRINT_REP_CASE(SURFACE_WITH_EDGES);
    PRINT_REP_CASE(SLICE);
    PRINT_REP_CASE(USER_DEFINED);
    default:
      os << "(unknown)" << endl;;
    }
}


