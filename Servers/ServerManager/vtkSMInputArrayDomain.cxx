/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputArrayDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputArrayDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMInputArrayDomain);
vtkCxxRevisionMacro(vtkSMInputArrayDomain, "1.2");

//---------------------------------------------------------------------------
vtkSMInputArrayDomain::vtkSMInputArrayDomain()
{
  this->AttributeType = vtkSMInputArrayDomain::ANY;
  this->NumberOfComponents = 0;
}

//---------------------------------------------------------------------------
vtkSMInputArrayDomain::~vtkSMInputArrayDomain()
{
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsInDomain(vtkSMProperty* property)
{
  if (!property)
    {
    return 0;
    }

  unsigned int i;

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(property);
  if (pp)
    {
    unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
    for (i=0; i<numProxs; i++)
      {
      if (!this->IsInDomain( 
            vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i)) ) )
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsInDomain(vtkSMSourceProxy* proxy)
{
  if (!proxy)
    {
    return 0;
    }

  // Make sure the outputs are created.
  proxy->CreateParts();
  vtkPVDataInformation* info = proxy->GetDataInformation();
  if (!info)
    {
    return 0;
    }

  if (this->AttributeType == vtkSMInputArrayDomain::POINT ||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(info->GetPointDataInformation()))
      {
      return 1;
      }
    }

  if (this->AttributeType == vtkSMInputArrayDomain::CELL||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(info->GetCellDataInformation()))
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsFieldValid(vtkPVArrayInformation* arrayInfo)
{
  if (this->NumberOfComponents > 0 && 
      this->NumberOfComponents != arrayInfo->GetNumberOfComponents())
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::AttributeInfoContainsArray(
  vtkPVDataSetAttributesInformation* attrInfo)
{
  if (!attrInfo)
    {
    return 0;
    }

  int num = attrInfo->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(idx);
    if (this->IsFieldValid(arrayInfo))
      {
      return 1;
      }
    }

  return 0;
}


//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::SaveState(
  const char* name, ofstream* file, vtkIndent indent)
{
  *file << indent 
        << "<Domain name=\"" << this->XMLName << "\" id=\"" << name << "\">"
        << endl;
  *file << indent.GetNextIndent() 
        << "<InputArray attribute_type=\"" << this->GetAttributeType()
        << "\" number_of_components=\"" << this->GetNumberOfComponents()
        << "\"/>" << endl;
  
  *file << indent
        << "</Domain>" << endl;
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  const char* attribute_type = element->GetAttribute("attribute_type");
  if (attribute_type)
    {
    if (strcmp(attribute_type, "cell") == 0)
      {
      this->SetAttributeType(vtkSMInputArrayDomain::CELL);
      }
    else if (strcmp(attribute_type, "point") == 0)
      {
      this->SetAttributeType(vtkSMInputArrayDomain::POINT);
      }
    else
      {
      vtkErrorMacro("Unrecognize attribute type.");
      return 0;
      }
    }

  int numComponents;
  if (element->GetScalarAttribute("number_of_components", &numComponents))
    {
    this->SetNumberOfComponents(numComponents);
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}
