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

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSurfaceRepresentationProxy.h"

#include <vtkstd/map>
#include <vtkstd/set>

inline void vtkSMPVRepresentationProxySetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    proxy->UpdateProperty(pname);
    }
}

class vtkSMPVRepresentationProxy::vtkInternals
{
public:
  struct vtkValue
    {
    vtkSMDataRepresentationProxy* Representation;
    int Value;
    vtkstd::string Text;
    vtkValue(vtkSMDataRepresentationProxy* repr= 0, int value=-1, const char* text="")
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
  typedef vtkstd::set<vtkSMDataRepresentationProxy*> RepresentationProxiesSet;
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
  this->Representation = vtkSMPVRepresentationProxy::SURFACE;
  this->BackfaceRepresentation = vtkSMPVRepresentationProxy::FOLLOW_FRONTFACE;
  this->CubeAxesRepresentation = 0;
  this->CubeAxesVisibility = 0;
  this->ActiveRepresentation = 0;

  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetViewInformation(vtkInformation* info)
{
  this->Superclass::SetViewInformation(info);

  vtkInternals::RepresentationProxiesSet::iterator iter;
  for (iter = this->Internals->UniqueRepresentationProxies.begin();
    iter != this->Internals->UniqueRepresentationProxies.end(); ++iter)
    {
    (*iter)->SetViewInformation(info);
    }

  if (this->BackfaceSurfaceRepresentation)
    {
    this->BackfaceSurfaceRepresentation->SetViewInformation(info);
    }

  if (this->CubeAxesRepresentation)
    {
    this->CubeAxesRepresentation->SetViewInformation(info);
    }
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::EndCreateVTKObjects()
{
  vtkSMProxy* inputProxy = this->GetInputProxy();
  this->CubeAxesRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("CubeAxesRepresentation"));
  if (this->CubeAxesRepresentation)
    {
    this->Connect(inputProxy, this->CubeAxesRepresentation,
      "Input", this->OutputPort);
    vtkSMPVRepresentationProxySetInt(this->CubeAxesRepresentation, "Visibility", 0);
    }

  vtkCommand* observer = this->GetObserver();

  this->BackfaceSurfaceRepresentation
    = vtkSMDataRepresentationProxy::SafeDownCast(
                            this->GetSubProxy("BackfaceSurfaceRepresentation"));
  if (this->BackfaceSurfaceRepresentation)
    {
    this->Connect(inputProxy, this->BackfaceSurfaceRepresentation,
                  "Input", this->OutputPort);
    vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                     "Visibility", 0);
    vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                     "FrontfaceCulling", 1);

    this->BackfaceSurfaceRepresentation->AddObserver(vtkCommand::StartEvent,
                                                     observer);
    this->BackfaceSurfaceRepresentation->AddObserver(vtkCommand::EndEvent,
                                                     observer);
    }

  vtkSMSurfaceRepresentationProxy* surfaceRepr = 
    vtkSMSurfaceRepresentationProxy::SafeDownCast(
      this->GetSubProxy("SurfaceRepresentation"));

  vtkInternals::RepresentationProxiesSet::iterator iter;
  for (iter = this->Internals->UniqueRepresentationProxies.begin();
    iter != this->Internals->UniqueRepresentationProxies.end(); ++iter)
    {
    vtkSMDataRepresentationProxy* repr = (*iter);
    this->Connect(inputProxy, repr, "Input", this->OutputPort);
    vtkSMPVRepresentationProxySetInt(repr, "Visibility", 0);

    // Fire start/end events fired by the representations so that the world knows
    // that the representation has been updated,
    // FIXME: need to revisit this following the data information update
    // changes.
    repr->AddObserver(vtkCommand::StartEvent, observer);
    repr->AddObserver(vtkCommand::EndEvent, observer);

    if (!surfaceRepr)
      {
      surfaceRepr = vtkSMSurfaceRepresentationProxy::SafeDownCast(repr);
      }
    }

  // Setup the ActiveRepresentation pointer.
  int repr = this->Representation;
  this->Representation = -1;
  this->SetRepresentation(repr);

  if (surfaceRepr)
    {
    this->LinkSelectionProp(surfaceRepr->GetProp3D());
    }

  // This will pass the ViewInformation to all the representations.
  this->SetViewInformation(this->ViewInformation);

  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  if (this->CubeAxesRepresentation)
    {
    this->CubeAxesRepresentation->AddToView(view);
    }

  if (this->BackfaceSurfaceRepresentation)
    {
    this->BackfaceSurfaceRepresentation->AddToView(view);
    }

  vtkInternals::RepresentationProxiesSet::iterator iter;
  for (iter = this->Internals->UniqueRepresentationProxies.begin();
    iter != this->Internals->UniqueRepresentationProxies.end(); ++iter)
    {
    (*iter)->AddToView(view);
    }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkInternals::RepresentationProxiesSet::iterator iter;
  for (iter = this->Internals->UniqueRepresentationProxies.begin();
    iter != this->Internals->UniqueRepresentationProxies.end(); ++iter)
    {
    (*iter)->RemoveFromView(view);
    }

