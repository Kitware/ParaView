/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataInformation.cxx
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
#include "vtkPVDataInformation.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkCollection.h"
#include "vtkPVDataSetAttributesInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataInformation);
vtkCxxRevisionMacro(vtkPVDataInformation, "1.1");


//----------------------------------------------------------------------------
vtkPVDataInformation::vtkPVDataInformation()
{
  this->DataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->MemorySize = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
  this->PointDataInformation = vtkPVDataSetAttributesInformation::New();
  this->CellDataInformation = vtkPVDataSetAttributesInformation::New();
}

//----------------------------------------------------------------------------
vtkPVDataInformation::~vtkPVDataInformation()
{
  this->PointDataInformation->Delete();
  this->PointDataInformation = NULL;  
  this->CellDataInformation->Delete();
  this->CellDataInformation = NULL;  
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::GetBounds(float *bds)
{
  int idx;
  for (idx = 0; idx < 6; ++idx)
    {
    bds[idx] = (float)(this->Bounds[idx]);
    }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::Initialize()
{
  this->DataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->MemorySize = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
  this->PointDataInformation->Initialize();
  this->CellDataInformation->Initialize();
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::DeepCopy(vtkPVDataInformation *dataInfo)
{
  int idx;
  double *bounds;

  this->DataSetType = dataInfo->GetDataSetType();

  this->NumberOfPoints = dataInfo->GetNumberOfPoints();
  this->NumberOfCells = dataInfo->GetNumberOfCells();
  this->MemorySize = dataInfo->GetMemorySize();

  bounds = dataInfo->GetBounds();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Bounds[idx] = bounds[idx];
    }

  // Copy attribute information.
  this->PointDataInformation->DeepCopy(dataInfo->GetPointDataInformation());
  this->CellDataInformation->DeepCopy(dataInfo->GetCellDataInformation());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromData(vtkDataSet *data)
{
  int idx;
  float *bds;

  this->NumberOfPoints = data->GetNumberOfPoints();
  this->NumberOfCells = data->GetNumberOfCells();
  bds = data->GetBounds();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Bounds[idx] = bds[idx];
    }
  this->MemorySize = data->GetActualMemorySize();
  
  this->DataSetType = data->GetDataObjectType();

  // Copy Point Data information
  this->PointDataInformation->CopyFromDataSetAttributes(data->GetPointData());

  // Copy Cell Data information
  this->CellDataInformation->CopyFromDataSetAttributes(data->GetCellData());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(vtkPVDataInformation *info)
{
  int             i,j;
  double*         bounds;

  // Empty data set? Just use nonempty one.
  if (info->GetNumberOfCells() == 0 && info->GetNumberOfPoints() == 0)
    {
    return;
    }
  if (this->NumberOfPoints == 0 && this->NumberOfCells == 0)
    { // Just copy the other array information.
    this->DeepCopy(info);
    return;
    }

  // First the easy stuff.
  this->NumberOfPoints += info->GetNumberOfPoints();
  this->NumberOfCells += info->GetNumberOfCells();
  this->MemorySize += info->GetMemorySize();

  // Bounds are only a little harder.
  bounds = info->GetBounds();
  for (i = 0; i < 3; ++i)
    {
    j = i*2;
    if (bounds[j] < this->Bounds[j])
      {
      this->Bounds[j] = bounds[j];
      }
    ++j;
    if (bounds[j] > this->Bounds[j])
      {
      this->Bounds[j] = bounds[j];
      }
    }

  // For data set, lets pick the common super class.
  // Initially, groups will be homogenous, but If we find a good user 
  // interface, we sould be able to support heterogeneous groups.
  if (this->DataSetType != info->GetDataSetType())
    { // IsTypeOf method will not work here.  Must be done manually.
    if (this->DataSetType == VTK_IMAGE_DATA ||
        this->DataSetType == VTK_RECTILINEAR_GRID ||
        this->DataSetType == VTK_DATA_SET ||
        info->GetDataSetType() == VTK_IMAGE_DATA == 0 ||
        info->GetDataSetType() == VTK_RECTILINEAR_GRID == 0 ||
        info->GetDataSetType() == VTK_DATA_SET == 0)
      {
      this->DataSetType = VTK_DATA_SET;
      }
    else
      {
      this->DataSetType = VTK_POINT_SET;
      }
    }

  // Now for the messy part, all of the arrays.
  this->PointDataInformation->AddInformation(info->GetPointDataInformation());
  this->CellDataInformation->AddInformation(info->GetCellDataInformation());
}


//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetDataSetTypeAsString()
{
  if (this->DataSetType == VTK_IMAGE_DATA)
    {
    return "vtkImageData";
    }
  if (this->DataSetType == VTK_RECTILINEAR_GRID)
    {
    return "vtkRectilinearGrid";
    }
  if (this->DataSetType == VTK_STRUCTURED_GRID)
    {
    return "vtkStructuredGrid";
    }
  if (this->DataSetType == VTK_POLY_DATA)
    {
    return "vtkPolyData";
    }
  if (this->DataSetType == VTK_UNSTRUCTURED_GRID)
    {
    return "vtkUnstructuredGrid";
    }
  if (this->DataSetType == VTK_DATA_SET)
    {
    return "vtkDataSet";
    }
  if (this->DataSetType == VTK_POINT_SET)
    {
    return "vtkPointSet";
    }

  return "UnknownType";
}

//----------------------------------------------------------------------------
// Need to do this manually.
int vtkPVDataInformation::DataSetTypeIsA(const char* type)
{
  if (strcmp(type, "vtkDataSet") == 0)
    { // Every type is of type vtkDataSet.
    return 1;
    }
  if (strcmp(type, this->GetDataSetTypeAsString()) == 0)
    { // If class names are the same, then they are of the same type.
    return 1;
    }
  if (strcmp(type, "vtkPointSet") == 0)
    {
    if (this->DataSetType == VTK_POLY_DATA || 
        this->DataSetType == VTK_STRUCTURED_GRID ||
        this->DataSetType == VTK_UNSTRUCTURED_GRID)
      {
      return 1;
      }
    }

  return 0;
}



//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(vtkDataSet *data)
{
  vtkPVDataInformation *info = vtkPVDataInformation::New();

  info->CopyFromData(data);
  this->AddInformation(info);
  info->Delete();
}


//----------------------------------------------------------------------------
unsigned char* vtkPVDataInformation::NewMessage(int &length)
{
  unsigned char* msg;
  unsigned char* tmp;
  int attrMsgLength;
  int idx;

  // Figure out message length, and allocate memory.
  // 1- First byte is a flag specifying big or little endian (ignore for now).
  // 1- Second byte specifies the data set type.  
  // - vtkIdType for numberOfPoints
  // - vtkIdType for numberOfCells
  // - 6 doubles for bounds.
  // - For each data set attributes ...

  length = 2 + 2*sizeof(vtkIdType) + 6*sizeof(double);

  // Now add space for all of the cell and point data information.
  length += this->PointDataInformation->GetMessageLength();
  length += this->CellDataInformation->GetMessageLength();

  // Allocate memory for the message. 
  msg = new unsigned char[length];

  // Start filling in the message.
  tmp = msg;
#ifdef VTK_WORDS_BIGENDIAN
  *tmp = (unsigned char)(1);
#else
  *tmp = (unsigned char)(0);
#endif
  tmp += 1;
  *tmp = (unsigned char)(this->DataSetType);
  tmp += 1;
  *((vtkIdType*)tmp) = this->NumberOfPoints;
  tmp += sizeof(vtkIdType);
  *((vtkIdType*)tmp) = this->NumberOfCells;
  tmp += sizeof(vtkIdType);
  // Bounds
  for (idx = 0; idx < 6; ++idx)
    {
    *((double*)tmp) = this->Bounds[idx];
    tmp += sizeof(double);
    }

  // Point data
  attrMsgLength = this->PointDataInformation->WriteMessage(tmp);
  tmp += attrMsgLength;

  // Cell data
  attrMsgLength = this->CellDataInformation->WriteMessage(tmp);
  tmp += attrMsgLength;

  return msg;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromMessage(unsigned char *msg)
{
  unsigned char bigEndianFlag;
  unsigned char* tmp;
  int attrMsgLength;
  int idx;

#ifdef VTK_WORDS_BIGENDIAN
  bigEndianFlag = (unsigned char)(1);
#else
  bigEndianFlag = (unsigned char)(0);
#endif

  tmp = msg;
  if (bigEndianFlag != *tmp)
    {
    vtkErrorMacro("We do not support heterogenious communication yet.");
    return;
    }
  tmp += 1;

  this->DataSetType = *tmp;
  tmp += 1;

  this->NumberOfPoints = *((vtkIdType*)tmp);
  tmp += sizeof(vtkIdType);

  this->NumberOfCells = *((vtkIdType*)tmp);
  tmp += sizeof(vtkIdType);

  // Bounds
  for (idx = 0; idx < 6; ++idx)
    {
    this->Bounds[idx] = *((double*)tmp);
    tmp += sizeof(double);
    }

  // Point data
  attrMsgLength = this->PointDataInformation->CopyFromMessage(tmp);
  tmp += attrMsgLength;

  // Cell data
  attrMsgLength = this->CellDataInformation->CopyFromMessage(tmp);
  tmp += attrMsgLength;
}


//----------------------------------------------------------------------------
void vtkPVDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DataSetType: " << this->DataSetType << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "NumberOfCells: " << this->NumberOfCells << endl;
  os << indent << "MemorySize: " << this->MemorySize << endl;
  os << indent << "Bounds: " << this->Bounds[0] << ", " << this->Bounds[1] 
     << ", " << this->Bounds[2] << ", " << this->Bounds[3] 
     << ", " << this->Bounds[4] << ", " << this->Bounds[5] << endl;

  os << indent << "PointDataInformation " << endl;
  this->PointDataInformation->PrintSelf(os, i2);
  os << indent << "CellDataInformation " << endl;
  this->CellDataInformation->PrintSelf(os, i2);
}


  



