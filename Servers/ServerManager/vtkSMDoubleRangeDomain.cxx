/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleRangeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDoubleRangeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMDoubleRangeDomain);
vtkCxxRevisionMacro(vtkSMDoubleRangeDomain, "1.1");

struct vtkSMDoubleRangeDomainInternals
{
  struct EntryType
  {
    double Min;
    double Max;
    int MinSet;
    int MaxSet;
    
    EntryType() : Min(0), Max(0), MinSet(0), MaxSet(0) {}
  };
  vtkstd::vector<EntryType> Entries;
};

//---------------------------------------------------------------------------
vtkSMDoubleRangeDomain::vtkSMDoubleRangeDomain()
{
  this->DRInternals = new vtkSMDoubleRangeDomainInternals;
}

//---------------------------------------------------------------------------
vtkSMDoubleRangeDomain::~vtkSMDoubleRangeDomain()
{
  delete this->DRInternals;
}

//---------------------------------------------------------------------------
int vtkSMDoubleRangeDomain::IsInDomain(vtkSMProperty* property)
{
  if (!property)
    {
    return 0;
    }
  vtkSMDoubleVectorProperty* dp = 
    vtkSMDoubleVectorProperty::SafeDownCast(property);
  if (dp)
    {
    unsigned int numElems = dp->GetNumberOfElements();
    for (unsigned int i=0; i<numElems; i++)
      {
      if (!this->IsInDomain(i, dp->GetElement(i)))
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMDoubleRangeDomain::IsInDomain(unsigned int idx, double val)
{
  if (idx >= this->DRInternals->Entries.size())
    {
    return 1;
    }
  if ( this->DRInternals->Entries[idx].MinSet &&
       val < this->DRInternals->Entries[idx].Min )
    {
    return 0;
    }

  if ( this->DRInternals->Entries[idx].MaxSet &&
       val > this->DRInternals->Entries[idx].Max )
    {
    return 0;
    }

  return 1;
}

//---------------------------------------------------------------------------
double vtkSMDoubleRangeDomain::GetMinimum(unsigned int idx, int& exists)
{
  exists = 0;
  if (idx >= this->DRInternals->Entries.size())
    {
    return 0;
    }
  if (this->DRInternals->Entries[idx].MinSet)
    {
    exists=1;
    return this->DRInternals->Entries[idx].Min;
    }
  return 0;
}

//---------------------------------------------------------------------------
double vtkSMDoubleRangeDomain::GetMaximum(unsigned int idx, int& exists)
{
  exists = 0;
  if (idx >= this->DRInternals->Entries.size())
    {
    return 0;
    }
  if (this->DRInternals->Entries[idx].MaxSet)
    {
    exists=1;
    return this->DRInternals->Entries[idx].Max;
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::AddMinimum(unsigned int idx, double val)
{
  this->SetEntry(idx, vtkSMDoubleRangeDomain::MIN, 1, val);
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::RemoveMinimum(unsigned int idx)
{
  this->SetEntry(idx, vtkSMDoubleRangeDomain::MIN, 0, 0);
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::AddMaximum(unsigned int idx, double val)
{
  this->SetEntry(idx, vtkSMDoubleRangeDomain::MAX, 1, val);
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::RemoveMaximum(unsigned int idx)
{
  this->SetEntry(idx, vtkSMDoubleRangeDomain::MAX, 0, 0);
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::SetEntry(
  unsigned int idx, int minOrMax, int set, double value)
{
  if (idx >= this->DRInternals->Entries.size())
    {
    this->DRInternals->Entries.resize(idx+1);
    }
  if (minOrMax == MIN)
    {
    if (set)
      {
      this->DRInternals->Entries[idx].MinSet = 1;
      this->DRInternals->Entries[idx].Min = value;
      }
    else
      {
      this->DRInternals->Entries[idx].MinSet = 0;
      }
    }
  else
    {
    if (set)
      {
      this->DRInternals->Entries[idx].MaxSet = 1;
      this->DRInternals->Entries[idx].Max = value;
      }
    else
      {
      this->DRInternals->Entries[idx].MaxSet = 0;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMDoubleRangeDomain::ReadXMLAttributes(vtkPVXMLElement* /*element*/)
{
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
