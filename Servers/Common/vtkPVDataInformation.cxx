/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataInformation.h"

#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkGenericDataSet.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVDataInformation);
vtkCxxRevisionMacro(vtkPVDataInformation, "1.5");

//----------------------------------------------------------------------------
vtkPVDataInformation::vtkPVDataInformation()
{
  this->BaseDataSetType = -1;
  this->DataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->MemorySize = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_LARGE_INTEGER;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_LARGE_INTEGER;
  this->PointDataInformation = vtkPVDataSetAttributesInformation::New();
  this->CellDataInformation = vtkPVDataSetAttributesInformation::New();
  
  this->Name = 0;
  this->DataClassName = 0;
  this->BaseDataClassName = 0;
  this->NumberOfDataSets = 0;
}

//----------------------------------------------------------------------------
vtkPVDataInformation::~vtkPVDataInformation()
{
  this->PointDataInformation->Delete();
  this->PointDataInformation = NULL;
  this->CellDataInformation->Delete();
  this->CellDataInformation = NULL;
  
  this->SetName(0);
  this->SetDataClassName(0);
  this->SetBaseDataClassName(0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DataSetType: " << this->DataSetType << endl;
  os << indent << "BaseDataSetType: " << this->BaseDataSetType << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "NumberOfCells: " << this->NumberOfCells << endl;
  os << indent << "NumberOfDataSets: " << this->NumberOfDataSets  << endl;
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

  os << indent << "DataClassName: " 
     << (this->DataClassName?this->DataClassName:"(none)") << endl;
  os << indent << "BaseDataClassName: " 
     << (this->BaseDataClassName?this->BaseDataClassName:"(none)") << endl;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::Initialize()
{
  this->DataSetType = -1;
  this->BaseDataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->NumberOfDataSets = 0;
  this->MemorySize = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_LARGE_INTEGER;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_LARGE_INTEGER;
  this->PointDataInformation->Initialize();
  this->CellDataInformation->Initialize();
  
  this->SetName(0);
  this->SetDataClassName(0);
  this->SetBaseDataClassName(0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::DeepCopy(vtkPVDataInformation *dataInfo)
{
  int idx;
  double *bounds;
  int *ext;

  this->DataSetType = dataInfo->GetDataSetType();
  this->BaseDataSetType = dataInfo->GetBaseDataSetType();
  this->SetDataClassName(dataInfo->GetDataClassName());
  this->SetBaseDataClassName(dataInfo->GetBaseDataClassName());

  this->NumberOfDataSets = dataInfo->NumberOfDataSets;

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

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromCompositeDataSet(vtkCompositeDataSet* data)
{
  int numDataSets = 0;
  vtkCompositeDataIterator* iter = data->NewIterator();
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
    {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    vtkPVDataInformation* dinf = vtkPVDataInformation::New();
    dinf->CopyFromObject(dobj);
    dinf->SetDataClassName(dobj->GetClassName());
    dinf->DataSetType = dobj->GetDataObjectType();
    this->AddInformation(dinf);
    dinf->Delete();
    numDataSets++;
    iter->GoToNextItem();
    }
  iter->Delete();

  this->SetBaseDataClassName(data->GetClassName());
  this->BaseDataSetType = data->GetDataObjectType();
  this->NumberOfDataSets = numDataSets;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromDataSet(vtkDataSet* data)
{
  int idx;
  double *bds;
  int *ext = NULL;

  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();

  this->NumberOfDataSets = 1;

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
void vtkPVDataInformation::CopyFromGenericDataSet(vtkGenericDataSet *data)
{
  int idx;
  double *bds;

  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();

  this->NumberOfDataSets = 1;

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

  // Copy Point Data information
  this->PointDataInformation->CopyFromGenericAttributesOnPoints(
    data->GetAttributes());

  // Copy Cell Data information
  this->CellDataInformation->CopyFromGenericAttributesOnCells(
    data->GetAttributes());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromObject(vtkObject* object)
{
  vtkDataObject* dobj = vtkDataObject::SafeDownCast(object);

  if (!dobj)
    {
    vtkErrorMacro("Could not cast object to a known data set: " << object);
    return;
    }

  vtkInformation* pinfo = dobj->GetPipelineInformation();

  vtkCompositeDataSet* cds = 0;
  if (pinfo && pinfo->Has(vtkCompositeDataSet::COMPOSITE_DATA_SET()))
    {
    cds = vtkCompositeDataSet::SafeDownCast(
      pinfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
    }
  
  if (!cds)
    {
    cds = vtkCompositeDataSet::SafeDownCast(object);
    }

  if (cds)
    {
    this->CopyFromCompositeDataSet(cds);
    return;
    }

  vtkDataSet* ds = vtkDataSet::SafeDownCast(object);
  if (ds)
    {
    this->CopyFromDataSet(ds);
    return;
    }

  vtkGenericDataSet* ads = vtkGenericDataSet::SafeDownCast(object);
  if (ads)
    {
    this->CopyFromGenericDataSet(ads);
    return;
    }

  vtkErrorMacro("Could not cast object to a known data set: " << object);
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

  if (info->NumberOfDataSets == 0)
    {
    return;
    }

  if (this->NumberOfPoints == 0 && this->NumberOfCells == 0 && this->NumberOfDataSets == 0)
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
      this->SetDataClassName("vtkDataSet");
      }
    else
      {
      if(this->DataSetType == VTK_GENERIC_DATA_SET ||
         info->GetDataSetType() == VTK_GENERIC_DATA_SET )
        {
        this->DataSetType = VTK_GENERIC_DATA_SET;
        this->SetDataClassName("vtkGenericDataSet");
        }
      else
        {
        this->DataSetType = VTK_POINT_SET;
        this->SetDataClassName("vtkPointSet");
        }
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
  this->NumberOfDataSets += info->GetNumberOfDataSets();

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
//  this->GenericAttributesInformation->AddInformation(info->GetGenericAttributesInformation());

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
  if (this->DataSetType == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    return "vtkHierarchicalBoxDataSet";
    }
  if (this->DataSetType == VTK_GENERIC_DATA_SET)
    {
    return "vtkGenericDataSet";
    }
  
  return "UnknownType";
}

//----------------------------------------------------------------------------
// Need to do this manually.
int vtkPVDataInformation::DataSetTypeIsA(const char* type)
{
  if (strcmp(type, "vtkDataObject") == 0)
    { // Every type is of type vtkDataObject.
    return 1;
    }
  if (strcmp(type, "vtkDataSet") == 0)
    { // Every type is of type vtkDataObject.
    if (this->DataSetType == VTK_POLY_DATA ||
        this->DataSetType == VTK_STRUCTURED_GRID ||
        this->DataSetType == VTK_UNSTRUCTURED_GRID ||
        this->DataSetType == VTK_IMAGE_DATA ||
        this->DataSetType == VTK_RECTILINEAR_GRID ||
        this->DataSetType == VTK_UNSTRUCTURED_GRID ||
        this->DataSetType == VTK_STRUCTURED_POINTS)
      {
      return 1;
      }
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
       << this->DataClassName
       << this->DataSetType
       << this->NumberOfDataSets
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

  *css << this->BaseDataClassName;
  *css << this->BaseDataSetType;

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
  const char* dataclassname = 0;
  if(!css->GetArgument(0, 1, &dataclassname))
    {
    vtkErrorMacro("Error parsing class name of data.");
    return;
    }
  this->SetDataClassName(dataclassname);
  if(!css->GetArgument(0, 2, &this->DataSetType))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
  if(!css->GetArgument(0, 3, &this->NumberOfDataSets))
    {
    vtkErrorMacro("Error parsing number of datasets.");
    return;
    }
  if(!css->GetArgument(0, 4, &this->NumberOfPoints))
    {
    vtkErrorMacro("Error parsing number of points.");
    return;
    }
  if(!css->GetArgument(0, 5, &this->NumberOfCells))
    {
    vtkErrorMacro("Error parsing number of cells.");
    return;
    }
  if(!css->GetArgument(0, 6, &this->MemorySize))
    {
    vtkErrorMacro("Error parsing memory size.");
    return;
    }
  if(!css->GetArgument(0, 7, this->Bounds, 6))
    {
    vtkErrorMacro("Error parsing bounds.");
    return;
    }
  if(!css->GetArgument(0, 8, this->Extent, 6))
    {
    vtkErrorMacro("Error parsing extent.");
    return;
    }

  vtkTypeUInt32 length;
  vtkstd::vector<unsigned char> data;
  vtkClientServerStream dcss;

  // Point data array information.
  if(!css->GetArgumentLength(0, 9, &length))
    {
    vtkErrorMacro("Error parsing length of point data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 9, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing point data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->PointDataInformation->CopyFromStream(&dcss);

  // Cell data array information.
  if(!css->GetArgumentLength(0, 10, &length))
    {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 10, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing cell data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->CellDataInformation->CopyFromStream(&dcss);

  const char* basedataclassname = 0;
  if(!css->GetArgument(0, 11, &basedataclassname))
    {
    vtkErrorMacro("Error parsing class name of data.");
    return;
    }
  this->SetBaseDataClassName(basedataclassname);

  if(!css->GetArgument(0, 12, &this->BaseDataSetType))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
}
