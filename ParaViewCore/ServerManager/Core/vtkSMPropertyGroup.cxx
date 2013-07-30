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
#include "vtkSMProperty.h"
#include "vtkWeakPointer.h"

#include <map>
#include <string>
#include <vector>

class vtkSMPropertyGroupInternals
{
public:
  std::vector<vtkSMProperty *> Properties;
  std::map<std::string, vtkWeakPointer<vtkSMProperty> > PropertiesMap;
};

vtkStandardNewMacro(vtkSMPropertyGroup)
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
}

//---------------------------------------------------------------------------
vtkSMPropertyGroup::~vtkSMPropertyGroup()
{
  this->SetXMLLabel(0);
  this->SetName(0);
  this->SetPanelWidget(0);
  this->SetPanelVisibility(0);
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMPropertyGroup::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
bool vtkSMPropertyGroup::IsEmpty() const
{
  return this->Internals->Properties.empty();
}

//---------------------------------------------------------------------------
void vtkSMPropertyGroup::AddProperty(const char* function, vtkSMProperty *property)
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
    this->Internals->PropertiesMap.find(function) !=
    this->Internals->PropertiesMap.end())
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
