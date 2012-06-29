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

#include <vector>

#include "vtkObjectFactory.h"

class vtkSMPropertyGroupInternals
{
public:
  std::vector<vtkSMProperty *> Properties;
};

vtkStandardNewMacro(vtkSMPropertyGroup)
 //---------------------------------------------------------------------------
vtkSMPropertyGroup::vtkSMPropertyGroup()
  : Internals(new vtkSMPropertyGroupInternals)
{
  this->Name = 0;
  this->XMLLabel = 0;
  this->Type = 0;
  this->PanelVisibility = 0;

  // by default, properties are set to show only in advanced mode
  SetPanelVisibility("advanced");
}

//---------------------------------------------------------------------------
vtkSMPropertyGroup::~vtkSMPropertyGroup()
{
  this->SetXMLLabel(0);
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
void vtkSMPropertyGroup::AddProperty(vtkSMProperty *property)
{
  this->Internals->Properties.push_back(property);
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMPropertyGroup::GetProperty(unsigned int index) const
{
  return this->Internals->Properties[index];
}

//---------------------------------------------------------------------------
unsigned int vtkSMPropertyGroup::GetNumberOfProperties() const
{
  return this->Internals->Properties.size();
}
