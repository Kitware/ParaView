/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyGroup.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyGroup.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDocumentation.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h"

#include <map>
#include <string>
#include <vector>

class vtkSMPropertyGroupInternals
{
public:
  std::vector<vtkWeakPointer<vtkSMProperty> > Properties;
  typedef std::map<std::string, vtkWeakPointer<vtkSMProperty> > PropertiesMapType;
  PropertiesMapType PropertiesMap;
};

vtkStandardNewMacro(vtkSMPropertyGroup);
vtkCxxSetObjectMacro(vtkSMPropertyGroup, Hints, vtkPVXMLElement);

//---------------------------------------------------------------------------
vtkSMPropertyGroup::vtkSMPropertyGroup()
  : Internals(new vtkSMPropertyGroupInternals)
{
  this->Name = 0;
  this->XMLLabel = 0;
  this->PanelWidget = 0;
  this->PanelVisibility = 0;

  // by default, properties are set to always shown
  this->SetPanelVisibility("default");

  this->Documentation = vtkSMDocumentation::New();
  this->Hints = 0;
}

//---------------------------------------------------------------------------
vtkSMPropertyGroup::~vtkSMPropertyGroup()
{
  this->SetXMLLabel(0);
  this->SetName(0);
  this->SetPanelWidget(0);
  this->SetPanelVisibility(0);
  delete this->Internals;
  this->Documentation->Delete();
  this->Documentation = NULL;
  this->SetHints(0);
}

//---------------------------------------------------------------------------
void vtkSMPropertyGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
bool vtkSMPropertyGroup::IsEmpty() const
{
  return this->Internals->Properties.empty();
}

//---------------------------------------------------------------------------
void vtkSMPropertyGroup::AddProperty(const char* function, vtkSMProperty* property)
{
  if (function)
  {
    this->Internals->PropertiesMap[function] = property;
  }
  this->Internals->Properties.push_back(property);
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMPropertyGroup::GetProperty(unsigned int index) const
{
  return this->Internals->Properties[index];
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMPropertyGroup::GetProperty(const char* function) const
{
  if (function &&
    this->Internals->PropertiesMap.find(function) != this->Internals->PropertiesMap.end())
  {
    return this->Internals->PropertiesMap[function];
  }

  return NULL;
}

//---------------------------------------------------------------------------
unsigned int vtkSMPropertyGroup::GetNumberOfProperties() const
{
  return static_cast<unsigned int>(this->Internals->Properties.size());
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyGroup::GetFunction(vtkSMProperty* property) const
{
  if (property)
  {
    for (vtkSMPropertyGroupInternals::PropertiesMapType::iterator iter =
           this->Internals->PropertiesMap.begin();
         iter != this->Internals->PropertiesMap.end(); ++iter)
    {
      if (iter->second.GetPointer() == property)
      {
        return iter->first.c_str();
      }
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
int vtkSMPropertyGroup::ReadXMLAttributes(vtkSMProxy* proxy, vtkPVXMLElement* groupElem)
{
  if (!proxy || !groupElem)
  {
    return 0;
  }

  // FIXME: should we use group-name as the "key" for the property groups?
  const char* groupName = groupElem->GetAttribute("name");
  if (groupName)
  {
    this->SetName(groupName);
  }

  const char* groupLabel = groupElem->GetAttribute("label");
  if (groupLabel)
  {
    this->SetXMLLabel(groupLabel);
  }

  // this is deprecated attribute that's replaced by "panel_widget". We still
  // process it for backwards compatibility.
  const char* groupType = groupElem->GetAttribute("type");
  if (groupType)
  {
    vtkWarningMacro("Found deprecated attribute 'type' of PropertyGroup. "
                    "Please use 'panel_widget' instead.");
    this->SetPanelWidget(groupType);
  }

  groupType = groupElem->GetAttribute("panel_widget");
  if (groupType)
  {
    this->SetPanelWidget(groupType);
  }

  const char* panelVisibility = groupElem->GetAttribute("panel_visibility");
  if (panelVisibility)
  {
    this->SetPanelVisibility(panelVisibility);
  }

  for (unsigned int k = 0; k < groupElem->GetNumberOfNestedElements(); k++)
  {
    vtkPVXMLElement* elem = groupElem->GetNestedElement(k);
    if (elem->GetName() && strcmp(elem->GetName(), "Documentation") == 0)
    {
      this->Documentation->SetDocumentationElement(elem);
      continue;
    }
    else if (elem->GetName() && strcmp(elem->GetName(), "Hints") == 0)
    {
      this->SetHints(elem);
      continue;
    }

    // if elem has "exposed_name", use that to locate the property, else use the
    // "name".
    const char* propname = elem->GetAttribute("exposed_name") ? elem->GetAttribute("exposed_name")
                                                              : elem->GetAttribute("name");
    vtkSMProperty* property = propname ? proxy->GetProperty(propname) : NULL;
    if (!property)
    {
      vtkWarningMacro("Failed to locate property '" << (propname ? propname : "(none)")
                                                    << "' for PropertyGroup. Skipping.");
    }
    else
    {
      const char* functionAttribute = elem->GetAttribute("function");
      if (functionAttribute == 0)
      {
        functionAttribute = elem->GetAttribute("name");
      }
      this->AddProperty(functionAttribute, property);
    }
  }
  return 1;
}
