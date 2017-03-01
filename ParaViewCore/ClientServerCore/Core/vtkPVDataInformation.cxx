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
#include "vtkBoundingBox.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkGenericDataSet.h"
#include "vtkGraph.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVCompositeDataInformationIterator.h"
#include "vtkPVDataInformationHelper.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInformationKeys.h"
#include "vtkPVInstantiator.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkUniformGrid.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkPVDataInformation);

std::map<std::string, std::string> helpers;

//----------------------------------------------------------------------------
vtkPVDataInformation::vtkPVDataInformation()
{
  this->CompositeDataSetType = -1;
  this->DataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->NumberOfRows = 0;
  this->MemorySize = 0;
  this->PolygonCount = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_INT_MAX;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_INT_MAX;
  this->PointDataInformation = vtkPVDataSetAttributesInformation::New();
  this->CellDataInformation = vtkPVDataSetAttributesInformation::New();
  this->FieldDataInformation = vtkPVDataSetAttributesInformation::New();
  this->VertexDataInformation = vtkPVDataSetAttributesInformation::New();
  this->EdgeDataInformation = vtkPVDataSetAttributesInformation::New();
  this->RowDataInformation = vtkPVDataSetAttributesInformation::New();
  this->CompositeDataInformation = vtkPVCompositeDataInformation::New();
  this->PointArrayInformation = vtkPVArrayInformation::New();

  this->DataClassName = 0;
  this->CompositeDataClassName = 0;
  this->CompositeDataSetName = 0;
  this->NumberOfDataSets = 0;
  this->TimeSpan[0] = VTK_DOUBLE_MAX;
  this->TimeSpan[1] = -VTK_DOUBLE_MAX;
  this->HasTime = 0;
  this->Time = 0.0;
  this->NumberOfTimeSteps = 0;
  this->TimeLabel = NULL;

  this->PortNumber = -1;

  // Update field association information on the all the
  // vtkPVDataSetAttributesInformation instances.
  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ASSOCIATIONS; cc++)
  {
    if (vtkPVDataSetAttributesInformation* dsa = this->GetAttributeInformation(cc))
    {
      dsa->SetFieldAssociation(cc);
    }
  }
}

