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

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMInputArrayDomain);
vtkCxxRevisionMacro(vtkSMInputArrayDomain, "1.8");

//---------------------------------------------------------------------------
static const char* const vtkSMInputArrayDomainAttributeTypes[] = {
  "point",
  "cell",
  "any"
};

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
  if (this->IsOptional)
    {
    return 1;
    }

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
    if (this->AttributeInfoContainsArray(proxy, info->GetPointDataInformation()))
      {
      return 1;
      }
    }

  if (this->AttributeType == vtkSMInputArrayDomain::CELL||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(proxy, info->GetCellDataInformation()))
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::CheckForArray(
  vtkPVArrayInformation* arrayInfo, vtkPVDataSetAttributesInformation* attrInfo)
{
  if (!attrInfo || !arrayInfo)
    {
    return 0;
    }

  int num = attrInfo->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* curInfo = attrInfo->GetArrayInformation(idx);
    if (curInfo == arrayInfo)
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsFieldValid(
  vtkSMSourceProxy* proxy, vtkPVArrayInformation* arrayInfo)
{
  return this->IsFieldValid(proxy, arrayInfo, 0);
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsFieldValid(
  vtkSMSourceProxy* proxy, vtkPVArrayInformation* arrayInfo, int bypass)
{
  vtkPVDataInformation* info = proxy->GetDataInformation();
  if (!info)
    {
    return 0;
    }

  int attributeType = this->AttributeType;
  if (!bypass)
    {
    vtkSMIntVectorProperty* fds = vtkSMIntVectorProperty::SafeDownCast(
      this->GetRequiredProperty("FieldDataSelection"));
    if (fds)
      {
      int val = fds->GetUncheckedElement(0);
      if (val == vtkDataSet::POINT_DATA_FIELD)
        {
        attributeType = vtkSMInputArrayDomain::POINT;
        }
      else if (val == vtkDataSet::CELL_DATA_FIELD)
        {
        attributeType = vtkSMInputArrayDomain::CELL;
        }
      }
    }

  int isField = 0;
  if (attributeType == vtkSMInputArrayDomain::POINT ||
      attributeType == vtkSMInputArrayDomain::ANY)
    {
    isField = this->CheckForArray(arrayInfo, info->GetPointDataInformation());
    }

  if (!isField &&
      (attributeType == vtkSMInputArrayDomain::CELL||
       attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetCellDataInformation());
    }

  if (!isField)
    {
    return 0;
    }

  if (this->NumberOfComponents > 0 && 
      this->NumberOfComponents != arrayInfo->GetNumberOfComponents())
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::AttributeInfoContainsArray(
  vtkSMSourceProxy* proxy, vtkPVDataSetAttributesInformation* attrInfo)
{
  if (!attrInfo)
    {
    return 0;
    }

  int num = attrInfo->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(idx);
    if (this->IsFieldValid(proxy, arrayInfo))
      {
      return 1;
      }
    }

  return 0;
}


//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::SaveState(
  const char* name, ostream* file, vtkIndent indent)
{
  *file << indent 
        << "<Domain name=\"" << this->XMLName << "\" id=\"" << name << "\">"
        << endl;
  *file << indent.GetNextIndent() 
        << "<InputArray attribute_type=\"" << this->GetAttributeTypeAsString()
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
      this->SetAttributeType(
        static_cast<unsigned char>(vtkSMInputArrayDomain::POINT));
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
const char* vtkSMInputArrayDomain::GetAttributeTypeAsString()
{
  return vtkSMInputArrayDomainAttributeTypes[this->AttributeType];
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::SetAttributeType(const char* type)
{
  if ( ! type )
    {
    vtkErrorMacro("No type specified");
    return;
    }
  unsigned char cc;
  for ( cc = 0; cc < vtkSMInputArrayDomain::LAST_ATTRIBUTE_TYPE; cc ++ )
    {
    if ( strcmp(type, vtkSMInputArrayDomainAttributeTypes[cc]) == 0 )
      {
      this->SetAttributeType(cc);
      return;
      }
    }
  vtkErrorMacro("No such attribute type: " << type);
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}
