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

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMArrayListDomain);
vtkCxxRevisionMacro(vtkSMArrayListDomain, "1.1");

//---------------------------------------------------------------------------
vtkSMArrayListDomain::vtkSMArrayListDomain()
{
}

//---------------------------------------------------------------------------
vtkSMArrayListDomain::~vtkSMArrayListDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::AddArrays(vtkPVDataSetAttributesInformation* info, 
                                     vtkSMInputArrayDomain* iad)
{
  int attrIdx=-1;
  //vtkPVArrayInformation* attrInfo = info->GetAttributeInformation(
  //this->AttributeType);
  int num = info->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* arrayInfo = info->GetArrayInformation(idx);
    if ( iad->IsFieldValid(info->GetArrayInformation(idx)) )
      {
      this->AddString(arrayInfo->GetName());
      }
    //if (arrayInfo == attrInfo)
    //{
    //attrIdx = idx;
    //}
    }
  //if (attrIdx >= 0)
  //{
  //this->SetDefaultElement(attrIdx);
  //}
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
    this->AddArrays(info->GetPointDataInformation(), iad);
    this->AddArrays(info->GetCellDataInformation(), iad);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::POINT )
    {
    this->AddArrays(info->GetPointDataInformation(), iad);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::CELL )
    {
    this->AddArrays(info->GetCellDataInformation(), iad);
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProxyProperty* pp,
                                  vtkSMSourceProxy* sp)
{
  vtkSMDomainIterator* di = pp->NewDomainIterator();
  di->Begin();
  while (!di->IsAtEnd())
    {
    vtkSMInputArrayDomain* iad = vtkSMInputArrayDomain::SafeDownCast(
      di->GetDomain());
    if (iad)
      {
      this->Update(sp, iad);
      break;
      }
    di->Next();
    }
  di->Delete();
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProxyProperty* pp)
{
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (unsigned i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      this->Update(pp, sp);
      break;
      }
    }

}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update()
{
  this->RemoveAllStrings();

  unsigned int numReq = this->GetNumberOfRequiredProperties();
  for(unsigned int i=0; i<numReq; i++)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->GetRequiredProperty(i));
    if (pp)
      {
      this->Update(pp);
      break;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
