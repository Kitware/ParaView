/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringListRangeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStringListRangeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMStringListRangeDomain);

//---------------------------------------------------------------------------
vtkSMStringListRangeDomain::vtkSMStringListRangeDomain()
{
  this->IRDomain = vtkSMIntRangeDomain::New();
  this->BDomain = vtkSMBooleanDomain::New();
  this->SLDomain = vtkSMStringListDomain::New();

  this->IntDomainMode = vtkSMStringListRangeDomain::RANGE;
}

//---------------------------------------------------------------------------
vtkSMStringListRangeDomain::~vtkSMStringListRangeDomain()
{
  this->IRDomain->Delete();
  this->BDomain->Delete();
  this->SLDomain->Delete();
}

//---------------------------------------------------------------------------
int vtkSMStringListRangeDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!property)
    {
    return 0;
    }

  vtkSMStringVectorProperty* sp = 
    vtkSMStringVectorProperty::SafeDownCast(property);
  if (sp)
    {
    unsigned int numElems = sp->GetNumberOfUncheckedElements();
    unsigned int i;
    for (i=0; i<numElems; i+=2)
      {
      unsigned int idx;
      if (!this->SLDomain->IsInDomain(sp->GetUncheckedElement(i), idx))
        {
        return 0;
        }
      }
    for (i=1; i<numElems; i+=2)
      {
      switch (this->IntDomainMode)
        {
        case vtkSMStringListRangeDomain::RANGE:
          if (!this->IRDomain->IsInDomain(i/2, atoi(sp->GetUncheckedElement(i))))
            {
            return 0;
            }
          break;
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringListRangeDomain::GetNumberOfStrings()
{
  return this->SLDomain->GetNumberOfStrings();
}

//---------------------------------------------------------------------------
const char* vtkSMStringListRangeDomain::GetString(unsigned int idx)
{
  return this->SLDomain->GetString(idx);
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringListRangeDomain::AddString(const char* string)
{
  return this->SLDomain->AddString(string);
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::RemoveString(const char* string)
{
  this->SLDomain->RemoveString(string);
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::RemoveAllStrings()
{
  this->SLDomain->RemoveAllStrings();
}

//---------------------------------------------------------------------------
int vtkSMStringListRangeDomain::GetMinimum(unsigned int idx, int& exists)
{
  return this->IRDomain->GetMinimum(idx, exists);
}

//---------------------------------------------------------------------------
int vtkSMStringListRangeDomain::GetMaximum(unsigned int idx, int& exists)
{
  return this->IRDomain->GetMaximum(idx, exists);
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::AddMinimum(unsigned int idx, int value)
{
  this->IRDomain->AddMinimum(idx, value);
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::RemoveMinimum(unsigned int idx)
{
  this->IRDomain->RemoveMinimum(idx);
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::RemoveAllMinima()
{
  this->IRDomain->RemoveAllMinima();
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::AddMaximum(unsigned int idx, int value)
{
  this->IRDomain->AddMaximum(idx, value);
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::RemoveMaximum(unsigned int idx)
{
  this->IRDomain->RemoveMaximum(idx);
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::RemoveAllMaxima()
{
  this->IRDomain->RemoveAllMaxima();
}

//---------------------------------------------------------------------------
int vtkSMStringListRangeDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  int retVal = this->Superclass::ReadXMLAttributes(prop, element);
  if (!retVal)
    {
    return 0;
    }

  const char* int_domain_mode = element->GetAttribute("int_domain_mode");
  if (int_domain_mode)
    {
    if (strcmp(int_domain_mode, "range") == 0)
      {
      this->SetIntDomainMode(vtkSMStringListRangeDomain::RANGE);
      }
    else if (strcmp(int_domain_mode, "boolean") == 0)
      {
      this->SetIntDomainMode(vtkSMStringListRangeDomain::BOOLEAN);
      }
    else
      {
      vtkErrorMacro("Unknown int_domain_mode: " << int_domain_mode);
      return 0;
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);

  unsigned int i;
  unsigned int size = this->SLDomain->GetNumberOfStrings();
  for(i=0; i<size; i++)
    {
    vtkPVXMLElement* stringElem = vtkPVXMLElement::New();
    stringElem->SetName("String");
    stringElem->AddAttribute("text", this->SLDomain->GetString(i));
    domainElement->AddNestedElement(stringElem);
    stringElem->Delete();
    }

  size = this->IRDomain->GetNumberOfEntries();
  for(i=0; i<size; i++)
    {
    int exists;
    int min = this->IRDomain->GetMinimum(i, exists);
    if (exists)
      {
      vtkPVXMLElement* minElem = vtkPVXMLElement::New();
      minElem->SetName("Min");
      minElem->AddAttribute("index", i);
      minElem->AddAttribute("value", min);
      domainElement->AddNestedElement(minElem);
      minElem->Delete();
      }
    }
  for(i=0; i<size; i++)
    {
    int exists;
    int max = this->IRDomain->GetMaximum(i, exists);
    if (exists)
      {
      vtkPVXMLElement* maxElem = vtkPVXMLElement::New();
      maxElem->SetName("Max");
      maxElem->AddAttribute("index", i);
      maxElem->AddAttribute("value", max);
      domainElement->AddNestedElement(maxElem);
      maxElem->Delete();
      }
    }
      
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::SetAnimationValue(
  vtkSMProperty *property, int idx, double value)
{
  if (!property)
    {
    return;
    }
  
  vtkSMStringVectorProperty *svp = 
    vtkSMStringVectorProperty::SafeDownCast(property);
  if (svp)
    {
    char val[128];
    sprintf(val, "%d", static_cast<int>(floor(value)));
    svp->SetElement(2*idx+1, val);
    }
}

//---------------------------------------------------------------------------
void vtkSMStringListRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "IntDomainMode: " << this->IntDomainMode << endl;
  unsigned int size = this->SLDomain->GetNumberOfStrings();
  os << indent << "Strings(" << size << "):" << endl;
  unsigned int i;
  for(i=0; i<size; i++)
    {
    os << indent.GetNextIndent() 
      << i << ". " << this->SLDomain->GetString(i) << endl;
    }

  size = this->IRDomain->GetNumberOfEntries();
  os << indent << "Min" << endl;
  for(i=0; i<size; i++)
    {
    int exists;
    int min = this->IRDomain->GetMinimum(i, exists);
    if (exists)
      {
      os << indent.GetNextIndent() 
        << i << ". " << min << endl;
      }
    }
  os << indent << "Max" << endl;
  for(i=0; i<size; i++)
    {
    int exists;
    int max = this->IRDomain->GetMaximum(i, exists);
    if (exists)
      {
      os << indent.GetNextIndent() 
        << i << ". " << max << endl;
      }
    }
}
