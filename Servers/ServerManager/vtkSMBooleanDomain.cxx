/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBooleanDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBooleanDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMBooleanDomain);
vtkCxxRevisionMacro(vtkSMBooleanDomain, "1.1");

//---------------------------------------------------------------------------
vtkSMBooleanDomain::vtkSMBooleanDomain()
{
}

//---------------------------------------------------------------------------
vtkSMBooleanDomain::~vtkSMBooleanDomain()
{
}

//---------------------------------------------------------------------------
int vtkSMBooleanDomain::IsInDomain(vtkSMProperty* property)
{
  if (!property)
    {
    return 0;
    }
  vtkSMIntVectorProperty* ip = vtkSMIntVectorProperty::SafeDownCast(property);
  if (ip)
    {
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMBooleanDomain::ReadXMLAttributes(vtkPVXMLElement* /*element*/)
{
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMBooleanDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
