/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyAdaptor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyAdaptor.h"

#include "vtkSMDomainIterator.h"
#include "vtkObjectFactory.h"

#include "vtkSMBooleanDomain.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringListRangeDomain.h"

#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMPropertyAdaptor);
vtkCxxRevisionMacro(vtkSMPropertyAdaptor, "1.4");

//---------------------------------------------------------------------------
vtkSMPropertyAdaptor::vtkSMPropertyAdaptor()
{
  this->InitializeDomains();
  this->InitializeProperties();

  this->Property = 0;
}

//---------------------------------------------------------------------------
vtkSMPropertyAdaptor::~vtkSMPropertyAdaptor()
{
  this->SetProperty(0);
}

//---------------------------------------------------------------------------
void vtkSMPropertyAdaptor::SetProperty(vtkSMProperty* property)
{
  if (this->Property != property)
    {                                                          
    if (this->Property != NULL) { this->Property->UnRegister(this); } 
    this->Property = property;
    if (this->Property != NULL) { this->Property->Register(this); }
    this->Modified();
    }

  this->InitializeProperties();
  this->ProxyProperty = vtkSMProxyProperty::SafeDownCast(property);
  this->DoubleVectorProperty = vtkSMDoubleVectorProperty::SafeDownCast(property);
  this->IdTypeVectorProperty = vtkSMIdTypeVectorProperty::SafeDownCast(property);
  this->IntVectorProperty = vtkSMIntVectorProperty::SafeDownCast(property);
  this->StringVectorProperty = vtkSMStringVectorProperty::SafeDownCast(property);

  this->InitializeDomains();
  if (property)
    {
    vtkSMDomainIterator* iter = property->NewDomainIterator();
    iter->Begin();
    while(!iter->IsAtEnd())
      {
      this->SetDomain(iter->GetDomain());
      iter->Next();
      }
    iter->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSMPropertyAdaptor::SetDomain(vtkSMDomain* domain)
{
  if (!this->BooleanDomain)
    {
    this->BooleanDomain = vtkSMBooleanDomain::SafeDownCast(domain);
    }
  if (!this->DoubleRangeDomain)
    {
    this->DoubleRangeDomain = vtkSMDoubleRangeDomain::SafeDownCast(domain);
    }
  if (!this->EnumerationDomain)
    {
    this->EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(domain);
    }
  if (!this->IntRangeDomain)
    {
    this->IntRangeDomain = vtkSMIntRangeDomain::SafeDownCast(domain);
    }
  if (!this->ProxyGroupDomain)
    {
    this->ProxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(domain);
    }
  if (!this->StringListDomain)
    {
    this->StringListDomain = vtkSMStringListDomain::SafeDownCast(domain);
    }
  if (!this->StringListDomain)
    {
    this->StringListDomain = vtkSMStringListDomain::SafeDownCast(domain);
    }
  if (!this->StringListRangeDomain)
    {
    this->StringListRangeDomain = 
      vtkSMStringListRangeDomain::SafeDownCast(domain);
    }
}

//---------------------------------------------------------------------------
void vtkSMPropertyAdaptor::InitializeDomains()
{
  this->BooleanDomain = 0;
  this->DoubleRangeDomain = 0;
  this->EnumerationDomain = 0;
  this->IntRangeDomain = 0;
  this->ProxyGroupDomain = 0;
  this->StringListDomain = 0;
  this->StringListRangeDomain = 0;
}

//---------------------------------------------------------------------------
void vtkSMPropertyAdaptor::InitializeProperties()
{
  this->ProxyProperty = 0;
  this->DoubleVectorProperty = 0;
  this->IdTypeVectorProperty = 0;
  this->IntVectorProperty = 0;
  this->StringVectorProperty = 0;
}

//---------------------------------------------------------------------------
int vtkSMPropertyAdaptor::GetPropertyType()
{
  if (this->BooleanDomain)
    {
    return vtkSMPropertyAdaptor::ENUMERATION;
    }

  if (this->DoubleRangeDomain)
    {
    return vtkSMPropertyAdaptor::RANGE;
    }

  if (this->EnumerationDomain)
    {
    return vtkSMPropertyAdaptor::ENUMERATION;
    }

  if (this->IntRangeDomain)
    {
    return vtkSMPropertyAdaptor::RANGE;
    }

  if (this->ProxyGroupDomain)
    {
    return vtkSMPropertyAdaptor::ENUMERATION;
    }

  if (this->StringListDomain)
    {
    return vtkSMPropertyAdaptor::ENUMERATION;
    }

  if (this->StringListRangeDomain)
    {
    return vtkSMPropertyAdaptor::SELECTION;
    }

  return vtkSMPropertyAdaptor::UNKNOWN;
}

//---------------------------------------------------------------------------
int vtkSMPropertyAdaptor::GetSelectionType()
{
  if (this->StringListRangeDomain)
    {
    int intDomainMode = this->StringListRangeDomain->GetIntDomainMode();
    if (intDomainMode == vtkSMStringListRangeDomain::BOOLEAN)
      {
      return vtkSMPropertyAdaptor::BOOLEAN;
      }
    else
      {
      return vtkSMPropertyAdaptor::RANGE;
      }
    }

  return vtkSMPropertyAdaptor::UNKNOWN;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetMinimum(unsigned int idx)
{
  if (this->DoubleRangeDomain)
    {
    int exists;
    double min = this->DoubleRangeDomain->GetMinimum(idx, exists);
    if (exists)
      {
      sprintf(this->Minimum, "%g", min);
      return this->Minimum;
      }
    }

  if (this->IntRangeDomain)
    {
    int exists;
    int min = this->IntRangeDomain->GetMinimum(idx, exists);
    if (exists)
      {
      sprintf(this->Minimum, "%d", min);
      return this->Minimum;
      }
    }

  if (this->StringListRangeDomain)
    {
    int intDomainMode = this->StringListRangeDomain->GetIntDomainMode();
    if (intDomainMode == vtkSMStringListRangeDomain::BOOLEAN)
      {
      sprintf(this->Minimum, "%d", 0);
      return this->Minimum;
      }
    else
      {
      int exists;
      int min = this->StringListRangeDomain->GetMinimum(idx, exists);
      if (exists)
        {
        sprintf(this->Minimum, "%d", min);
        return this->Minimum;
        }
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetMaximum(unsigned int idx)
{
  if (this->DoubleRangeDomain)
    {
    int exists;
    double max = this->DoubleRangeDomain->GetMaximum(idx, exists);
    if (exists)
      {
      sprintf(this->Maximum, "%g", max);
      return this->Maximum;
      }
    }

  if (this->IntRangeDomain)
    {
    int exists;
    int max = this->IntRangeDomain->GetMaximum(idx, exists);
    if (exists)
      {
      sprintf(this->Maximum, "%d", max);
      return this->Maximum;
      }
    }

  if (this->StringListRangeDomain)
    {
    int intDomainMode = this->StringListRangeDomain->GetIntDomainMode();
    if (intDomainMode == vtkSMStringListRangeDomain::BOOLEAN)
      {
      sprintf(this->Maximum, "%d", 0);
      return this->Maximum;
      }
    else
      {
      int exists;
      int max = this->StringListRangeDomain->GetMaximum(idx, exists);
      if (exists)
        {
        sprintf(this->Maximum, "%d", max);
        return this->Maximum;
        }
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMPropertyAdaptor::GetNumberOfEnumerationEntries()
{
  if (this->BooleanDomain)
    {
    return 2;
    }
  if (this->EnumerationDomain)
    {
    return this->EnumerationDomain->GetNumberOfEntries();
    }
  if (this->ProxyGroupDomain)
    {
    return this->ProxyGroupDomain->GetNumberOfProxies();
    }
  if (this->StringListDomain)
    {
    return this->StringListDomain->GetNumberOfStrings();
    }
  if (this->StringListRangeDomain)
    {
    return this->StringListRangeDomain->GetNumberOfStrings();
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetEnumerationValue(unsigned int idx)
{
  if (this->BooleanDomain)
    {
    if (idx == 0)
      {
      return "0";
      }
    return "1";
    }
  if (this->EnumerationDomain)
    {
    return this->EnumerationDomain->GetEntryText(idx);
    }
  if (this->ProxyGroupDomain)
    {
    return this->ProxyGroupDomain->GetProxyName(idx);
    }
  if (this->StringListDomain)
    {
    return this->StringListDomain->GetString(idx);
    }
  if (this->StringListRangeDomain)
    {
    return this->StringListRangeDomain->GetString(idx);
    }
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMPropertyAdaptor::GetNumberOfElements()
{
  if (this->ProxyProperty)
    {
    return this->ProxyProperty->GetNumberOfProxies();
    }
  if (this->DoubleVectorProperty)
    {
    return this->DoubleVectorProperty->GetNumberOfElements();
    }
  if (this->IdTypeVectorProperty)
    {
    return this->IdTypeVectorProperty->GetNumberOfElements();
    }
  if (this->IntVectorProperty)
    {
    return this->IntVectorProperty->GetNumberOfElements();
    }
  if (this->StringVectorProperty)
    {
    return this->StringVectorProperty->GetNumberOfElements();
    }
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMPropertyAdaptor::GetEnumerationElementIndex(const char* element)
{
  if ( !element )
    {
    return 0;
    }
  unsigned int cc;
  for ( cc = 0; cc < this->GetNumberOfEnumerationEntries(); cc ++ )
    {
    if ( strcmp(element, this->GetEnumerationValue(cc)) == 0 )
      {
      return cc;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetElement(unsigned int idx)
{
  if (this->ProxyProperty && this->ProxyGroupDomain)
    {
    return this->ProxyGroupDomain->GetProxyName(
      this->ProxyProperty->GetProxy(idx));
    }
  if (this->DoubleVectorProperty)
    {
    sprintf(this->ElemValue, 
            "%g", 
            this->DoubleVectorProperty->GetElement(idx));
    return this->ElemValue;
    }
  if (this->IdTypeVectorProperty)
    {
    ostrstream elemV(this->ElemValue, 128);
    elemV << this->IdTypeVectorProperty->GetElement(idx) << ends;
    return this->ElemValue;
    }
  if (this->IntVectorProperty)
    {
    ostrstream elemV(this->ElemValue, 128);
    elemV << this->IntVectorProperty->GetElement(idx) << ends;
    return this->ElemValue;
    }
  if (this->StringVectorProperty)
    {
    return this->StringVectorProperty->GetElement(idx);
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMPropertyAdaptor::SetElement(unsigned int idx, const char* value)
{
  if (this->ProxyProperty && this->ProxyGroupDomain)
    {
    return this->ProxyProperty->SetProxy(
      idx, this->ProxyGroupDomain->GetProxy(value));
    }
  if (this->DoubleVectorProperty)
    {
    return this->DoubleVectorProperty->SetElement(idx, atof(value));
    }
  if (this->IdTypeVectorProperty)
    {
    return this->IdTypeVectorProperty->SetElement(idx, atoi(value));
    }
  if (this->IntVectorProperty)
    {
    return this->IntVectorProperty->SetElement(idx, atoi(value));
    }
  if (this->StringVectorProperty)
    {
    return this->StringVectorProperty->SetElement(idx, value);
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMPropertyAdaptor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Property: " << this->Property << endl;
}
