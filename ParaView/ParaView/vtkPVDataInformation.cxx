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
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkPVConfig.h" // needed for PARAVIEW_BUILD_DEVELOPMENT
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

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVDataInformation);
vtkCxxRevisionMacro(vtkPVDataInformation, "1.19");

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

  this->DataSetType = data->GetDataObjectType();

  // Look for a name stored in Field Data.
  vtkDataArray *nameArray = data->GetFieldData()->GetArray("Name");
  if (nameArray)
    {
    char* str = static_cast<char*>(nameArray->GetVoidPointer(0));
    this->SetName(str);
    }

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
void vtkPVDataInformation::CopyToStream(vtkClientServerStream* css) const
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->Name
       << this->DataSetType
       << this->NumberOfPoints
       << this->NumberOfCells
       << this->MemorySize
       << vtkClientServerStream::InsertArray(this->Bounds, 6)
       << vtkClientServerStream::InsertArray(this->Extent, 6);

  size_t length;
  const unsigned char* data;
  vtkClientServerStream dcss;

  this->PointDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  dcss.Reset();

  this->CellDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromStream(const vtkClientServerStream* css)
{
  const char* name = 0;
  if(!css->GetArgument(0, 0, &name))
    {
    vtkErrorMacro("Error parsing name of data.");
    return;
    }
  this->SetName(name);
  if(!css->GetArgument(0, 1, &this->DataSetType))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
  if(!css->GetArgument(0, 2, &this->NumberOfPoints))
    {
    vtkErrorMacro("Error parsing number of points.");
    return;
    }
  if(!css->GetArgument(0, 3, &this->NumberOfCells))
    {
    vtkErrorMacro("Error parsing number of cells.");
    return;
    }
  if(!css->GetArgument(0, 4, &this->MemorySize))
    {
    vtkErrorMacro("Error parsing memory size.");
    return;
    }
  if(!css->GetArgument(0, 5, this->Bounds, 6))
    {
    vtkErrorMacro("Error parsing bounds.");
    return;
    }
  if(!css->GetArgument(0, 6, this->Extent, 6))
    {
    vtkErrorMacro("Error parsing extent.");
    return;
    }

  vtkTypeUInt32 length;
  vtkstd::vector<unsigned char> data;
  vtkClientServerStream dcss;

  // Point data array information.
  if(!css->GetArgumentLength(0, 7, &length))
    {
    vtkErrorMacro("Error parsing length of point data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 7, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing point data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->PointDataInformation->CopyFromStream(&dcss);

  // Cell data array information.
  if(!css->GetArgumentLength(0, 8, &length))
    {
    vtkErrorMacro("Error parsing length of point data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 8, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing point data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->CellDataInformation->CopyFromStream(&dcss);
}
