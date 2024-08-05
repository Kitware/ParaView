// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVDataSetAttributesInformation.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
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

  vtkSmartPointer<vtkPVArrayInformation>& GetOrCreateArrayInformation(const std::string& name)
  {
    auto it = this->ArrayInformationLookupMap.find(name);
    if (it == this->ArrayInformationLookupMap.end())
    {
      this->ArrayInformationLookupMap[name] = this->ArrayInformations.size();
      this->ArrayInformations.emplace_back();
      return this->ArrayInformations.back();
    }

    return this->ArrayInformations[it->second];
  }

  typedef std::map<std::string, size_t, comparator> lookupMap_t;
  typedef std::vector<vtkSmartPointer<vtkPVArrayInformation>> storageVector_t;
  storageVector_t ArrayInformations;
  lookupMap_t ArrayInformationLookupMap;
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
  for (auto iter = this->Internals->ArrayInformations.begin();
       iter != this->Internals->ArrayInformations.end(); ++iter)
  {
    iter->Get()->PrintSelf(os, indent.GetNextIndent());
    os << endl;
  }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::Initialize()
{
  vtkInternals& internals = (*this->Internals);
  internals.ArrayInformations.clear();
  internals.ArrayInformationLookupMap.clear();
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

  internals.ArrayInformations.clear();
  internals.ArrayInformationLookupMap.clear();
  Internals->ArrayInformations.reserve(ointernals.ArrayInformations.size());
  for (const auto& oinfo : ointernals.ArrayInformations)
  {
    vtkNew<vtkPVArrayInformation> arrayInfo;
    arrayInfo->DeepCopy(oinfo);

    internals.ArrayInformationLookupMap[arrayInfo->GetName()] = internals.ArrayInformations.size();
    internals.ArrayInformations.push_back(arrayInfo);
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

  // Handle vtkCellGrid cell-data specially.
  auto* cg = vtkCellGrid::SafeDownCast(dobj);
  if (cg)
  {
    if (this->FieldAssociation == vtkDataObject::CELL)
    {
      for (const auto& attId : cg->GetCellAttributeIds())
      {
        auto* cellAtt = cg->GetCellAttributeById(attId);
        if (!cellAtt)
        {
          continue; // TODO: Warn?
        }
        auto ainfo = vtkPVArrayInformation::New();
        ainfo->CopyFromCellAttribute(cg, cellAtt);
        internals.GetOrCreateArrayInformation(cellAtt->GetName().Data()).TakeReference(ainfo);
      }
    }
    internals.ValuesPopulated = true;
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
        ainfo->CopyFromArray(fd, cc);
        internals.GetOrCreateArrayInformation(array->GetName()).TakeReference(ainfo);
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
        internals.GetOrCreateArrayInformation(attr->GetName()).TakeReference(ainfo);
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

  std::vector<bool> duplicateArrays(this->Internals->ArrayInformations.size(), false);

  const auto& ointernals = (*other->Internals);
  for (const auto& oinfo : ointernals.ArrayInformations)
  {
    auto iter = internals.ArrayInformationLookupMap.find(oinfo->GetName());
    if (iter == internals.ArrayInformationLookupMap.end())
    {
      auto ainfo = vtkPVArrayInformation::New();
      ainfo->DeepCopy(oinfo);
      ainfo->SetIsPartial(true); // since only present in `other`.
      internals.GetOrCreateArrayInformation(oinfo->GetName()).TakeReference(ainfo);
    }
    else
    {
      auto ainfo = internals.ArrayInformations[iter->second].GetPointer();
      ainfo->AddInformation(oinfo, this->FieldAssociation);
      duplicateArrays[iter->second] = true;
    }
  }

  // Mark partial arrays not present in `other`
  for (size_t arrayIdx = 0; arrayIdx < duplicateArrays.size(); ++arrayIdx)
  {
    if (!duplicateArrays[arrayIdx])
    {
      internals.ArrayInformations[arrayIdx]->SetIsPartial(true); // since only present in `this`.
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
  return static_cast<int>(this->Internals->ArrayInformations.size());
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::GetMaximumNumberOfTuples() const
{
  const vtkInternals& internals = (*this->Internals);
  vtkTypeInt64 maxvalue = 0;
  for (auto& arrayInfo : internals.ArrayInformations)
  {
    maxvalue = std::max(maxvalue, arrayInfo->GetNumberOfTuples());
  }
  return maxvalue;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataSetAttributesInformation::GetArrayInformation(int idx) const
{
  const auto& internals = (*this->Internals);
  if (idx >= 0 && idx < static_cast<int>(internals.ArrayInformations.size()))
  {
    return internals.ArrayInformations[idx];
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
    auto iter = internals.ArrayInformationLookupMap.find(name);
    return iter != internals.ArrayInformationLookupMap.end()
      ? internals.ArrayInformations[iter->second]
      : nullptr;
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
  *css << internals.ValuesPopulated << static_cast<int>(internals.ArrayInformations.size());

  // Serialize each array's information.
  for (auto& arrayInfo : internals.ArrayInformations)
  {
    vtkClientServerStream temp;
    arrayInfo->CopyToStream(&temp);
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
    internals.GetOrCreateArrayInformation(ainfo->GetName()).TakeReference(ainfo);
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

//----------------------------------------------------------------------------
struct vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator::vtkInternals
{
  vtkPVDataSetAttributesInformation* DSAttributesInformation = nullptr;
  vtkPVDataSetAttributesInformation::vtkInternals::lookupMap_t::iterator LookupMapIter;
};

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator::
  AlphabeticalArrayInformationIterator(vtkPVDataSetAttributesInformation* dsAttributesInformation)
{
  this->Internals = std::unique_ptr<vtkInternals>(new vtkInternals());
  this->Internals->DSAttributesInformation = dsAttributesInformation;
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator::
  ~AlphabeticalArrayInformationIterator() = default;

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator*
vtkPVDataSetAttributesInformation::NewAlphabeticalArrayInformationIterator()
{
  return new vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator(this);
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator::GoToFirstItem()
{
  this->Internals->LookupMapIter =
    this->Internals->DSAttributesInformation->Internals->ArrayInformationLookupMap.begin();
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator::GoToNextItem()
{
  this->Internals->LookupMapIter++;
}

//----------------------------------------------------------------------------
bool vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator::IsDoneWithTraversal()
  const
{
  return this->Internals->LookupMapIter ==
    this->Internals->DSAttributesInformation->Internals->ArrayInformationLookupMap.end();
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator::
  GetCurrentArrayInformation()
{
  auto& infoStorage = this->Internals->DSAttributesInformation->Internals->ArrayInformations;
  return infoStorage[this->Internals->LookupMapIter->second];
}
