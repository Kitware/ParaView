/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataSetAttributesInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataSetAttributesInformation.h"

#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace
{
bool vtkSkipArray(const char* aname)
{
  return (aname == nullptr || strcmp(aname, "vtkOriginalCellIds") == 0 ||
    strcmp(aname, "vtkOriginalPointIds") == 0);
}
}

class vtkPVDataSetAttributesInformation::vtkInternals
{
public:
  struct comparator
  {
    bool operator()(const std::string& s1, const std::string& s2) const
    {
      int strcasecmpResult = 0;
#if defined(_WIN32)
      strcasecmpResult = stricmp(s1.c_str(), s2.c_str());
#else
      strcasecmpResult = strcasecmp(s1.c_str(), s2.c_str());
#endif
      if (strcasecmpResult == 0)
      {
        return (strcmp(s1.c_str(), s2.c_str()) < 0);
      }
      else
      {
        return (strcasecmpResult < 0);
      }
    }
  };

  typedef std::map<std::string, vtkSmartPointer<vtkPVArrayInformation>, comparator>
    ArrayInformationType;
  ArrayInformationType ArrayInformation;
  std::string AttributesInformation[vtkDataSetAttributes::NUM_ATTRIBUTES];
  bool ValuesPopulated = false;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataSetAttributesInformation);
