/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntRangeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIntRangeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMIntVectorProperty.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMIntRangeDomain);
vtkCxxRevisionMacro(vtkSMIntRangeDomain, "1.1");

struct vtkSMIntRangeDomainInternals
{
  struct EntryType
  {
    int Min;
    int Max;
    int MinSet;
    int MaxSet;
    
    EntryType() : Min(0), Max(0), MinSet(0), MaxSet(0) {}
  };
  vtkstd::vector<EntryType> Entries;
};

//---------------------------------------------------------------------------
vtkSMIntRangeDomain::vtkSMIntRangeDomain()
{
  this->IRInternals = new vtkSMIntRangeDomainInternals;
}

//---------------------------------------------------------------------------
vtkSMIntRangeDomain::~vtkSMIntRangeDomain()
{
  delete this->IRInternals;
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::IsInDomain(vtkSMProperty* property)
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
      if (!this->IsInDomain(i, ip->GetElement(i)))
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::IsInDomain(unsigned int idx, int val)
{
  if (idx >= this->IRInternals->Entries.size())
    {
    return 1;
    }
  if ( this->IRInternals->Entries[idx].MinSet &&
       val < this->IRInternals->Entries[idx].Min )
    {
    return 0;
    }

  if ( this->IRInternals->Entries[idx].MaxSet &&
       val > this->IRInternals->Entries[idx].Max )
    {
    return 0;
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::GetMinimum(unsigned int idx, int& exists)
{
  exists = 0;
  if (idx >= this->IRInternals->Entries.size())
    {
    return 0;
    }
  if (this->IRInternals->Entries[idx].MinSet)
    {
    exists=1;
    return this->IRInternals->Entries[idx].Min;
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::GetMaximum(unsigned int idx, int& exists)
{
  exists = 0;
  if (idx >= this->IRInternals->Entries.size())
    {
    return 0;
    }
  if (this->IRInternals->Entries[idx].MaxSet)
    {
    exists=1;
    return this->IRInternals->Entries[idx].Max;
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::AddMinimum(unsigned int idx, int val)
{
  this->SetEntry(idx, vtkSMIntRangeDomain::MIN, 1, val);
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::RemoveMinimum(unsigned int idx)
{
  this->SetEntry(idx, vtkSMIntRangeDomain::MIN, 0, 0);
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::AddMaximum(unsigned int idx, int val)
{
  this->SetEntry(idx, vtkSMIntRangeDomain::MAX, 1, val);
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::RemoveMaximum(unsigned int idx)
{
  this->SetEntry(idx, vtkSMIntRangeDomain::MAX, 0, 0);
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::SetEntry(
  unsigned int idx, int minOrMax, int set, int value)
{
  if (idx >= this->IRInternals->Entries.size())
    {
    this->IRInternals->Entries.resize(idx+1);
    }
  if (minOrMax == MIN)
    {
    if (set)
      {
      this->IRInternals->Entries[idx].MinSet = 1;
      this->IRInternals->Entries[idx].Min = value;
      }
    else
      {
      this->IRInternals->Entries[idx].MinSet = 0;
      }
    }
  else
    {
    if (set)
      {
      this->IRInternals->Entries[idx].MaxSet = 1;
      this->IRInternals->Entries[idx].Max = value;
      }
    else
      {
      this->IRInternals->Entries[idx].MaxSet = 0;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::ReadXMLAttributes(vtkPVXMLElement* /*element*/)
{
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
