/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayRangeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArrayRangeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMArrayRangeDomain);
vtkCxxRevisionMacro(vtkSMArrayRangeDomain, "1.5");

//---------------------------------------------------------------------------
vtkSMArrayRangeDomain::vtkSMArrayRangeDomain()
{
}

//---------------------------------------------------------------------------
vtkSMArrayRangeDomain::~vtkSMArrayRangeDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::Update(vtkSMProperty* prop)
{
  unsigned int numMinMax = 1;
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(prop);
  if (dvp)
    {
    // Use the larger of the number of checked and unchecked elements
    int numElems   = dvp->GetNumberOfElements();
    int numUnElems = dvp->GetNumberOfUncheckedElements();
    if ( numElems > numUnElems )
      {
      numMinMax = numElems;
      }
    else
      {
      numMinMax = numUnElems;
      }
    }

  this->RemoveAllMinima();
  this->RemoveAllMaxima();
  // This creates numMinMax entries but leaves them unset (no min or max)
  this->SetNumberOfEntries(numMinMax);

  vtkSMProxyProperty* ip = 0;
  vtkSMStringVectorProperty* array = 0;

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    ip = pp;
    }

  // Get the array name for the array selection
  vtkSMStringVectorProperty* sp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetRequiredProperty("ArraySelection"));
  if (sp)
    {
    array = sp;
    }

  if (!ip || !array)
    {
    return;
    }
  
  const char* arrayName = array->GetUncheckedElement(0);
  if (!arrayName || arrayName[0] == '\0')
    {
    arrayName = array->GetElement(0);
    }

  if (!arrayName || arrayName[0] == '\0')
    {
    return;
    }

  unsigned int i;
  unsigned int numProxs = ip->GetNumberOfUncheckedProxies();
  for (i=0; i<numProxs; i++)
    {
    // Use the first input
    vtkSMSourceProxy* source = 
      vtkSMSourceProxy::SafeDownCast(ip->GetUncheckedProxy(i));
    if (source)
      {
      this->Update(arrayName, ip, source);
      return;
      }
    }

  // In case there is no valid unchecked proxy, use the actual
  // proxy values
  numProxs = ip->GetNumberOfProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* source = 
      vtkSMSourceProxy::SafeDownCast(ip->GetProxy(i));
    if (source)
      {
      this->Update(arrayName, ip, source);
      return;
      }
    }
  

}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::Update(const char* arrayName,
                                   vtkSMProxyProperty* ip,
                                   vtkSMSourceProxy* sp)
{
  vtkSMDomainIterator* di = ip->NewDomainIterator();
  di->Begin();
  while (!di->IsAtEnd())
    {
    // We have to figure out whether we are working with cell data,
    // point data or both.
    vtkSMInputArrayDomain* iad = vtkSMInputArrayDomain::SafeDownCast(
      di->GetDomain());
    if (iad)
      {
      this->Update(arrayName, sp, iad);
      break;
      }
    di->Next();
    }
  di->Delete();
}
  
//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::Update(const char* arrayName,
                                   vtkSMSourceProxy* sp,
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
    this->SetArrayRange(info->GetPointDataInformation(), arrayName);
    this->SetArrayRange(info->GetCellDataInformation(), arrayName);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::POINT )
    {
    this->SetArrayRange(info->GetPointDataInformation(), arrayName);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::CELL )
    {
    this->SetArrayRange(info->GetCellDataInformation(), arrayName);
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::SetArrayRange(
  vtkPVDataSetAttributesInformation* info, const char* arrayName)
{
  vtkPVArrayInformation* ai = info->GetArrayInformation(arrayName);
  if (!ai)
    {
    return;
    }

  unsigned int numEntries = this->GetNumberOfEntries();
  for (unsigned int i=0; i<numEntries; i++)
    {
    this->AddMinimum(i, ai->GetComponentRange(i)[0]);
    this->AddMaximum(i, ai->GetComponentRange(i)[1]);
    }
  if (numEntries > 1) // vector magnitude range
    {
    this->AddMinimum(numEntries, ai->GetComponentRange(-1)[0]);
    this->AddMinimum(numEntries, ai->GetComponentRange(-1)[1]);
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
