/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataSetAttributesInformation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVDataSetAttributesInformation.h"

#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkCollection.h"
#include "vtkPVArrayInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataSetAttributesInformation);
vtkCxxRevisionMacro(vtkPVDataSetAttributesInformation, "1.1");


//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation::vtkPVDataSetAttributesInformation()
{
  int idx;

  this->ArrayInformation = vtkCollection::New();
  for (idx = 0; idx < 5; ++idx)
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
void vtkPVDataSetAttributesInformation::Initialize()
{
  int idx;

  this->ArrayInformation->RemoveAllItems();
  for (idx = 0; idx < 5; ++idx)
    {
    this->AttributeIndices[idx] = -1;
    }

}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::DeepCopy(vtkPVDataSetAttributesInformation *dataInfo)
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
  for (idx = 0; idx < 5; ++idx)
    {
    this->AttributeIndices[idx] = dataInfo->AttributeIndices[idx];
    }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::CopyFromDataSetAttributes(vtkDataSetAttributes *da)
{
  int idx;
  int num;
  vtkDataArray *array;
  int infoArrayIndex;
  int attribute;

  // Clear array information.
  this->ArrayInformation->RemoveAllItems();
  for (idx = 0; idx < 5; ++idx)
    {
    this->AttributeIndices[idx] = -1;
    }

  // Copy Point Data
  num = da->GetNumberOfArrays(); 
  infoArrayIndex = 0;
  for (idx = 0; idx < num; ++idx)
    {
    array = da->GetArray(idx);
    if (array->GetName() )
      {
      vtkPVArrayInformation *info = vtkPVArrayInformation::New();
      info->CopyFromArray(array);
      this->ArrayInformation->AddItem(info);
      info->Delete();
      // Record default attributes.
      attribute = da->IsArrayAnAttribute(idx);
      if (attribute > -1)
        {
        this->AttributeIndices[attribute] = infoArrayIndex;
        }
      ++infoArrayIndex; 
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::AddInformation(vtkPVDataSetAttributesInformation *info)
{
  int                    num1, num2, idx1, idx2;
  vtkPVArrayInformation* ai1;
  vtkPVArrayInformation* ai2;
  int                    attribute1, attribute2;
  int                    infoArrayIndex;
  int                    newAttributeIndices[5]; 
  vtkCollection*         newArrayInformation;

  // Combine point array information.
  infoArrayIndex = 0;
  newArrayInformation = vtkCollection::New();
  num1 = this->GetNumberOfArrays();
  num2 = info->GetNumberOfArrays();
  for (idx1 = 0; idx1 < num1; ++idx1)
    {
    ai1 = this->GetArrayInformation(idx1);
    // First find a match for array1 in info.
    idx2 = idx1;
    ai2 = info->GetArrayInformation(idx1);
    if ( ai2 && ai1->Compare(ai2) == 0 )
      { // Arrays are not in the same order.  Try to find a match.
      for (idx2 = 0; idx2 < num2; ++idx2)
        {
        ai2 = info->GetArrayInformation(idx2);
        if ( ai1->Compare(ai2) )
          {
          break;
          }
        else
          {
          ai2 = NULL;
          }
        }
      } 
    // If match, save.
    if (ai2)
      {
      // Take union of range.
      ai1->AddRanges(ai2);
      // Now add to new collection.
      newArrayInformation->AddItem(ai1);
      // Record default attributes.
      attribute1 = this->IsArrayAnAttribute(idx1);
      attribute2 = info->IsArrayAnAttribute(idx2);
      if (attribute1 > -1 && attribute1 == attribute2)
        {
        newAttributeIndices[attribute1] = infoArrayIndex;
        }
      ++infoArrayIndex; 
      }
    }
  // Now set the new array information and attributes.
  this->ArrayInformation->Delete();
  this->ArrayInformation = newArrayInformation;
  for (idx1  = 0; idx1 < 5; ++idx1)
    {
    this->AttributeIndices[idx1] = newAttributeIndices[idx1];
    }
}

//----------------------------------------------------------------------------
void vtkPVDataSetAttributesInformation::AddInformation(vtkDataSetAttributes *da)
{
  vtkPVDataSetAttributesInformation *info = vtkPVDataSetAttributesInformation::New();

  info->CopyFromDataSetAttributes(da);
  this->AddInformation(info);
  info->Delete();
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::IsArrayAnAttribute(int arrayIndex)
{
  int i;

  for (i = 0; i < 5; ++i)
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
int vtkPVDataSetAttributesInformation::GetMessageLength()
{
  int length = 0;
  int num, idx;
  vtkPVArrayInformation *ai;

  //   - 5 shorts for default attributes
  //   - A short for number of point arrays.
  //     Each Array taken care of by the vtkPVArrayInformation.

  length = 6*sizeof(short);

  // Now add space for all of the arrays.
  num = this->GetNumberOfArrays();
  for (idx = 0; idx < num; ++idx)
    {
    ai = this->GetArrayInformation(idx);
    length += ai->GetMessageLength();
    }

  return length;
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::WriteMessage(unsigned char *msg)
{
  int idx;
  int length = 0;
  int arrayMsgLength;
  short num;
  vtkPVArrayInformation *ai;
  
  // Shorts for default attributes.
  for (idx = 0; idx < 5; ++idx)
    {
    *((short*)msg) = static_cast<short>(this->AttributeIndices[0]);
    msg += sizeof(short); 
    length += sizeof(short);
    }

  // Number of arrays
  num = (short)(this->GetNumberOfArrays());
  *((short*)msg) = num;
  msg += sizeof(short);
  length += sizeof(short);
  for (idx = 0; idx < num; ++idx)
    {
    ai = this->GetArrayInformation(idx);
    arrayMsgLength = ai->WriteMessage(msg);
    msg += arrayMsgLength;
    length += arrayMsgLength;
    }

  return length;
}

//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::CopyFromMessage(unsigned char *msg)
{
  int length = 0;
  int idx;
  short num;
  vtkPVArrayInformation *ai;
  int arrayMsgLength;

  // Clear out all arrays.
  this->ArrayInformation->RemoveAllItems();

  // Standard attributes
  for (idx = 0; idx < 5; ++idx)
    {
    this->AttributeIndices[idx] = *((short*)msg);
    msg += sizeof(short);
    length += sizeof(short);
    }
  // Number of arrays
  num = *((short*)msg);
  msg += sizeof(short);
  length += sizeof(short);
  for (idx = 0; idx < num; ++idx)
    {
    ai = vtkPVArrayInformation::New();
    arrayMsgLength = ai->CopyFromMessage(msg);
    msg += arrayMsgLength;
    length += arrayMsgLength;
    this->ArrayInformation->AddItem(ai);
    ai->Delete();
    ai = NULL;
    }

  return length;
}



//----------------------------------------------------------------------------
int vtkPVDataSetAttributesInformation::GetNumberOfArrays()
{
  return this->ArrayInformation->GetNumberOfItems();
}
//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataSetAttributesInformation::GetArrayInformation(int idx)
{
  return static_cast<vtkPVArrayInformation*>(this->ArrayInformation->GetItemAsObject(idx));
}
//----------------------------------------------------------------------------
vtkPVArrayInformation* 
vtkPVDataSetAttributesInformation::GetArrayInformation(const char *name)
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
void vtkPVDataSetAttributesInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPVArrayInformation *ai;
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os,indent);

  int num, idx;
  num = this->GetNumberOfArrays();
  os << indent << "ArrayInformation " << num << endl;
  for (idx = 0; idx < num; ++idx)
    {
    ai = this->GetArrayInformation(idx);
    ai->PrintSelf(os, i2);
    }
}


  



