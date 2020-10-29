/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMVectorPropertyTemplate
 *
 *
*/

#ifndef vtkSMVectorPropertyTemplate_h
#define vtkSMVectorPropertyTemplate_h

#include "vtkCommand.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

// clang-format off
#include "vtk_doubleconversion.h"
#include VTK_DOUBLECONVERSION_HEADER(double-conversion.h)
// clang-format on

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string>
#include <vector>

class vtkSMProperty;

namespace
{

template <typename T>
std::string AsString(const T& var)
{
  std::ostringstream str;
  str << var;
  return str.str();
}

template <>
vtkMaybeUnused("not used in non-double specializations") std::string AsString(const double& var)
{
  char buf[256];
  const double_conversion::DoubleToStringConverter& converter =
    double_conversion::DoubleToStringConverter::EcmaScriptConverter();
  double_conversion::StringBuilder builder(buf, sizeof(buf));
  builder.Reset();
  converter.ToShortest(var, &builder);
  return builder.Finalize();
}

template <class B>
B vtkSMVPConvertFromString(const std::string& string_representation)
{
  B value = B();
  std::istringstream buffer(string_representation);
  buffer >> value;
  return value;
}

template <>
vtkMaybeUnused("not used in non-string specializations") std::string
  vtkSMVPConvertFromString<std::string>(const std::string& string_representation)
{
  return string_representation;
}
}

template <class T>
class vtkSMVectorPropertyTemplate
{
  vtkSMProperty* Property;

public:
  std::vector<T> Values;
  std::vector<T> UncheckedValues;
  std::vector<T> DefaultValues; // Values set in the XML configuration.
  bool DefaultsValid;
  bool Initialized;

  //---------------------------------------------------------------------------
  vtkSMVectorPropertyTemplate(vtkSMProperty* property)
  {
    this->Property = property;
    this->DefaultsValid = false;
    this->Initialized = false;
  }

  //---------------------------------------------------------------------------
  void UpdateDefaultValues()
  {
    this->DefaultValues.clear();
    this->DefaultValues.insert(this->DefaultValues.end(), this->Values.begin(), this->Values.end());
    this->DefaultsValid = true;
  }

  //---------------------------------------------------------------------------
  void SetNumberOfUncheckedElements(unsigned int num)
  {
    this->UncheckedValues.resize(num);
    this->Property->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }

  //---------------------------------------------------------------------------
  unsigned int GetNumberOfUncheckedElements()
  {
    return static_cast<unsigned int>(this->UncheckedValues.size());
  }

  //---------------------------------------------------------------------------
  unsigned int GetNumberOfElements() { return static_cast<unsigned int>(this->Values.size()); }

  //---------------------------------------------------------------------------
  void SetNumberOfElements(unsigned int num)
  {
    if (num == this->Values.size())
    {
      return;
    }
    this->Values.resize(num);
    this->UncheckedValues.resize(num);
    if (num == 0)
    {
      // If num == 0, then we already have the initialized values (so to speak).
      this->Initialized = true;
    }
    else
    {
      this->Initialized = false;
    }
    this->Property->Modified();
  }

  //---------------------------------------------------------------------------
  T& GetElement(unsigned int idx)
  {
    assert(idx < this->Values.size());
    return this->Values[idx];
  }

  //---------------------------------------------------------------------------
  // seems weird that this idx is "int".
  T& GetDefaultValue(int idx)
  {
    if (idx >= 0 && idx < static_cast<int>(this->DefaultValues.size()))
    {
      return this->DefaultValues[idx];
    }

    static T empty_value = T();
    return empty_value;
  }

  //---------------------------------------------------------------------------
  T* GetElements() { return (this->Values.size() > 0) ? &this->Values[0] : NULL; }

  //---------------------------------------------------------------------------
  T* GetUncheckedElements()
  {
    return (this->UncheckedValues.size() > 0) ? &this->UncheckedValues[0] : NULL;
  }
  //---------------------------------------------------------------------------
  T& GetUncheckedElement(unsigned int idx)
  {
    assert(idx < this->UncheckedValues.size());
    return this->UncheckedValues[idx];
  }

  //---------------------------------------------------------------------------
  void SetUncheckedElement(unsigned int idx, T value)
  {
    if (idx >= this->GetNumberOfUncheckedElements())
    {
      this->UncheckedValues.resize(idx + 1);
    }

    if (this->UncheckedValues[idx] != value)
    {
      this->UncheckedValues[idx] = value;
      this->Property->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
    }
  }

  //---------------------------------------------------------------------------
  int SetUncheckedElements(const T* values)
  {
    return this->SetUncheckedElements(values, this->GetNumberOfUncheckedElements());
  }

  //---------------------------------------------------------------------------
  int SetUncheckedElements(const T* values, unsigned int numValues)
  {
    bool modified = false;
    unsigned int numArgs = this->GetNumberOfUncheckedElements();
    if (numArgs != numValues)
    {
      this->UncheckedValues.resize(numValues);
      numArgs = numValues;
      modified = true;
    }
    else
    {
      modified = !std::equal(this->UncheckedValues.begin(), this->UncheckedValues.end(), values);
    }

    if (!modified)
    {
      return 1;
    }

    std::copy(values, values + numArgs, this->UncheckedValues.begin());

    this->Property->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
    return 1;
  }

  //---------------------------------------------------------------------------
  int SetElement(unsigned int idx, T value)
  {
    unsigned int numElems = this->GetNumberOfElements();

    if (this->Initialized && idx < numElems && value == this->GetElement(idx))
    {
      return 1;
    }

    if (idx >= numElems)
    {
      this->SetNumberOfElements(idx + 1);
    }
    this->Values[idx] = value;

    // Make sure to initialize BEFORE Modified() is called. Otherwise,
    // the value would not be pushed.
    this->Initialized = true;
    this->Property->Modified();
    this->ClearUncheckedElements();
    return 1;
  }

