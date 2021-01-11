/*=========================================================================

 Program:   ParaView
 Module:    vtkPVArrayInformation.cxx

 Copyright (c) Kitware, Inc.
 All rights reserved.
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 cxx     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPVArrayInformation.h"

#include "vtkAbstractArray.h"
#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKey.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilter.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace
{
static constexpr size_t MAX_STRING_VALUES = 6;

typedef std::vector<std::string*> vtkInternalComponentNameBase;

struct vtkPVArrayInformationInformationKey
{
  std::string Location;
  std::string Name;
};

typedef std::vector<vtkPVArrayInformationInformationKey> vtkInternalInformationKeysBase;
}

class vtkPVArrayInformation::vtkInternalComponentNames : public vtkInternalComponentNameBase
{
};

class vtkPVArrayInformation::vtkInternalInformationKeys : public vtkInternalInformationKeysBase
{
};

vtkStandardNewMacro(vtkPVArrayInformation);

//----------------------------------------------------------------------------
vtkPVArrayInformation::vtkPVArrayInformation()
{
  this->Name = NULL;
  this->Ranges = NULL;
  this->FiniteRanges = NULL;
  this->ComponentNames = NULL;
  this->DefaultComponentName = NULL;
  this->InformationKeys = NULL;
  this->IsPartial = 0;
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkPVArrayInformation::~vtkPVArrayInformation()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::Initialize()
{
  this->SetName(0);
  this->DataType = VTK_VOID;
  this->NumberOfComponents = 0;
  this->NumberOfTuples = 0;
  this->StringValues.clear();
  if (this->ComponentNames)
  {
    for (unsigned int i = 0; i < this->ComponentNames->size(); ++i)
    {
      if (this->ComponentNames->at(i))
      {
        delete this->ComponentNames->at(i);
      }
    }
    this->ComponentNames->clear();
    delete this->ComponentNames;
    this->ComponentNames = 0;
  }

  if (this->DefaultComponentName)
  {
    delete this->DefaultComponentName;
    this->DefaultComponentName = 0;
  }

  if (this->Ranges)
  {
    delete[] this->Ranges;
    this->Ranges = NULL;
  }
  if (this->FiniteRanges)
  {
    delete[] this->FiniteRanges;
    this->FiniteRanges = NULL;
  }

  this->IsPartial = 0;

  if (this->InformationKeys)
  {
    this->InformationKeys->clear();
    delete this->InformationKeys;
    this->InformationKeys = 0;
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  int num, idx;
  vtkIndent i2 = indent.GetNextIndent();

  this->Superclass::PrintSelf(os, indent);
  if (this->Name)
  {
    os << indent << "Name: " << this->Name << endl;
  }
  os << indent << "DataType: " << this->DataType << endl;
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  if (this->ComponentNames)
  {
    os << indent << "ComponentNames:" << endl;
    for (unsigned int i = 0; i < this->ComponentNames->size(); ++i)
    {
      os << i2 << this->ComponentNames->at(i) << endl;
    }
  }
  os << indent << "NumberOfTuples: " << this->NumberOfTuples << endl;
  os << indent << "IsPartial: " << this->IsPartial << endl;

  os << indent << "Ranges :" << endl;
  num = this->NumberOfComponents;
  if (num > 1)
  {
    ++num;
  }
  for (idx = 0; idx < num; ++idx)
  {
    os << i2 << this->Ranges[2 * idx] << ", " << this->Ranges[2 * idx + 1] << endl;
  }

  os << indent << "InformationKeys :" << endl;
  if (this->InformationKeys)
  {
    num = this->GetNumberOfInformationKeys();
    for (idx = 0; idx < num; ++idx)
    {
      os << i2 << this->GetInformationKeyLocation(idx) << "::" << this->GetInformationKeyName(idx)
         << endl;
    }
  }
  else
  {
    os << i2 << "None" << endl;
  }

  if (this->DataType == VTK_STRING)
  {
    os << indent << "StringValues (count=" << this->StringValues.size() << ")" << endl;
    for (const auto& val : this->StringValues)
    {
      os << i2 << val.c_str() << endl;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetNumberOfComponents(int numComps)
{
  if (this->NumberOfComponents == numComps)
  {
    return;
  }
  if (this->Ranges)
  {
    delete[] this->Ranges;
    this->Ranges = NULL;
  }
  if (this->FiniteRanges)
  {
    delete[] this->FiniteRanges;
    this->FiniteRanges = NULL;
  }
  this->NumberOfComponents = numComps;
  if (numComps <= 0)
  {
    this->NumberOfComponents = 0;
    return;
  }
  if (numComps > 1)
  { // Extra range for vector magnitude (first in array).
    numComps = numComps + 1;
  }

  this->Ranges = new double[numComps * 2];
  this->FiniteRanges = new double[numComps * 2];
  for (int idx = 0; idx < numComps; ++idx)
  {
    this->Ranges[2 * idx] = VTK_DOUBLE_MAX;
    this->Ranges[2 * idx + 1] = -VTK_DOUBLE_MAX;
    this->FiniteRanges[2 * idx] = VTK_DOUBLE_MAX;
    this->FiniteRanges[2 * idx + 1] = -VTK_DOUBLE_MAX;
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetComponentName(vtkIdType component, const char* name)
{
  if (component < 0 || name == NULL)
  {
    return;
  }

  unsigned int index = static_cast<unsigned int>(component);
  if (this->ComponentNames == NULL)
  {
    // delayed allocate
    this->ComponentNames = new vtkPVArrayInformation::vtkInternalComponentNames();
  }

  if (index == this->ComponentNames->size())
  {
    // the array isn't large enough, so we will resize
    this->ComponentNames->push_back(new std::string(name));
    return;
  }
  else if (index > this->ComponentNames->size())
  {
    this->ComponentNames->resize(index + 1, NULL);
  }

  // replace an existing element
  std::string* compName = this->ComponentNames->at(index);
  if (!compName)
  {
    compName = new std::string(name);
    this->ComponentNames->at(index) = compName;
  }
  else
  {
    compName->assign(name);
  }
}

//----------------------------------------------------------------------------
const char* vtkPVArrayInformation::GetComponentName(vtkIdType component)
{
  unsigned int index = static_cast<unsigned int>(component);
  // check signed component for less than zero
  if (this->ComponentNames && component >= 0 && index < this->ComponentNames->size())
  {
    std::string* compName = this->ComponentNames->at(index);
    if (compName)
    {
      return compName->c_str();
    }
  }
  else if (this->ComponentNames && component == -1 && this->ComponentNames->size() == 1)
  {
    // the array is a scalar i.e. single component. In that case, when one asks
    // for the magnitude (i.e. component == -1) it's same as asking for the
    // scalar. So return the scalar's component name.
    std::string* compName = this->ComponentNames->at(0);
    if (compName)
    {
      return compName->c_str();
    }
  }
  // we have failed to find a user set component name, use the default component name
  this->DetermineDefaultComponentName(component, this->GetNumberOfComponents());
  return this->DefaultComponentName->c_str();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetComponentRange(int comp, double min, double max)
{
  if (comp >= this->NumberOfComponents || this->NumberOfComponents <= 0)
  {
    vtkErrorMacro("Bad component");
  }
  if (this->NumberOfComponents > 1)
  { // Shift over vector mag range.
    ++comp;
  }
  if (comp < 0)
  { // anything less than 0 just defaults to the vector mag.
    comp = 0;
  }
  this->Ranges[comp * 2] = min;
  this->Ranges[comp * 2 + 1] = max;
}

//----------------------------------------------------------------------------
double* vtkPVArrayInformation::GetComponentRange(int comp)
{
  if (comp >= this->NumberOfComponents || this->NumberOfComponents <= 0)
  {
    vtkErrorMacro("Bad component");
    return NULL;
  }
  if (this->NumberOfComponents > 1)
  { // Shift over vector mag range.
    ++comp;
  }
  if (comp < 0)
  { // anything less than 0 just defaults to the vector mag.
    comp = 0;
  }
  return this->Ranges + comp * 2;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetComponentRange(int comp, double range[2])
{
  double* ptr;

  ptr = this->GetComponentRange(comp);

  if (ptr == NULL)
  {
    range[0] = VTK_DOUBLE_MAX;
    range[1] = -VTK_DOUBLE_MAX;
    return;
  }

  range[0] = ptr[0];
  range[1] = ptr[1];
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetComponentFiniteRange(int comp, double min, double max)
{
  if (comp >= this->NumberOfComponents || this->NumberOfComponents <= 0)
  {
    vtkErrorMacro("Bad component");
  }
  if (this->NumberOfComponents > 1)
  { // Shift over vector mag range.
    ++comp;
  }
  if (comp < 0)
  { // anything less than 0 just defaults to the vector mag.
    comp = 0;
  }
  this->FiniteRanges[comp * 2] = min;
  this->FiniteRanges[comp * 2 + 1] = max;
}

//----------------------------------------------------------------------------
double* vtkPVArrayInformation::GetComponentFiniteRange(int comp)
{
  if (comp >= this->NumberOfComponents || this->NumberOfComponents <= 0)
  {
    vtkErrorMacro("Bad component");
    return NULL;
  }
  if (this->NumberOfComponents > 1)
  { // Shift over vector mag range.
    ++comp;
  }
  if (comp < 0)
  { // anything less than 0 just defaults to the vector mag.
    comp = 0;
  }
  return this->FiniteRanges + comp * 2;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetComponentFiniteRange(int comp, double range[2])
{
  double* ptr = this->GetComponentFiniteRange(comp);

  if (ptr == NULL)
  {
    range[0] = VTK_DOUBLE_MAX;
    range[1] = -VTK_DOUBLE_MAX;
    return;
  }

  range[0] = ptr[0];
  range[1] = ptr[1];
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetDataTypeRange(double range[2])
{
  int dataType = this->GetDataType();
  switch (dataType)
  {
    case VTK_BIT:
      range[0] = VTK_BIT_MAX;
      range[1] = VTK_BIT_MAX;
      break;
    case VTK_UNSIGNED_CHAR:
      range[0] = VTK_UNSIGNED_CHAR_MIN;
      range[1] = VTK_UNSIGNED_CHAR_MAX;
      break;
    case VTK_CHAR:
      range[0] = VTK_CHAR_MIN;
      range[1] = VTK_CHAR_MAX;
      break;
    case VTK_UNSIGNED_SHORT:
      range[0] = VTK_UNSIGNED_SHORT_MIN;
      range[1] = VTK_UNSIGNED_SHORT_MAX;
      break;
    case VTK_SHORT:
      range[0] = VTK_SHORT_MIN;
      range[1] = VTK_SHORT_MAX;
      break;
    case VTK_UNSIGNED_INT:
      range[0] = VTK_UNSIGNED_INT_MIN;
      range[1] = VTK_UNSIGNED_INT_MAX;
      break;
    case VTK_INT:
      range[0] = VTK_INT_MIN;
      range[1] = VTK_INT_MAX;
      break;
    case VTK_UNSIGNED_LONG:
      range[0] = VTK_UNSIGNED_LONG_MIN;
      range[1] = VTK_UNSIGNED_LONG_MAX;
      break;
    case VTK_LONG:
      range[0] = VTK_LONG_MIN;
      range[1] = VTK_LONG_MAX;
      break;
    case VTK_FLOAT:
      range[0] = VTK_FLOAT_MIN;
      range[1] = VTK_FLOAT_MAX;
      break;
    case VTK_DOUBLE:
      range[0] = VTK_DOUBLE_MIN;
      range[1] = VTK_DOUBLE_MAX;
      break;
    default:
      // Default value:
      range[0] = 0;
      range[1] = 1;
      break;
  }
}
//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddRanges(vtkPVArrayInformation* info)
{
  const double* range;
  double* ptr = this->Ranges;
  int idx;

  if (this->NumberOfComponents != info->GetNumberOfComponents())
  {
    vtkErrorMacro("Component mismatch.");
  }

  if (this->NumberOfComponents > 1)
  {
    range = info->GetComponentRange(-1);
    ptr[0] = std::min(ptr[0], range[0]);
    ptr[1] = std::max(ptr[1], range[1]);
    ptr += 2;
  }

  for (idx = 0; idx < this->NumberOfComponents; ++idx)
  {
    range = info->GetComponentRange(idx);
    ptr[0] = std::min(ptr[0], range[0]);
    ptr[1] = std::max(ptr[1], range[1]);
    ptr += 2;
  }

  if (this->DataType == VTK_STRING && info->DataType == VTK_STRING)
  {
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddFiniteRanges(vtkPVArrayInformation* info)
{
  const double* range;
  double* ptr = this->FiniteRanges;
  int idx;

  if (this->NumberOfComponents != info->GetNumberOfComponents())
  {
    vtkErrorMacro("Component mismatch.");
  }

  if (this->NumberOfComponents > 1)
  {
    range = info->GetComponentFiniteRange(-1);
    ptr[0] = std::min(ptr[0], range[0]);
    ptr[1] = std::max(ptr[1], range[1]);
    ptr += 2;
  }

  for (idx = 0; idx < this->NumberOfComponents; ++idx)
  {
    range = info->GetComponentFiniteRange(idx);
    ptr[0] = std::min(ptr[0], range[0]);
    ptr[1] = std::max(ptr[1], range[1]);
    ptr += 2;
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddValues(vtkPVArrayInformation* info)
{
  if (this->DataType != VTK_STRING || info->DataType != VTK_STRING)
  {
    // currently, we only do this for string arrays
    return;
  }

  this->StringValues.insert(
    this->StringValues.end(), info->StringValues.begin(), info->StringValues.end());
  if (this->StringValues.size() > MAX_STRING_VALUES)
  {
    this->StringValues.resize(MAX_STRING_VALUES);
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::DeepCopy(vtkPVArrayInformation* info)
{
  int num, idx;

  this->SetName(info->GetName());
  this->DataType = info->GetDataType();
  this->SetNumberOfComponents(info->GetNumberOfComponents());
  this->SetNumberOfTuples(info->GetNumberOfTuples());
  this->IsPartial = info->IsPartial;
  this->StringValues = info->StringValues;

  num = 2 * this->NumberOfComponents;
  if (this->NumberOfComponents > 1)
  {
    num += 2;
  }
  for (idx = 0; idx < num; ++idx)
  {
    this->Ranges[idx] = info->Ranges[idx];
  }
  for (idx = 0; idx < num; ++idx)
  {
    this->FiniteRanges[idx] = info->FiniteRanges[idx];
  }
  // clear the vector of old data
  if (this->ComponentNames)
  {
    for (unsigned int i = 0; i < this->ComponentNames->size(); ++i)
    {
      if (this->ComponentNames->at(i))
      {
        delete this->ComponentNames->at(i);
      }
    }
    this->ComponentNames->clear();
    delete this->ComponentNames;
    this->ComponentNames = 0;
  }

  if (info->ComponentNames)
  {
    this->ComponentNames = new vtkPVArrayInformation::vtkInternalComponentNames();
    // copy the passed in components if they exist
    this->ComponentNames->reserve(info->ComponentNames->size());
    const char* name;
    for (unsigned i = 0; i < info->ComponentNames->size(); ++i)
    {
      name = info->GetComponentName(i);
      if (name)
      {
        this->SetComponentName(i, name);
      }
    }
  }

  if (!this->InformationKeys)
  {
    this->InformationKeys = new vtkPVArrayInformation::vtkInternalInformationKeys();
  }

  // clear the vector of old data
  this->InformationKeys->clear();

  if (info->InformationKeys)
  {
    // copy the passed in components if they exist
    for (unsigned i = 0; i < info->InformationKeys->size(); ++i)
    {
      this->InformationKeys->push_back(info->InformationKeys->at(i));
    }
  }
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::Compare(vtkPVArrayInformation* info)
{
  if (info == NULL)
  {
    return 0;
  }
  if (strcmp(info->GetName(), this->Name) == 0 &&
    info->GetNumberOfComponents() == this->NumberOfComponents &&
    this->GetNumberOfInformationKeys() == info->GetNumberOfInformationKeys())
  {
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromObject(vtkObject* obj)
{
  if (!obj)
  {
    this->Initialize();
  }

  vtkAbstractArray* const array = vtkAbstractArray::SafeDownCast(obj);
  if (!array)
  {
    vtkErrorMacro("Cannot downcast to abstract array.");
    this->Initialize();
    return;
  }

  this->SetName(array->GetName());
  this->DataType = array->GetDataType();
  this->SetNumberOfComponents(array->GetNumberOfComponents());
  this->SetNumberOfTuples(array->GetNumberOfTuples());

  if (array->HasAComponentName())
  {
    const char* name;
    // copy the component names over
    for (int i = 0; i < this->GetNumberOfComponents(); ++i)
    {
      name = array->GetComponentName(i);
      if (name)
      {
        // each component doesn't have to be named
        this->SetComponentName(i, name);
      }
    }
  }

  if (vtkDataArray* const data_array = vtkDataArray::SafeDownCast(obj))
  {
    double range[2];
    double* ptr;
    int idx;

    ptr = this->Ranges;
    if (this->NumberOfComponents > 1)
    {
      // First store range of vector magnitude.
      data_array->GetRange(range, -1);
      *ptr++ = range[0];
      *ptr++ = range[1];
    }
    for (idx = 0; idx < this->NumberOfComponents; ++idx)
    {
      data_array->GetRange(range, idx);
      *ptr++ = range[0];
      *ptr++ = range[1];
    }
    ptr = this->FiniteRanges;
    if (this->NumberOfComponents > 1)
    {
      // First store range of vector magnitude.
      data_array->GetFiniteRange(range, -1);
      *ptr++ = range[0];
      *ptr++ = range[1];
    }
    for (idx = 0; idx < this->NumberOfComponents; ++idx)
    {
      data_array->GetFiniteRange(range, idx);
      *ptr++ = range[0];
      *ptr++ = range[1];
    }
  }
  else if (auto sarray = vtkStringArray::SafeDownCast(obj))
  {
    for (vtkIdType cc = 0;
         this->StringValues.size() < MAX_STRING_VALUES && cc < sarray->GetNumberOfValues(); ++cc)
    {
      auto value = sarray->GetValue(cc);
      if (!value.empty())
      {
        this->StringValues.push_back(value);
      }
    }
  }

  if (this->InformationKeys)
  {
    this->InformationKeys->clear();
    delete this->InformationKeys;
    this->InformationKeys = 0;
  }
  if (array->HasInformation())
  {
    vtkInformation* info = array->GetInformation();
    vtkInformationIterator* it = vtkInformationIterator::New();
    it->SetInformationWeak(info);
    it->GoToFirstItem();
    while (!it->IsDoneWithTraversal())
    {
      vtkInformationKey* key = it->GetCurrentKey();
      this->AddInformationKey(key->GetLocation(), key->GetName());
      it->GoToNextItem();
    }
    it->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddInformation(vtkPVInformation* info)
{
  if (!info)
  {
    return;
  }

  vtkPVArrayInformation* aInfo = vtkPVArrayInformation::SafeDownCast(info);
  if (!aInfo)
  {
    vtkErrorMacro("Could not downcast info to array info.");
    return;
  }
  if (aInfo->GetNumberOfComponents() > 0)
  {
    if (this->NumberOfComponents == 0)
    {
      // If this object is uninitialized, copy.
      this->DeepCopy(aInfo);
    }
    else
    {
      // Leave everything but ranges and unique values as original, add ranges and unique values.
      this->AddRanges(aInfo);
      this->AddFiniteRanges(aInfo);
      this->AddValues(aInfo);
      this->AddInformationKeys(aInfo);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  // Array name, data type, and number of components.
  *css << this->Name;
  *css << this->DataType;
  *css << this->NumberOfTuples;
  *css << this->NumberOfComponents;
  *css << this->IsPartial;

  // Range of each component.
  int num = this->NumberOfComponents;
  if (this->NumberOfComponents > 1)
  {
    // First range is range of vector magnitude.
    ++num;
  }
  for (int i = 0; i < num; ++i)
  {
    *css << vtkClientServerStream::InsertArray(this->Ranges + 2 * i, 2);
  }
  for (int i = 0; i < num; ++i)
  {
    *css << vtkClientServerStream::InsertArray(this->FiniteRanges + 2 * i, 2);
  }
  // add in the component names
  num = static_cast<int>(this->ComponentNames ? this->ComponentNames->size() : 0);
  *css << num;
  std::string* compName;
  for (int i = 0; i < num; ++i)
  {
    compName = this->ComponentNames->at(i);
    *css << (compName ? compName->c_str() : static_cast<const char*>(NULL));
  }

  int nkeys = this->GetNumberOfInformationKeys();
  *css << nkeys;
  for (int key = 0; key < nkeys; key++)
  {
    const char* location = this->GetInformationKeyLocation(key);
    const char* name = this->GetInformationKeyName(key);
    *css << location << name;
  }

  *css << this->GetNumberOfStringValues();
  for (const auto& val : this->StringValues)
  {
    *css << val.c_str();
  }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromStream(const vtkClientServerStream* css)
{
  // Array name.
  const char* name = 0;
  if (!css->GetArgument(0, 0, &name))
  {
    vtkErrorMacro("Error parsing array name from message.");
    return;
  }
  this->SetName(name);

  // Data type.
  if (!css->GetArgument(0, 1, &this->DataType))
  {
    vtkErrorMacro("Error parsing array data type from message.");
    return;
  }

  // Number of tuples.
  if (!css->GetArgument(0, 2, &this->NumberOfTuples))
  {
    vtkErrorMacro("Error parsing number of tuples from message.");
    return;
  }

  // Number of components.
  int num;
  if (!css->GetArgument(0, 3, &num))
  {
    vtkErrorMacro("Error parsing number of components from message.");
    return;
  }
  // This needs to be called since it allocates the this->Ranges array.
  this->SetNumberOfComponents(num);
  if (num > 1)
  {
    num++;
  }

  // Is Partial
  if (!css->GetArgument(0, 4, &this->IsPartial))
  {
    vtkErrorMacro("Error parsing IsPartial from message.");
    return;
  }

  // Range of each component.
  for (int i = 0; i < num; ++i)
  {
    if (!css->GetArgument(0, 5 + i, this->Ranges + 2 * i, 2))
    {
      vtkErrorMacro("Error parsing range of component.");
      return;
    }
  }

  // Range of each component.
  for (int i = 0; i < num; ++i)
  {
    if (!css->GetArgument(0, 5 + num + i, this->FiniteRanges + 2 * i, 2))
    {
      vtkErrorMacro("Error parsing range of component.");
      return;
    }
  }
  int pos = 5 + 2 * num;
  int numOfComponentNames;
  if (!css->GetArgument(0, pos++, &numOfComponentNames))
  {
    vtkErrorMacro("Error parsing number of component names.");
    return;
  }

  if (numOfComponentNames > 0)
  {
    // clear the vector of old data
    if (this->ComponentNames)
    {
      for (unsigned int i = 0; i < this->ComponentNames->size(); ++i)
      {
        if (this->ComponentNames->at(i))
        {
          delete this->ComponentNames->at(i);
        }
      }
      this->ComponentNames->clear();
      delete this->ComponentNames;
      this->ComponentNames = 0;
    }
    this->ComponentNames = new vtkPVArrayInformation::vtkInternalComponentNames();

    this->ComponentNames->reserve(numOfComponentNames);

    for (int cc = 0; cc < numOfComponentNames; cc++)
    {
      const char* comp_name;
      if (!css->GetArgument(0, pos++, &comp_name))
      {
        vtkErrorMacro("Error parsing component name from message.");
        return;
      }
      // note comp_name may be NULL, but that's okay.
      this->SetComponentName(cc, comp_name);
    }
  }

  // information keys
  int nkeys;
  if (!css->GetArgument(0, pos++, &nkeys))
  {
    return;
  }

  // Location and name for each key.
  if (this->InformationKeys)
  {
    this->InformationKeys->clear();
    delete this->InformationKeys;
    this->InformationKeys = 0;
  }
  for (int i = 0; i < nkeys; ++i)
  {
    if (!css->GetArgument(0, pos++, &name))
    {
      vtkErrorMacro("Error parsing information key location from message.");
      return;
    }
    std::string key_location = name;

    if (!css->GetArgument(0, pos++, &name))
    {
      vtkErrorMacro("Error parsing information key name from message.");
      return;
    }
    std::string key_name = name;
    this->AddInformationKey(key_location.c_str(), key_name.c_str());
  }

  int nvalues;
  this->StringValues.clear();
  if (!css->GetArgument(0, pos++, &nvalues))
  {
    return;
  }

  this->StringValues.reserve(nvalues);
  for (int cc = 0; cc < nvalues; ++cc)
  {
    if (!css->GetArgument(0, pos++, &name))
    {
      vtkErrorMacro("Error passed string value from message.");
      return;
    }
    this->StringValues.push_back(name);
  }
}

//-----------------------------------------------------------------------------
void vtkPVArrayInformation::DetermineDefaultComponentName(
  const int& component_no, const int& num_components)
{
  if (!this->DefaultComponentName)
  {
    this->DefaultComponentName = new std::string();
  }

  this->DefaultComponentName->assign(
    vtkPVPostFilter::DefaultComponentName(component_no, num_components));
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddInformationKeys(vtkPVArrayInformation* info)
{
  for (int k = 0; k < info->GetNumberOfInformationKeys(); k++)
  {
    this->AddUniqueInformationKey(
      info->GetInformationKeyLocation(k), info->GetInformationKeyName(k));
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddInformationKey(const char* location, const char* name)
{
  if (this->InformationKeys == NULL)
  {
    this->InformationKeys = new vtkInternalInformationKeys();
  }
  vtkPVArrayInformationInformationKey info = vtkPVArrayInformationInformationKey();
  info.Location = location;
  info.Name = name;
  this->InformationKeys->push_back(info);
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddUniqueInformationKey(const char* location, const char* name)
{
  if (!this->HasInformationKey(location, name))
  {
    this->AddInformationKey(location, name);
  }
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::GetNumberOfInformationKeys()
{
  return static_cast<int>(this->InformationKeys ? this->InformationKeys->size() : 0);
}

//----------------------------------------------------------------------------
const char* vtkPVArrayInformation::GetInformationKeyLocation(int index)
{
  if (index < 0 || index >= this->GetNumberOfInformationKeys())
    return NULL;

  return this->InformationKeys->at(index).Location.c_str();
}

//----------------------------------------------------------------------------
const char* vtkPVArrayInformation::GetInformationKeyName(int index)
{
  if (index < 0 || index >= this->GetNumberOfInformationKeys())
    return NULL;

  return this->InformationKeys->at(index).Name.c_str();
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::HasInformationKey(const char* location, const char* name)
{
  for (int k = 0; k < this->GetNumberOfInformationKeys(); k++)
  {
    const char* key_location = this->GetInformationKeyLocation(k);
    const char* key_name = this->GetInformationKeyName(k);
    if (strcmp(location, key_location) == 0 && strcmp(name, key_name) == 0)
    {
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::GetNumberOfStringValues()
{
  return this->DataType == VTK_STRING ? static_cast<int>(this->StringValues.size()) : 0;
}

//----------------------------------------------------------------------------
const char* vtkPVArrayInformation::GetStringValue(int index)
{
  return (index >= 0 && index < this->GetNumberOfStringValues()) ? this->StringValues[index].c_str()
                                                                 : nullptr;
}
