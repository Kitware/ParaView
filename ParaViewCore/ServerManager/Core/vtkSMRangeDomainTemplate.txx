/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRangeDomainTemplate.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMRangeDomainTemplate_txx
#define vtkSMRangeDomainTemplate_txx

#include "vtkSMRangeDomainTemplate.h"

#include "vtkPVXMLElement.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMVectorProperty.h"

#include <assert.h>
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
template <class T>
vtkSMRangeDomainTemplate<T>::vtkSMRangeDomainTemplate()
{
  this->DefaultDefaultMode = MID;
  this->Resolution = -1;
}

//-----------------------------------------------------------------------------
template <class T>
vtkSMRangeDomainTemplate<T>::~vtkSMRangeDomainTemplate()
{
}

//-----------------------------------------------------------------------------
template <class T>
int vtkSMRangeDomainTemplate<T>::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
  {
    return 1;
  }

  if (property == NULL)
  {
    return 0;
  }

  vtkSMUncheckedPropertyHelper helper(property);
  for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
  {
    if (!this->IsInDomain(cc, helper.GetAs<T>(cc)))
    {
      return 0;
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::IsInDomain(unsigned int idx, T val)
{
  if (idx >= static_cast<unsigned int>(this->Entries.size()))
  {
    return 1;
  }
  if (this->Entries[idx].Valid[0] && val < this->Entries[idx].Value[0])
  {
    return 0;
  }
  if (this->Entries[idx].Valid[1] && val < this->Entries[idx].Value[1])
  {
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
template <class T>
T vtkSMRangeDomainTemplate<T>::GetMinimum(unsigned int idx, int& exists)
{
  exists = 0;
  if (idx < static_cast<unsigned int>(this->Entries.size()) && this->Entries[idx].Valid[0])
  {
    exists = 1;
    return this->Entries[idx].Value[0];
  }
  return 0;
}

//-----------------------------------------------------------------------------
template <class T>
T vtkSMRangeDomainTemplate<T>::GetMaximum(unsigned int idx, int& exists)
{
  exists = 0;
  if (idx < static_cast<unsigned int>(this->Entries.size()) && this->Entries[idx].Valid[1])
  {
    exists = 1;
    return this->Entries[idx].Value[1];
  }
  return 0;
}

//-----------------------------------------------------------------------------
template <class T>
int vtkSMRangeDomainTemplate<T>::GetResolution()
{
  return this->Resolution;
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::GetMinimumExists(unsigned int idx)
{
  return (
    idx < static_cast<unsigned int>(this->Entries.size()) ? this->Entries[idx].Valid[0] : false);
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::GetMaximumExists(unsigned int idx)
{
  return (
    idx < static_cast<unsigned int>(this->Entries.size()) ? this->Entries[idx].Valid[1] : false);
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::GetResolutionExists()
{
  return this->Resolution != -1;
}

//-----------------------------------------------------------------------------
template <class T>
unsigned int vtkSMRangeDomainTemplate<T>::GetNumberOfEntries()
{
  return static_cast<unsigned int>(this->Entries.size());
}

//-----------------------------------------------------------------------------
template <class T>
void vtkSMRangeDomainTemplate<T>::Update(vtkSMProperty* property)
{
  if (property)
  {
    vtkSMUncheckedPropertyHelper helper(property);
    std::vector<vtkEntry> new_entries;

    for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
    {
      unsigned int entry_index = cc / 2;
      // ensure new_entries is of the size
      if (entry_index >= static_cast<unsigned int>(new_entries.size()))
      {
        new_entries.resize(entry_index + 1);
      }

      unsigned int min_max_index = cc % 2;
      new_entries[entry_index].Valid[min_max_index] = true;
      new_entries[entry_index].Value[min_max_index] = helper.GetAs<T>(cc);
    }

    this->SetEntries(new_entries);
  }
}

//-----------------------------------------------------------------------------
template <class T>
void vtkSMRangeDomainTemplate<T>::SetAnimationValue(vtkSMProperty* property, int idx, double value)
{
  if (property)
  {
    vtkSMPropertyHelper(property).Set(idx, value);
  }
}

//-----------------------------------------------------------------------------
template <class T>
int vtkSMRangeDomainTemplate<T>::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  std::vector<vtkEntry> new_entries;
  const int MAX_NUM = 128;
  T values[MAX_NUM];
  int numRead = element->GetVectorAttribute("min", MAX_NUM, values);
  if (numRead > 0)
  {
    if (numRead > static_cast<int>(new_entries.size()))
    {
      new_entries.resize(numRead);
    }
    for (int i = 0; i < numRead; i++)
    {
      new_entries[i].Valid[0] = true;
      new_entries[i].Value[0] = values[i];
    }
  }

  numRead = element->GetVectorAttribute("max", MAX_NUM, values);
  if (numRead > 0)
  {
    if (numRead > static_cast<int>(new_entries.size()))
    {
      new_entries.resize(numRead);
    }
    for (int i = 0; i < numRead; i++)
    {
      new_entries[i].Valid[1] = true;
      new_entries[i].Value[1] = values[i];
    }
  }

  const char* default_mode = element->GetAttribute("default_mode");
  if (default_mode)
  {
    typedef std::vector<std::string> VStrings;
    const VStrings modes = vtksys::SystemTools::SplitString(default_mode, ',');

    this->DefaultModeVector.clear();
    for (VStrings::const_iterator iter = modes.begin(); iter != modes.end(); ++iter)
    {
      if (*iter == "min")
      {
        this->DefaultModeVector.push_back(MIN);
        this->DefaultDefaultMode = MIN;
      }
      else if (*iter == "max")
      {
        this->DefaultModeVector.push_back(MAX);
        this->DefaultDefaultMode = MAX;
      }
      else if (iter->empty() || (*iter == "mid"))
      {
        this->DefaultModeVector.push_back(MID);
        this->DefaultDefaultMode = MID;
      }
      else
      {
        vtkWarningMacro("Invalid 'default_mode': " << iter->c_str() << ". 'mid' assumed.");
        this->DefaultModeVector.push_back(MID);
        this->DefaultDefaultMode = MID;
      }
    }
  }

  double resolution;
  numRead = element->GetScalarAttribute("resolution", &resolution);
  if (numRead > 0)
  {
    this->Resolution = resolution;
  }

  this->SetEntries(new_entries);
  return 1;
}

//-----------------------------------------------------------------------------
template <class T>
int vtkSMRangeDomainTemplate<T>::SetDefaultValues(
  vtkSMProperty* property, bool use_unchecked_values)
{
  vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(property);
  if (!vp)
  {
    vtkErrorMacro("Property must be a vtkSMVectorProperty subclass.");
    return 0;
  }

  if (this->GetNumberOfRequiredProperties() == 0)
  {
    // no required properties, the domain doesn't depend on run-time values.
    // Typically implies that the default values set for the property are what
    // the user intended.
    return 0;
  }

  vtkSMPropertyHelper helper(vp);
  helper.SetUseUnchecked(use_unchecked_values);
  if (vp->GetRepeatCommand())
  {
    unsigned int vectorSize = static_cast<unsigned int>(this->DefaultModeVector.size());
    if (helper.GetNumberOfElements() > 0 && helper.GetNumberOfElements() == vectorSize)
    {
      // The domain is defining exactly the same number of modes than number
      // of elements in the property, let's use them
      for (unsigned int cc = 0; cc < vectorSize; cc++)
      {
        T value;
        if (this->GetComputedDefaultValue(cc, value))
        {
          helper.Set(cc, value);
        }
      }
      return 1;
    }
    else
    {
      // this is a resizable property, set just 1 value in it. This happens in
      // cases like ContourValues for ContourFilter.
      T value;
      if (this->GetComputedDefaultValue(0, value))
      {
        helper.Set(&value, 1);
        return 1;
      }
    }
  }
  else if (helper.GetNumberOfElements() == this->GetNumberOfEntries() * 2)
  {
    // the property is expecting a range.
    std::vector<T> values = helper.GetArray<T>();
    for (unsigned int cc = 0; cc < this->GetNumberOfEntries(); cc++)
    {
      int minExists, maxExists;
      T value[2];
      value[0] = this->GetMinimum(cc, minExists);
      value[1] = this->GetMaximum(cc, maxExists);
      if (minExists && maxExists)
      {
        values[2 * cc] = value[0];
        values[2 * cc + 1] = value[1];
      }
      // else leave values unchanged.
    }
    if (values.size() > 0)
    {
      helper.Set(&values[0], static_cast<unsigned int>(values.size()));
      return 1;
    }
  }
  else
  {
    std::vector<T> values = helper.GetArray<T>();
    for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
    {
      T value;
      if (this->GetComputedDefaultValue(cc, value))
      {
        values[cc] = value;
      }
    }
    helper.Set(&values[0], static_cast<unsigned int>(values.size()));
    return 1;
  }

  return this->Superclass::SetDefaultValues(property, use_unchecked_values);
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::GetComputedDefaultValue(unsigned int index, T& value)
{
  int minExists, maxExists;
  T min, max;

  // Using a modulo to be able to recover correct minimum and maximum with repeatable properties
  min = this->GetMinimum(
    this->GetNumberOfEntries() > 0 ? index % this->GetNumberOfEntries() : index, minExists);
  max = this->GetMaximum(
    this->GetNumberOfEntries() > 0 ? index % this->GetNumberOfEntries() : index, maxExists);

  DefaultModes defaultMode = this->GetDefaultMode(index);

  if ((minExists && !maxExists) || defaultMode == MIN)
  {
    value = min;
    return minExists != 0;
  }
  else if ((maxExists && !minExists) || defaultMode == MAX)
  {
    value = max;
    return maxExists != 0;
  }
  else if (minExists && maxExists && defaultMode == MID)
  {
    value = (min + max) / 2;
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
template <class T>
typename vtkSMRangeDomainTemplate<T>::DefaultModes vtkSMRangeDomainTemplate<T>::GetDefaultMode(
  unsigned int index)
{
  return (index < static_cast<unsigned int>(this->DefaultModeVector.size()))
    ? this->DefaultModeVector[index]
    : this->DefaultDefaultMode;
}

//-----------------------------------------------------------------------------
template <class T>
void vtkSMRangeDomainTemplate<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultMode: ";
  for (typename std::vector<DefaultModes>::const_iterator iter = this->DefaultModeVector.begin();
       iter != this->DefaultModeVector.end(); ++iter)
  {
    switch (*iter)
    {
      case MIN:
        os << "min,";
        break;
      case MAX:
        os << "max,";
        break;
      case MID:
        os << "mid,";
        break;
    }
  }
  os << endl;
  os << indent << "Resolution: " << this->Resolution << std::endl;
}
#endif
