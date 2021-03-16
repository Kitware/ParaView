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
#include "vtkGenericAttribute.h"
#include "vtkInformation.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKey.h"
#include "vtkNew.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVPostFilter.h"
#include "vtkStringArray.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace
{
static constexpr size_t MAX_STRING_VALUES = 6;

vtkTuple<double, 2> MergeRanges(const vtkTuple<double, 2>& r1, const vtkTuple<double, 2>& r2)
{
  if (r1[0] > r1[1])
  {
    return r2;
  }
  else if (r2[0] > r2[1])
  {
    return r1;
  }
  return vtkTuple<double, 2>({ std::min(r1[0], r2[0]), std::max(r1[1], r2[1]) });
}

} // end of namespace

vtkStandardNewMacro(vtkPVArrayInformation);
//----------------------------------------------------------------------------
vtkPVArrayInformation::vtkPVArrayInformation() = default;

//----------------------------------------------------------------------------
vtkPVArrayInformation::~vtkPVArrayInformation() = default;

//----------------------------------------------------------------------------
void vtkPVArrayInformation::Initialize()
{
  this->Name.clear();
  this->DataType = -1;
  this->NumberOfTuples = 0;
  this->IsPartial = false;
  this->Components.clear();
  this->StringValues.clear();
  this->InformationKeys.clear();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  const auto i2 = indent.GetNextIndent();
  os << indent << "Name: " << this->Name.c_str() << endl;
  os << indent << "DataType: " << this->DataType << endl;
  os << indent << "NumberOfComponents: " << this->Components.size() << endl;
  os << indent << "NumberOfTuples: " << this->NumberOfTuples << endl;
  os << indent << "IsPartial: " << this->IsPartial << endl;
  os << indent << "InformationKeys (count=" << this->InformationKeys.size() << "):" << endl;
  for (auto& pair : this->InformationKeys)
  {
    os << i2 << pair.first << "::" << pair.second << endl;
  }

  auto cname = [](size_t comp) {
    return (comp == 0 ? std::string("(magnitude)")
                      : std::string("(comp=") + std::to_string(comp - 1) + ")");
  };

  os << indent << "Ranges: " << endl;
  for (size_t cc = 0; cc < this->Components.size(); ++cc)
  {
    os << i2 << cname(cc).c_str() << ": " << this->Components[cc].Range[0] << ", "
       << this->Components[cc].Range[1] << endl;
  }

  os << indent << "FiniteRanges: " << endl;
  for (size_t cc = 0; cc < this->Components.size(); ++cc)
  {
    os << i2 << cname(cc).c_str() << ": " << this->Components[cc].FiniteRange[0] << ", "
       << this->Components[cc].FiniteRange[1] << endl;
  }

  os << indent << "ComponentNames" << endl;
  for (int cc = 0; cc < this->Components.size(); ++cc)
  {
    os << i2 << cname(cc).c_str() << ": " << this->Components[cc].Name.c_str() << endl;
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
int vtkPVArrayInformation::GetNumberOfComponents() const
{
  int count = static_cast<int>(this->Components.size());
  assert(count == 0 || count >= 2); // sanity check.
  return count >= 2 ? (count - 1) : 0;
}

//----------------------------------------------------------------------------
const char* vtkPVArrayInformation::GetComponentName(int component) const
{
  try
  {
    auto& compInfo = this->Components.at(component + 1);
    return compInfo.Name.empty() ? compInfo.DefaultName.c_str() : compInfo.Name.c_str();
  }
  catch (std::out_of_range&)
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
const double* vtkPVArrayInformation::GetComponentRange(int comp) const
{
  try
  {
    return this->Components.at(comp + 1).Range.GetData();
  }
  catch (std::out_of_range&)
  {
    vtkErrorMacro("Invalid component number " << comp);
    return nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetComponentRange(int comp, double range[2]) const
{
  auto r = this->GetComponentRange(comp);
  range[0] = r[0];
  range[1] = r[1];
}

//----------------------------------------------------------------------------
const double* vtkPVArrayInformation::GetComponentFiniteRange(int comp) const
{
  try
  {
    return this->Components.at(comp + 1).FiniteRange.GetData();
  }
  catch (std::out_of_range&)
  {
    vtkErrorMacro("Invalid component number " << comp);
    return nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetComponentFiniteRange(int comp, double range[2]) const
{
  auto r = this->GetComponentFiniteRange(comp);
  range[0] = r[0];
  range[1] = r[1];
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetDataTypeRange(double range[2]) const
{
  switch (this->DataType)
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
void vtkPVArrayInformation::DeepCopy(vtkPVArrayInformation* other)
{
  this->Name = other->Name;
  this->DataType = other->DataType;
  this->NumberOfTuples = other->NumberOfTuples;
  this->IsPartial = other->IsPartial;
  this->Components = other->Components;
  this->StringValues = other->StringValues;
  this->InformationKeys = other->InformationKeys;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromArray(vtkAbstractArray* array)
{
  assert(array != nullptr);
  this->Name = array->GetName() ? array->GetName() : "";
  this->DataType = array->GetDataType();
  this->NumberOfTuples = array->GetNumberOfTuples();

  const int numComponents = array->GetNumberOfComponents();
  this->Components.resize(numComponents + 1); // extra 1 for magnitude.

  // set default names.
  for (int comp = -1; comp < numComponents; ++comp)
  {
    auto& compInfo = this->Components.at(comp + 1);
    compInfo.DefaultName = vtkPVPostFilter::DefaultComponentName(comp, numComponents);
  }

  // set custom component names, if non-null.
  if (array->HasAComponentName())
  {
    for (int comp = 0; comp < numComponents; ++comp)
    {
      auto& compInfo = this->Components.at(comp + 1);
      if (auto name = array->GetComponentName(comp))
      {
        compInfo.Name = name;
      }
    }
  }

  auto dataArray = vtkDataArray::SafeDownCast(array);
  if (dataArray && dataArray->IsNumeric())
  {
    for (int comp = -1; comp < numComponents; ++comp)
    {
      auto& compInfo = this->Components.at(comp + 1);
      dataArray->GetRange(compInfo.Range.GetData(), comp);
      dataArray->GetFiniteRange(compInfo.FiniteRange.GetData(), comp);
    }
  }
  else if (auto sarray = vtkStringArray::SafeDownCast(array))
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

  if (array->HasInformation())
  {
    vtkInformation* info = array->GetInformation();
    vtkInformationIterator* it = vtkInformationIterator::New();
    it->SetInformationWeak(info);
    for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      vtkInformationKey* key = it->GetCurrentKey();
      this->InformationKeys.insert(
        std::make_pair<std::string, std::string>(key->GetLocation(), key->GetName()));
    }
    it->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromGenericAttribute(vtkGenericAttribute* array)
{
  assert(array != nullptr);
  assert(this->Components.size() == 0); // sanity check.

  this->Name = array->GetName() ? array->GetName() : "";
  this->DataType = array->GetComponentType();

  const int numComponents = array->GetNumberOfComponents();
  this->Components.resize(numComponents + 1); // extra 1 for magnitude.

  for (int comp = -1; comp < numComponents; ++comp)
  {
    auto& compInfo = this->Components.at(comp + 1);
    array->GetRange(comp, compInfo.Range.GetData());
    // since vtkGenericAttribute doesn't have separate finite-range,
    // we just copy the normal range.
    compInfo.FiniteRange = compInfo.Range;

    // set default names.
    compInfo.DefaultName = vtkPVPostFilter::DefaultComponentName(comp, numComponents);
  }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyToStream(vtkClientServerStream* css) const
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  // Array name, data type, and number of components.
  *css << this->Name;
  *css << this->DataType;
  *css << this->NumberOfTuples;
  *css << this->IsPartial;
  *css << static_cast<int>(this->Components.size());

  // components
  for (auto& info : this->Components)
  {
    *css << vtkClientServerStream::InsertArray(info.Range.GetData(), 2)
         << vtkClientServerStream::InsertArray(info.FiniteRange.GetData(), 2) << info.Name
         << info.DefaultName;
  }

  // string values.
  *css << static_cast<int>(this->StringValues.size());
  for (const auto& val : this->StringValues)
  {
    *css << val;
  }

  // information keys.
  *css << static_cast<int>(this->InformationKeys.size());
  for (auto& pair : this->InformationKeys)
  {
    *css << pair.first << pair.second;
  }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
bool vtkPVArrayInformation::CopyFromStream(const vtkClientServerStream* css)
{
  assert(this->Components.size() == 0); // sanity check.

  // argument counter;
  int argument = 0;
  if (!css->GetArgument(0, argument++, &this->Name) ||
    !css->GetArgument(0, argument++, &this->DataType) ||
    !css->GetArgument(0, argument++, &this->NumberOfTuples) ||
    !css->GetArgument(0, argument++, &this->IsPartial))
  {
    vtkErrorMacro("Error parsing message.");
    return false;
  }

  // components
  int count;
  if (!css->GetArgument(0, argument++, &count))
  {
    vtkErrorMacro("Error parsing message.");
    return false;
  }

  this->Components.resize(count);
  for (auto& info : this->Components)
  {
    if (!css->GetArgument(0, argument++, info.Range.GetData(), 2) ||
      !css->GetArgument(0, argument++, info.FiniteRange.GetData(), 2) ||
      !css->GetArgument(0, argument++, &info.Name) ||
      !css->GetArgument(0, argument++, &info.DefaultName))
    {
      vtkErrorMacro("Error parsing message.");
      return false;
    }
  }

  // string values
  if (!css->GetArgument(0, argument++, &count))
  {
    vtkErrorMacro("Error parsing message.");
    return false;
  }

  this->StringValues.resize(count);
  for (auto& value : this->StringValues)
  {
    if (!css->GetArgument(0, argument++, &value))
    {
      vtkErrorMacro("Error parsing message.");
      return false;
    }
  }

  // information keys
  if (!css->GetArgument(0, argument++, &count))
  {
    vtkErrorMacro("Error parsing message.");
    return false;
  }

  for (int cc = 0; cc < count; ++cc)
  {
    std::pair<std::string, std::string> pair;
    if (!css->GetArgument(0, argument++, &pair.first) ||
      !css->GetArgument(0, argument++, &pair.second))
    {
      vtkErrorMacro("Error parsing message.");
      return false;
    }

    this->InformationKeys.insert(pair);
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddInformation(vtkPVArrayInformation* other, int fieldAssociation)
{
  if (other == nullptr || other->Components.size() == 0)
  {
    return;
  }

  if (this->Components.size() == 0)
  {
    this->DeepCopy(other);
    return;
  }

  if (this->Components.size() != other->Components.size())
  {
    // this happens when the same array has different number of components
    // across blocks or ranks.
    vtkLogF(WARNING, "mismatching in number of components for array "
                     "'%s'. '%d' != '%d'.",
      this->Name.c_str(), this->GetNumberOfComponents(), other->GetNumberOfComponents());
  }

  assert(this->Name == other->Name);
  // it so happens that we do encounter arrays with same names but slightly
  // different types often; so this check is not reasonable.
  // Fixes pv.ColorOpacityTableEditorHistogram test.
  // assert(this->DataType == other->DataType);
  if (fieldAssociation == vtkDataObject::FIELD)
  {
    this->NumberOfTuples = std::max(this->NumberOfTuples, other->NumberOfTuples);
  }
  else
  {
    this->NumberOfTuples += other->NumberOfTuples;
  }

  this->Components.resize(std::max(this->Components.size(), other->Components.size()));
  for (size_t cc = 0; cc < other->Components.size(); ++cc)
  {
    auto& info = this->Components.at(cc);
    auto& oinfo = other->Components.at(cc);
    info.Range = ::MergeRanges(info.Range, oinfo.Range);
    info.FiniteRange = ::MergeRanges(info.FiniteRange, oinfo.FiniteRange);
    info.Name = info.Name.empty() ? oinfo.Name : info.Name;
    info.DefaultName = info.DefaultName.empty() ? oinfo.DefaultName : info.DefaultName;
  }

  if (this->DataType == VTK_STRING && other->DataType == VTK_STRING)
  {
    this->StringValues.insert(
      this->StringValues.end(), other->StringValues.begin(), other->StringValues.end());
    if (this->StringValues.size() > MAX_STRING_VALUES)
    {
      this->StringValues.resize(MAX_STRING_VALUES);
    }
  }

  this->InformationKeys.insert(other->InformationKeys.begin(), other->InformationKeys.end());
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::GetNumberOfInformationKeys() const
{
  return static_cast<int>(this->InformationKeys.size());
}

//----------------------------------------------------------------------------
const char* vtkPVArrayInformation::GetInformationKeyLocation(int index) const
{
  if (index >= 0 && index < this->GetNumberOfInformationKeys())
  {
    auto iter = std::next(this->InformationKeys.begin(), index);
    return iter->first.c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
const char* vtkPVArrayInformation::GetInformationKeyName(int index) const
{
  if (index >= 0 && index < this->GetNumberOfInformationKeys())
  {
    auto iter = std::next(this->InformationKeys.begin(), index);
    return iter->second.c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVArrayInformation::HasInformationKey(const char* location, const char* name) const
{
  std::pair<std::string, std::string> item(location, name);
  return (this->InformationKeys.find(item) != this->InformationKeys.end());
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

//----------------------------------------------------------------------------
std::string vtkPVArrayInformation::GetRangesAsString() const
{
  std::ostringstream stream;

  if (this->DataType == VTK_STRING)
  {
    int cc = 0;
    for (auto& val : this->StringValues)
    {
      stream << ((cc > 0) ? ", " : "");
      stream << val.c_str();
    }
  }
  else
  {
    vtkNumberToString formatter;
    bool first = true;
    for (int cc = 0, max = this->GetNumberOfComponents(); cc < max; ++cc)
    {
      auto& cinfo = this->Components[cc + 1];
      if (cinfo.Range[0] <= cinfo.Range[1])
      {
        stream << (first ? "" : ", ") << "[" << formatter(cinfo.Range[0]) << ", "
               << formatter(cinfo.Range[1]) << "]";
        first = false;
      }
    }
  }
  return stream.str();
}
