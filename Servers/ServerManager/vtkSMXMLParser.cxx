/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXMLParser.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMXMLParser.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMXMLParser);

//----------------------------------------------------------------------------
vtkSMXMLParser::vtkSMXMLParser()
{
}

//----------------------------------------------------------------------------
vtkSMXMLParser::~vtkSMXMLParser()
{
}

//----------------------------------------------------------------------------
void vtkSMXMLParser::ProcessGroup(
  vtkPVXMLElement* group, vtkSMProxyManager* manager)
{
  const char* groupName = group->GetAttribute("name");

  // Loop over the top-level elements.
  for(unsigned int i=0; i < group->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* element = group->GetNestedElement(i);
    const char* name = element->GetAttribute("name");
    if(name)
      {
      manager->AddElement(groupName, name, element);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMXMLParser::ProcessConfiguration(vtkSMProxyManager* manager)
{
  // Get the root element.
  vtkPVXMLElement* root = this->GetRootElement();
  if(!root)
    {
    vtkErrorMacro("Must parse a configuration before storing it.");
    return;
    }

  // Loop over the top-level elements.
  unsigned int i;
  for(i=0; i < root->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* element = root->GetNestedElement(i);
    this->ProcessGroup(element, manager);
    }
}

//----------------------------------------------------------------------------
void vtkSMXMLParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

