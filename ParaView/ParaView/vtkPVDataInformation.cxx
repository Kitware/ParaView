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

#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#ifdef PARAVIEW_BUILD_DEVELOPMENT
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#endif
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataInformation);
vtkCxxRevisionMacro(vtkPVDataInformation, "1.16");


//----------------------------------------------------------------------------
vtkPVDataInformation::vtkPVDataInformation()
{
  this->DataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->MemorySize = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_LARGE_INTEGER;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_LARGE_INTEGER;
  this->PointDataInformation = vtkPVDataSetAttributesInformation::New();
  this->CellDataInformation = vtkPVDataSetAttributesInformation::New();

  this->Name = NULL;
}

//----------------------------------------------------------------------------
vtkPVDataInformation::~vtkPVDataInformation()
{
  this->PointDataInformation->Delete();
  this->PointDataInformation = NULL;  
  this->CellDataInformation->Delete();
  this->CellDataInformation = NULL;  
  this->SetName(NULL);
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
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_LARGE_INTEGER;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_LARGE_INTEGER;
  this->PointDataInformation->Initialize();
  this->CellDataInformation->Initialize();

  this->SetName(NULL);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::DeepCopy(vtkPVDataInformation *dataInfo)
{
  int idx;
  double *bounds;
  int *ext;

  this->DataSetType = dataInfo->GetDataSetType();

  this->NumberOfPoints = dataInfo->GetNumberOfPoints();
  this->NumberOfCells = dataInfo->GetNumberOfCells();
  this->MemorySize = dataInfo->GetMemorySize();

  bounds = dataInfo->GetBounds();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Bounds[idx] = bounds[idx];
    }
  ext = dataInfo->GetExtent();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Extent[idx] = ext[idx];
    }

  // Copy attribute information.
  this->PointDataInformation->DeepCopy(dataInfo->GetPointDataInformation());
  this->CellDataInformation->DeepCopy(dataInfo->GetCellDataInformation());

  this->SetName(dataInfo->GetName());
}

