/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFieldDataDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFieldDataDomain.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMFieldDataDomain);
vtkCxxRevisionMacro(vtkSMFieldDataDomain, "1.2");

//---------------------------------------------------------------------------
vtkSMFieldDataDomain::vtkSMFieldDataDomain()
{
}

//---------------------------------------------------------------------------
vtkSMFieldDataDomain::~vtkSMFieldDataDomain()
{
}

//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::CheckForArray(
  vtkSMSourceProxy* sp, 
  vtkPVDataSetAttributesInformation* info, 
  vtkSMInputArrayDomain* iad)
{
  int num = info->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    if ( iad->IsFieldValid(sp, info->GetArrayInformation(idx), 1) )
      {
      return 1;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::Update(vtkSMSourceProxy* sp, 
                                  vtkSMInputArrayDomain* iad)
{
  // Make sure the outputs are created.
  sp->CreateParts();
  vtkPVDataInformation* info = sp->GetDataInformation();

  if (!info)
    {
    return;
    }

  if (this->CheckForArray(sp, info->GetPointDataInformation(), iad))
    {
    this->AddEntry("Point Data", vtkDataObject::FIELD_ASSOCIATION_POINTS);
    }

  if (this->CheckForArray(sp, info->GetCellDataInformation(), iad))
    {
    this->AddEntry("Cell Data",  vtkDataObject::FIELD_ASSOCIATION_CELLS);
    }

}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::Update(vtkSMProxyProperty* pp, vtkSMSourceProxy* sp)
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
void vtkSMFieldDataDomain::Update(vtkSMProperty*)
{
  this->RemoveAllEntries();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));

  if (!pp)
    {
    return;
    }

  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (unsigned int i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      this->Update(pp, sp);
      }
    }


}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
