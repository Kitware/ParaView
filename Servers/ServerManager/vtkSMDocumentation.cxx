/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDocumentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDocumentation.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSMDocumentation);
vtkCxxRevisionMacro(vtkSMDocumentation, "1.1");
vtkCxxSetObjectMacro(vtkSMDocumentation, DocumentationElement,
  vtkPVXMLElement);
//-----------------------------------------------------------------------------
vtkSMDocumentation::vtkSMDocumentation()
{
  this->DocumentationElement = 0;
}

//-----------------------------------------------------------------------------
vtkSMDocumentation::~vtkSMDocumentation()
{
  this->SetDocumentationElement(0);
}

//-----------------------------------------------------------------------------
const char* vtkSMDocumentation::GetLongHelp()
{
  if (!this->DocumentationElement)
    {
    return 0;
    }
  return this->DocumentationElement->GetAttribute("long_help");
}

//-----------------------------------------------------------------------------
const char* vtkSMDocumentation::GetShortHelp()
{
  if (!this->DocumentationElement)
    {
    return 0;
    }
  return this->DocumentationElement->GetAttribute("short_help");
}

//-----------------------------------------------------------------------------
const char* vtkSMDocumentation::GetDescription()
{
  if (!this->DocumentationElement)
    {
    return 0;
    }
  return this->DocumentationElement->GetCharacterData();
}

//-----------------------------------------------------------------------------
void vtkSMDocumentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DocumentationElement: " 
    << this->DocumentationElement << endl;

}
