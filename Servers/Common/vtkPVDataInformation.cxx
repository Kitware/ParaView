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

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGenericDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkRectilinearGrid.h"
#include "vtkSource.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVDataInformation);
vtkCxxRevisionMacro(vtkPVDataInformation, "1.18");

//----------------------------------------------------------------------------
vtkPVDataInformation::vtkPVDataInformation()
{
  this->CompositeDataSetType = -1;
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

  this->CompositeDataInformation = vtkPVCompositeDataInformation::New();

  this->PointArrayInformation = vtkPVArrayInformation::New();
  
  this->Name = 0;
  this->DataClassName = 0;
  this->CompositeDataClassName = 0;
  this->NumberOfDataSets = 0;
  this->NameSetToDefault = 0;
}

//----------------------------------------------------------------------------
vtkPVDataInformation::~vtkPVDataInformation()
{
  this->PointDataInformation->Delete();
  this->PointDataInformation = NULL;
  this->CellDataInformation->Delete();
  this->CellDataInformation = NULL;
  this->CompositeDataInformation->Delete();
  this->CompositeDataInformation = NULL;
  this->PointArrayInformation->Delete();
  this->PointArrayInformation = NULL;
  
  this->SetName(0);
  this->SetDataClassName(0);
  this->SetCompositeDataClassName(0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DataSetType: " << this->DataSetType << endl;
  os << indent << "CompositeDataSetType: " << this->CompositeDataSetType << endl;
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
  os << indent << "CompositeDataInformation " << endl;
  this->CompositeDataInformation->PrintSelf(os, i2);
  os << indent << "PointArrayInformation " << endl;
  this->PointArrayInformation->PrintSelf(os, i2);

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
  os << indent << "CompositeDataClassName: " 
     << (this->CompositeDataClassName?this->CompositeDataClassName:"(none)") << endl;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::Initialize()
{
  this->DataSetType = -1;
  this->CompositeDataSetType = -1;
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
  this->CompositeDataInformation->Initialize();
  this->PointArrayInformation->Initialize();
  
  this->SetName(0);
  this->SetDataClassName(0);
  this->SetCompositeDataClassName(0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::DeepCopy(vtkPVDataInformation *dataInfo)
{
  int idx;
  double *bounds;
  int *ext;

  this->DataSetType = dataInfo->GetDataSetType();
  this->CompositeDataSetType = dataInfo->GetCompositeDataSetType();
  this->SetDataClassName(dataInfo->GetDataClassName());
  this->SetCompositeDataClassName(dataInfo->GetCompositeDataClassName());

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
  this->CompositeDataInformation->AddInformation(
    dataInfo->GetCompositeDataInformation());
  this->PointArrayInformation->AddInformation(
    dataInfo->GetPointArrayInformation());

  this->SetName(dataInfo->GetName());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromCompositeDataSet(vtkCompositeDataSet* data)
{
  this->Initialize();

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
    this->AddInformation(dinf, 1);
    dinf->Delete();
    numDataSets++;
    iter->GoToNextItem();
    }
  iter->Delete();

  this->CompositeDataInformation->CopyFromObject(data);
  this->SetCompositeDataClassName(data->GetClassName());
  this->CompositeDataSetType = data->GetDataObjectType();
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
  // We do not want to get the number of dual cells from an octree
  // because this triggers generation of connectivity arrays.
  if (data->GetDataObjectType() != VTK_HYPER_OCTREE)
    {
    this->NumberOfCells = data->GetNumberOfCells();
    }
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  ofstream *tmpFile = pm->GetLogFile();
  if (tmpFile)
    {
    if (data->GetSource())
      {
      *tmpFile << "output of " << data->GetSource()->GetClassName()
               << " contains\n";
      }
    else if (data->GetProducerPort())
      {
      *tmpFile << "output of "
               << data->GetProducerPort()->GetProducer()->GetClassName()
               << " contains\n";
      }
    *tmpFile << "\t" << this->NumberOfPoints << " points" << endl;
    *tmpFile << "\t" << this->NumberOfCells << " cells" << endl;
    }
  
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
  if (this->DataSetType == VTK_UNIFORM_GRID)
    {
    ext = static_cast<vtkUniformGrid*>(data)->GetExtent();
    }
  if (ext)
    {
    for (idx = 0; idx < 6; ++idx)
      {
      this->Extent[idx] = ext[idx];
      }
    }

  vtkPointSet* ps = vtkPointSet::SafeDownCast(data);
  if (ps && ps->GetPoints())
    {
    this->PointArrayInformation->CopyFromObject(ps->GetPoints()->GetData());
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
  // We do not want to get the number of dual cells from an octree
  // because this triggers generation of connectivity arrays.
  if (data->GetDataObjectType() != VTK_HYPER_OCTREE)
    {
    this->NumberOfCells = data->GetNumberOfCells();
    }
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
  this->AddInformation(pvi, 0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(
  vtkPVInformation* pvi, int addingParts)
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

  this->SetCompositeDataClassName(info->GetCompositeDataClassName());
  this->CompositeDataSetType = info->CompositeDataSetType;

  this->CompositeDataInformation->AddInformation(
    info->CompositeDataInformation);

  if (info->NumberOfDataSets == 0)
    {
    return;
    }

  if (this->NumberOfPoints == 0 && 
      this->NumberOfCells == 0 && 
      this->NumberOfDataSets == 0)
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
  if (addingParts)
    {
    // Adding data information of parts
    this->NumberOfDataSets += info->GetNumberOfDataSets();
    }
  else 
    {
    // Adding data information of 1 part across processors
    if (this->GetCompositeDataClassName())
      {
      // Composite data blocks are not distributed across processors.
      // Simply add their number.
      this->NumberOfDataSets += info->GetNumberOfDataSets();
      }
    else
      {
      // Simple data blocks are distributed across processors, use
      // the largest number (actually, NumberOfDataSets should always
      // be 1 since the data information is for a part)
      if (this->NumberOfDataSets < info->GetNumberOfDataSets())
        {
        this->NumberOfDataSets = info->GetNumberOfDataSets();
        }
      }
    }

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
  this->PointArrayInformation->AddInformation(info->GetPointArrayInformation());
  this->PointDataInformation->AddInformation(info->GetPointDataInformation());
  this->CellDataInformation->AddInformation(info->GetCellDataInformation());
//  this->GenericAttributesInformation->AddInformation(info->GetGenericAttributesInformation());

  if (this->Name == NULL)
    {
    this->SetName(info->GetName());
    }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::SetName(const char* name)
{
  if (!this->Name && !name) { return;}
  if ( this->Name && name && (!strcmp(this->Name, name))) 
    { 
    return;
    }
  if (this->Name) { delete [] this->Name; }
  if (name) 
    { 
    this->Name = new char[strlen(name)+1];
    strcpy(this->Name, name);
    } 
  else 
    { 
    this->Name = 0; 
    } 
  this->Modified(); 
  this->NameSetToDefault = 0;
}

//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetName()
{
  if (!this->Name || this->Name[0] == '\0' || this->NameSetToDefault)
    {
    if (this->Name)
      {
      delete[] this->Name;
      }
    char str[256];
    if (this->GetDataSetType() == VTK_POLY_DATA)
      {
      long nc = this->GetNumberOfCells();
      sprintf(str, "Polygonal: %ld cells", nc);
      }
    else if (this->GetDataSetType() == VTK_UNSTRUCTURED_GRID)
      {
      long nc = this->GetNumberOfCells();
      sprintf(str, "Unstructured Grid: %ld cells", nc);
      }
    else if (this->GetDataSetType() == VTK_IMAGE_DATA)
      {
      int *ext = this->GetExtent();
      if (ext)
        {
        sprintf(str, "Uniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
                ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        }
      else
        {
        sprintf(str, "Uniform Rectilinear: invalid extent");
        }
      }
    else if (this->GetDataSetType() == VTK_UNIFORM_GRID)
      {
      int *ext = this->GetExtent();
      sprintf(str, 
              "Uniform Rectilinear (with blanking): "
              "extent (%d, %d) (%d, %d) (%d, %d)", 
              ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else if (this->GetDataSetType() == VTK_RECTILINEAR_GRID)
      {
      int *ext = this->GetExtent();
      if (ext)
        {
        sprintf(str, 
                "Nonuniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
                ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        }
      else
        {
        sprintf(str, "Nonuniform Rectilinear: invalid extent");
        }
      }
    else if (this->GetDataSetType() == VTK_STRUCTURED_GRID)
      {
      int *ext = this->GetExtent();
      if (ext)
        {
        sprintf(str, "Curvilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
                ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        }
      else
        {
        sprintf(str, "Curvilinear: invalid extent");
        }
      }
    else
      {
      sprintf(str, "Part of unknown type");
      }
    this->Name = new char[256];
    strncpy(this->Name, str, 256);
    this->NameSetToDefault = 1;
    }
  return this->Name;
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
  if (this->DataSetType == VTK_MULTIGROUP_DATA_SET)
    {
    return "vtkMultiGroupDataSet";
    }
  if (this->DataSetType == VTK_MULTIBLOCK_DATA_SET)
    {
    return "vtkMultiBlockDataSet";
    }
  if (this->DataSetType == VTK_HIERARCHICAL_DATA_SET)
    {
    return "vtkHierarchicalDataSet";
    }
  if (this->DataSetType == VTK_UNIFORM_GRID)
    {
    return "vtkUniformGrid";
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

  this->PointArrayInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  dcss.Reset();

  this->PointDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  dcss.Reset();

  this->CellDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  *css << this->CompositeDataClassName;
  *css << this->CompositeDataSetType;

  dcss.Reset();

  this->CompositeDataInformation->CopyToStream(&dcss);
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

  // Point array information.
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
  this->PointArrayInformation->CopyFromStream(&dcss);

  // Point data array information.
  if(!css->GetArgumentLength(0, 10, &length))
    {
    vtkErrorMacro("Error parsing length of point data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 10, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing point data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->PointDataInformation->CopyFromStream(&dcss);

  // Cell data array information.
  if(!css->GetArgumentLength(0, 11, &length))
    {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 11, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing cell data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->CellDataInformation->CopyFromStream(&dcss);

  const char* compositedataclassname = 0;
  if(!css->GetArgument(0, 12, &compositedataclassname))
    {
    vtkErrorMacro("Error parsing class name of data.");
    return;
    }
  this->SetCompositeDataClassName(compositedataclassname);

  if(!css->GetArgument(0, 13, &this->CompositeDataSetType))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }

  // Composite data information.
  if(!css->GetArgumentLength(0, 14, &length))
    {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 14, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing cell data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->CompositeDataInformation->CopyFromStream(&dcss);
}
