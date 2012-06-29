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
#include "vtkSMFileListDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringListRangeDomain.h"

#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtksys/ios/sstream"

vtkStandardNewMacro(vtkSMPropertyAdaptor);

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
  if (!this->FileListDomain)
    {
    this->FileListDomain = vtkSMFileListDomain::SafeDownCast(domain);
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
  this->FileListDomain = 0;
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

  if (this->FileListDomain)
    {
    return vtkSMPropertyAdaptor::FILE_LIST;
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
int vtkSMPropertyAdaptor::GetElementType()
{
  if (this->ProxyProperty)
    {
    return vtkSMPropertyAdaptor::PROXY;
    }
  if (this->DoubleVectorProperty)
    {
    return vtkSMPropertyAdaptor::DOUBLE;
    }
  if (this->IdTypeVectorProperty)
    {
    return vtkSMPropertyAdaptor::INT;
    }
  if (this->IntVectorProperty)
    {
    if (this->BooleanDomain)
      {
      return vtkSMPropertyAdaptor::BOOLEAN;
      }
    else
      {
      return vtkSMPropertyAdaptor::INT;
      }
    }
  if (this->StringVectorProperty)
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
    else
      {
      return vtkSMPropertyAdaptor::STRING;
      }
    }
  return vtkSMPropertyAdaptor::UNKNOWN;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetRangeMinimum(unsigned int idx)
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
    return 0;
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
    return 0;
    }

  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetRangeMaximum(unsigned int idx)
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
    return 0;
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
    return 0;
    }

  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMPropertyAdaptor::GetNumberOfRangeElements()
{
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
const char* vtkSMPropertyAdaptor::GetRangeValue(unsigned int idx)
{
  if (this->DoubleVectorProperty)
    {
    sprintf(this->ElemValue, 
            "%g", 
            this->DoubleVectorProperty->GetElement(idx));
    return this->ElemValue;
    }
  if (this->IdTypeVectorProperty)
    {
    vtksys_ios::ostringstream elemV;
    elemV << this->IdTypeVectorProperty->GetElement(idx) << ends;
    strncpy(this->ElemValue, elemV.str().c_str(), 128);
    return this->ElemValue;
    }
  if (this->IntVectorProperty)
    {
    vtksys_ios::ostringstream elemV;
    elemV << this->IntVectorProperty->GetElement(idx) << ends;
    strncpy(this->ElemValue, elemV.str().c_str(), 128);
    return this->ElemValue;
    }
  if (this->StringVectorProperty)
    {
    return this->StringVectorProperty->GetElement(idx);
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMPropertyAdaptor::SetGenericValue(unsigned int idx, const char* value)
{
  return this->SetRangeValue(idx, value);
}

//---------------------------------------------------------------------------
int vtkSMPropertyAdaptor::SetRangeValue(unsigned int idx, const char* value)
{
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
  return 0;
}


//---------------------------------------------------------------------------
unsigned int vtkSMPropertyAdaptor::GetNumberOfEnumerationElements()
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
  if (this->FileListDomain)
    {
    return this->FileListDomain->GetNumberOfStrings();
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetEnumerationName(unsigned int idx)
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
  if (this->FileListDomain)
    {
    return this->FileListDomain->GetString(idx);
    }
  if (this->StringListDomain)
    {
    return this->StringListDomain->GetString(idx);
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetEnumerationValue()
{
  const char* name = 0;

  if (this->BooleanDomain && 
      this->IntVectorProperty &&
      this->IntVectorProperty->GetNumberOfElements() > 0)
    {
    int val = this->IntVectorProperty->GetElement(0);
    if (val)
      {
      name = "1";
      }
    else
      {
      name = "0";
      }
    }

  if (this->EnumerationDomain && this->IntVectorProperty && 
    this->IntVectorProperty->GetNumberOfElements() > 0)
    {
    int val = this->IntVectorProperty->GetElement(0);
    for (unsigned int i=0; i<this->EnumerationDomain->GetNumberOfEntries(); i++)
      {
      if (this->EnumerationDomain->GetEntryValue(i) == val)
        {
        name = this->EnumerationDomain->GetEntryText(i);
        break;
        }
      }
    }

  if ((this->StringListDomain || this->FileListDomain) && 
      this->StringVectorProperty && 
      this->StringVectorProperty->GetNumberOfElements() > 0)
    {
    unsigned int nos = this->StringVectorProperty->GetNumberOfElements();
    for (unsigned int i=0; i < nos; i++)
      {
      if (this->StringVectorProperty->GetElementType(i) == 
        vtkSMStringVectorProperty::STRING)
        {
        name = this->StringVectorProperty->GetElement(i);
        break;
        }
      }
    }

  if (this->ProxyGroupDomain && this->ProxyProperty
    && this->ProxyProperty->GetNumberOfProxies() > 0)
    {
    name = 
      this->ProxyGroupDomain->GetProxyName(this->ProxyProperty->GetProxy(0));
    }

  if (!name)
    {
    return 0;
    }

  // For empty domains, assume value is always correct.
  if ( this->GetNumberOfEnumerationElements() == 0 )
    {
    return name;
    }

  unsigned int cc;
  for ( cc = 0; cc < this->GetNumberOfEnumerationElements(); cc ++ )
    {
    if ( strcmp(name, this->GetEnumerationName(cc)) == 0 )
      {
      sprintf(this->EnumValue, "%d", cc);
      return this->EnumValue;
      }
    }

  return 0;

}

//---------------------------------------------------------------------------
int vtkSMPropertyAdaptor::SetEnumerationValue(const char* sidx)
{
  unsigned int idx = atoi(sidx);

  const char* enumName = this->GetEnumerationName(idx);
  if (!enumName)
    {
    return 0;
    }

  if (this->BooleanDomain && 
      this->IntVectorProperty &&
      this->IntVectorProperty->GetNumberOfElements() > 0)
    {
    int val = atoi(enumName);
    return this->IntVectorProperty->SetElement(0, val);
    }

  if (this->EnumerationDomain && this->IntVectorProperty)
    {
    return this->IntVectorProperty->SetElement(
      0, this->EnumerationDomain->GetEntryValue(idx) );
    }

  if ((this->StringListDomain || this->FileListDomain) && 
      this->StringVectorProperty)
    {
    unsigned int nos = this->StringVectorProperty->GetNumberOfElements();
    for (unsigned int i=0; i < nos ; i++)
      {
      if (this->StringVectorProperty->GetElementType(i) == 
        vtkSMStringVectorProperty::STRING)
        {
        return this->StringVectorProperty->SetElement(i, enumName);
        }
      }
    }

  if (this->ProxyGroupDomain && this->ProxyProperty)
    {
    vtkSMProxy* toadd = this->ProxyGroupDomain->GetProxy(enumName);
    if (this->ProxyProperty->GetNumberOfProxies() < 1)
      {
      this->ProxyProperty->AddProxy(toadd);
      }
    else
      {
      return this->ProxyProperty->SetProxy(0, toadd);
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMPropertyAdaptor::GetNumberOfSelectionElements()
{
  if (this->StringListRangeDomain)
    {
    return this->StringListRangeDomain->GetNumberOfStrings();
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetSelectionName(unsigned int idx)
{
  if (this->StringListRangeDomain)
    {
    return this->StringListRangeDomain->GetString(idx);
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetSelectionValue(unsigned int idx)
{
  if (this->StringListRangeDomain)
    {
    const char* name = 
      this->StringListRangeDomain->GetString(idx);

    if (this->StringVectorProperty)
      {
      unsigned int numElems = this->StringVectorProperty->GetNumberOfElements();
      if (numElems % 2 != 0)
        {
        return 0;
        }
      for(unsigned int i=0; i<numElems; i+=2)
        {
        if (strcmp(this->StringVectorProperty->GetElement(i), name)==0)
          {
          return this->StringVectorProperty->GetElement(i+1);
          }
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMPropertyAdaptor::SetSelectionValue(unsigned int idx, const char* value)
{
  if (this->StringListRangeDomain)
    {
    const char* name = 
      this->StringListRangeDomain->GetString(idx);

    if (this->StringVectorProperty)
      {
      unsigned int numElems = this->StringVectorProperty->GetNumberOfElements();
      if (numElems % 2 != 0)
        {
        return 0;
        }
      unsigned int i;
      for(i=0; i<numElems; i+=2)
        {
        if (strcmp(this->StringVectorProperty->GetElement(i), name)==0)
          {
          return this->StringVectorProperty->SetElement(i+1, value);
          }
        }
      // If we didn't find the name, find the first empty spot
      for(i=0; i<numElems; i+=2)
        {
        const char* elem = this->StringVectorProperty->GetElement(i);
        if (!elem || elem[0] == '\0')
          {
          this->StringVectorProperty->SetElement(i, name);
          return this->StringVectorProperty->SetElement(i+1, value);
          }
        }
      // If we didn't find any empty spots, append to the vector
      this->StringVectorProperty->SetElement(numElems, name);
      return this->StringVectorProperty->SetElement(numElems+1, value);
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetSelectionMinimum(unsigned int idx)
{
  if (this->StringListRangeDomain)
    {
    int exists=0;
    int min = this->StringListRangeDomain->GetMinimum(idx, exists);
    if (exists)
      {
      sprintf(this->Minimum, "%d", min);
      return this->Minimum;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyAdaptor::GetSelectionMaximum(unsigned int idx)
{
  if (this->StringListRangeDomain)
    {
    int exists=0;
    int max = this->StringListRangeDomain->GetMaximum(idx, exists);
    if (exists)
      {
      sprintf(this->Maximum, "%d", max);
      return this->Maximum;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMPropertyAdaptor::InitializePropertyFromInformation()
{
  if (this->DoubleVectorProperty)
    {
    vtkSMDoubleVectorProperty* ip = vtkSMDoubleVectorProperty::SafeDownCast(
      this->DoubleVectorProperty->GetInformationProperty());
    if (ip)
      {
      unsigned int numElems = ip->GetNumberOfElements();
      this->DoubleVectorProperty->SetNumberOfElements(numElems);
      this->DoubleVectorProperty->SetElements(ip->GetElements());
      }
    }
  if (this->IdTypeVectorProperty)
    {
    vtkSMIdTypeVectorProperty* ip = vtkSMIdTypeVectorProperty::SafeDownCast(
      this->IdTypeVectorProperty->GetInformationProperty());
    if (ip)
      {
      unsigned int numElems = ip->GetNumberOfElements();
      this->IdTypeVectorProperty->SetNumberOfElements(numElems);
      for (unsigned int i=0; i<numElems; i++)
        {
        this->IdTypeVectorProperty->SetElement(i, ip->GetElement(i));
        }
      }
    }
  if (this->IntVectorProperty)
    {
    vtkSMIntVectorProperty* ip = vtkSMIntVectorProperty::SafeDownCast(
      this->IntVectorProperty->GetInformationProperty());
    if (ip)
      {
      unsigned int numElems = ip->GetNumberOfElements();
      this->IntVectorProperty->SetNumberOfElements(numElems);
      this->IntVectorProperty->SetElements(ip->GetElements());
      }
    }
  if (this->StringVectorProperty)
    {
    vtkSMStringVectorProperty* ip = vtkSMStringVectorProperty::SafeDownCast(
      this->StringVectorProperty->GetInformationProperty());
    if (ip)
      {
      unsigned int numElems = ip->GetNumberOfElements();
      this->StringVectorProperty->SetNumberOfElements(numElems);
      for (unsigned int i=0; i<numElems; i++)
        {
        this->StringVectorProperty->SetElement(i, ip->GetElement(i));
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMPropertyAdaptor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Property: ";
  if ( this->Property )
    {
    os << this->Property->GetClassName() << " (" << this->Property << ")" << endl;
    this->Property->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(null)" << endl;
    }
  os << indent << "Domains: " << endl;
#define PRINT_DOMAIN(type) \
  if ( this->type##Domain ) \
    { \
    os << indent << #type " domain: " << this->type##Domain << endl; \
    this->type##Domain->PrintSelf(os, indent.GetNextIndent()); \
    }
  PRINT_DOMAIN(Boolean);
  PRINT_DOMAIN(DoubleRange);
  PRINT_DOMAIN(Enumeration);
  PRINT_DOMAIN(IntRange);
  PRINT_DOMAIN(ProxyGroup);
  PRINT_DOMAIN(StringList);
  PRINT_DOMAIN(StringListRange);
}