#ifdef PARAVIEW_BUILD_DEVELOPMENT
//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromCompositeDataSet(vtkCompositeDataSet* data)
{
  vtkCompositeDataIterator* iter = data->NewIterator();
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
    {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    vtkPVDataInformation* dinf = vtkPVDataInformation::New();
    dinf->CopyFromObject(dobj);
    this->AddInformation(dinf);
    dinf->Delete();
    iter->GoToNextItem();
    }
  iter->Delete();
  this->DataSetType = data->GetDataObjectType();
}
#else
class vtkCompositeDataSet
{
};
//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromCompositeDataSet(vtkCompositeDataSet*)
{
}
#endif

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromDataSet(vtkDataSet* data)
{
  int idx;
  float *bds;
  int *ext = NULL;

  this->NumberOfPoints = data->GetNumberOfPoints();
  if (!this->NumberOfPoints)
    {
    return;
    }
  this->NumberOfCells = data->GetNumberOfCells();
  bds = data->GetBounds();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Bounds[idx] = bds[idx];
    }
  this->MemorySize = data->GetActualMemorySize();
  
  this->DataSetType = data->GetDataObjectType();
  if (this->DataSetType == VTK_IMAGE_DATA)
    {
    ext = static_cast<vtkImageData*>(data)->GetExtent();
    }
  if (this->DataSetType == VTK_STRUCTURED_GRID)
    {
    ext = static_cast<vtkStructuredGrid*>(data)->GetExtent();
    }
  if (this->DataSetType == VTK_RECTILINEAR_GRID)
    {
    ext = static_cast<vtkRectilinearGrid*>(data)->GetExtent();
    }
  if (ext)
    {
    for (idx = 0; idx < 6; ++idx)
      {
      this->Extent[idx] = ext[idx];
      }
    }

  // Copy Point Data information
  this->PointDataInformation->CopyFromDataSetAttributes(data->GetPointData());

  // Copy Cell Data information
  this->CellDataInformation->CopyFromDataSetAttributes(data->GetCellData());
 
  // Look for a name stored in Field Data.
  vtkDataArray *nameArray = data->GetFieldData()->GetArray("Name");
  if (nameArray)
    {
    char* str = static_cast<char*>(nameArray->GetVoidPointer(0));
    this->SetName(str);
    }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromObject(vtkObject* object)
{
  vtkDataSet* ds = vtkDataSet::SafeDownCast(object);
  if (ds)
    {
    this->CopyFromDataSet(ds);
    return;
    }

#ifdef PARAVIEW_BUILD_DEVELOPMENT
  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(object);
  if (cds)
    {
    this->CopyFromCompositeDataSet(cds);
    return;
    }
#endif

  vtkErrorMacro("Cound not cast object to data set.");
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(vtkPVInformation* pvi)
{
  vtkPVDataInformation *info;
  int             i,j;
  double*         bounds;
  int*            ext;

  info = vtkPVDataInformation::SafeDownCast(pvi);
  if (info == NULL)
    {
    vtkErrorMacro("Cound not cast object to data information.");
    return;
    }

  if (this->NumberOfPoints == 0 && this->NumberOfCells == 0)
    { // Just copy the other array information.
    this->DeepCopy(info);
    return;
    }

  // For data set, lets pick the common super class.
  // This supports Heterogeneous collections.
  // We need a new classification: Structured.
  // This would allow extracting grid from mixed structured collections.
  if (this->DataSetType != info->GetDataSetType())
    { // IsTypeOf method will not work here.  Must be done manually.
    if (this->DataSetType == VTK_IMAGE_DATA ||
        this->DataSetType == VTK_RECTILINEAR_GRID ||
        this->DataSetType == VTK_DATA_SET ||
        info->GetDataSetType() == VTK_IMAGE_DATA ||
        info->GetDataSetType() == VTK_RECTILINEAR_GRID ||
        info->GetDataSetType() == VTK_DATA_SET)
      {
      this->DataSetType = VTK_DATA_SET;
      }
    else
      {
      this->DataSetType = VTK_POINT_SET;
      }
    }

  // Empty data set? Ignore bounds, extent and array info.
  if (info->GetNumberOfCells() == 0 && info->GetNumberOfPoints() == 0)
    {
    return;
    }

  // First the easy stuff.
  this->NumberOfPoints += info->GetNumberOfPoints();
  this->NumberOfCells += info->GetNumberOfCells();
  this->MemorySize += info->GetMemorySize();

  // Bounds are only a little harder.
  bounds = info->GetBounds();
  ext = info->GetExtent();
  for (i = 0; i < 3; ++i)
    {
    j = i*2;
    if (bounds[j] < this->Bounds[j])
      {
      this->Bounds[j] = bounds[j];
      }
    if (ext[j] < this->Extent[j])
      {
      this->Extent[j] = ext[j];
      }
    ++j;
    if (bounds[j] > this->Bounds[j])
      {
      this->Bounds[j] = bounds[j];
      }
    if (ext[j] > this->Extent[j])
      {
      this->Extent[j] = ext[j];
      }
    }


  // Now for the messy part, all of the arrays.
  this->PointDataInformation->AddInformation(info->GetPointDataInformation());
  this->CellDataInformation->AddInformation(info->GetCellDataInformation());

  if (this->Name == NULL)
    {
    this->SetName(info->GetName());
    }
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
  if (this->DataSetType == VTK_COMPOSITE_DATA_SET)
    {
    return "vtkCompositeDataSet";
    }
  if (this->DataSetType == VTK_MULTI_BLOCK_DATA_SET)
    {
    return "vtkMultiBlockDataSet";
    }
  if (this->DataSetType == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    return "vtkHierarchicalBoxDataSet";
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
  if (strcmp(type, "vtkStructuredData") == 0)
    {
    if (this->DataSetType == VTK_IMAGE_DATA || 
        this->DataSetType == VTK_STRUCTURED_GRID ||
        this->DataSetType == VTK_RECTILINEAR_GRID)
      {
      return 1;
      }
    }

  return 0;
}



//----------------------------------------------------------------------------
//void vtkPVDataInformation::AddInformation(vtkDataSet *data)
//{
// vtkPVDataInformation *info = vtkPVDataInformation::New();
//
//  info->CopyFromData(data);
//  this->AddInformation(info);
//  info->Delete();
//}


//----------------------------------------------------------------------------
int vtkPVDataInformation::GetMessageLength()
{
  int length;
 
  // Figure out message length, and allocate memory.
  // 1- First byte is a flag specifying big or little endian (ignore for now).
  // 1- Second byte specifies the data set type.
  length = 2*sizeof(unsigned char);
  
  // - vtkIdType for numberOfPoints
  // - vtkIdType for numberOfCells
  length += 2*sizeof(vtkIdType);

  // 1 int for memory size.
  length += 1*sizeof(int);

  // - 6 doubles for bounds.
  // - 6 integers for extent.
  length +=  + 6*sizeof(double) + 6*sizeof(int);

  // - For each data set attributes ...
  // Now add space for all of the cell and point data information.
  length += this->PointDataInformation->GetMessageLength();
  length += this->CellDataInformation->GetMessageLength();

  // One char for length,
  int nameLength = 0;
  if (this->Name)
    {
    nameLength = static_cast<int>(strlen(this->Name));
    }
  if (nameLength > 255)
    {
    nameLength = 255;
    }
  length += (nameLength+1) * sizeof(unsigned char);

  return length;
}




//----------------------------------------------------------------------------
void vtkPVDataInformation::WriteMessage(unsigned char* msg)
{
  int attrMsgLength;
  unsigned char* tmp;
  int idx;

  int nameLength = 0;
  if (this->Name)
    {
    nameLength = static_cast<int>(strlen(this->Name));
    }
  if (nameLength > 255)
    {
    nameLength = 255;
    }

  // Start filling in the message.
  tmp = msg;
#ifdef VTK_WORDS_BIGENDIAN
  *tmp = (unsigned char)(1);
#else
  *tmp = (unsigned char)(0);
#endif
  tmp += sizeof(unsigned char);
  *tmp = (unsigned char)(this->DataSetType);
  tmp += sizeof(unsigned char);
  memcpy(tmp, (unsigned char*)&this->NumberOfPoints, sizeof(vtkIdType));
  tmp += sizeof(vtkIdType);
  memcpy(tmp, (unsigned char*)&this->NumberOfCells, sizeof(vtkIdType));
  tmp += sizeof(vtkIdType);
  // Memory Size
  memcpy(tmp, (unsigned char*)&this->MemorySize, sizeof(int));
  tmp += sizeof(int);

  // Bounds
  for (idx = 0; idx < 6; ++idx)
    {
    memcpy(tmp, (unsigned char*)&this->Bounds[idx], sizeof(double));
    tmp += sizeof(double);
    }
  // Extent
  for (idx = 0; idx < 6; ++idx)
    {
    memcpy(tmp, (unsigned char*)&this->Extent[idx], sizeof(int));
    tmp += sizeof(int);
    }

  // Point data
  attrMsgLength = this->PointDataInformation->WriteMessage(tmp);
  tmp += attrMsgLength;

  // Cell data
  attrMsgLength = this->CellDataInformation->WriteMessage(tmp);
  tmp += attrMsgLength;

  // Name
  *tmp = static_cast<unsigned char>(nameLength);
  ++tmp;
  for (idx = 0; idx < nameLength; ++idx)
    {
    tmp[idx] = static_cast<unsigned char>(this->Name[idx]);
    }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromMessage(unsigned char *msg)
{
  unsigned char bigEndianFlag;
  int swap = 0;
  unsigned char* tmp;
  int attrMsgLength;
  int idx;

  if (msg == NULL)
    { // Something really bad has happend ...
    return;
    }

#ifdef VTK_WORDS_BIGENDIAN
  bigEndianFlag = (unsigned char)(1);
#else
  bigEndianFlag = (unsigned char)(0);
#endif

  tmp = msg;
  if (bigEndianFlag != *tmp)
    {
    swap = 1;
    }
  tmp += 1;

  this->DataSetType = *tmp;
  tmp += 1;

  if (swap) {vtkByteSwap::SwapVoidRange((void *)tmp, 1, sizeof(vtkIdType));}
  memcpy((unsigned char*)&this->NumberOfPoints, tmp, sizeof(vtkIdType));
  tmp += sizeof(vtkIdType);

  if (swap) {vtkByteSwap::SwapVoidRange((void *)tmp, 1, sizeof(vtkIdType));}
  memcpy((unsigned char*)&this->NumberOfCells, tmp, sizeof(vtkIdType));
  tmp += sizeof(vtkIdType);

  if (swap) {vtkByteSwap::SwapVoidRange((void *)tmp, 1, sizeof(int));}
  memcpy((unsigned char*)&this->MemorySize, tmp, sizeof(int));
  tmp += sizeof(int);

  // Bounds
  if (swap) {vtkByteSwap::SwapVoidRange((void *)tmp, 6, sizeof(double));}
  for (idx = 0; idx < 6; ++idx)
    {
    memcpy((unsigned char*)&this->Bounds[idx], tmp, sizeof(double));
    tmp += sizeof(double);
    }

  // Extent
  if (swap) {vtkByteSwap::SwapVoidRange((void *)tmp, 6, sizeof(int));}
  for (idx = 0; idx < 6; ++idx)
    {
    memcpy((unsigned char*)&this->Extent[idx], tmp, sizeof(int));
    tmp += sizeof(int);
    }

  // Point data
  attrMsgLength = this->PointDataInformation->CopyFromMessage(tmp, swap);
  tmp += attrMsgLength;

  // Cell data
  attrMsgLength = this->CellDataInformation->CopyFromMessage(tmp, swap);
  tmp += attrMsgLength;

  int nameLength = *tmp;
  ++tmp;
  // Manually allocate and copy because their is no ending \0.
  this->SetName(NULL);
  this->Name = new char[nameLength + 1];
  strncpy(this->Name, (char*)(tmp), nameLength);
  this->Name[nameLength] = '\0';
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
  os << indent << "Extent: " << this->Extent[0] << ", " << this->Extent[1] 
     << ", " << this->Extent[2] << ", " << this->Extent[3] 
     << ", " << this->Extent[4] << ", " << this->Extent[5] << endl;
  os << indent << "PointDataInformation " << endl;
  this->PointDataInformation->PrintSelf(os, i2);
  os << indent << "CellDataInformation " << endl;
  this->CellDataInformation->PrintSelf(os, i2);

  if (this->Name)
    {
    os << indent << "Name: " << this->Name << endl;
    }
  else  
    {
    os << indent << "Name: NULL\n";
    }
}

  










