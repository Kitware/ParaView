/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayInformation.cxx
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
#include "vtkPVArrayInformation.h"

#include "vtkObjectFactory.h"
#include "vtkDataArray.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVArrayInformation);
vtkCxxRevisionMacro(vtkPVArrayInformation, "1.1");

//----------------------------------------------------------------------------
vtkPVArrayInformation::vtkPVArrayInformation()
{
  this->Name = NULL;
  this->DataType = VTK_VOID;
  this->NumberOfComponents = 0;
  this->Ranges = NULL;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation::~vtkPVArrayInformation()
{  
  this->SetName(NULL);
  if (this->Ranges)
    {
    delete [] this->Ranges;
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
    delete [] this->Ranges;
    this->Ranges = NULL;
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
 
  int idx;
  this->Ranges = new double[numComps*2];
  for (idx = 0; idx < numComps; ++idx)
    {
    this->Ranges[2*idx] = VTK_LARGE_FLOAT;
    this->Ranges[2*idx+1] = -VTK_LARGE_FLOAT;
    }
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
  this->Ranges[comp*2] = min;
  this->Ranges[comp*2 + 1] = max;
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
  return this->Ranges + comp*2;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetComponentRange(int comp, double *range)
{
  double *ptr;
  
  ptr = this->GetComponentRange(comp);

  if (ptr == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    return;
    }

  range[0] = ptr[0];
  range[1] = ptr[1];
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetComponentRange(int comp, float *range)
{
  double *ptr;
  
  ptr = this->GetComponentRange(comp);

  if (ptr == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    return;
    }

  range[0] = (float)(ptr[0]);
  range[1] = (float)(ptr[1]);
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddRanges(vtkPVArrayInformation *info)
{
  double *range;
  double *ptr = this->Ranges;
  int idx;

  if (this->NumberOfComponents != info->GetNumberOfComponents())
    {
    vtkErrorMacro("Component mismatch.");
    }

  if (this->NumberOfComponents > 1)
    {
    range = info->GetComponentRange(-1);
    if (range[0] < ptr[0])
      {
      ptr[0] = range[0];
      }
    if (range[1] > ptr[1])
      {
      ptr[1] = range[1];
      }
    ptr += 2;
    }

  for (idx = 0; idx < this->NumberOfComponents; ++idx)
    {
    range = info->GetComponentRange(idx);
    if (range[0] < ptr[0])
      {
      ptr[0] = range[0];
      }
    if (range[1] > ptr[1])
      {
      ptr[1] = range[1];
      }
    ptr += 2;
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::DeepCopy(vtkPVArrayInformation *info)
{
  int num, idx;

  this->SetName(info->GetName());
  this->DataType = info->GetDataType();
  this->SetNumberOfComponents(info->GetNumberOfComponents());

  num = 2*this->NumberOfComponents;
  if (this->NumberOfComponents > 1)
    {
    num += 2;
    }
  for (idx = 0; idx < num; ++idx)
    {
    this->Ranges[idx] = info->Ranges[idx];
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromArray(vtkDataArray *array)
{
  float range[2];
  double *ptr;
  int idx;

  this->SetName(array->GetName());
  this->DataType = array->GetDataType();
  this->SetNumberOfComponents(array->GetNumberOfComponents());
  ptr = this->Ranges;
  if (this->NumberOfComponents > 1)
    {
    array->GetRange(range, -1);
    *ptr++ = range[0];
    *ptr++ = range[1];
    }
  for (idx = 0; idx < this->NumberOfComponents; ++idx)
    {
    array->GetRange(range, idx);
    *ptr++ = range[0];
    *ptr++ = range[1];
    }
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::Compare(vtkPVArrayInformation *info)
{
  if (strcmp(info->GetName(), this->Name) == 0 && 
      info->GetNumberOfComponents() == this->NumberOfComponents)
    {
    return 1;
    }
  return 0;
}



//----------------------------------------------------------------------------
int vtkPVArrayInformation::GetMessageLength()
{
  int length = 0;
  
  // Short for name length.
  length += sizeof(short);
  // N (length) charaters for name (including last /0 charater).
  if (this->Name)
    {
    length += static_cast<int>(strlen(this->Name)) + 1;
    } 
  // char for a data type.
  length += 1;
  // char for number of components
  length += 1;
  // doubles for ranges.
  if (this->NumberOfComponents == 1)
    {
    length += 2*sizeof(double);
    }
  else if (this->NumberOfComponents > 1)
    {
    length += 2*(this->NumberOfComponents+1)*sizeof(double);
    }

  return length;
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::WriteMessage(unsigned char *msg)
{
  int length = 0;
  short nameLength;
  
  if (this->Name == NULL)
    {
    nameLength = 0;
    }
  else
    {
    nameLength = (int)(strlen(this->Name)) + 1;
    }

  // Short for name length.
  *((short*)msg) = nameLength;
  msg += sizeof(short); 
  length += sizeof(short);
  if (this->Name)
    {
    // N (length) charaters for name (including last /0 charater).
    memcpy(msg, this->Name, nameLength);
    msg += nameLength;
    length += nameLength;
    } 

  // char for a data type.
  *msg = (unsigned char)(this->DataType);
  msg += 1;
  length += 1;
  // char for number of components
  *msg = (unsigned char)(this->NumberOfComponents);
  msg += 1;
  length += 1;

  // doubles for ranges.
  int num, idx;
  num = this->NumberOfComponents;
  if (num > 1)
    {
    ++num;
    }
  for (idx = 0; idx < num; ++idx)
    {
    *((double*)msg) = this->Ranges[2*idx];
    msg += sizeof(double);
    length += sizeof(double);
    *((double*)msg) = this->Ranges[2*idx + 1];
    msg += sizeof(double);
    length += sizeof(double);
    }

  return length;
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::CopyFromMessage(unsigned char *msg)
{
  int length = 0;
  short nameLength;
  
  // Clear out some info.
  if (this->Name)
    {
    delete [] this->Name;
    this->Name = NULL;
    }

  // Short for name length.
  nameLength = *((short*)msg);
  msg += sizeof(short); 
  length += sizeof(short);

  if (nameLength > 0)
    {
    this->SetName((char*)msg);
    msg += nameLength;
    length += nameLength;
    } 

  // char for a data type.
  this->DataType = *((unsigned char*)msg);
  msg += 1;
  length += 1;
  // char for number of components
  this->SetNumberOfComponents(*((unsigned char*)msg));
  msg += 1;
  length += 1;

  // doubles for ranges.
  int num, idx;
  num = this->NumberOfComponents;
  if (num > 1)
    {
    ++num;
    }
  for (idx = 0; idx < num; ++idx)
    {
    this->Ranges[2*idx] = *((double*)msg);
    msg += sizeof(double);
    length += sizeof(double);
    this->Ranges[2*idx + 1] = *((double*)msg);
    msg += sizeof(double);
    length += sizeof(double);
    }

  return length;
}


//----------------------------------------------------------------------------
void vtkPVArrayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  int num, idx;
  vtkIndent i2 = indent.GetNextIndent();

  this->Superclass::PrintSelf(os,indent);
  if (this->Name)
    {
    os << indent << "Name: " << this->Name << endl;
    }
  os << indent << "DataType: " << this->DataType << endl;
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;

  os << indent << "Ranges :" << endl;
  num = this->NumberOfComponents;
  if (num > 1)
    {
    ++num;
    }
  for (idx = 0; idx < num; ++idx)
    {
    os << i2 << this->Ranges[2*idx] << ", " << this->Ranges[2*idx+1] << endl;
    }
}


  



