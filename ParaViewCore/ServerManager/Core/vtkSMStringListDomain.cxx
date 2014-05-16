/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStringListDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStringVectorProperty.h"

#include <vector>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMStringListDomain);

struct vtkSMStringListDomainInternals
{
  std::vector<vtkStdString> Strings;
};

//---------------------------------------------------------------------------
vtkSMStringListDomain::vtkSMStringListDomain()
{
  this->SLInternals = new vtkSMStringListDomainInternals;
}

//---------------------------------------------------------------------------
vtkSMStringListDomain::~vtkSMStringListDomain()
{
  delete this->SLInternals;
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::SetStrings(const std::vector<vtkStdString>& strings)
{
  if (this->SLInternals->Strings != strings)
    {
    this->SLInternals->Strings = strings;
    this->DomainModified();
    }
}

//---------------------------------------------------------------------------
const std::vector<vtkStdString>& vtkSMStringListDomain::GetStrings()
{
  return this->SLInternals->Strings;
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringListDomain::GetNumberOfStrings()
{
  return static_cast<unsigned int>(this->SLInternals->Strings.size());
}

//---------------------------------------------------------------------------
const char* vtkSMStringListDomain::GetString(unsigned int idx)
{
  return this->SLInternals->Strings[idx].c_str();
}

//---------------------------------------------------------------------------
int vtkSMStringListDomain::IsInDomain(vtkSMProperty* property)
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
    for (unsigned int i=0; i<numElems; i++)
      {
      unsigned int idx;
      if (!this->IsInDomain(sp->GetUncheckedElement(i), idx))
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMStringListDomain::IsInDomain(const char* val, unsigned int& idx)
{
  unsigned int numStrings = this->GetNumberOfStrings();
  if (numStrings == 0)
    {
    return 1;
    }

  for (unsigned int i=0; i<numStrings; i++)
    {
    if (strcmp(val, this->GetString(i)) == 0)
      {
      idx = i;
      return 1;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::Update(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp && svp->GetInformationOnly())
    {
    std::vector<vtkStdString> values;
    unsigned int numStrings = svp->GetNumberOfElements();
    if (svp->GetNumberOfElementsPerCommand()==2)
      {
      // if the information property is something like a array-status-info
      // property on readers where the first value is the array name while the
      // second is it's status.
      for (unsigned int i=0; i<numStrings; i+=2)
        {
        values.push_back(svp->GetElement(i));
        }
      }
    else
      {
      for (unsigned int i=0; i<numStrings; i++)
        {
        values.push_back(svp->GetElement(i));
        }
      }
    this->SetStrings(values);
    }
}

//---------------------------------------------------------------------------
int vtkSMStringListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  int retVal = this->Superclass::ReadXMLAttributes(prop, element);
  if (!retVal)
    {
    return 0;
    }

  std::vector<vtkStdString> values; 
  // Loop over the top-level elements.
  unsigned int i;
  for(i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* selement = element->GetNestedElement(i);
    if ( strcmp("String", selement->GetName()) != 0 )
      {
      continue;
      }
    const char* value = selement->GetAttribute("value");
    if (!value)
      {
      vtkErrorMacro(<< "Can not find required attribute: value. "
                    << "Can not parse domain xml.");
      return 0;
      }

    values.push_back(value);
    }
  this->SetStrings(values);
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);

  unsigned int size = this->GetNumberOfStrings();
  for(unsigned int i=0; i<size; i++)
    {
    vtkPVXMLElement* stringElem = vtkPVXMLElement::New();
    stringElem->SetName("String");
    stringElem->AddAttribute("text", this->GetString(i));
    domainElement->AddNestedElement(stringElem);
    stringElem->Delete();
    }

}

//---------------------------------------------------------------------------
int vtkSMStringListDomain::LoadState(vtkPVXMLElement* domainElement, 
    vtkSMProxyLocator* loader)
{
  this->Superclass::LoadState(domainElement, loader);
  
  std::vector<vtkStdString> values; 
  unsigned int numElems = domainElement->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* child = domainElement->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "String") == 0)
      {
      const char* text = child->GetAttribute("text");
      if (text)
        {
        values.push_back(text);
        }
      }
    }

  this->SetStrings(values);
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::SetAnimationValue(vtkSMProperty *prop, int idx,
                                              double value)
{
  if (!prop)
    {
    return;
    }

  vtkSMStringVectorProperty *svp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp)
    {
    svp->SetElement(idx, this->GetString((int)(floor(value))));
    }
}

//---------------------------------------------------------------------------
int vtkSMStringListDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = 
    vtkSMStringVectorProperty::SafeDownCast(prop);
  unsigned int num_string = this->GetNumberOfStrings();
  if (svp && num_string > 0)
    {
    if (svp->GetNumberOfElements() == 1 && 
      !svp->GetRepeatCommand() )
      {
      const char* defaultValue = svp->GetDefaultValue(0);
      unsigned int temp;
      if (defaultValue && this->IsInDomain(defaultValue, temp))
        {
        svp->SetElement(0, defaultValue);
        }
      else
        {
        svp->SetElement(0, this->GetString(0));
        }

      return 1;
      }
    if (svp->GetRepeatCommand() && svp->GetNumberOfElementsPerCommand()==1)
      {
      svp->SetNumberOfElements(num_string);
      for (unsigned int cc=0; cc < num_string; cc++)
        {
        svp->SetElement(cc, this->GetString(cc));
        }
      return 1;
      }
    }

  return this->Superclass::SetDefaultValues(prop);
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  unsigned int size = this->GetNumberOfStrings();
  os << indent << "Strings(" << size << "):" << endl;
  for(unsigned int i=0; i<size; i++)
    {
    os << indent.GetNextIndent() 
          << i << ". " << this->GetString(i) << endl;
    }
}
