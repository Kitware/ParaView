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
vtkCxxRevisionMacro(vtkSMDoubleRangeDomain, "1.9");

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
void vtkSMDoubleRangeDomain::SaveState(
  const char* name, ostream* file, vtkIndent indent)
{
  *file << indent 
        << "<Domain name=\"" << this->XMLName << "\" id=\"" << name << "\">"
        << endl;
  unsigned int size = this->DRInternals->Entries.size();
  unsigned int i;
  for(i=0; i<size; i++)
    {
    if (this->DRInternals->Entries[i].MinSet)
      {
      *file << indent.GetNextIndent() 
            << "<Min index=\"" << i << "\" value=\"" 
            << this->DRInternals->Entries[i].Min
            << "\"/>" << endl;
      }
    }
  for(i=0; i<size; i++)
    {
    if (this->DRInternals->Entries[i].MaxSet)
      {
      *file << indent.GetNextIndent() 
            << "<Max index=\"" << i << "\" value=\"" 
            << this->DRInternals->Entries[i].Max
            << "\"/>" << endl;
      }
    }
      
  *file << indent
        << "</Domain>" << endl;
    
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

  return 1;
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
void vtkSMDoubleRangeDomain::SetAnimationValueInBatch(
  ofstream *file, vtkSMProperty *property, vtkClientServerID sourceID,
  int idx, double value)
{
  if (!file || !property || !sourceID.ID)
    {
    return;
    }

  *file << "  [$pvTemp" << sourceID << " GetProperty "
        << property->GetXMLName() << "] SetElement " << idx << " " << value
        << endl;
  *file << "  $pvTemp" << sourceID << " UpdateVTKObjects" << endl;
}

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
