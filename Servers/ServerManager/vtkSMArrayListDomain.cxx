/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArrayListDomain.h"

#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

#include <vtkstd/map>
#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMArrayListDomain);
vtkCxxRevisionMacro(vtkSMArrayListDomain, "1.6");

struct vtkSMArrayListDomainInternals
{
  vtkstd::map<vtkStdString, int> PartialMap;
};

//---------------------------------------------------------------------------
vtkSMArrayListDomain::vtkSMArrayListDomain()
{
  this->AttributeType = vtkDataSetAttributes::SCALARS;
  this->DefaultElement = 0;
  this->InputDomainName = 0;
  this->ALDInternals = new vtkSMArrayListDomainInternals;
}

//---------------------------------------------------------------------------
vtkSMArrayListDomain::~vtkSMArrayListDomain()
{
  this->SetInputDomainName(0);
  delete this->ALDInternals;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::IsArrayPartial(unsigned int idx)
{
  const char* name = this->GetString(idx);
  return this->ALDInternals->PartialMap[name];
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::AddArrays(vtkSMSourceProxy* sp,
                                     vtkPVDataSetAttributesInformation* info, 
                                     vtkSMInputArrayDomain* iad)
{
  this->DefaultElement = 0;

  int attrIdx=-1;
  vtkPVArrayInformation* attrInfo = info->GetAttributeInformation(
    this->AttributeType);
  int num = info->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* arrayInfo = info->GetArrayInformation(idx);
    if ( iad->IsFieldValid(sp, info->GetArrayInformation(idx)) )
      {
      this->ALDInternals->PartialMap[arrayInfo->GetName()] = arrayInfo->GetIsPartial();
      unsigned int newidx = this->AddString(arrayInfo->GetName());
      if (arrayInfo == attrInfo)
        {
        attrIdx = newidx;
        }
      }
    }
  if (attrIdx >= 0)
    {
    this->SetDefaultElement(attrIdx);
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMSourceProxy* sp, 
                                  vtkSMInputArrayDomain* iad)
{
  // Make sure the outputs are created.
  sp->CreateParts();
  vtkPVDataInformation* info = sp->GetDataInformation();

  if (!info)
    {
    return;
    }

  if ( iad->GetAttributeType() == vtkSMInputArrayDomain::ANY )
    {
    this->AddArrays(sp, info->GetPointDataInformation(), iad);
    this->AddArrays(sp, info->GetCellDataInformation(), iad);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::POINT )
    {
    this->AddArrays(sp, info->GetPointDataInformation(), iad);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::CELL )
    {
    this->AddArrays(sp, info->GetCellDataInformation(), iad);
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProxyProperty* pp,
                                  vtkSMSourceProxy* sp)
{
  vtkSMInputArrayDomain* iad = 0;
  if (this->InputDomainName)
    {
    iad = vtkSMInputArrayDomain::SafeDownCast(
      pp->GetDomain(this->InputDomainName));
    }
  else
    {
    vtkSMDomainIterator* di = pp->NewDomainIterator();
    di->Begin();
    while (!di->IsAtEnd())
      {
      iad = vtkSMInputArrayDomain::SafeDownCast(di->GetDomain());
      if (iad)
        {
        break;
        }
      di->Next();
      }
    di->Delete();
    }

  if (iad)
    {
    this->Update(sp, iad);
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProxyProperty* pp)
{
  unsigned int i;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      this->Update(pp, sp);
      return;
      }
    }

  // In case there is no valid unchecked proxy, use the actual
  // proxy values
  numProxs = pp->GetNumberOfProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetProxy(i));
    if (sp)
      {
      this->Update(pp, sp);
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProperty*)
{
  this->RemoveAllStrings();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    this->Update(pp);
    }
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  const char* input_domain_name = 
    element->GetAttribute("input_domain_name");
  if (input_domain_name)
    {
    this->SetInputDomainName(input_domain_name);
    }

  // Search for attribute type with matching name.
  const char* attribute_type = element->GetAttribute("attribute_type");
  unsigned int i = vtkDataSetAttributes::NUM_ATTRIBUTES;
  if(attribute_type)
    {
    for(i=0; i<vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
      {
      if(strcmp(vtkDataSetAttributes::GetAttributeTypeAsString(i),
                attribute_type) == 0)
        {
        this->SetAttributeType(i);
        break;
        }
      }
    }
  if(i == vtkDataSetAttributes::NUM_ATTRIBUTES)
    {
    this->SetAttributeType(vtkDataSetAttributes::SCALARS);
    }
  
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DefaultElement: " << this->DefaultElement << endl;
}
