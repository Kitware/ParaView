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
#include "vtkCollection.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVGenericAttributeInformation.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace
{
bool vtkSkipArray(const char* aname)
{
  return (aname == NULL || strcmp(aname, "vtkOriginalCellIds") == 0 ||
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
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::DeepCopy(vtkPVDataSetAttributesInformation* dataInfo)
{
  vtkInternals& internals = (*this->Internals);
  internals.ArrayInformation.clear();

  const vtkInternals& otherInternals = (*dataInfo->Internals);

  for (vtkInternals::ArrayInformationType::const_iterator iter =
         otherInternals.ArrayInformation.begin();
       iter != otherInternals.ArrayInformation.end(); ++iter)
  {
    vtkNew<vtkPVArrayInformation> arrayInfo;
    arrayInfo->DeepCopy(iter->second);
    internals.ArrayInformation[iter->first] = arrayInfo.Get();
  }

  // Now the default attributes.
  std::copy(otherInternals.AttributesInformation,
    otherInternals.AttributesInformation + vtkDataSetAttributes::NUM_ATTRIBUTES,
    internals.AttributesInformation);
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromFieldData(vtkFieldData* da)
{
  vtkInternals& internals = (*this->Internals);

  // Clear array information.
  std::fill_n(internals.AttributesInformation,
    static_cast<int>(vtkDataSetAttributes::NUM_ATTRIBUTES), std::string());
  internals.ArrayInformation.clear();

  // Copy Field Data
  int num = da ? da->GetNumberOfArrays() : 0;
  for (int idx = 0; idx < num; ++idx)
  {
    vtkAbstractArray* const array = da->GetAbstractArray(idx);
    if (array != NULL && !vtkSkipArray(array->GetName()))
    {
      vtkNew<vtkPVArrayInformation> info;
      info->CopyFromObject(array);
      internals.ArrayInformation[array->GetName()] = info.Get();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromDataSetAttributes(vtkDataSetAttributes* da)
{
  this->CopyFromFieldData(da);

  // update attribute information.
  vtkInternals& internals = (*this->Internals);

  int attrIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
  da->GetAttributeIndices(attrIndices);

  for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
  {
    if (attrIndices[i] >= 0)
    {
      vtkAbstractArray* arr = da->GetAbstractArray(attrIndices[i]);
      if (arr)
      {
        const char* name = arr->GetName();
        internals.AttributesInformation[i] = name ? name : "";
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromGenericAttributes(
  vtkGenericAttributeCollection* da, int centering)
{
  vtkInternals& internals = (*this->Internals);

  // Clear array information.
  std::fill_n(internals.AttributesInformation,
    static_cast<int>(vtkDataSetAttributes::NUM_ATTRIBUTES), std::string());
  internals.ArrayInformation.clear();

  for (int cc = 0, max = da->GetNumberOfAttributes(); cc < max; ++cc)
  {
    vtkGenericAttribute* array = da->GetAttribute(cc);
    if (array != NULL && array->GetName() != NULL && !vtkSkipArray(array->GetName()) &&
      array->GetCentering() == centering)
    {
      vtkNew<vtkPVGenericAttributeInformation> info;
      info->CopyFromObject(array);
      internals.ArrayInformation[array->GetName()] = info.Get();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromGenericAttributesOnPoints(
  vtkGenericAttributeCollection* da)
{
  this->CopyFromGenericAttributes(da, vtkPointCentered);
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromGenericAttributesOnCells(
  vtkGenericAttributeCollection* da)
{
  this->CopyFromGenericAttributes(da, vtkCellCentered);
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::AddInformation(vtkPVDataSetAttributesInformation* info)
{
  // merge `info` with `this`.

  vtkInternals& internals = (*this->Internals);
  const vtkInternals& otherInternals = (*info->Internals);

  // Mark partial arrays not present in `info`
  for (vtkInternals::ArrayInformationType::const_iterator miter =
         internals.ArrayInformation.begin();
       miter != internals.ArrayInformation.end(); ++miter)
  {
    if (otherInternals.ArrayInformation.find(miter->first) == otherInternals.ArrayInformation.end())
    {
      miter->second->SetIsPartial(1);
    }
  }

  for (vtkInternals::ArrayInformationType::const_iterator oiter =
         otherInternals.ArrayInformation.begin();
       oiter != otherInternals.ArrayInformation.end(); ++oiter)
  {
    vtkInternals::ArrayInformationType::iterator miter =
      internals.ArrayInformation.find(oiter->first);
    if (miter == internals.ArrayInformation.end())
    {
      vtkNew<vtkPVArrayInformation> ainfo;
      ainfo->DeepCopy(oiter->second);
      internals.ArrayInformation[oiter->first] = ainfo.Get();
      ainfo->SetIsPartial(1);
    }
    else
    {
      vtkTypeInt64 numTuples = miter->second->GetNumberOfTuples();
      miter->second->AddInformation(oiter->second);
      if (this->FieldAssociation == vtkDataObject::FIELD)
      {
        // For field data, we accumulate the number of tuples as the maximum
        // number of tuples since field data is not appended together when the
        // geometries are reduced. This seems like a fair assumption that
        // addresses BUG #0015503.
        miter->second->SetNumberOfTuples(std::max(numTuples, oiter->second->GetNumberOfTuples()));
      }
      else
      {
        miter->second->SetNumberOfTuples(numTuples + oiter->second->GetNumberOfTuples());
      }
    }
  }

  // Merge AttributesInformation.
  for (int idx = 0; idx < vtkDataSetAttributes::NUM_ATTRIBUTES; idx++)
  {
    if (internals.AttributesInformation[idx].empty() ||
      internals.AttributesInformation[idx] == otherInternals.AttributesInformation[idx])
    {
      internals.AttributesInformation[idx] = otherInternals.AttributesInformation[idx];
    }
    else
    {
      internals.AttributesInformation[idx].clear();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVDataSetAttributesInformation* p = vtkPVDataSetAttributesInformation::SafeDownCast(info);
  if (p)
  {
    this->AddInformation(p);
  }
  else
  {
    vtkErrorMacro(
      "AddInformation called with object of type " << (info ? info->GetClassName() : "<unknown>"));
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
  return NULL;
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
  for (vtkInternals::ArrayInformationType::const_iterator miter =
         internals.ArrayInformation.begin();
       miter != internals.ArrayInformation.end(); ++miter)
  {
    maxvalue = std::max(maxvalue, miter->second->GetNumberOfTuples());
  }
  return maxvalue;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataSetAttributesInformation::GetArrayInformation(int idx) const
{
  const vtkInternals& internals = (*this->Internals);
  int cc = 0;
  for (vtkInternals::ArrayInformationType::const_iterator
         miter = internals.ArrayInformation.begin();
       miter != internals.ArrayInformation.end(); ++miter, ++cc)
  {
    if (cc == idx)
    {
      return miter->second;
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataSetAttributesInformation::GetArrayInformation(
  const char* name) const
{
  if (name)
  {
    const vtkInternals& internals = (*this->Internals);
    vtkInternals::ArrayInformationType::const_iterator iter = internals.ArrayInformation.find(name);
    return iter != internals.ArrayInformation.end() ? iter->second : NULL;
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyToStream(vtkClientServerStream* css)
{
  const vtkInternals& internals = (*this->Internals);

  // doing this to avoid issues when client-server mismatch.
  short attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
  std::fill_n(attributeIndices, static_cast<int>(vtkDataSetAttributes::NUM_ATTRIBUTES), -1);

  int arrayIdx = 0;
  for (vtkInternals::ArrayInformationType::const_iterator
         miter = internals.ArrayInformation.begin();
       miter != internals.ArrayInformation.end(); ++miter, ++arrayIdx)
  {
    for (int idx = 0; idx < vtkDataSetAttributes::NUM_ATTRIBUTES; ++idx)
    {
      if (internals.AttributesInformation[idx] == miter->first)
      {
        attributeIndices[idx] = arrayIdx;
      }
    }
  }

  css->Reset();
  *css << vtkClientServerStream::Reply;

  // Default attributes.
  *css << vtkClientServerStream::InsertArray(
    attributeIndices, vtkDataSetAttributes::NUM_ATTRIBUTES);

  // Number of arrays.
  *css << this->GetNumberOfArrays();

  // Serialize each array's information.
  for (vtkInternals::ArrayInformationType::const_iterator miter =
         internals.ArrayInformation.begin();
       miter != internals.ArrayInformation.end(); ++miter)
  {
    vtkClientServerStream acss;
    miter->second->CopyToStream(&acss);

    const unsigned char* data;
    size_t length;
    acss.GetData(&data, &length);
    *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromStream(const vtkClientServerStream* css)
{
  vtkInternals& internals = (*this->Internals);
  internals.ArrayInformation.clear();
  std::fill_n(internals.AttributesInformation,
    static_cast<int>(vtkDataSetAttributes::NUM_ATTRIBUTES), std::string());

  short attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];

  // Default attributes.
  if (!css->GetArgument(0, 0, attributeIndices, vtkDataSetAttributes::NUM_ATTRIBUTES))
  {
    vtkErrorMacro("Error parsing default attributes from message.");
    return;
  }

  // Number of arrays.
  int numArrays = 0;
  if (!css->GetArgument(0, 1, &numArrays))
  {
    vtkErrorMacro("Error parsing number of arrays from message.");
    return;
  }

  // Each array's information.
  std::vector<unsigned char> data;
  std::vector<std::string> arraynames;

  for (int i = 0; i < numArrays; ++i)
  {
    vtkTypeUInt32 length;
    if (!css->GetArgumentLength(0, i + 2, &length))
    {
      vtkErrorMacro(
        "Error parsing length of information for array number " << i << " from message.");
      return;
    }
    data.resize(length);
    if (!css->GetArgument(0, i + 2, &*data.begin(), length))
    {
      vtkErrorMacro("Error parsing information for array number " << i << " from message.");
      return;
    }

    vtkClientServerStream acss;
    acss.SetData(&*data.begin(), length);
    vtkNew<vtkPVArrayInformation> ai;
    ai->CopyFromStream(&acss);
    internals.ArrayInformation[ai->GetName()] = ai.Get();
    arraynames.push_back(ai->GetName());
  }

  for (int cc = 0; cc < vtkDataSetAttributes::NUM_ATTRIBUTES; ++cc)
  {
    internals.AttributesInformation[cc] =
      attributeIndices[cc] != -1 ? arraynames[attributeIndices[cc]] : std::string();
  }
}
