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
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"

#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkPVGenericAttributeInformation.h"

#include <string.h>
#include <algorithm>
#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataSetAttributesInformation);

//----------------------------------------------------------------------------
struct  vtkPVDataSetAttributesInformationSortArray
{
  int          arrayIndx;
  const char * arrayName;
};

bool    vtkPVDataSetAttributesInfromationAlphabeticSorting
( const vtkPVDataSetAttributesInformationSortArray & thisArray,
  const vtkPVDataSetAttributesInformationSortArray & thatArray )
{
#if defined(_WIN32)
  return  (  stricmp( thisArray.arrayName, thatArray.arrayName )  <=  0  )
          ?  true  :  false;
#else
  return  (  strcasecmp( thisArray.arrayName, thatArray.arrayName )  <=  0  )
          ?  true  :  false;
#endif
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation::vtkPVDataSetAttributesInformation()
{
  int idx;

  this->ArrayInformation = vtkCollection::New();
  for (idx = 0; idx < vtkDataSetAttributes::NUM_ATTRIBUTES; ++idx)
    {
    this->AttributeIndices[idx] = -1;
    }
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation::~vtkPVDataSetAttributesInformation()
{
  this->ArrayInformation->Delete();
  this->ArrayInformation = NULL;
}

//----------------------------------------------------------------------------
void
vtkPVDataSetAttributesInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPVArrayInformation *ai;
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os,indent);

  int num, idx;
  num = this->GetNumberOfArrays();
  os << indent << "ArrayInformation, number of arrays: " << num << endl;
  for (idx = 0; idx < num; ++idx)
    {
    ai = this->GetArrayInformation(idx);
    ai->PrintSelf(os, i2);
    os << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::Initialize()
{
  int idx;

  this->ArrayInformation->RemoveAllItems();
  for (idx = 0; idx < vtkDataSetAttributes::NUM_ATTRIBUTES; ++idx)
    {
    this->AttributeIndices[idx] = -1;
    }
}

//----------------------------------------------------------------------------
void
vtkPVDataSetAttributesInformation
::DeepCopy(vtkPVDataSetAttributesInformation *dataInfo)
{
  int idx, num;
  vtkPVArrayInformation* arrayInfo;
  vtkPVArrayInformation* newArrayInfo;

  // Copy array information.
  this->ArrayInformation->RemoveAllItems();
  num = dataInfo->GetNumberOfArrays();
  for (idx = 0; idx < num; ++idx)
    {
    arrayInfo = dataInfo->GetArrayInformation(idx);
    newArrayInfo = vtkPVArrayInformation::New();
    newArrayInfo->DeepCopy(arrayInfo);
    this->ArrayInformation->AddItem(newArrayInfo);
    newArrayInfo->Delete();
    newArrayInfo = NULL;
    }
  // Now the default attributes.
  for (idx = 0; idx < vtkDataSetAttributes::NUM_ATTRIBUTES; ++idx)
    {
    this->AttributeIndices[idx] = dataInfo->AttributeIndices[idx];
    }
}

//----------------------------------------------------------------------------
void
vtkPVDataSetAttributesInformation
::CopyFromFieldData(vtkFieldData *da)
{
  // Clear array information.
  this->ArrayInformation->RemoveAllItems();
  for (int idx = 0; idx < vtkDataSetAttributes::NUM_ATTRIBUTES; ++idx)
    {
    this->AttributeIndices[idx] = -1;
    }

  // Copy Field Data
  int num = da->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkAbstractArray* const array = da->GetAbstractArray(idx);
    if (array->GetName())
      {
      vtkPVArrayInformation *info = vtkPVArrayInformation::New();
      info->CopyFromObject(array);
      this->ArrayInformation->AddItem(info);
      info->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void
vtkPVDataSetAttributesInformation
::CopyFromDataSetAttributes(vtkDataSetAttributes *da)
{
  int idx;
  int num;
  int infoArrayIndex;
  int attribute;

  // Clear array information.
  this->ArrayInformation->RemoveAllItems();
  for (idx = 0; idx < vtkDataSetAttributes::NUM_ATTRIBUTES; ++idx)
    {
    this->AttributeIndices[idx] = -1;
    }

  // Copy Point Data
  num = da->GetNumberOfArrays();

  // sort the arrays alphabetically
  int   arrayIndx = 0;
  std::vector < vtkPVDataSetAttributesInformationSortArray > sortArays;
  sortArays.clear();

  if ( num > 0 )
    {
    sortArays.resize( num );
    for ( int i = 0; i < num; i ++ )
      {
      sortArays[i].arrayIndx = i;
      sortArays[i].arrayName = da->GetArrayName( i ) ?
        da->GetArrayName(i) : "";
      }

    std::sort( sortArays.begin(), sortArays.end(),
                  vtkPVDataSetAttributesInfromationAlphabeticSorting );
    }

  infoArrayIndex = 0;
  for (idx = 0; idx < num; ++idx)
    {
    arrayIndx = sortArays[idx].arrayIndx;
    vtkAbstractArray* const array = da->GetAbstractArray( arrayIndx );

    if (array->GetName() && 
        strcmp(array->GetName(),"vtkGhostLevels") != 0 &&
        strcmp(array->GetName(), "vtkOriginalCellIds") != 0 &&
        strcmp(array->GetName(), "vtkOriginalPointIds") != 0)
      {
      vtkPVArrayInformation *info = vtkPVArrayInformation::New();
      info->CopyFromObject(array);
      this->ArrayInformation->AddItem(info);
      info->Delete();
      // Record default attributes.
      attribute = da->IsArrayAnAttribute( arrayIndx );
      if (attribute > -1)
        {
        this->AttributeIndices[attribute] = infoArrayIndex;
        }
      ++infoArrayIndex;
      }
    }

  sortArays.clear();
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::
CopyFromGenericAttributesOnPoints(vtkGenericAttributeCollection *da)
{
  int idx;
  int num;
  vtkGenericAttribute *array;
  short infoArrayIndex;
//  int attribute;

  // Clear array information.
  this->ArrayInformation->RemoveAllItems();
  for (idx = 0; idx < 5; ++idx)
    {
    this->AttributeIndices[idx] = -1;
    }

  // Copy Point Data
  num = da->GetNumberOfAttributes();
  infoArrayIndex = 0;
  for (idx = 0; idx < num; ++idx)
    {
    array = da->GetAttribute(idx);
    if(array->GetCentering()==vtkPointCentered)
      {
      if (array->GetName() && strcmp(array->GetName(),"vtkGhostLevels") != 0)
        {
        vtkPVGenericAttributeInformation *info = vtkPVGenericAttributeInformation::New();
        info->CopyFromObject(array);
        this->ArrayInformation->AddItem(info);
        info->Delete();
#if 0
        // Record default attributes.
        attribute = da->IsArrayAnAttribute(idx);
        if (attribute > -1)
          {
          this->AttributeIndices[attribute] = infoArrayIndex;
          }
#endif
        ++infoArrayIndex;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::
CopyFromGenericAttributesOnCells(vtkGenericAttributeCollection *da)
{
    int idx;
  int num;
  vtkGenericAttribute *array;
  short infoArrayIndex;
//  int attribute;

  // Clear array information.
  this->ArrayInformation->RemoveAllItems();
  for (idx = 0; idx < 5; ++idx)
    {
    this->AttributeIndices[idx] = -1;
    }

  // Copy Cell Data
  num = da->GetNumberOfAttributes();
  infoArrayIndex = 0;
  for (idx = 0; idx < num; ++idx)
    {
    array = da->GetAttribute(idx);
    if(array->GetCentering()==vtkCellCentered)
      {
      if (array->GetName() && strcmp(array->GetName(),"vtkGhostLevels") != 0)
        {
        vtkPVGenericAttributeInformation *info = vtkPVGenericAttributeInformation::New();
        info->CopyFromObject(array);
        this->ArrayInformation->AddItem(info);
        info->Delete();
#if 0
        // Record default attributes.
        attribute = da->IsArrayAnAttribute(idx);
        if (attribute > -1)
          {
          this->AttributeIndices[attribute] = infoArrayIndex;
          }
#endif
        ++infoArrayIndex;
        }
      }
    }
}
  
//----------------------------------------------------------------------------
void
vtkPVDataSetAttributesInformation
::AddInformation(vtkPVDataSetAttributesInformation *info)
{
  int idx1, idx2;
  int num1 = this->GetNumberOfArrays();
  int num2 = info->GetNumberOfArrays();
  short  newAttributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];

  for (idx1 = 0; idx1 < vtkDataSetAttributes::NUM_ATTRIBUTES; ++idx1)
    {
    newAttributeIndices[idx1] = -1;
    }

  // First add ranges from all common arrays
  for (idx1 = 0; idx1 < num1; ++idx1)
    {
    int found=0;
    vtkPVArrayInformation* ai1 = this->GetArrayInformation(idx1);
    for (idx2 = 0; idx2 < num2; ++idx2)
      {
      vtkPVArrayInformation* ai2 = info->GetArrayInformation(idx2);
      if ( ai1->Compare(ai2) )
        {
        // Take union of range.
        ai1->AddRanges(ai2);
        found = 1;
        // Record default attributes.
        int attribute1 = this->IsArrayAnAttribute(idx1);
        int attribute2 = info->IsArrayAnAttribute(idx2);
        if (attribute1 > -1 && attribute1 == attribute2)
          {
          newAttributeIndices[attribute1] = idx1;
          }
        break;
        }
      }
    if (!found)
      {
      ai1->SetIsPartial(1);
      }
    }

  for (idx1 = 0; idx1 < vtkDataSetAttributes::NUM_ATTRIBUTES; ++idx1)
    {
    this->AttributeIndices[idx1] = newAttributeIndices[idx1];
    }

  // Now add arrays that don't exist
  for (idx2 = 0; idx2 < num2; ++idx2)
    {
    vtkPVArrayInformation* ai2 = info->GetArrayInformation(idx2);
    int found=0;
    for (idx1 = 0; idx1 < this->GetNumberOfArrays(); ++idx1)
      {
      vtkPVArrayInformation* ai1 = this->GetArrayInformation(idx1);
      if ( ai1->Compare(ai2) )
        {
        found=1;
        break;
        }
      }
    if (!found)
      {
      ai2->SetIsPartial(1);
      this->ArrayInformation->AddItem(ai2);
      int attribute = info->IsArrayAnAttribute(idx2);
      if (attribute > -1 && this->AttributeIndices[attribute] == -1)
        {
        this->AttributeIndices[attribute] = idx2;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVDataSetAttributesInformation* p =
    vtkPVDataSetAttributesInformation::SafeDownCast(info);
  if(p)
    {
    this->AddInformation(p);
    }
  else
    {
    vtkErrorMacro("AddInformation called with object of type "
                  << (info? info->GetClassName():"<unknown>"));
    }
}

//----------------------------------------------------------------------------
void
vtkPVDataSetAttributesInformation
::AddInformation(vtkDataSetAttributes *da)
{
  vtkPVDataSetAttributesInformation* info =
    vtkPVDataSetAttributesInformation::New();

  info->CopyFromDataSetAttributes(da);
  this->AddInformation(info);
  info->Delete();
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::IsArrayAnAttribute(int arrayIndex)
{
  int i;

  for (i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
    if (this->AttributeIndices[i] == arrayIndex)
      {
      return i;
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation*
vtkPVDataSetAttributesInformation::GetAttributeInformation(int attributeType)
{
  int arrayIdx = this->AttributeIndices[attributeType];

  if (arrayIdx < 0)
    {
    return NULL;
    }
  return this->GetArrayInformation(arrayIdx);
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::GetNumberOfArrays() const
{
  return this->ArrayInformation->GetNumberOfItems();
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::GetMaximumNumberOfTuples() const
{
  vtkPVArrayInformation* info;
  int maxNumVals = 0;

  this->ArrayInformation->InitTraversal();
  while ( (info = static_cast<vtkPVArrayInformation*>(this->ArrayInformation->GetNextItemAsObject())) )
    {
    maxNumVals = info->GetNumberOfTuples() > maxNumVals ? info->GetNumberOfTuples() : maxNumVals;
    }

  return maxNumVals;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation*
vtkPVDataSetAttributesInformation::GetArrayInformation(int idx) const
{
  return static_cast<vtkPVArrayInformation*>(this->ArrayInformation->GetItemAsObject(idx));
}

//----------------------------------------------------------------------------
vtkPVArrayInformation*
vtkPVDataSetAttributesInformation::GetArrayInformation(const char *name) const
{
  vtkPVArrayInformation* info;

  if (name == NULL)
    {
    return NULL;
    }

  this->ArrayInformation->InitTraversal();
  while ( (info = static_cast<vtkPVArrayInformation*>(this->ArrayInformation->GetNextItemAsObject())) )
    {
    if (strcmp(info->GetName(), name) == 0)
      {
      return info;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void
vtkPVDataSetAttributesInformation
::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  // Default attributes.
  *css << vtkClientServerStream::InsertArray(this->AttributeIndices, vtkDataSetAttributes::NUM_ATTRIBUTES);

  // Number of arrays.
  *css << this->GetNumberOfArrays();

  // Serialize each array's information.
  vtkClientServerStream acss;
  for(int idx=0; idx < this->GetNumberOfArrays(); ++idx)
    {
    const unsigned char* data;
    size_t length;
    this->GetArrayInformation(idx)->CopyToStream(&acss);
    acss.GetData(&data, &length);
    *css << vtkClientServerStream::InsertArray(data,
      static_cast<int>(length));
    acss.Reset();
    }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void
vtkPVDataSetAttributesInformation
::CopyFromStream(const vtkClientServerStream* css)
{
  this->ArrayInformation->RemoveAllItems();

  // Default attributes.
  if(!css->GetArgument(0, 0, this->AttributeIndices, vtkDataSetAttributes::NUM_ATTRIBUTES))
    {
    vtkErrorMacro("Error parsing default attributes from message.");
    return;
    }

  // Number of arrays.
  int numArrays = 0;
  if(!css->GetArgument(0, 1, &numArrays))
    {
    vtkErrorMacro("Error parsing number of arrays from message.");
    return;
    }

  // Each array's information.
  vtkClientServerStream acss;
  std::vector<unsigned char> data;
  for(int i=0; i < numArrays; ++i)
    {
    vtkTypeUInt32 length;
    if(!css->GetArgumentLength(0, i+2, &length))
      {
      vtkErrorMacro("Error parsing length of information for array number "
                    << i << " from message.");
      return;
      }
    data.resize(length);
    if(!css->GetArgument(0, i+2, &*data.begin(), length))
      {
      vtkErrorMacro("Error parsing information for array number "
                    << i << " from message.");
      return;
      }
    acss.SetData(&*data.begin(), length);
    vtkPVArrayInformation* ai = vtkPVArrayInformation::New();
    ai->CopyFromStream(&acss);
    this->ArrayInformation->AddItem(ai);
    ai->Delete();
    }
}
