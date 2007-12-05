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
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <vtkstd/map>
#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMArrayListDomain);
vtkCxxRevisionMacro(vtkSMArrayListDomain, "1.13");

struct vtkSMArrayListDomainInternals
{
  vtkstd::map<vtkStdString, int> PartialMap;
};

//---------------------------------------------------------------------------
vtkSMArrayListDomain::vtkSMArrayListDomain()
{
  this->AttributeType = vtkDataSetAttributes::SCALARS;
  this->DataType = VTK_VOID;
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
                                     int outputport,
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
    if ( iad->IsFieldValid(sp, outputport, info->GetArrayInformation(idx)) )
      {
      this->ALDInternals->PartialMap[arrayInfo->GetName()] = arrayInfo->GetIsPartial();
      if (!this->DataType || (arrayInfo->GetDataType() == this->DataType))
        {
        unsigned int newidx = this->AddString(arrayInfo->GetName());
        if (arrayInfo == attrInfo)
          {
          attrIdx = newidx;
          }
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
                                  vtkSMInputArrayDomain* iad,
                                  int outputport)
{
  // Make sure the outputs are created.
  sp->CreateOutputPorts();
  vtkPVDataInformation* info = sp->GetDataInformation(outputport);

  if (!info)
    {
    return;
    }

  if ( iad->GetAttributeType() == vtkSMInputArrayDomain::ANY )
    {
    this->AddArrays(sp, outputport, info->GetPointDataInformation(), iad);
    this->AddArrays(sp, outputport, info->GetCellDataInformation(), iad);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::POINT )
    {
    this->AddArrays(sp, outputport, info->GetPointDataInformation(), iad);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::CELL )
    {
    this->AddArrays(sp, outputport, info->GetCellDataInformation(), iad);
    }

  this->InvokeModified();
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProxyProperty* pp,
                                  vtkSMSourceProxy* sp,
                                  int outputport)
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
    this->Update(sp, iad, outputport);
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProxyProperty* pp)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);

  unsigned int i;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      this->Update(pp, sp, 
        (ip? ip->GetUncheckedOutputPortForConnection(i) : 0));
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
      this->Update(pp, sp,
        (ip? ip->GetOutputPortForConnection(i) : 0));
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

  const char* data_type = element->GetAttribute("data_type");  
  if(data_type)
    {
    //from vtkType.h
    this->DataType = -1;
    if (!strcmp(data_type, "VTK_VOID")) 
      {
      this->DataType=0;    
      }
    if (!strcmp(data_type, "VTK_BIT")) 
      {
      this->DataType=1;
      }
    if (!strcmp(data_type, "VTK_CHAR")) 
      {
      this->DataType=2;
      }
    if (!strcmp(data_type, "VTK_SIGNED_CHAR")) 
      {
      this->DataType=15;
      }
    if (!strcmp(data_type, "VTK_UNSIGNED_CHAR")) 
      {
      this->DataType=3;
      }
    if (!strcmp(data_type, "VTK_SHORT")) 
      {
      this->DataType=4;
      }
    if (!strcmp(data_type, "VTK_UNSIGNED_SHORT")) 
      {
      this->DataType=5;
      }
    if (!strcmp(data_type, "VTK_INT")) 
      {
      this->DataType=6;
      }
    if (!strcmp(data_type, "VTK_UNSIGNED_INT")) 
      {
      this->DataType=7;
      }
    if (!strcmp(data_type, "VTK_LONG")) 
      {
      this->DataType=8;
      }
    if (!strcmp(data_type, "VTK_UNSIGNED_LONG")) 
      {
      this->DataType=9;
      }
    if (!strcmp(data_type, "VTK_FLOAT")) 
      {
      this->DataType=10;
      }
    if (!strcmp(data_type, "VTK_DOUBLE")) 
      {
      this->DataType=11;
      }
    if (!strcmp(data_type, "VTK_ID_TYPE")) 
      {
      this->DataType=12;
      }
    if (!strcmp(data_type, "VTK_STRING")) 
      {
      this->DataType=13;
      }
    if (!strcmp(data_type, "VTK_OPAQUE")) 
      {
      this->DataType=14;
      }
    if (!strcmp(data_type, "VTK_LONG_LONG")) 
      {
      this->DataType=16;
      }
    if (!strcmp(data_type, "VTK_UNSIGNED_LONG_LONG")) 
      {
      this->DataType=17;
      }
    if (!strcmp(data_type, "VTK___INT64")) 
      {
      this->DataType=18;
      }
    if (!strcmp(data_type, "VTK_UNSIGNED___INT64")) 
      {
      this->DataType=19;
      }
    if (this->DataType == -1)
      {
      this->DataType = atoi(data_type);
      }
    }

  
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = 
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!prop)
    {
    return 0;
    }

  if (this->GetNumberOfStrings() > 0)
    {
    const char* array = this->GetString(0);
    if (svp->GetNumberOfElements() == 5)
      {
      svp->SetElement(4, array);
      return 1;
      }
    else if (svp->GetNumberOfElements() == 1)
      {
      svp->SetElement(0, array);
      return 1;
      }
    }
  return this->Superclass::SetDefaultValues(prop);
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultElement: " << this->DefaultElement << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
  os << indent << "DataType: " << this->DataType << endl;
}
