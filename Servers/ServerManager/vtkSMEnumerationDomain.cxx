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

#include <vtkstd/vector>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMEnumerationDomain);
vtkCxxRevisionMacro(vtkSMEnumerationDomain, "1.1");

struct vtkSMEnumerationDomainInternals
{
  struct EntryType
  {
    EntryType(vtkStdString text, int value) : Text(text), Value(value) {}
    vtkStdString Text;
    int Value;
  };
  vtkstd::vector<EntryType> Entries;
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
  return this->EInternals->Entries.size();
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::GetEntryValue(unsigned int idx)
{
  return this->EInternals->Entries[idx].Value;
}

//---------------------------------------------------------------------------
const char* vtkSMEnumerationDomain::GetEntryText(unsigned int idx)
{
  return this->EInternals->Entries[idx].Text.c_str();
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::IsInDomain(vtkSMProperty* property)
{
  if (!property)
    {
    return 0;
    }
  vtkSMIntVectorProperty* ip = vtkSMIntVectorProperty::SafeDownCast(property);
  if (ip)
    {
    unsigned int numElems = ip->GetNumberOfElements();
    for (unsigned int i=0; i<numElems; i++)
      {
      unsigned int idx;
      if (!this->IsInDomain(ip->GetElement(i), idx))
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

  for (unsigned int i=0; i<numEntries; i++)
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
int vtkSMEnumerationDomain::ReadXMLAttributes(vtkPVXMLElement* /*element*/)
{
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