//----------------------------------------------------------------------------
vtkPVDataInformation::~vtkPVDataInformation()
{
  this->PointDataInformation->Delete();
  this->PointDataInformation = NULL;
  this->CellDataInformation->Delete();
  this->CellDataInformation = NULL;
  this->FieldDataInformation->Delete();
  this->FieldDataInformation = NULL;
  this->VertexDataInformation->Delete();
  this->VertexDataInformation = NULL;
  this->EdgeDataInformation->Delete();
  this->EdgeDataInformation = NULL;
  this->RowDataInformation->Delete();
  this->RowDataInformation = NULL;
  this->CompositeDataInformation->Delete();
  this->CompositeDataInformation = NULL;
  this->PointArrayInformation->Delete();
  this->PointArrayInformation = NULL;
  this->SetDataClassName(0);
  this->SetCompositeDataClassName(0);
  this->SetCompositeDataSetName(0);
  this->SetTimeLabel(NULL);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyParametersToStream(vtkMultiProcessStream& str)
{
  str << 828792 << this->PortNumber;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyParametersFromStream(vtkMultiProcessStream& str)
{
  int magic_number;
  str >> magic_number >> this->PortNumber;
  if (magic_number != 828792)
  {
    vtkErrorMacro("Magic number mismatch.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PortNumber: " << this->PortNumber << endl;
  os << indent << "DataSetType: " << this->DataSetType << endl;
  os << indent << "CompositeDataSetType: " << this->CompositeDataSetType << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "NumberOfRows: " << this->NumberOfRows << endl;
  os << indent << "NumberOfCells: " << this->NumberOfCells << endl;
  os << indent << "NumberOfDataSets: " << this->NumberOfDataSets << endl;
  os << indent << "MemorySize: " << this->MemorySize << endl;
  os << indent << "PolygonCount: " << this->PolygonCount << endl;
  os << indent << "Bounds: " << this->Bounds[0] << ", " << this->Bounds[1] << ", "
     << this->Bounds[2] << ", " << this->Bounds[3] << ", " << this->Bounds[4] << ", "
     << this->Bounds[5] << endl;
  os << indent << "Extent: " << this->Extent[0] << ", " << this->Extent[1] << ", "
     << this->Extent[2] << ", " << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << endl;

  os << indent << "PointDataInformation " << endl;
  this->PointDataInformation->PrintSelf(os, i2);
  os << indent << "CellDataInformation " << endl;
  this->CellDataInformation->PrintSelf(os, i2);
  os << indent << "VertexDataInformation" << endl;
  this->VertexDataInformation->PrintSelf(os, i2);
  os << indent << "EdgeDataInformation" << endl;
  this->EdgeDataInformation->PrintSelf(os, i2);
  os << indent << "RowDataInformation" << endl;
  this->RowDataInformation->PrintSelf(os, i2);
  os << indent << "FieldDataInformation " << endl;
  this->FieldDataInformation->PrintSelf(os, i2);
  os << indent << "CompositeDataInformation " << endl;
  this->CompositeDataInformation->PrintSelf(os, i2);
  os << indent << "PointArrayInformation " << endl;
  this->PointArrayInformation->PrintSelf(os, i2);

  os << indent << "DataClassName: " << (this->DataClassName ? this->DataClassName : "(none)")
     << endl;
  os << indent << "CompositeDataClassName: "
     << (this->CompositeDataClassName ? this->CompositeDataClassName : "(none)") << endl;
  os << indent << "CompositeDataSetName: "
     << (this->CompositeDataSetName ? this->CompositeDataSetName : "(none)") << endl;

  os << indent << "TimeSpan: " << this->TimeSpan[0] << ", " << this->TimeSpan[1] << endl;
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;

  if (this->TimeLabel)
  {
    os << indent << "TimeLabel: " << this->TimeLabel << endl;
  }
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* vtkPVDataInformation::GetAttributeInformation(
  int fieldAssociation)
{
  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      return this->PointDataInformation;

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      return this->CellDataInformation;

    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      return this->VertexDataInformation;

    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      return this->EdgeDataInformation;

    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      return this->RowDataInformation;

    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      return this->FieldDataInformation;
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::Initialize()
{
  this->DataSetType = -1;
  this->CompositeDataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->NumberOfRows = 0;
  this->NumberOfDataSets = 0;
  this->MemorySize = 0;
  this->PolygonCount = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_INT_MAX;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_INT_MAX;
  this->PointDataInformation->Initialize();
  this->CellDataInformation->Initialize();
  this->VertexDataInformation->Initialize();
  this->EdgeDataInformation->Initialize();
  this->RowDataInformation->Initialize();
  this->FieldDataInformation->Initialize();
  this->CompositeDataInformation->Initialize();
  this->PointArrayInformation->Initialize();
  this->SetDataClassName(0);
  this->SetCompositeDataClassName(0);
  this->SetCompositeDataSetName(0);
  this->TimeSpan[0] = VTK_DOUBLE_MAX;
  this->TimeSpan[1] = -VTK_DOUBLE_MAX;
  this->HasTime = 0;
  this->Time = 0.0;
  this->NumberOfTimeSteps = 0;
  this->SetTimeLabel(NULL);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::DeepCopy(
  vtkPVDataInformation* dataInfo, bool copyCompositeInformation /*=true*/)
{
  int idx;
  double* bounds;
  int* ext;

  this->DataSetType = dataInfo->GetDataSetType();
  this->CompositeDataSetType = dataInfo->GetCompositeDataSetType();
  this->SetDataClassName(dataInfo->GetDataClassName());
  this->SetCompositeDataClassName(dataInfo->GetCompositeDataClassName());
  this->SetCompositeDataSetName(dataInfo->GetCompositeDataSetName());

  this->NumberOfDataSets = dataInfo->NumberOfDataSets;

  this->NumberOfPoints = dataInfo->GetNumberOfPoints();
  this->NumberOfCells = dataInfo->GetNumberOfCells();
  this->NumberOfRows = dataInfo->GetNumberOfRows();
  this->MemorySize = dataInfo->GetMemorySize();
  this->PolygonCount = dataInfo->GetPolygonCount();

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
  this->VertexDataInformation->DeepCopy(dataInfo->GetVertexDataInformation());
  this->EdgeDataInformation->DeepCopy(dataInfo->GetEdgeDataInformation());
  this->RowDataInformation->DeepCopy(dataInfo->GetRowDataInformation());
  this->FieldDataInformation->DeepCopy(dataInfo->GetFieldDataInformation());
  if (copyCompositeInformation)
  {
    this->CompositeDataInformation->AddInformation(dataInfo->GetCompositeDataInformation());
  }
  this->PointArrayInformation->AddInformation(dataInfo->GetPointArrayInformation());

  double* timespan;
  timespan = dataInfo->GetTimeSpan();
  this->TimeSpan[0] = timespan[0];
  this->TimeSpan[1] = timespan[1];
  this->NumberOfTimeSteps = dataInfo->GetNumberOfTimeSteps();
  this->SetTimeLabel(dataInfo->GetTimeLabel());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::AddFromMultiPieceDataSet(vtkCompositeDataSet* data)
{
  vtkCompositeDataIterator* iter = data->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    if (dobj)
    {
      vtkPVDataInformation* dinf = vtkPVDataInformation::New();
      dinf->CopyFromObject(dobj);
      dinf->SetDataClassName(dobj->GetClassName());
      dinf->DataSetType = dobj->GetDataObjectType();
      this->AddInformation(dinf, /*addingParts=*/1);
      dinf->FastDelete();
    }
  }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromCompositeDataSetInitialize(vtkCompositeDataSet* data)
{
  this->Initialize();
  this->CompositeDataInformation->CopyFromObject(data);

  // Copy Field Data information, if any
  vtkFieldData* fd = data->GetFieldData();
  if (fd && fd->GetNumberOfArrays() > 0)
  {
    this->FieldDataInformation->CopyFromFieldData(fd);
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromCompositeDataSetFinalize(vtkCompositeDataSet* data)
{
  this->SetCompositeDataClassName(data->GetClassName());
  this->CompositeDataSetType = data->GetDataObjectType();

  if (this->DataSetType == -1)
  {
    // This is a composite dataset with no non-empty leaf node. Set some data
    // type (Look at BUG #7144).
    this->SetDataClassName("vtkDataSet");
    this->DataSetType = VTK_DATA_SET;
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromCompositeDataSet(vtkCompositeDataSet* data)
{
  this->CopyFromCompositeDataSetInitialize(data);

  unsigned int numDataSets = this->CompositeDataInformation->GetNumberOfChildren();
  if (this->CompositeDataInformation->GetDataIsMultiPiece())
  {
    // For vtkMultiPieceDataSet, the vtkPVCompositeDataInformation does not
    // give us individual piece information, we collect that explicitly.
    this->AddFromMultiPieceDataSet(data);
  }
  else
  {
    for (unsigned int cc = 0; cc < numDataSets; cc++)
    {
      vtkPVDataInformation* childInfo = this->CompositeDataInformation->GetDataInformation(cc);
      if (childInfo)
      {
        this->AddInformation(childInfo, /*addingParts=*/1);
      }
    }
  }

  this->CopyFromCompositeDataSetFinalize(data);

  // AddInformation should have updated NumberOfDataSets correctly to count
  // number of non-zero datasets. We don't need to fix it here.
  // this->NumberOfDataSets = numDataSets;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyCommonMetaData(vtkDataObject* data, vtkInformation* pinfo)
{
  // Gather some common stuff
  if (pinfo)
  {
    if (pinfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
      double* times = pinfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      this->TimeSpan[0] = times[0];
      this->TimeSpan[1] = times[1];
    }
    if (pinfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      this->NumberOfTimeSteps = pinfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }
  }

  this->SetTimeLabel((pinfo && pinfo->Has(vtkPVInformationKeys::TIME_LABEL_ANNOTATION()))
      ? pinfo->Get(vtkPVInformationKeys::TIME_LABEL_ANNOTATION())
      : NULL);

  vtkInformation* dinfo = data->GetInformation();
  if (dinfo->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    double time = dinfo->Get(vtkDataObject::DATA_TIME_STEP());
    this->Time = time;
    this->HasTime = 1;
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromDataSet(vtkDataSet* data)
{
  int idx;
  double* bds;
  int* ext = NULL;

  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();

  this->NumberOfDataSets = 1;

  switch (this->DataSetType)
  {
    case VTK_IMAGE_DATA:
      ext = static_cast<vtkImageData*>(data)->GetExtent();
      break;
    case VTK_STRUCTURED_GRID:
      ext = static_cast<vtkStructuredGrid*>(data)->GetExtent();
      break;
    case VTK_RECTILINEAR_GRID:
      ext = static_cast<vtkRectilinearGrid*>(data)->GetExtent();
      break;
    case VTK_UNIFORM_GRID:
      ext = static_cast<vtkUniformGrid*>(data)->GetExtent();
      break;
    case VTK_UNSTRUCTURED_GRID:
    case VTK_POLY_DATA:
      this->PolygonCount = data->GetNumberOfCells();
      break;
  }
  if (ext)
  {
    for (idx = 0; idx < 6; ++idx)
    {
      this->Extent[idx] = ext[idx];
    }
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
#if 0
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
#endif

  bds = data->GetBounds();
  for (idx = 0; idx < 6; ++idx)
  {
    this->Bounds[idx] = bds[idx];
  }
  this->MemorySize = data->GetActualMemorySize();

  vtkPointSet* ps = vtkPointSet::SafeDownCast(data);
  if (ps && ps->GetPoints())
  {
    this->PointArrayInformation->CopyFromObject(ps->GetPoints()->GetData());
  }

  // Copy Point Data information
  if (this->NumberOfPoints > 0)
  {
    this->PointDataInformation->CopyFromDataSetAttributes(data->GetPointData());
  }

  // Copy Cell Data information
  if (this->NumberOfCells > 0)
  {
    this->CellDataInformation->CopyFromDataSetAttributes(data->GetCellData());
  }

  // Copy Field Data information, if any
  vtkFieldData* fd = data->GetFieldData();
  if (fd && fd->GetNumberOfArrays() > 0)
  {
    this->FieldDataInformation->CopyFromFieldData(fd);
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromGenericDataSet(vtkGenericDataSet* data)
{
  int idx;
  double* bds;

  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();

  this->NumberOfDataSets = 1;
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
  switch (this->DataSetType)
  {
    case VTK_POLY_DATA:
      this->PolygonCount = data->GetNumberOfCells();
      break;
  }

  // Copy Point Data information
  if (this->NumberOfPoints > 0)
  {
    this->PointDataInformation->CopyFromGenericAttributesOnPoints(data->GetAttributes());
  }

  // Copy Cell Data information
  if (this->NumberOfCells > 0)
  {
    this->CellDataInformation->CopyFromGenericAttributesOnCells(data->GetAttributes());
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromSelection(vtkSelection* data)
{
  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();
  this->NumberOfDataSets = 1;

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;

  this->MemorySize = data->GetActualMemorySize();
  this->NumberOfCells = 0;
  this->NumberOfPoints = 0;

  this->FieldDataInformation->CopyFromFieldData(data->GetFieldData());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromGraph(vtkGraph* data)
{
  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();
  this->NumberOfDataSets = 1;

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;

  if (data->GetPoints())
    data->GetPoints()->GetBounds(this->Bounds);

  this->MemorySize = data->GetActualMemorySize();
  this->NumberOfCells = data->GetNumberOfEdges();
  this->NumberOfPoints = data->GetNumberOfVertices();
  this->NumberOfRows = 0;

  // For whatever reason, the code above maps edges to cells and vertices to
  // points. We should just add new ivars to track vertices and edges.
  if (this->NumberOfPoints > 0)
  {
    this->VertexDataInformation->CopyFromFieldData(data->GetVertexData());
  }
  if (this->NumberOfCells > 0)
  {
    this->EdgeDataInformation->CopyFromFieldData(data->GetEdgeData());
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromTable(vtkTable* data)
{
  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();
  this->NumberOfDataSets = 1;

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;

  this->MemorySize = data->GetActualMemorySize();
  this->NumberOfCells = data->GetNumberOfRows() * data->GetNumberOfColumns();
  this->NumberOfPoints = 0;
  this->NumberOfRows = data->GetNumberOfRows();

  if (this->NumberOfRows > 0)
  {
    this->RowDataInformation->CopyFromFieldData(data->GetRowData());
  }
  this->FieldDataInformation->CopyFromFieldData(data->GetFieldData());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromObject(vtkObject* object)
{
  vtkDataObject* dobj = vtkDataObject::SafeDownCast(object);
  vtkInformation* info = NULL;
  // Handle the case where the a vtkAlgorithmOutput is passed instead of
  // the data object. vtkSMPart uses vtkAlgorithmOutput.
  if (!dobj)
  {
    vtkAlgorithmOutput* algOutput = vtkAlgorithmOutput::SafeDownCast(object);
    vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(object);
    if (algOutput && algOutput->GetProducer())
    {
      if (strcmp(algOutput->GetProducer()->GetClassName(), "vtkPVNullSource") == 0)
      {
        // Don't gather any data information from the hypothetical null source.
        return;
      }

      if (algOutput->GetProducer()->IsA("vtkPVPostFilter"))
      {
        algOutput = algOutput->GetProducer()->GetInputConnection(0, 0);
      }
      info = algOutput->GetProducer()->GetOutputInformation(this->PortNumber);
      dobj = algOutput->GetProducer()->GetOutputDataObject(algOutput->GetIndex());
    }
    else if (algo)
    {
      // We don't use vtkAlgorithm::GetOutputDataObject() since that call a
      // UpdateDataObject() pass, which may raise errors if the algo is not
      // fully setup yet.
      if (strcmp(algo->GetClassName(), "vtkPVNullSource") == 0)
      {
        // Don't gather any data information from the hypothetical null source.
        return;
      }
      info = algo->GetExecutive()->GetOutputInformation(this->PortNumber);
      if (!info || vtkDataObject::GetData(info) == NULL)
      {
        return;
      }
      dobj = algo->GetOutputDataObject(this->PortNumber);
    }
  }

  if (!dobj)
  {
    vtkErrorMacro("Could not cast object to a known data set: " << (object ? object->GetClassName()
                                                                           : "(null)"));
    return;
  }

  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(dobj);
  if (cds)
  {
    this->CopyFromCompositeDataSet(cds);
    this->CopyCommonMetaData(dobj, info);
    return;
  }

  vtkDataSet* ds = vtkDataSet::SafeDownCast(dobj);
  if (ds)
  {
    this->CopyFromDataSet(ds);
    this->CopyCommonMetaData(dobj, info);
    return;
  }

  vtkGenericDataSet* ads = vtkGenericDataSet::SafeDownCast(dobj);
  if (ads)
  {
    this->CopyFromGenericDataSet(ads);
    this->CopyCommonMetaData(dobj, info);
    return;
  }

  vtkGraph* graph = vtkGraph::SafeDownCast(dobj);
  if (graph)
  {
    this->CopyFromGraph(graph);
    this->CopyCommonMetaData(dobj, info);
    return;
  }

  vtkTable* table = vtkTable::SafeDownCast(dobj);
  if (table)
  {
    this->CopyFromTable(table);
    this->CopyCommonMetaData(dobj, info);
    return;
  }

  vtkSelection* selection = vtkSelection::SafeDownCast(dobj);
  if (selection)
  {
    this->CopyFromSelection(selection);
    this->CopyCommonMetaData(dobj, info);
    return;
  }

  const char* cname = dobj->GetClassName();
  vtkPVDataInformationHelper* dhelper = vtkPVDataInformation::FindHelper(cname);
  if (dhelper)
  {
    dhelper->CopyFromDataObject(this, dobj);
    this->CopyCommonMetaData(dobj, info);
    dhelper->Delete();
    return;
  }

  // Because custom applications may implement their own data
  // object types, this isn't an error condition - just
  // display the name of the data object and return quietly.
  this->SetDataClassName(dobj->GetClassName());
  this->CopyCommonMetaData(dobj, info);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(vtkPVInformation* pvi)
{
  this->AddInformation(pvi, 0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(vtkPVInformation* pvi, int addingParts)
{
  vtkPVDataInformation* info;
  int i;
  int* ext;

  info = vtkPVDataInformation::SafeDownCast(pvi);
  if (info == NULL)
  {
    vtkErrorMacro("Cound not cast object to data information.");
    return;
  }

  if (!addingParts)
  {
    this->SetCompositeDataClassName(info->GetCompositeDataClassName());
    this->SetCompositeDataSetName(info->GetCompositeDataSetName());
    this->CompositeDataSetType = info->CompositeDataSetType;
    this->CompositeDataInformation->AddInformation(info->CompositeDataInformation);
  }

  if (info->NumberOfDataSets == 0)
  {
    return;
  }

  if (this->NumberOfPoints == 0 && this->NumberOfCells == 0 && this->NumberOfDataSets == 0 &&
    this->FieldDataInformation->GetNumberOfArrays() == 0)
  {
    // Just copy the other array information.
    this->DeepCopy(info, !addingParts);
    return;
  }

  // For data set, lets pick the common super class.
  // This supports Heterogeneous collections.
  // We need a new classification: Structured.
  // This would allow extracting grid from mixed structured collections.
  if (this->DataSetType != info->GetDataSetType())
  { // IsTypeOf method will not work here.  Must be done manually.
    if (this->DataSetType == -1)
    {
      this->DataSetType = info->GetDataSetType();
      this->SetDataClassName(info->GetDataClassName());
    }
    else if (this->DataSetType == VTK_IMAGE_DATA || this->DataSetType == VTK_RECTILINEAR_GRID ||
      this->DataSetType == VTK_DATA_SET || info->GetDataSetType() == VTK_IMAGE_DATA ||
      info->GetDataSetType() == VTK_RECTILINEAR_GRID || info->GetDataSetType() == VTK_DATA_SET)
    {
      this->DataSetType = VTK_DATA_SET;
      this->SetDataClassName("vtkDataSet");
    }
    else
    {
      if (this->DataSetType == VTK_GENERIC_DATA_SET ||
        info->GetDataSetType() == VTK_GENERIC_DATA_SET)
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

  switch (this->DataSetType)
  {
    case VTK_POLY_DATA:
      this->PolygonCount += info->GetNumberOfCells();
      break;
  }
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

  vtkBoundingBox bbox;
  bbox.AddBounds(this->Bounds);
  bbox.AddBounds(info->GetBounds());
  if (bbox.IsValid())
  {
    bbox.GetBounds(this->Bounds);
  }
  else
  {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  }

  // Extents are only a little harder.
  ext = info->GetExtent();
  if ((ext[0] > ext[1]) || (ext[2] > ext[3]) || (ext[4] > ext[5]))
  {
    // ext is invalid. ignore it.
  }
  else if ((this->Extent[0] > this->Extent[1]) || (this->Extent[2] > this->Extent[3]) ||
    (this->Extent[4] > this->Extent[5]))
  {
    std::copy(ext, ext + 6, this->Extent);
  }
  else
  {
    for (i = 0; i < 3; ++i)
    {
      int j = i * 2;
      if (ext[j] < this->Extent[j])
      {
        this->Extent[j] = ext[j];
      }
      ++j;
      if (ext[j] > this->Extent[j])
      {
        this->Extent[j] = ext[j];
      }
    }
  }

  // Now for the messy part, all of the arrays.
  if (this->NumberOfPoints > 0 && info->GetNumberOfPoints() > 0)
  {
    this->PointArrayInformation->AddInformation(info->GetPointArrayInformation());
    this->PointDataInformation->AddInformation(info->GetPointDataInformation());
  }
  else if (info->GetNumberOfPoints() > 0)
  {
    this->PointArrayInformation->DeepCopy(info->GetPointArrayInformation());
    this->PointDataInformation->DeepCopy(info->GetPointDataInformation());
  }
  if (this->NumberOfCells > 0 && info->GetNumberOfCells() > 0)
  {
    this->CellDataInformation->AddInformation(info->GetCellDataInformation());
  }
  else if (info->GetNumberOfCells() > 0)
  {
    this->CellDataInformation->DeepCopy(info->GetCellDataInformation());
  }
  if (this->NumberOfPoints > 0 && info->GetNumberOfPoints() > 0)
  {
    this->VertexDataInformation->AddInformation(info->GetVertexDataInformation());
  }
  else if (info->GetNumberOfPoints() > 0)
  {
    this->VertexDataInformation->DeepCopy(info->GetVertexDataInformation());
  }
  if (this->NumberOfCells > 0 && info->GetNumberOfCells() > 0)
  {
    this->EdgeDataInformation->AddInformation(info->GetEdgeDataInformation());
  }
  else if (info->GetNumberOfCells() > 0)
  {
    this->EdgeDataInformation->DeepCopy(info->GetEdgeDataInformation());
  }
  if (this->NumberOfRows > 0 && info->GetNumberOfRows() > 0)
  {
    this->RowDataInformation->AddInformation(info->GetRowDataInformation());
  }
  else if (info->GetNumberOfRows() > 0)
  {
    this->RowDataInformation->DeepCopy(info->GetRowDataInformation());
  }
  this->FieldDataInformation->AddInformation(info->GetFieldDataInformation());
  //  this->GenericAttributesInformation->AddInformation(info->GetGenericAttributesInformation());

  this->NumberOfPoints += info->GetNumberOfPoints();
  this->NumberOfCells += info->GetNumberOfCells();
  this->MemorySize += info->GetMemorySize();
  this->NumberOfRows += info->GetNumberOfRows();

  double* times = info->GetTimeSpan();
  if (times[0] < this->TimeSpan[0])
  {
    this->TimeSpan[0] = times[0];
  }
  if (times[1] > this->TimeSpan[1])
  {
    this->TimeSpan[1] = times[1];
  }

  int numTimeSteps = info->GetNumberOfTimeSteps();
  if (numTimeSteps > this->NumberOfTimeSteps)
  {
    this->NumberOfTimeSteps = numTimeSteps;
  }

  if (!this->HasTime && info->GetHasTime())
  {
    this->Time = info->GetTime();
    this->HasTime = 1;
  }

  this->SetTimeLabel(info->GetTimeLabel());
}

//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetPrettyDataTypeString()
{
  int dataType = this->DataSetType;
  if (this->CompositeDataSetType >= 0)
  {
    dataType = this->CompositeDataSetType;
  }

  switch (dataType)
  {
    case VTK_POLY_DATA:
      return "Polygonal Mesh";
    case VTK_STRUCTURED_POINTS:
      return "Image (Uniform Rectilinear Grid)";
    case VTK_STRUCTURED_GRID:
      return "Structured (Curvilinear) Grid";
    case VTK_RECTILINEAR_GRID:
      return "Rectilinear Grid";
    case VTK_UNSTRUCTURED_GRID:
      return "Unstructured Grid";
    case VTK_PIECEWISE_FUNCTION:
      return "Piecewise function";
    case VTK_IMAGE_DATA:
      return "Image (Uniform Rectilinear Grid)";
    case VTK_DATA_OBJECT:
      return "Data Object";
    case VTK_DATA_SET:
      return "Data Set";
    case VTK_POINT_SET:
      return "Point Set";
    case VTK_UNIFORM_GRID:
      return "Image (Uniform Rectilinear Grid) with blanking";
    case VTK_COMPOSITE_DATA_SET:
      return "Composite Dataset";
    case VTK_MULTIGROUP_DATA_SET:
      return "Multi-group Dataset";
    case VTK_MULTIBLOCK_DATA_SET:
      return "Multi-block Dataset";
    case VTK_HIERARCHICAL_DATA_SET:
      return "Hierarchical DataSet (Deprecated)";
    case VTK_HIERARCHICAL_BOX_DATA_SET:
      return "AMR Dataset (Deprecated)";
    case VTK_NON_OVERLAPPING_AMR:
      return "Non-Overlapping AMR Dataset";
    case VTK_OVERLAPPING_AMR:
      return "Overlapping AMR Dataset";
    case VTK_GENERIC_DATA_SET:
      return "Generic Dataset";
    case VTK_HYPER_OCTREE:
      return "Hyper-octree";
    case VTK_HYPER_TREE_GRID:
      return "Hyper-tree Grid";
    case VTK_TEMPORAL_DATA_SET:
      return "Temporal Dataset";
    case VTK_TABLE:
      return "Table";
    case VTK_GRAPH:
      return "Graph";
    case VTK_TREE:
      return "Tree";
    case VTK_SELECTION:
      return "Selection";
    case VTK_DIRECTED_GRAPH:
      return "Directed Graph";
    case VTK_UNDIRECTED_GRAPH:
      return "Undirected Graph";
    case VTK_MULTIPIECE_DATA_SET:
      return "Multi-piece Dataset";
    case VTK_DIRECTED_ACYCLIC_GRAPH:
      return "Directed Acyclic Graph";
    default:
      vtkPVDataInformationHelper* dhelper = vtkPVDataInformation::FindHelper(this->DataClassName);
      if (dhelper)
      {
        const char* namestr = dhelper->GetPrettyDataTypeString();
        dhelper->Delete();
        return namestr;
      }
  }

  return "UnknownType";
}

//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetDataSetTypeAsString()
{
  if (this->DataSetType == -1)
  {
    return "UnknownType";
  }
  else
  {
    return vtkDataObjectTypes::GetClassNameFromTypeId(this->DataSetType);
  }
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
    if (this->DataSetType == VTK_POLY_DATA || this->DataSetType == VTK_STRUCTURED_GRID ||
      this->DataSetType == VTK_UNSTRUCTURED_GRID || this->DataSetType == VTK_IMAGE_DATA ||
      this->DataSetType == VTK_RECTILINEAR_GRID || this->DataSetType == VTK_UNSTRUCTURED_GRID ||
      this->DataSetType == VTK_HYPER_TREE_GRID || this->DataSetType == VTK_STRUCTURED_POINTS)
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
    if (this->DataSetType == VTK_POLY_DATA || this->DataSetType == VTK_STRUCTURED_GRID ||
      this->DataSetType == VTK_UNSTRUCTURED_GRID)
    {
      return 1;
    }
  }
  if (strcmp(type, "vtkStructuredData") == 0)
  {
    if (this->DataSetType == VTK_IMAGE_DATA || this->DataSetType == VTK_STRUCTURED_GRID ||
      this->DataSetType == VTK_RECTILINEAR_GRID)
    {
      return 1;
    }
  }
  if (strcmp(type, "vtkImageData") == 0)
  {
    if (this->DataSetType == VTK_IMAGE_DATA || this->DataSetType == VTK_UNIFORM_GRID ||
      this->DataSetType == VTK_STRUCTURED_POINTS)
    {
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVDataInformation::IsDataStructured()
{
  switch (this->DataSetType)
  {
    case VTK_IMAGE_DATA:
    case VTK_STRUCTURED_GRID:
    case VTK_RECTILINEAR_GRID:
    case VTK_UNIFORM_GRID:
    case VTK_GENERIC_DATA_SET:
      return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVDataInformation::GetDataInformationForCompositeIndex(int index)
{
  return this->GetDataInformationForCompositeIndex(&index);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVDataInformation::GetDataInformationForCompositeIndex(int* index)
{
  if (*index == 0)
  {
    (*index)--;
    return this;
  }

  (*index)--;
  return this->CompositeDataInformation->GetDataInformationForCompositeIndex(index);
}

//----------------------------------------------------------------------------
unsigned int vtkPVDataInformation::GetNumberOfBlockLeafs(bool skipEmpty)
{
  vtkNew<vtkPVCompositeDataInformationIterator> iter;
  iter->SetDataInformation(this);
  unsigned int nLeafs = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkPVDataInformation* info = iter->GetCurrentDataInformation();
    if (info)
    {
      vtkPVCompositeDataInformation* cinfo = info->GetCompositeDataInformation();
      if (!cinfo->GetDataIsComposite() || cinfo->GetDataIsMultiPiece())
      {
        nLeafs++;
      }
    }
    else if (!skipEmpty)
    {
      // without skipEmpty, NULL data are counted as well.
      nLeafs++;
    }
  }
  return nLeafs;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->DataClassName << this->DataSetType << this->NumberOfDataSets << this->NumberOfPoints
       << this->NumberOfCells << this->NumberOfRows << this->MemorySize << this->PolygonCount
       << this->Time << this->HasTime << this->NumberOfTimeSteps << this->TimeLabel
       << vtkClientServerStream::InsertArray(this->Bounds, 6)
       << vtkClientServerStream::InsertArray(this->Extent, 6);

  size_t length;
  const unsigned char* data;
  vtkClientServerStream dcss;

  this->PointArrayInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();

  this->PointDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();

  this->CellDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();

  this->VertexDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();

  this->EdgeDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();

  this->RowDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  *css << this->CompositeDataClassName;
  *css << this->CompositeDataSetType;
  *css << this->CompositeDataSetName;

  dcss.Reset();

  this->CompositeDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();

  this->FieldDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  *css << vtkClientServerStream::InsertArray(this->TimeSpan, 2);

  *css << vtkClientServerStream::End;
}

// Macros used to make it easy to insert/remove entries when serializing
// to/from a stream.

#define CSS_ARGUMENT_BEGIN()                                                                       \
  {                                                                                                \
  int _vtk__css_argument_int_counter = 0

#define CSS_GET_NEXT_ARGUMENT(css, msg, var)                                                       \
  css->GetArgument(msg, _vtk__css_argument_int_counter++, var)

#define CSS_GET_NEXT_ARGUMENT2(css, msg, var, len)                                                 \
  css->GetArgument(msg, _vtk__css_argument_int_counter++, var, len)

#define CSS_GET_CUR_INDEX() _vtk__css_argument_int_counter

#define CSS_ARGUMENT_END() }

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromStream(const vtkClientServerStream* css)
{
  CSS_ARGUMENT_BEGIN();

  const char* dataclassname = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &dataclassname))
  {
    vtkErrorMacro("Error parsing class name of data.");
    return;
  }
  this->SetDataClassName(dataclassname);

  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->DataSetType))
  {
    vtkErrorMacro("Error parsing data set type.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->NumberOfDataSets))
  {
    vtkErrorMacro("Error parsing number of datasets.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->NumberOfPoints))
  {
    vtkErrorMacro("Error parsing number of points.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->NumberOfCells))
  {
    vtkErrorMacro("Error parsing number of cells.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->NumberOfRows))
  {
    vtkErrorMacro("Error parsing number of cells.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->MemorySize))
  {
    vtkErrorMacro("Error parsing memory size.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->PolygonCount))
  {
    vtkErrorMacro("Error parsing memory size.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->Time))
  {
    vtkErrorMacro("Error parsing Time.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->HasTime))
  {
    vtkErrorMacro("Error parsing has-time.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->NumberOfTimeSteps))
  {
    vtkErrorMacro("Error parsing number of time steps.");
    return;
  }
  const char* timeLabel = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &timeLabel))
  {
    vtkErrorMacro("Error parsing time label.");
    return;
  }
  this->SetTimeLabel(timeLabel);
  if (!CSS_GET_NEXT_ARGUMENT2(css, 0, this->Bounds, 6))
  {
    vtkErrorMacro("Error parsing bounds.");
    return;
  }
  if (!CSS_GET_NEXT_ARGUMENT2(css, 0, this->Extent, 6))
  {
    vtkErrorMacro("Error parsing extent.");
    return;
  }

  vtkTypeUInt32 length;
  std::vector<unsigned char> data;
  vtkClientServerStream dcss;

  // Point array information.
  if (!css->GetArgumentLength(0, CSS_GET_CUR_INDEX(), &length))
  {
    vtkErrorMacro("Error parsing length of point data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, CSS_GET_CUR_INDEX(), &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing point data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->PointArrayInformation->CopyFromStream(&dcss);
  CSS_GET_CUR_INDEX()++;

  // Point data array information.
  if (!css->GetArgumentLength(0, CSS_GET_CUR_INDEX(), &length))
  {
    vtkErrorMacro("Error parsing length of point data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, CSS_GET_CUR_INDEX(), &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing point data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->PointDataInformation->CopyFromStream(&dcss);
  CSS_GET_CUR_INDEX()++;

  // Cell data array information.
  if (!css->GetArgumentLength(0, CSS_GET_CUR_INDEX(), &length))
  {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, CSS_GET_CUR_INDEX(), &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing cell data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->CellDataInformation->CopyFromStream(&dcss);
  CSS_GET_CUR_INDEX()++;

  // Vertex data array information.
  if (!css->GetArgumentLength(0, CSS_GET_CUR_INDEX(), &length))
  {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, CSS_GET_CUR_INDEX(), &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing cell data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->VertexDataInformation->CopyFromStream(&dcss);
  CSS_GET_CUR_INDEX()++;

  // Edge data array information.
  if (!css->GetArgumentLength(0, CSS_GET_CUR_INDEX(), &length))
  {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, CSS_GET_CUR_INDEX(), &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing cell data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->EdgeDataInformation->CopyFromStream(&dcss);
  CSS_GET_CUR_INDEX()++;

  // Row data array information.
  if (!css->GetArgumentLength(0, CSS_GET_CUR_INDEX(), &length))
  {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, CSS_GET_CUR_INDEX(), &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing cell data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->RowDataInformation->CopyFromStream(&dcss);
  CSS_GET_CUR_INDEX()++;

  const char* compositedataclassname = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &compositedataclassname))
  {
    vtkErrorMacro("Error parsing class name of data.");
    return;
  }
  this->SetCompositeDataClassName(compositedataclassname);

  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->CompositeDataSetType))
  {
    vtkErrorMacro("Error parsing data set type.");
    return;
  }

  const char* compositedatasetname = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &compositedatasetname))
  {
    vtkErrorMacro("Error parsing composite dataset name of data.");
    return;
  }
  this->SetCompositeDataSetName(compositedatasetname);

  // Composite data information.
  if (!css->GetArgumentLength(0, CSS_GET_CUR_INDEX(), &length))
  {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, CSS_GET_CUR_INDEX(), &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing cell data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  if (dcss.GetNumberOfMessages() > 0)
  {
    this->CompositeDataInformation->CopyFromStream(&dcss);
  }
  else
  {
    this->CompositeDataInformation->Initialize();
  }
  CSS_GET_CUR_INDEX()++;

  // Field data array information.
  if (!css->GetArgumentLength(0, CSS_GET_CUR_INDEX(), &length))
  {
    vtkErrorMacro("Error parsing length of field data information.");
    return;
  }

  data.resize(length);
  if (!css->GetArgument(0, CSS_GET_CUR_INDEX(), &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing field data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->FieldDataInformation->CopyFromStream(&dcss);
  CSS_GET_CUR_INDEX()++;

  if (!CSS_GET_NEXT_ARGUMENT2(css, 0, this->TimeSpan, 2))
  {
    vtkErrorMacro("Error parsing timespan.");
    return;
  }

  CSS_ARGUMENT_END();
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::RegisterHelper(const char* classname, const char* helper)
{
  helpers[classname] = helper;
}

//----------------------------------------------------------------------------
vtkPVDataInformationHelper* vtkPVDataInformation::FindHelper(const char* classname)
{
  std::map<std::string, std::string>::iterator iter =
    helpers.find(classname ? classname : std::string());
  if (iter != helpers.end())
  {
    std::string helperclassname = iter->second;
    vtkObject* obj = vtkPVInstantiator::CreateInstance(helperclassname.c_str());
    vtkPVDataInformationHelper* helper = vtkPVDataInformationHelper::SafeDownCast(obj);
    if (!helper)
    {
      obj->Delete();
    }
    return helper;
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataInformation::GetArrayInformation(
  const char* arrayname, int attribute_type)
{
  vtkPVDataSetAttributesInformation* attrInfo = this->GetAttributeInformation(attribute_type);
  return attrInfo ? attrInfo->GetArrayInformation(arrayname) : NULL;
}
