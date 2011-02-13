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

struct vtkSMIntRangeDomainInternals
{
  struct EntryType
  {
    int Min;
    int Max;
    int Resolution;
    int MinSet;
    int MaxSet;
    int ResolutionSet;

    EntryType() : Min(0), Max(0), Resolution(0), MinSet(0), MaxSet(0), ResolutionSet(0) {}
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
    for (unsigned int i=0; i<numElems; i++)
      {
      if (!this->IsInDomain(i, ip->GetUncheckedElement(i)))
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
  // User has not put any condition so domains is always valid
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

  if ( this->IRInternals->Entries[idx].ResolutionSet )
    {
    // check if value is a multiple of resolution + min:
    int exists;
    int min = this->GetMinimum(idx,exists); //set to 0 if necesseary
    int res = this->IRInternals->Entries[idx].Resolution;
    int multi = (int)((val - min) / res);
    return (multi*res + min - val) == 0.;
    }
  //else the resolution is not taken into account

  return 1;
}

//---------------------------------------------------------------------------
unsigned int vtkSMIntRangeDomain::GetNumberOfEntries()
{
  return this->IRInternals->Entries.size();
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::SetNumberOfEntries(unsigned int size)
{
  this->IRInternals->Entries.resize(size);
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
int vtkSMIntRangeDomain::GetMinimumExists(unsigned int idx)
{
  if (idx >= this->IRInternals->Entries.size())
    {
    return 0;
    }
  return this->IRInternals->Entries[idx].MinSet;
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::GetMaximumExists(unsigned int idx)
{
  if (idx >= this->IRInternals->Entries.size())
    {
    return 0;
    }
  return this->IRInternals->Entries[idx].MaxSet;
}


//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::GetMaximum(unsigned int idx)
{
  if (!this->GetMaximumExists(idx))
    {
    return 0;
    }
  return this->IRInternals->Entries[idx].Max;
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::GetMinimum(unsigned int idx)
{
  if (!this->GetMinimumExists(idx))
    {
    return 0;
    }
  return this->IRInternals->Entries[idx].Min;
}
//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::GetResolution(unsigned int idx, int& exists)
{
  exists = 0;
  if (idx >= this->IRInternals->Entries.size())
    {
    return 0;
    }
  if (this->IRInternals->Entries[idx].ResolutionSet)
    {
    exists=1;
    return this->IRInternals->Entries[idx].Resolution;
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::GetResolutionExists(unsigned int idx)
{
  if (idx >= this->IRInternals->Entries.size())
    {
    return 0;
    }
  return this->IRInternals->Entries[idx].ResolutionSet;
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::GetResolution(unsigned int idx)
{
  if (!this->GetResolutionExists(idx))
    {
    return 0;
    }
  return this->IRInternals->Entries[idx].Resolution;
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
void vtkSMIntRangeDomain::RemoveAllMinima()
{
  unsigned int numEntries = this->GetNumberOfEntries();
  for(unsigned int idx=0; idx<numEntries; idx++)
    {
    this->SetEntry(idx, vtkSMIntRangeDomain::MIN, 0, 0);
    }
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
void vtkSMIntRangeDomain::RemoveAllMaxima()
{
  unsigned int numEntries = this->GetNumberOfEntries();
  for(unsigned int idx=0; idx<numEntries; idx++)
    {
    this->SetEntry(idx, vtkSMIntRangeDomain::MAX, 0, 0);
    }
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::AddResolution(unsigned int idx, int val)
{
  this->SetEntry(idx, vtkSMIntRangeDomain::RESOLUTION, 1, val);
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::RemoveResolution(unsigned int idx)
{
  this->SetEntry(idx, vtkSMIntRangeDomain::RESOLUTION, 0, 0);
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::RemoveAllResolutions()
{
  unsigned int numEntries = this->GetNumberOfEntries();
  for(unsigned int idx=0; idx<numEntries; idx++)
    {
    this->SetEntry(idx, vtkSMIntRangeDomain::RESOLUTION, 0, 0);
    }
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::SetEntry(
  unsigned int idx, int minOrMaxOrRes, int set, int value)
{
  if (idx >= this->IRInternals->Entries.size())
    {
    this->IRInternals->Entries.resize(idx+1);
    }
  if (minOrMaxOrRes == MIN)
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
  else if(minOrMaxOrRes == MAX)
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
  else //if (minOrMaxOrRes == RESOLUTION)
    {
    if (set)
      {
      this->IRInternals->Entries[idx].ResolutionSet = 1;
      this->IRInternals->Entries[idx].Resolution = value;
      }
    else
      {
      this->IRInternals->Entries[idx].ResolutionSet = 0;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);
  const int MAX_NUM = 128;
  int values[MAX_NUM];

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
void vtkSMIntRangeDomain::Update(vtkSMProperty* prop)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (ivp && ivp->GetInformationOnly())
    {
    this->RemoveAllMinima();
    this->RemoveAllMaxima();
    this->RemoveAllResolutions();

    unsigned int numEls = ivp->GetNumberOfElements();
    for (unsigned int i=0; i<numEls; i++)
      {
      if ( i % 2 == 0)
        {
        this->AddMinimum(i/2, ivp->GetElement(i));
        }
      else
        {
        this->AddMaximum(i/2, ivp->GetElement(i));
        }
      }
    this->InvokeModified();
    }
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::SetAnimationValue(vtkSMProperty *property, int idx,
                                            double value)
{
  if (!property)
    {
    return;
    }
  
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(property);
  if (ivp)
    {
    ivp->SetElement(idx, (int)(floor(value)));
    }
}

//---------------------------------------------------------------------------
int vtkSMIntRangeDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (!ivp || this->GetNumberOfRequiredProperties() == 0)
    {
    // If number of required properties is 0, this this domain
    // does not change based on values for some other property
    // in that case, the property default does not depend on the domain.
    return this->Superclass::SetDefaultValues(prop);
    }
  int updated = 0;
  unsigned int numEls = ivp->GetNumberOfElements();
  for (unsigned int i=0; i<numEls; i++)
    {
    if ( i % 2 == 0)
      {
      if (this->GetMinimumExists(i/2))
        {
        ivp->SetElement(i, this->GetMinimum(i/2));
        updated = 1;
        }
      }
    else
      {
      if (this->GetMaximumExists(i/2))
        {
        ivp->SetElement(i, this->GetMaximum(i/2));
        updated = 1;
        }
      }
    }
  return updated;
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