  //---------------------------------------------------------------------------
  int SetElements(const T* values)
  {
    return this->SetElements(values, this->GetNumberOfElements());
  }

  //---------------------------------------------------------------------------
  int SetElements(const T* values, unsigned int numValues)
  {
    bool modified = false;
    unsigned int numArgs = this->GetNumberOfElements();
    if (numArgs != numValues)
    {
      this->Values.resize(numValues);
      this->UncheckedValues.resize(numValues);
      numArgs = numValues;
      modified = true;
    }
    else
    {
      modified = !std::equal(this->Values.begin(), this->Values.end(), values);
    }
    if (!modified && this->Initialized)
    {
      return 1;
    }

    std::copy(values, values + numArgs, this->Values.begin());
    this->Initialized = true;
    if (!modified && numValues == 0)
    {
      // handle the case when the property didn't have valid values but the new
      // values don't really change anything. In that case, the property hasn't
      // really been modified, so skip invoking the event. This keeps Python
      // trace from ending up with lots of properties such as EdgeBlocks etc for
      // ExodusIIReader which haven't really changed at all.
    }
    else
    {
      this->Property->Modified();
      this->ClearUncheckedElements();
    }
    return 1;
  }

  //---------------------------------------------------------------------------
  int AppendUncheckedElements(const T* values, unsigned int numValues)
  {
    this->UncheckedValues.insert(std::end(this->UncheckedValues), values, values + numValues);
    this->Property->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);

    return 1;
  }

  //---------------------------------------------------------------------------
  int AppendElements(const T* values, unsigned int numValues)
  {
    this->Values.insert(std::end(this->Values), values, values + numValues);
    this->Initialized = true;
    this->Property->Modified();
    this->ClearUncheckedElements();

    return 1;
  }

  //---------------------------------------------------------------------------
  void Copy(vtkSMVectorPropertyTemplate<T>* dsrc)
  {
    if (dsrc && dsrc->Initialized)
    {
      bool modified = false;

      if (this->Values != dsrc->Values)
      {
        this->Values = dsrc->Values;
        modified = true;
      }
      // If we were not initialized, we are now modified even if the value
      // did not change
      modified = modified || !this->Initialized;
      this->Initialized = true;

      if (modified)
      {
        this->Property->Modified();
      }

      // now copy unchecked values.
      if (this->UncheckedValues != dsrc->Values)
      {
        this->UncheckedValues = dsrc->Values;
        modified = true;
      }
      if (modified)
      {
        this->Property->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
      }
    }
  }

  //---------------------------------------------------------------------------
  void ResetToXMLDefaults()
  {
    if (this->DefaultsValid && this->DefaultValues != this->Values)
    {
      this->Values = this->DefaultValues;
      // Make sure to initialize BEFORE Modified() is called. Otherwise,
      // the value would not be pushed.
      this->Initialized = true;
      this->Property->Modified();
      this->ClearUncheckedElements();
    }
    else if (this->Property->GetRepeatable())
    {
      this->Values.clear();
      this->Initialized = true;
      this->Property->Modified();
      this->ClearUncheckedElements();
    }
  }

  //---------------------------------------------------------------------------
  bool LoadStateValues(vtkPVXMLElement* element)
  {
    if (!element)
    {
      return false;
    }

    std::vector<T> new_values;
    unsigned int numElems = element->GetNumberOfNestedElements();
    for (unsigned int i = 0; i < numElems; i++)
    {
      vtkPVXMLElement* current = element->GetNestedElement(i);
      if (current->GetName() && strcmp(current->GetName(), "Element") == 0)
      {
        int index;
        const char* str_value = current->GetAttribute("value");
        if (str_value && current->GetScalarAttribute("index", &index) && index >= 0)
        {
          if (index <= static_cast<int>(new_values.size()))
          {
            new_values.resize(index + 1);
          }

          new_values[index] = vtkSMVPConvertFromString<T>(str_value);
        }
      }
    }
    if (new_values.size() > 0)
    {
      this->SetElements(&new_values[0], static_cast<unsigned int>(new_values.size()));
    }
    else
    {
      this->SetNumberOfElements(0);
    }

    return true;
  }

  //---------------------------------------------------------------------------
  void SaveStateValues(vtkPVXMLElement* propertyElement)
  {
    unsigned int size = this->GetNumberOfElements();
    if (size > 0)
    {
      propertyElement->AddAttribute("number_of_elements", size);
    }

    // helps save full precision doubles and floats.
    for (unsigned int i = 0; i < size; i++)
    {
      vtkPVXMLElement* elementElement = vtkPVXMLElement::New();
      elementElement->SetName("Element");
      elementElement->AddAttribute("index", i);
      elementElement->AddAttribute("value", ::AsString(this->GetElement(i)).c_str());
      propertyElement->AddNestedElement(elementElement);
      elementElement->Delete();
    }
  }

  //---------------------------------------------------------------------------
  void ClearUncheckedElements()
  {
    // copy values to unchecked values
    this->UncheckedValues = this->Values;
    this->Property->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }

  //---------------------------------------------------------------------------
  bool IsValueDefault()
  {
    if (this->Values.size() != this->DefaultValues.size())
    {
      return false;
    }

    return std::equal(this->Values.begin(), this->Values.end(), this->DefaultValues.begin());
  }
};
#endif

// VTK-HeaderTest-Exclude: vtkSMVectorPropertyTemplate.h