  if (this->BackfaceSurfaceRepresentation)
    {
    this->BackfaceSurfaceRepresentation->RemoveFromView(view);
    }

  if (this->CubeAxesRepresentation)
    {    
    this->CubeAxesRepresentation->RemoveFromView(view);
    }
  return this->Superclass::RemoveFromView(view);
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
    if (this->ActiveRepresentation)
      {
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 0);
      }

    this->ActiveRepresentation = iter->second.Representation;
    if (this->ActiveRepresentation->GetProperty("Representation") &&
      iter->second.Value != -1)
      {
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, 
        "Representation", iter->second.Value);
      }

    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 
      this->GetVisibility());

    // Make sure the backface visibility and all culling, which also depends on
    // the front face representation, is set correctly.
    this->SetBackfaceRepresentation(this->BackfaceRepresentation);

    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetBackfaceRepresentation(int repr)
{
  if (this->BackfaceRepresentation != repr)
    {
    this->BackfaceRepresentation = repr;
    this->Modified();
    }

  if (!this->BackfaceSurfaceRepresentation)
    {
    return;
    }

  if (!this->ActiveRepresentationIsSurface())
    {
    // Not rendering surfaces.
    vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                     "Visibility", 0);
    }
  else if (this->BackfaceRepresentation == FOLLOW_FRONTFACE)
    {
    // Let the "frontface" surface also be the backface.
    vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                     "Visibility", 0);
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation,
                                     "BackfaceCulling", 0);
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation,
                                     "FrontfaceCulling", 0);
    }
  else if (this->BackfaceRepresentation == CULL_BACKFACE)
    {
    // Turn off the back face.
    vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                     "Visibility", 0);
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation,
                                     "BackfaceCulling", 1);
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation,
                                     "FrontfaceCulling", 0);
    }
  else if (this->BackfaceRepresentation == CULL_FRONTFACE)
    {
    // Just let SurfaceRepresentation be the backface
    vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                     "Visibility", 0);
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation,
                                     "BackfaceCulling", 0);
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation,
                                     "FrontfaceCulling", 1);
    }
  else
    {
    // Both surfaces will be rendered.
    vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                     "Visibility", this->GetVisibility());
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation,
                                     "BackfaceCulling", 1);
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation,
                                     "FrontfaceCulling", 0);
    switch (this->BackfaceRepresentation)
      {
      case POINTS:
        vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                         "Representation", VTK_POINTS);
        break;
      case WIREFRAME:
        vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                         "Representation", VTK_WIREFRAME);
        break;
      case SURFACE_WITH_EDGES:
        vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                         "Representation",
                                         VTK_SURFACE_WITH_EDGES);
        break;
      case SURFACE:
      default:
        vtkSMPVRepresentationProxySetInt(this->BackfaceSurfaceRepresentation,
                                         "Representation", VTK_SURFACE);
        break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetVisibility(int visible)
{
  if (this->ActiveRepresentation)
    {
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 
      visible);
    }
  if (this->CubeAxesRepresentation)
    {    
    vtkSMPVRepresentationProxySetInt(this->CubeAxesRepresentation, "Visibility",
      visible && this->CubeAxesVisibility);
    this->CubeAxesRepresentation->UpdateVTKObjects();
    }
  // Visibility is a bit more complicated for back face.
  this->SetBackfaceRepresentation(this->BackfaceRepresentation);
     
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetCubeAxesVisibility(int visible)
{
  if (this->CubeAxesRepresentation)
    {    
    this->CubeAxesVisibility = visible;
    vtkSMPVRepresentationProxySetInt(this->CubeAxesRepresentation, "Visibility",
      visible && this->GetVisibility());
    this->CubeAxesRepresentation->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->Update(view);
    }
  if (   this->BackfaceSurfaceRepresentation
      && this->ActiveRepresentationIsSurface() )
    {
    this->BackfaceSurfaceRepresentation->Update(view);
    }
  if (this->CubeAxesRepresentation)
    {    
    this->CubeAxesRepresentation->Update(view);
    }

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* 
vtkSMPVRepresentationProxy::GetRepresentedDataInformation(bool update)
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetRepresentedDataInformation(update);
    }

  return this->Superclass::GetRepresentedDataInformation(update);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::UpdateRequired()
{
  if (this->ActiveRepresentation)
    {
    if (this->ActiveRepresentation->UpdateRequired())
      {
      return true;
      }
    }

  if (this->CubeAxesRepresentation->UpdateRequired())
    {
    return true;
    }

  return this->Superclass::UpdateRequired();
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetUpdateTime(double time)
{
  this->Superclass::SetUpdateTime(time);

  vtkInternals::RepresentationProxiesSet::iterator iter;
  for (iter = this->Internals->UniqueRepresentationProxies.begin();
    iter != this->Internals->UniqueRepresentationProxies.end(); ++iter)
    {
    (*iter)->SetUpdateTime(time);
    }

  if (this->BackfaceSurfaceRepresentation)
    {
    this->BackfaceSurfaceRepresentation->SetUpdateTime(time);
    }

  if (this->CubeAxesRepresentation)
    {    
    this->CubeAxesRepresentation->SetUpdateTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetUseViewUpdateTime(bool use)
{
  this->Superclass::SetUseViewUpdateTime(use);

  vtkInternals::RepresentationProxiesSet::iterator iter;
  for (iter = this->Internals->UniqueRepresentationProxies.begin();
    iter != this->Internals->UniqueRepresentationProxies.end(); ++iter)
    {
    (*iter)->SetUseViewUpdateTime(use);
    }

  if (this->BackfaceSurfaceRepresentation)
    {
    this->BackfaceSurfaceRepresentation->SetUseViewUpdateTime(use);
    }

  if (this->CubeAxesRepresentation)
    {    
    this->CubeAxesRepresentation->SetUseViewUpdateTime(use);
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);

  vtkInternals::RepresentationProxiesSet::iterator iter;
  for (iter = this->Internals->UniqueRepresentationProxies.begin();
    iter != this->Internals->UniqueRepresentationProxies.end(); ++iter)
    {
    (*iter)->SetViewUpdateTime(time);
    }

  if (this->BackfaceSurfaceRepresentation)
    {
    this->BackfaceSurfaceRepresentation->SetViewUpdateTime(time);
    }

  if (this->CubeAxesRepresentation)
    {    
    this->CubeAxesRepresentation->SetViewUpdateTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy != this && this->ActiveRepresentation)
    {
    this->ActiveRepresentation->MarkDirty(modifiedProxy);
    }

  if (   (modifiedProxy != this) && this->BackfaceSurfaceRepresentation
      && this->ActiveRepresentationIsSurface() )
    {
    this->BackfaceSurfaceRepresentation->MarkDirty(modifiedProxy);
    }

  this->Superclass::MarkDirty(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::GetActiveStrategies(
  vtkSMRepresentationStrategyVector& activeStrategies)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->GetActiveStrategies(activeStrategies);
    }

  this->Superclass::GetActiveStrategies(activeStrategies);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::ConvertSelection(vtkSelection* input)
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->ConvertSelection(input);
    }

  return this->Superclass::ConvertSelection(input);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetOrderedCompositingNeeded()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetOrderedCompositingNeeded();
    }

  return this->Superclass::GetOrderedCompositingNeeded();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::GetProcessedConsumer()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetProcessedConsumer();
    }

  return this->Superclass::GetProcessedConsumer();
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::HasVisibleProp3D(vtkProp3D* prop)
{
  if(!prop)
  {
    return false;
  }

  if(this->Superclass::HasVisibleProp3D(prop))
  {
    return true;
  }

  if (this->GetVisibility() && this->ActiveRepresentation)
    {
    vtkSMPropRepresentationProxy* repr = 
      vtkSMPropRepresentationProxy::SafeDownCast(this->ActiveRepresentation);
    if (repr && repr->HasVisibleProp3D(prop))
      {
      return true;
      }
    }

  if (   this->GetVisibility() && this->BackfaceSurfaceRepresentation
      && this->ActiveRepresentationIsSurface() )
    {
    vtkSMPropRepresentationProxy* repr
      = vtkSMPropRepresentationProxy::SafeDownCast(
                                           this->BackfaceSurfaceRepresentation);
    if (repr && repr->HasVisibleProp3D(prop)) return true;
    }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetBounds(double bounds[6])
{
  return this->ActiveRepresentation->GetBounds(bounds);
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
      vtkSMDataRepresentationProxy* subproxy = 
        vtkSMDataRepresentationProxy::SafeDownCast(this->GetSubProxy(name));
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

//-----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::ActiveRepresentationIsSurface()
{
  vtkSMSurfaceRepresentationProxy *activeSurfaceRep
    = vtkSMSurfaceRepresentationProxy::SafeDownCast(this->ActiveRepresentation);
  return (activeSurfaceRep != NULL);
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
  os << indent << "BackfaceRepresentation: " ;
  switch (this->BackfaceRepresentation)
    {
    PRINT_REP_CASE(SURFACE);
    PRINT_REP_CASE(WIREFRAME);
    PRINT_REP_CASE(POINTS);
    PRINT_REP_CASE(OUTLINE);
    PRINT_REP_CASE(VOLUME);
    PRINT_REP_CASE(SURFACE_WITH_EDGES);
    PRINT_REP_CASE(SLICE);
    PRINT_REP_CASE(USER_DEFINED);
    PRINT_REP_CASE(FOLLOW_FRONTFACE);
    PRINT_REP_CASE(CULL_BACKFACE);
    PRINT_REP_CASE(CULL_FRONTFACE);
    default:
      os << "(unknown)" << endl;;
    }
  os << indent << "CubeAxesVisibility: " << this->CubeAxesVisibility << endl;
}


