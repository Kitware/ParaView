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

struct vtkSMDoubleRangeDomainInternals
{
  struct EntryType
  {
    double Min;
    double Max;
    double Resolution;
    int MinSet;
    int MaxSet;
    int ResolutionSet;

    EntryType() : Min(0), Max(0), Resolution(0), MinSet(0), MaxSet(0), ResolutionSet(0) {}
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
  if (this->IsOptional)
    {
    return 1;
    }

  if (!property)
    {
    return 0;
    }
  vtkSMDoubleVectorProperty* dp = 
    vtkSMDoubleVectorProperty::SafeDownCast(property);
  if (dp)
    {
    unsigned int numElems = dp->GetNumberOfUncheckedElements();
    for (unsigned int i=0; i<numElems; i++)
      {
      if (!this->IsInDomain(i, dp->GetUncheckedElement(i)))
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
  // User has not put any condition so domains is always valid
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
  
  if ( this->DRInternals->Entries[idx].ResolutionSet )
    {
    // check if value is a multiple of resolution + min:
    int exists;
    double min = this->GetMinimum(idx,exists); //set to 0 if necesseary
    double res = this->DRInternals->Entries[idx].Resolution;
    int multi = (int)((val - min) / res);
    return (multi*res + min - val) == 0.;
    }
  //else the resolution is not taken into account

  return 1;
}

//---------------------------------------------------------------------------
unsigned int vtkSMDoubleRangeDomain::GetNumberOfEntries()
{
  return this->DRInternals->Entries.size();
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::SetNumberOfEntries(unsigned int size)
{
  this->DRInternals->Entries.resize(size);
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
int vtkSMDoubleRangeDomain::GetMinimumExists(unsigned int idx)
{
  if (idx >= this->DRInternals->Entries.size())
    {
    return 0;
    }
  return this->DRInternals->Entries[idx].MinSet;
}

//---------------------------------------------------------------------------
int vtkSMDoubleRangeDomain::GetMaximumExists(unsigned int idx)
{
  if (idx >= this->DRInternals->Entries.size())
    {
    return 0;
    }
  return this->DRInternals->Entries[idx].MaxSet;
}


//---------------------------------------------------------------------------
double vtkSMDoubleRangeDomain::GetMaximum(unsigned int idx)
{
  if (!this->GetMaximumExists(idx))
    {
    return 0;
    }
  return this->DRInternals->Entries[idx].Max;
}

//---------------------------------------------------------------------------
double vtkSMDoubleRangeDomain::GetMinimum(unsigned int idx)
{
  if (!this->GetMinimumExists(idx))
    {
    return 0;
    }
  return this->DRInternals->Entries[idx].Min;
}

//---------------------------------------------------------------------------
double vtkSMDoubleRangeDomain::GetResolution(unsigned int idx, int& exists)
{
  exists = 0;
  if (idx >= this->DRInternals->Entries.size())
    {
    return 0;
    }
  if (this->DRInternals->Entries[idx].ResolutionSet)
    {
    exists=1;
    return this->DRInternals->Entries[idx].Resolution;
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMDoubleRangeDomain::GetResolutionExists(unsigned int idx)
{
  if (idx >= this->DRInternals->Entries.size())
    {
    return 0;
    }
  return this->DRInternals->Entries[idx].ResolutionSet;
}

//---------------------------------------------------------------------------
double vtkSMDoubleRangeDomain::GetResolution(unsigned int idx)
{
  if (!this->GetResolutionExists(idx))
    {
    return 0;
    }
  return this->DRInternals->Entries[idx].Resolution;
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
void vtkSMDoubleRangeDomain::RemoveAllMinima()
{
  unsigned int numEntries = this->GetNumberOfEntries();
  for(unsigned int idx=0; idx<numEntries; idx++)
    {
    this->SetEntry(idx, vtkSMDoubleRangeDomain::MIN, 0, 0);
    }
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
void vtkSMDoubleRangeDomain::RemoveAllMaxima()
{
  unsigned int numEntries = this->GetNumberOfEntries();
  for(unsigned int idx=0; idx<numEntries; idx++)
    {
    this->SetEntry(idx, vtkSMDoubleRangeDomain::MAX, 0, 0);
    }
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::AddResolution(unsigned int idx, double val)
{
  this->SetEntry(idx, vtkSMDoubleRangeDomain::RESOLUTION, 1, val);
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::RemoveResolution(unsigned int idx)
{
  this->SetEntry(idx, vtkSMDoubleRangeDomain::RESOLUTION, 0, 0);
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::RemoveAllResolutions()
{
  unsigned int numEntries = this->GetNumberOfEntries();
  for(unsigned int idx=0; idx<numEntries; idx++)
    {
    this->SetEntry(idx, vtkSMDoubleRangeDomain::RESOLUTION, 0, 0);
    }
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::SetEntry(
  unsigned int idx, int minOrMaxOrRes, int set, double value)
{
  if (idx >= this->DRInternals->Entries.size())
    {
    this->DRInternals->Entries.resize(idx+1);
    }
  if (minOrMaxOrRes == MIN)
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
  else if(minOrMaxOrRes == MAX)
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
  else //if (minOrMaxOrRes == RESOLUTION)
    {
    if (set)
      {
      this->DRInternals->Entries[idx].ResolutionSet = 1;
      this->DRInternals->Entries[idx].Resolution = value;
      }
    else
      {
      this->DRInternals->Entries[idx].ResolutionSet = 0;
      }
    }
  // TODO: we may want to invoke this event only when the values 
  // actually change.
  this->InvokeModified();
}

//---------------------------------------------------------------------------
int vtkSMDoubleRangeDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);
  const int MAX_NUM = 128;
  double values[MAX_NUM];

  int numRead = element->GetVectorAttribute("min",
                                            MAX_NUM,
                                            values);
  if (numRead > 0)
    {
    for (unsigned int i=0; i<(unsigned int)numRead; i++)
      {
      this->AddMinimum(i, values[i]);
      }
    }

  numRead = element->GetVectorAttribute("max",
                                        MAX_NUM,
                                        values);
  if (numRead > 0)
    {
    for (unsigned int i=0; i<(unsigned int)numRead; i++)
      {
      this->AddMaximum(i, values[i]);
      }
    }

  numRead = element->GetVectorAttribute("resolution",
                                        MAX_NUM,
                                        values);
  if (numRead > 0)
    {
    for (unsigned int i=0; i<(unsigned int)numRead; i++)
      {
      this->AddResolution(i, values[i]);
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::Update(vtkSMProperty* prop)
{
  vtkSMDoubleVectorProperty* dvp = 
      vtkSMDoubleVectorProperty::SafeDownCast(prop);
  if (dvp && dvp->GetInformationOnly())
    {
    this->RemoveAllMinima();
    this->RemoveAllMaxima();
    this->RemoveAllResolutions();

    unsigned int numEls = dvp->GetNumberOfElements();
    for (unsigned int i=0; i<numEls; i++)
      {
      if ( i % 2 == 0)
        {
        this->AddMinimum(i/2, dvp->GetElement(i));
        }
      else
        {
        this->AddMaximum(i/2, dvp->GetElement(i));
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::SetAnimationValue(vtkSMProperty *property,
                                               int idx, double value)
{
  if (!property)
    {
    return;
    }
  
  vtkSMDoubleVectorProperty *dvp =
    vtkSMDoubleVectorProperty::SafeDownCast(property);
  if (dvp)
    {
    dvp->SetElement(idx, value);
    }
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
