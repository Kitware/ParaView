/*=========================================================================

  Program:   ParaView
  Module:    vtkSMEnumerationDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMEnumerationDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMIntVectorProperty.h"

#include <sstream>
#include <vector>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMEnumerationDomain);

struct vtkSMEnumerationDomainInternals
{
  struct EntryType
  {
    EntryType(vtkStdString text, int value)
      : Text(text)
      , Value(value)
    {
    }
    vtkStdString Text;
    int Value;
  };

  typedef std::vector<EntryType> EntriesType;
  EntriesType Entries;
};

//---------------------------------------------------------------------------
vtkSMEnumerationDomain::vtkSMEnumerationDomain()
{
  this->EInternals = new vtkSMEnumerationDomainInternals;
}

//---------------------------------------------------------------------------
vtkSMEnumerationDomain::~vtkSMEnumerationDomain()
{
  delete this->EInternals;
}

//---------------------------------------------------------------------------
unsigned int vtkSMEnumerationDomain::GetNumberOfEntries()
{
  return static_cast<unsigned int>(this->EInternals->Entries.size());
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::GetEntryValue(unsigned int idx)
{
  if (idx >= static_cast<unsigned int>(this->EInternals->Entries.size()))
  {
    vtkErrorMacro("Invalid idx: " << idx);
    return 0;
  }
  return this->EInternals->Entries[idx].Value;
}

//---------------------------------------------------------------------------
const char* vtkSMEnumerationDomain::GetEntryText(unsigned int idx)
{
  if (idx >= static_cast<unsigned int>(this->EInternals->Entries.size()))
  {
    vtkErrorMacro("Invalid idx: " << idx);
    return NULL;
  }
  return this->EInternals->Entries[idx].Text.c_str();
}

//---------------------------------------------------------------------------
const char* vtkSMEnumerationDomain::GetEntryTextForValue(int value)
{
  unsigned int idx = 0;
  if (!this->IsInDomain(value, idx))
  {
    return NULL;
  }
  return this->GetEntryText(idx);
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::HasEntryText(const char* text)
{
  int valid;
  this->GetEntryValue(text, valid);
  return valid;
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::GetEntryValueForText(const char* text)
{
  int valid;
  return this->GetEntryValue(text, valid);
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::GetEntryValue(const char* text, int& valid)
{
  valid = 0;

  if (!text)
  {
    return -1;
  }

  vtkSMEnumerationDomainInternals::EntriesType::iterator iter = this->EInternals->Entries.begin();
  for (; iter != this->EInternals->Entries.end(); ++iter)
  {
    if (iter->Text == text)
    {
      valid = 1;
      return iter->Value;
    }
  }

  return -1;
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
  {
    return 1;
  }

  if (!property)
  {
    return 0;
  }
  vtkSMIntVectorProperty* ip = vtkSMIntVectorProperty::SafeDownCast(property);
  if (ip)
  {
    unsigned int numElems = ip->GetNumberOfUncheckedElements();
    for (unsigned int i = 0; i < numElems; i++)
    {
      unsigned int idx;
      if (!this->IsInDomain(ip->GetUncheckedElement(i), idx))
      {
        return 0;
      }
    }
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::IsInDomain(int val, unsigned int& idx)
{
  unsigned int numEntries = this->GetNumberOfEntries();
  if (numEntries == 0)
  {
    return 0;
  }

  for (unsigned int i = 0; i < numEntries; i++)
  {
    if (val == this->GetEntryValue(i))
    {
      idx = i;
      return 1;
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);
  unsigned int size = this->GetNumberOfEntries();
  for (unsigned int i = 0; i < size; i++)
  {
    vtkPVXMLElement* entryElem = vtkPVXMLElement::New();
    entryElem->SetName("Entry");
    entryElem->AddAttribute("value", this->GetEntryValue(i));
    entryElem->AddAttribute("text", this->GetEntryText(i));
    domainElement->AddNestedElement(entryElem);
    entryElem->Delete();
  }
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::AddEntry(const char* text, int value)
{
  this->EInternals->Entries.push_back(vtkSMEnumerationDomainInternals::EntryType(text, value));
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::RemoveAllEntries()
{
  this->EInternals->Entries.erase(
    this->EInternals->Entries.begin(), this->EInternals->Entries.end());
  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  // Loop over the top-level elements.
  unsigned int i;
  for (i = 0; i < element->GetNumberOfNestedElements(); ++i)
  {
    vtkPVXMLElement* selement = element->GetNestedElement(i);
    if (strcmp("Entry", selement->GetName()) != 0)
    {
      continue;
    }
    const char* text = selement->GetAttribute("text");
    if (!text)
    {
      vtkErrorMacro(<< "Can not find required attribute: text. "
                    << "Can not parse domain xml.");
      return 0;
    }

    int value;
    if (!selement->GetScalarAttribute("value", &value))
    {
      vtkErrorMacro(<< "Can not find required attribute: text. "
                    << "Can not parse domain xml.");
      return 0;
    }

    this->AddEntry(text, value);
  }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::Update(vtkSMProperty* prop)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (ivp && ivp->GetInformationOnly())
  {
    this->RemoveAllEntries();
    unsigned int max = ivp->GetNumberOfElements();
    for (unsigned int cc = 0; cc < max; ++cc)
    {
      std::ostringstream stream;
      stream << ivp->GetElement(cc);
      this->AddEntry(stream.str().c_str(), ivp->GetElement(cc));
    }
    this->InvokeModified();
  }
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (ivp && this->GetNumberOfEntries() > 0)
  {
    unsigned int idx = 0;
    if (!this->IsInDomain(ivp->GetDefaultValue(0), idx))
    {
      if (use_unchecked_values)
      {
        ivp->SetUncheckedElement(0, this->GetEntryValue(0));
      }
      else
      {
        ivp->SetElement(0, this->GetEntryValue(0));
      }
      return 1;
    }
  }
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
