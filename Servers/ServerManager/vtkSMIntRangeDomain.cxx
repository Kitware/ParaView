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
vtkCxxRevisionMacro(vtkSMIntRangeDomain, "1.8");

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
void vtkSMIntRangeDomain::SaveState(
  const char* name, ostream* file, vtkIndent indent)
{
  *file << indent 
        << "<Domain name=\"" << this->XMLName << "\" id=\"" << name << "\">"
        << endl;
  unsigned int size = this->IRInternals->Entries.size();
  unsigned int i;
  for(i=0; i<size; i++)
    {
    if (this->IRInternals->Entries[i].MinSet)
      {
      *file << indent.GetNextIndent() 
            << "<Min index=\"" << i << "\" value=\"" 
            << this->IRInternals->Entries[i].Min
            << "\"/>" << endl;
      }
    }
  for(i=0; i<size; i++)
    {
    if (this->IRInternals->Entries[i].MaxSet)
      {
      *file << indent.GetNextIndent() 
            << "<Max index=\"" << i << "\" value=\"" 
            << this->IRInternals->Entries[i].Max
            << "\"/>" << endl;
      }
    }
      
  *file << indent
        << "</Domain>" << endl;
    
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

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::Update(vtkSMProperty* prop)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (ivp)
    {
    this->RemoveAllMinima();
    this->RemoveAllMaxima();

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
void vtkSMIntRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