//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation::vtkPVDataSetAttributesInformation()
  : Internals(new vtkPVDataSetAttributesInformation::vtkInternals())
{
  this->FieldAssociation = vtkDataObject::NUMBER_OF_ASSOCIATIONS;
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation::~vtkPVDataSetAttributesInformation()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayInformation, number of arrays: " << this->GetNumberOfArrays() << endl;
  for (vtkInternals::ArrayInformationType::const_iterator iter =
         this->Internals->ArrayInformation.begin();
       iter != this->Internals->ArrayInformation.end(); ++iter)
  {
    iter->second->PrintSelf(os, indent.GetNextIndent());
    os << endl;
  }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::Initialize()
{
  vtkInternals& internals = (*this->Internals);
  internals.ArrayInformation.clear();
  std::fill_n(internals.AttributesInformation,
    static_cast<int>(vtkDataSetAttributes::NUM_ATTRIBUTES), std::string());
  internals.ValuesPopulated = false;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::DeepCopy(vtkPVDataSetAttributesInformation* other)
{
  assert(this->FieldAssociation == other->FieldAssociation);
  auto& internals = (*this->Internals);
  const auto& ointernals = (*other->Internals);

  if (!ointernals.ValuesPopulated)
  {
    this->Initialize();
    return;
  }

  internals.ArrayInformation.clear();
  for (const auto& pair : ointernals.ArrayInformation)
  {
    vtkNew<vtkPVArrayInformation> arrayInfo;
    arrayInfo->DeepCopy(pair.second);
    internals.ArrayInformation[pair.first] = arrayInfo.Get();
  }

  // Now the default attributes.
  std::copy(ointernals.AttributesInformation,
    ointernals.AttributesInformation + vtkDataSetAttributes::NUM_ATTRIBUTES,
    internals.AttributesInformation);

  internals.ValuesPopulated = true;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromDataObject(vtkDataObject* dobj)
{
  auto& internals = (*this->Internals);

  // sanity check: this must be called on a clean instance.
  assert(internals.ValuesPopulated == false);

  if (!dobj)
  {
    return;
  }

  auto fd = dobj->GetAttributesAsFieldData(this->FieldAssociation);
  if (fd)
  {
    if (dobj->GetNumberOfElements(this->FieldAssociation) == 0)
    {
      return;
    }

    for (int cc = 0, max = fd->GetNumberOfArrays(); cc < max; ++cc)
    {
      auto array = fd->GetAbstractArray(cc);
      if (array && !vtkSkipArray(array->GetName()))
      {
        vtkPVArrayInformation* ainfo = vtkPVArrayInformation::New();
        ainfo->CopyFromArray(array);
        internals.ArrayInformation[array->GetName()].TakeReference(ainfo);
      }
    }

    if (auto dsa = vtkDataSetAttributes::SafeDownCast(fd))
    {
      for (int cc = 0; cc < vtkDataSetAttributes::NUM_ATTRIBUTES; ++cc)
      {
        auto array = dsa->GetAbstractAttribute(cc);
        if (array && !vtkSkipArray(array->GetName()))
        {
          internals.AttributesInformation[cc] = array->GetName();
        }
      }
    }
  }
  else if (auto gdobj = vtkGenericDataSet::SafeDownCast(dobj))
  {
    // handle generic datasets specially.
    int centering;
    switch (this->FieldAssociation)
    {
      case vtkDataObject::POINT:
        centering = vtkPointCentered;
        break;
      case vtkDataObject::CELL:
        centering = vtkCellCentered;
        break;
      default:
        centering = -1; // invalid.
    }

    auto gattributes = gdobj->GetAttributes();
    if (dobj->GetNumberOfElements(this->FieldAssociation) == 0 &&
      gattributes->GetNumberOfAttributes() == 0)
    {
      return;
    }

    for (int cc = 0, max = gattributes->GetNumberOfAttributes(); cc < max; ++cc)
    {
      auto attr = gattributes->GetAttribute(cc);
      if (attr && attr->GetCentering() == centering && !vtkSkipArray(attr->GetName()))
      {
        auto ainfo = vtkPVArrayInformation::New();
        ainfo->CopyFromGenericAttribute(attr);
        internals.ArrayInformation[attr->GetName()].TakeReference(ainfo);
      }
    }
  }
  internals.ValuesPopulated = true;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::AddInformation(vtkPVDataSetAttributesInformation* other)
{
  // merge `other` with `this`.
  if (other == nullptr || other->Internals->ValuesPopulated == false)
  {
    return;
  }

  auto& internals = (*this->Internals);
  if (internals.ValuesPopulated == false)
  {
    this->DeepCopy(other);
    return;
  }

  const auto& ointernals = (*other->Internals);
  for (auto& pair : ointernals.ArrayInformation)
  {
    auto iter = internals.ArrayInformation.find(pair.first);
    if (iter == internals.ArrayInformation.end())
    {
      auto ainfo = vtkPVArrayInformation::New();
      ainfo->DeepCopy(pair.second);
      ainfo->SetIsPartial(1); // since only present in `other`.
      internals.ArrayInformation[pair.first].TakeReference(ainfo);
    }
    else
    {
      auto ainfo = iter->second.GetPointer();
      ainfo->AddInformation(pair.second, this->FieldAssociation);
    }
  }

  // Mark partial arrays not present in `other`
  for (auto& pair : internals.ArrayInformation)
  {
    auto iter = ointernals.ArrayInformation.find(pair.first);
    if (iter == ointernals.ArrayInformation.end())
    {
      pair.second->SetIsPartial(1); // since only present in `this`.
    }
  }

  // Merge AttributesInformation.
  for (int idx = 0; idx < vtkDataSetAttributes::NUM_ATTRIBUTES; idx++)
  {
    if (internals.AttributesInformation[idx].empty() ||
      internals.AttributesInformation[idx] == ointernals.AttributesInformation[idx])
    {
      internals.AttributesInformation[idx] = ointernals.AttributesInformation[idx];
    }
    else
    {
      internals.AttributesInformation[idx].clear();
    }
  }
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::IsArrayAnAttribute(int arrayIndex)
{
  if (vtkPVArrayInformation* ainfo = this->GetArrayInformation(arrayIndex))
  {
    vtkInternals& internals = (*this->Internals);
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      if (internals.AttributesInformation[i] == ainfo->GetName())
      {
        return i;
      }
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataSetAttributesInformation::GetAttributeInformation(int attributeType)
{
  const vtkInternals& internals = (*this->Internals);
  if (attributeType >= 0 && attributeType < vtkDataSetAttributes::NUM_ATTRIBUTES &&
    !internals.AttributesInformation[attributeType].empty())
  {
    return this->GetArrayInformation(internals.AttributesInformation[attributeType].c_str());
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::GetNumberOfArrays() const
{
  return static_cast<int>(this->Internals->ArrayInformation.size());
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::GetMaximumNumberOfTuples() const
{
  const vtkInternals& internals = (*this->Internals);
  vtkTypeInt64 maxvalue = 0;
  for (auto& pair : internals.ArrayInformation)
  {
    maxvalue = std::max(maxvalue, pair.second->GetNumberOfTuples());
  }
  return maxvalue;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataSetAttributesInformation::GetArrayInformation(int idx) const
{
  const auto& internals = (*this->Internals);
  if (idx >= 0 && idx < static_cast<int>(internals.ArrayInformation.size()))
  {
    auto iter = std::next(internals.ArrayInformation.begin(), idx);
    return iter->second;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataSetAttributesInformation::GetArrayInformation(
  const char* name) const
{
  if (name)
  {
    const auto& internals = (*this->Internals);
    auto iter = internals.ArrayInformation.find(name);
    return iter != internals.ArrayInformation.end() ? iter->second : nullptr;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyToStream(vtkClientServerStream* css)
{
  const auto& internals = (*this->Internals);

  css->Reset();
  *css << vtkClientServerStream::Reply;

  // Number of arrays.
  *css << internals.ValuesPopulated << static_cast<int>(internals.ArrayInformation.size());

  // Serialize each array's information.
  for (auto& pair : internals.ArrayInformation)
  {
    vtkClientServerStream temp;
    pair.second->CopyToStream(&temp);
    *css << temp;
  }

  for (int cc = 0; cc < vtkDataSetAttributes::NUM_ATTRIBUTES; ++cc)
  {
    *css << internals.AttributesInformation[cc];
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromStream(const vtkClientServerStream* css)
{
  auto& internals = (*this->Internals);
  assert(internals.ValuesPopulated == false);

  // argument counter.
  int argument = 0;
  int num_arrays = 0;
  if (!css->GetArgument(0, argument++, &internals.ValuesPopulated) ||
    !css->GetArgument(0, argument++, &num_arrays))
  {
    this->Initialize();
    vtkErrorMacro("Error parsing stream");
    return;
  }

  for (int cc = 0; cc < num_arrays; ++cc)
  {
    vtkClientServerStream temp;
    if (!css->GetArgument(0, argument++, &temp))
    {
      this->Initialize();
      vtkErrorMacro("Error parsing stream");
      return;
    }
    auto ainfo = vtkPVArrayInformation::New();
    if (!ainfo->CopyFromStream(&temp))
    {
      this->Initialize();
      vtkErrorMacro("Error parsing stream");
      return;
    }
    internals.ArrayInformation[ainfo->GetName()].TakeReference(ainfo);
  }

  for (int cc = 0; cc < vtkDataSetAttributes::NUM_ATTRIBUTES; ++cc)
  {
    if (!css->GetArgument(0, argument++, &internals.AttributesInformation[cc]))
    {
      this->Initialize();
      vtkErrorMacro("Error parsing stream");
      return;
    }
  }
}
