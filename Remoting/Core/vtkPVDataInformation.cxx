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
#include "vtkClientServerStreamInstantiator.h"
#include "vtkCollection.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkDataArray.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkExtractBlockUsingDataAssembly.h"
#include "vtkGraph.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInformationKeys.h"
#include "vtkPVLogger.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>

class vtkPVDataInformationAccumulator
{
  vtkNew<vtkPVDataInformation> Current;

public:
  std::set<int> UniqueBlockTypes;
  vtkPVDataInformation* operator()(vtkPVDataInformation* info, vtkDataObject* dobj)
  {
    if (!dobj)
    {
      return info;
    }
    assert(vtkCompositeDataSet::SafeDownCast(dobj) == nullptr);

    this->Current->Initialize();
    this->Current->CopyFromDataObject(dobj);
    if (this->Current->GetDataSetType() != -1)
    {
      assert(this->Current->GetCompositeDataSetType() == -1);
      this->UniqueBlockTypes.insert(this->Current->GetDataSetType());
      info->AddInformation(this->Current);
    }
    return info;
  }

  void AddFieldDataOnly(vtkPVDataInformation* info, vtkDataObject* dobj)
  {
    this->Current->Initialize();
    auto fdi = this->Current->GetFieldDataInformation();
    fdi->CopyFromDataObject(dobj);
    if (fdi->GetNumberOfArrays() > 0)
    {
      info->GetFieldDataInformation()->AddInformation(fdi);
    }
  }
};

namespace
{

void MergeBounds(double bds[6], const double obds[6])
{
  vtkBoundingBox bbox(bds);
  bbox.AddBounds(obds);
  if (bbox.IsValid())
  {
    bbox.GetBounds(bds);
  }
}

void MergeExtent(int ext[6], const int oext[6])
{
  for (int dim = 0; dim < 3; ++dim)
  {
    if (oext[2 * dim] <= oext[2 * dim + 1])
    {
      if (ext[2 * dim] <= ext[2 * dim + 1])
      {
        ext[2 * dim] = std::min(ext[2 * dim], oext[2 * dim]);
        ext[2 * dim + 1] = std::max(ext[2 * dim + 1], oext[2 * dim + 1]);
      }
      else
      {
        ext[2 * dim] = oext[2 * dim];
        ext[2 * dim + 1] = oext[2 * dim + 1];
      }
    }
  }
}

void MergeRange(double range[2], const double orange[2])
{
  if (orange[0] <= orange[1])
  {
    if (range[0] <= range[1])
    {
      range[0] = std::min(range[0], orange[0]);
      range[1] = std::max(range[1], orange[1]);
    }
    else
    {
      range[0] = orange[0];
      range[1] = orange[1];
    }
  }
}
}

vtkStandardNewMacro(vtkPVDataInformation);
//----------------------------------------------------------------------------
vtkPVDataInformation::vtkPVDataInformation()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkPVDataInformation::~vtkPVDataInformation()
{
  this->SetSubsetSelector(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyParametersToStream(vtkMultiProcessStream& str)
{
  str << 828792 << this->PortNumber << std::string(this->SubsetSelector ? SubsetSelector : "")
      << std::string(this->SubsetAssemblyName ? this->SubsetAssemblyName : "");
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyParametersFromStream(vtkMultiProcessStream& str)
{
  int magic_number;
  std::string path, name;
  str >> magic_number >> this->PortNumber >> path >> name;
  if (magic_number != 828792)
  {
    vtkErrorMacro("Magic number mismatch.");
  }
  this->SetSubsetSelector(path.empty() ? nullptr : path.c_str());
  this->SetSubsetAssemblyName(name.empty() ? nullptr : name.c_str());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  auto i2 = indent.GetNextIndent();

  this->Superclass::PrintSelf(os, indent);

  os << indent << "PortNumber: " << this->PortNumber << endl;
  os << indent << "SubsetSelector: " << (this->SubsetSelector ? this->SubsetSelector : "(nullptr)")
     << endl;
  os << indent << "SubsetAssemblyName: "
     << (this->SubsetAssemblyName ? this->SubsetAssemblyName : "(nullptr)") << endl;
  os << indent << "DataSetType: " << this->DataSetType << endl;
  os << indent << "CompositeDataSetType: " << this->CompositeDataSetType << endl;
  os << indent << "FirstLeafCompositeIndex: " << this->FirstLeafCompositeIndex << endl;
  os << indent << "UniqueBlockTypes: ";
  for (size_t cc = 0; cc < this->UniqueBlockTypes.size(); ++cc)
  {
    if (cc > 0 && (cc % 8) == 0)
    {
      os << endl << i2;
    }
    os << this->UniqueBlockTypes[cc];
  }
  os << endl;

  os << indent << "NumberOfPoints: " << this->GetNumberOfPoints() << endl;
  os << indent << "NumberOfCells: " << this->GetNumberOfCells() << endl;
  os << indent << "NumberOfVertices: " << this->GetNumberOfVertices() << endl;
  os << indent << "NumberOfEdges: " << this->GetNumberOfEdges() << endl;
  os << indent << "NumberOfRows: " << this->GetNumberOfRows() << endl;
  os << indent << "NumberOfTrees: " << this->GetNumberOfTrees() << endl;
  os << indent << "NumberOfLeaves: " << this->GetNumberOfLeaves() << endl;
  os << indent << "NumberOfAMRLevels: " << this->GetNumberOfAMRLevels() << endl;
  os << indent << "NumberOfDataSets: " << this->GetNumberOfDataSets() << endl;
  os << indent << "MemorySize: " << this->MemorySize << endl;
  os << indent << "Bounds: " << this->Bounds[0] << ", " << this->Bounds[1] << ", "
     << this->Bounds[2] << ", " << this->Bounds[3] << ", " << this->Bounds[4] << ", "
     << this->Bounds[5] << endl;
  os << indent << "Extent: " << this->Extent[0] << ", " << this->Extent[1] << ", "
     << this->Extent[2] << ", " << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << endl;

  os << indent << "PointDataInformation " << endl;
  this->GetPointDataInformation()->PrintSelf(os, i2);
  os << indent << "CellDataInformation " << endl;
  this->GetCellDataInformation()->PrintSelf(os, i2);
  os << indent << "VertexDataInformation" << endl;
  this->GetVertexDataInformation()->PrintSelf(os, i2);
  os << indent << "EdgeDataInformation" << endl;
  this->GetEdgeDataInformation()->PrintSelf(os, i2);
  os << indent << "RowDataInformation" << endl;
  this->GetRowDataInformation()->PrintSelf(os, i2);
  os << indent << "FieldDataInformation " << endl;
  this->GetFieldDataInformation()->PrintSelf(os, i2);
  os << indent << "PointArrayInformation " << endl;
  this->GetPointArrayInformation()->PrintSelf(os, i2);

  os << indent << "HasTime: " << this->HasTime << endl;
  os << indent << "Time: " << this->Time << endl;
  os << indent << "TimeRange: " << this->TimeRange[0] << ", " << this->TimeRange[1] << endl;
  os << indent << "TimeLabel: " << (this->TimeLabel.empty() ? "(none)" : this->TimeLabel.c_str())
     << endl;
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* vtkPVDataInformation::GetAttributeInformation(
  int fieldAssociation) const
{
  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
    case vtkDataObject::FIELD_ASSOCIATION_NONE:
    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      return this->AttributeInformations[fieldAssociation];
    default:
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkTypeInt64 vtkPVDataInformation::GetNumberOfElements(int elementType) const
{
  switch (elementType)
  {
    case vtkDataObject::POINT:
    case vtkDataObject::CELL:
    case vtkDataObject::FIELD:
    case vtkDataObject::VERTEX:
    case vtkDataObject::EDGE:
    case vtkDataObject::ROW:
      return this->NumberOfElements[elementType];
    default:
      return 0;
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::Initialize()
{
  this->DataSetType = -1;
  this->CompositeDataSetType = -1;
  this->FirstLeafCompositeIndex = 0;
  this->UniqueBlockTypes.clear();

  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++cc)
  {
    this->NumberOfElements[cc] = 0;
    this->AttributeInformations[cc]->Initialize();
    this->AttributeInformations[cc]->SetFieldAssociation(cc);
  }

  this->NumberOfTrees = 0;
  this->NumberOfLeaves = 0;
  this->NumberOfAMRLevels = 0;
  this->NumberOfDataSets = 0;
  this->MemorySize = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_INT_MAX;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_INT_MAX;

  this->PointArrayInformation->Initialize();
  this->HasTime = false;
  this->Time = 0.0;
  this->TimeRange[0] = VTK_DOUBLE_MAX;
  this->TimeRange[1] = -VTK_DOUBLE_MAX;
  this->TimeLabel.clear();
  this->NumberOfTimeSteps = 0;

  this->Hierarchy->Initialize();
  this->DataAssembly->Initialize();
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromObject(vtkObject* object)
{
  this->Initialize();

  if (object == nullptr)
  {
    return;
  }

  vtkDataObject* dobj = vtkDataObject::SafeDownCast(object);

  vtkInformation* pipelineInfo = nullptr;
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
      pipelineInfo = algOutput->GetProducer()->GetOutputInformation(this->PortNumber);
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
      pipelineInfo = algo->GetExecutive()->GetOutputInformation(this->PortNumber);
      if (!pipelineInfo || vtkDataObject::GetData(pipelineInfo) == nullptr)
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

  vtkSmartPointer<vtkDataObject> subset;
  if (vtkCompositeDataSet::SafeDownCast(dobj) && this->SubsetSelector != nullptr)
  {
    subset = this->GetSubset(dobj);
  }
  else
  {
    subset = dobj;
  }

  vtkPVDataInformationAccumulator accumulator;
  if (auto cd = vtkCompositeDataSet::SafeDownCast(subset))
  {
    decltype(this->FirstLeafCompositeIndex) leaf_index = 0;
    using Opts = vtk::CompositeDataSetOptions;
    for (const auto& item : vtk::Range(cd, Opts::None))
    {
      if (leaf_index == 0)
      {
        leaf_index = item.GetFlatIndex();
      }
      if (item)
      {
        assert(vtkCompositeDataSet::SafeDownCast(item) == nullptr);
        accumulator(this, item);
      }
    }

    // we miss the root node in the above iteration; the key is field data.
    // just handle it separately.
    accumulator.AddFieldDataOnly(this, subset);

    // if cd is a data-object tree, the iteration also misses non-leaf nodes and their
    // field data; so process that too.
    if (auto dtree = vtkDataObjectTree::SafeDownCast(cd))
    {
      using DTOpts = vtk::DataObjectTreeOptions;
      for (const auto& item : vtk::Range(dtree, DTOpts::TraverseSubTree))
      {
        if (auto nonleaf = vtkCompositeDataSet::SafeDownCast(item))
        {
          accumulator.AddFieldDataOnly(this, nonleaf);
        }
      }
    }

    this->CompositeDataSetType = subset->GetDataObjectType();
    this->FirstLeafCompositeIndex = leaf_index;
    vtkDataAssemblyUtilities::GenerateHierarchy(cd, this->Hierarchy);
    if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(cd))
    {
      this->DataAssembly->DeepCopy(pdc->GetDataAssembly());
    }
    else if (auto amr = vtkUniformGridAMR::SafeDownCast(subset))
    {
      this->NumberOfAMRLevels = amr->GetNumberOfLevels();
    }
  }
  else if (subset)
  {
    accumulator(this, subset);
  }

  this->UniqueBlockTypes.clear();
  if (this->CompositeDataSetType != -1)
  {
    this->UniqueBlockTypes.insert(this->UniqueBlockTypes.end(),
      accumulator.UniqueBlockTypes.begin(), accumulator.UniqueBlockTypes.end());
  }

  // Copy information from `pipelineInfo`
  this->CopyFromPipelineInformation(pipelineInfo);

  // Copy data time.
  auto dinfo = dobj->GetInformation();
  if (dinfo && dinfo->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    this->Time = dinfo->Get(vtkDataObject::DATA_TIME_STEP());
    this->HasTime = true;
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromPipelineInformation(vtkInformation* pinfo)
{
  // Gather some common stuff
  if (pinfo)
  {
    if (pinfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()) &&
      pinfo->Length(vtkStreamingDemandDrivenPipeline::TIME_RANGE()) == 2)
    {
      const double* times = pinfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      this->TimeRange[0] = times[0];
      this->TimeRange[1] = times[1];
    }

    if (pinfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      this->NumberOfTimeSteps = pinfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }

    if (pinfo->Has(vtkPVInformationKeys::TIME_LABEL_ANNOTATION()))
    {
      this->TimeLabel = pinfo->Get(vtkPVInformationKeys::TIME_LABEL_ANNOTATION());
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromDataObject(vtkDataObject* dobj)
{
  if (!dobj)
  {
    return;
  }
  assert(vtkCompositeDataSet::SafeDownCast(dobj) == nullptr);
  assert(this->DataSetType == -1); // ensure that we're not accidentally accumulating.

  this->DataSetType = dobj->GetDataObjectType();
  this->MemorySize = dobj->GetActualMemorySize();
  this->NumberOfDataSets = 1;

  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++cc)
  {
    this->AttributeInformations[cc]->CopyFromDataObject(dobj);
    switch (cc)
    {
      case vtkDataObject::FIELD:
        // for field data, we get the maximum number of tuples in field data.
        this->NumberOfElements[cc] = this->AttributeInformations[cc]->GetMaximumNumberOfTuples();
        break;

      default:
        this->NumberOfElements[cc] = dobj->GetNumberOfElements(cc);
        break;
    }
  }

  if (auto ds = vtkDataSet::SafeDownCast(dobj))
  {
    ds->GetBounds(this->Bounds);
    if (auto img = vtkImageData::SafeDownCast(ds))
    {
      img->GetExtent(this->Extent);
    }
    else if (auto rg = vtkRectilinearGrid::SafeDownCast(ds))
    {
      rg->GetExtent(this->Extent);
    }
    else if (auto sg = vtkStructuredGrid::SafeDownCast(ds))
    {
      sg->GetExtent(this->Extent);
    }
    else if (auto esg = vtkExplicitStructuredGrid::SafeDownCast(ds))
    {
      esg->GetExtent(this->Extent);
    }

    if (auto ps = vtkPointSet::SafeDownCast(dobj))
    {
      if (ps->GetPoints() && ps->GetPoints()->GetData())
      {
        this->PointArrayInformation->CopyFromArray(ps->GetPoints()->GetData());
        // irrespective of the name used by the internally vtkDataArray, always
        // rename the points as "Points" so the application always identifies
        // them as such.
        this->PointArrayInformation->SetName("Points");
      }
    }
  }
  else if (auto htg = vtkHyperTreeGrid::SafeDownCast(dobj))
  {
    // for vtkHyperTreeGrid, leaves and trees are counted specially.
    this->NumberOfTrees = htg->GetMaxNumberOfTrees();
    this->NumberOfLeaves = htg->GetNumberOfLeaves();
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(vtkPVInformation* oinfo)
{
  auto other = vtkPVDataInformation::SafeDownCast(oinfo);
  if (other == nullptr)
  {
    return;
  }
  if (this->DataSetType == -1)
  {
    this->DeepCopy(other);
    return;
  }

  this->DataSetType =
    vtkDataObjectTypes::GetCommonBaseTypeId(this->DataSetType, other->DataSetType);
  this->CompositeDataSetType = vtkDataObjectTypes::GetCommonBaseTypeId(
    this->CompositeDataSetType, other->CompositeDataSetType);
  this->FirstLeafCompositeIndex =
    std::max(this->FirstLeafCompositeIndex, other->FirstLeafCompositeIndex);
  this->NumberOfTrees += other->NumberOfTrees;
  this->NumberOfLeaves += other->NumberOfLeaves; // TODO: max?
  this->NumberOfAMRLevels = std::max(this->NumberOfAMRLevels, other->NumberOfAMRLevels);
  this->NumberOfDataSets += other->NumberOfDataSets;
  this->MemorySize += other->MemorySize;
  ::MergeBounds(this->Bounds, other->Bounds);
  ::MergeExtent(this->Extent, other->Extent);
  if (other->HasTime)
  {
    if (this->HasTime && this->Time != other->Time)
    {
      vtkLogF(WARNING, "time mismatch: %lf != %lf", this->Time, other->Time);
    }
    this->HasTime = other->HasTime;
    this->Time = other->Time;
  }
  ::MergeRange(this->TimeRange, other->TimeRange);
  if (!other->TimeLabel.empty())
  {
    if (!this->TimeLabel.empty() && this->TimeLabel != other->TimeLabel)
    {
      vtkLogF(WARNING, "time-label mismatch: '%s' != '%s'", this->TimeLabel.c_str(),
        other->TimeLabel.c_str());
    }
    this->TimeLabel = other->TimeLabel;
  }

  // TODO: not sure what's the best way.
  this->NumberOfTimeSteps = std::max(this->NumberOfTimeSteps, other->NumberOfTimeSteps);

  std::set<int> types;
  std::copy(this->UniqueBlockTypes.begin(), this->UniqueBlockTypes.end(),
    std::inserter(types, types.end()));
  std::copy(other->UniqueBlockTypes.begin(), other->UniqueBlockTypes.end(),
    std::inserter(types, types.end()));

  this->UniqueBlockTypes.clear();
  this->UniqueBlockTypes.insert(this->UniqueBlockTypes.end(), types.begin(), types.end());

  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++cc)
  {
    switch (cc)
    {
      case vtkDataObject::FIELD:
        this->NumberOfElements[cc] =
          std::max(this->NumberOfElements[cc], other->NumberOfElements[cc]);
      default:
        this->NumberOfElements[cc] += other->NumberOfElements[cc];
    }

    this->AttributeInformations[cc]->AddInformation(other->AttributeInformations[cc]);
  }
  this->PointArrayInformation->AddInformation(other->PointArrayInformation, vtkDataObject::POINT);

  // How to merge assemblies/structure?
  // we don't do anything currently since they are expected to be same on all
  // ranks.
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::DeepCopy(vtkPVDataInformation* other)
{
  if (!other || other->DataSetType == -1)
  {
    this->Initialize();
    return;
  }

  this->DataSetType = other->DataSetType;
  this->CompositeDataSetType = other->CompositeDataSetType;
  this->FirstLeafCompositeIndex = other->FirstLeafCompositeIndex;
  this->NumberOfTrees = other->NumberOfTrees;
  this->NumberOfLeaves = other->NumberOfLeaves;
  this->NumberOfAMRLevels = other->NumberOfAMRLevels;
  this->NumberOfDataSets = other->NumberOfDataSets;
  this->MemorySize = other->MemorySize;
  std::copy(other->Bounds, other->Bounds + 6, this->Bounds);
  std::copy(other->Extent, other->Extent + 6, this->Extent);
  this->HasTime = other->HasTime;
  this->Time = other->Time;
  std::copy(other->TimeRange, other->TimeRange + 2, this->TimeRange);
  this->TimeLabel = other->TimeLabel;
  this->NumberOfTimeSteps = other->NumberOfTimeSteps;
  this->UniqueBlockTypes = other->UniqueBlockTypes;
  std::copy(other->NumberOfElements,
    other->NumberOfElements + vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES, this->NumberOfElements);
  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++cc)
  {
    this->AttributeInformations[cc]->DeepCopy(other->AttributeInformations[cc]);
  }
  this->PointArrayInformation->DeepCopy(other->PointArrayInformation);
  this->Hierarchy->DeepCopy(other->Hierarchy);
  this->DataAssembly->DeepCopy(other->DataAssembly);
}

//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetPrettyDataTypeString() const
{
  const int dataType =
    this->CompositeDataSetType == -1 ? this->DataSetType : this->CompositeDataSetType;
  return vtkPVDataInformation::GetPrettyDataTypeString(dataType);
}
//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetPrettyDataTypeString(int dataType)
{
  const char* className = vtkPVDataInformation::GetDataSetTypeAsString(dataType);
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
    case VTK_EXPLICIT_STRUCTURED_GRID:
      return "Explicit Structured Grid";
    case VTK_MOLECULE:
      return "Molecule";
    case VTK_PARTITIONED_DATA_SET:
      return "Partitioned Dataset";
    case VTK_PARTITIONED_DATA_SET_COLLECTION:
      return "Partitioned Dataset Collection";
    default:
      break;
  }
  return (className && className[0]) ? className : "UnknownType";
}

//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetDataSetTypeAsString(int type)
{
  return (type == -1 ? "UnknownType" : vtkDataObjectTypes::GetClassNameFromTypeId(type));
}

//----------------------------------------------------------------------------
bool vtkPVDataInformation::DataSetTypeIsA(const char* type) const
{
  const int targetTypeId = vtkDataObjectTypes::GetTypeIdFromClassName(type);
  if (targetTypeId == -1)
  {
    vtkErrorMacro("Unknown data object type '" << (type ? type : "(nullptr)") << "'.");
    return false;
  }

  return this->DataSetTypeIsA(targetTypeId);
}

//----------------------------------------------------------------------------
bool vtkPVDataInformation::DataSetTypeIsA(int typeId) const
{
  if (this->CompositeDataSetType != -1 &&
    vtkDataObjectTypes::TypeIdIsA(this->CompositeDataSetType, typeId))
  {
    return true;
  }
  return vtkDataObjectTypes::TypeIdIsA(this->DataSetType, typeId);
}

//----------------------------------------------------------------------------
bool vtkPVDataInformation::HasDataSetType(const char* type) const
{
  const int targetTypeId = vtkDataObjectTypes::GetTypeIdFromClassName(type);
  if (targetTypeId == -1)
  {
    vtkErrorMacro("Unknown data object type '" << (type ? type : "(nullptr)") << "'.");
    return false;
  }

  return this->HasDataSetType(targetTypeId);
}

//----------------------------------------------------------------------------
bool vtkPVDataInformation::HasDataSetType(int typeId) const
{
  if (this->DataSetTypeIsA(typeId))
  {
    return true;
  }

  if (this->CompositeDataSetType != -1)
  {
    for (auto& leafType : this->UniqueBlockTypes)
    {
      if (vtkDataObjectTypes::TypeIdIsA(leafType, typeId))
      {
        return true;
      }
    }
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkPVDataInformation::IsDataStructured() const
{
  // if this->DataSetType is structured, means even for composite datasets
  // all non-null leaves are structured.
  if (this->DataSetType != -1 &&
    vtkPVDataInformation::GetExtentType(this->DataSetType) == VTK_3D_EXTENT)
  {
    return true;
  }

  if (this->CompositeDataSetType != -1 && this->UniqueBlockTypes.size() > 0)
  {
    for (auto& type : this->UniqueBlockTypes)
    {
      if (vtkPVDataInformation::GetExtentType(type) != VTK_3D_EXTENT)
      {
        return false;
      }
    }
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkPVDataInformation::HasStructuredData() const
{
  if (this->DataSetType != -1 &&
    vtkPVDataInformation::GetExtentType(this->DataSetType) == VTK_3D_EXTENT)
  {
    return true;
  }

  if (this->CompositeDataSetType != -1 && this->UniqueBlockTypes.size() > 0)
  {
    for (auto& type : this->UniqueBlockTypes)
    {
      if (vtkPVDataInformation::GetExtentType(type) == VTK_3D_EXTENT)
      {
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVDataInformation::HasUnstructuredData() const
{
  // TODO: this needs improvement. currently this will return true for "base classes"
  if (vtkPVDataInformation::GetExtentType(this->DataSetType) != VTK_3D_EXTENT)
  {
    return true;
  }

  if (this->CompositeDataSetType != -1 && this->UniqueBlockTypes.size() > 0)
  {
    for (auto& type : this->UniqueBlockTypes)
    {
      if (vtkPVDataInformation::GetExtentType(type) != VTK_3D_EXTENT)
      {
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVDataInformation::IsAttributeValid(int fieldAssociation) const
{
  auto f = [&fieldAssociation](int dtype) {
    switch (fieldAssociation)
    {
      case vtkDataObject::FIELD_ASSOCIATION_NONE:
        return true;

      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        return vtkDataObjectTypes::TypeIdIsA(dtype, VTK_DATA_SET);

      case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      case vtkDataObject::FIELD_ASSOCIATION_EDGES:
        return vtkDataObjectTypes::TypeIdIsA(dtype, VTK_GRAPH);

      case vtkDataObject::FIELD_ASSOCIATION_ROWS:
        return vtkDataObjectTypes::TypeIdIsA(dtype, VTK_TABLE);

      default:
        return false;
    }
  };

  if (this->IsCompositeDataSet())
  {
    for (auto& type : this->UniqueBlockTypes)
    {
      if (f(type))
      {
        return true;
      }
    }

    if (this->UniqueBlockTypes.empty())
    {
      // if uniqueBlockTypesLength is empty, we most likely don't
      // have complete input data information and hence we cant determine which
      // attribute is valid conclusively yet. Just return true.
      // Fixes issue with paraviewPython-AppendAttributes test.
      return true;
    }
  }
  return f(this->DataSetType);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->DataSetType << this->CompositeDataSetType << this->FirstLeafCompositeIndex
       << this->NumberOfTrees << this->NumberOfLeaves << this->NumberOfAMRLevels
       << this->NumberOfDataSets << this->MemorySize
       << vtkClientServerStream::InsertArray(this->Bounds, 6)
       << vtkClientServerStream::InsertArray(this->Extent, 6) << this->HasTime << this->Time
       << vtkClientServerStream::InsertArray(this->TimeRange, 2) << this->TimeLabel
       << this->NumberOfTimeSteps << vtkClientServerStream::InsertArray(this->NumberOfElements,
                                       vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES);

  *css << static_cast<int>(this->UniqueBlockTypes.size());
  if (this->UniqueBlockTypes.size() > 0)
  {
    *css << vtkClientServerStream::InsertArray(
      &this->UniqueBlockTypes.front(), static_cast<int>(this->UniqueBlockTypes.size()));
  }

  vtkClientServerStream temp;
  this->PointArrayInformation->CopyToStream(&temp);
  *css << temp;

  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++cc)
  {
    temp.Reset();
    this->AttributeInformations[cc]->CopyToStream(&temp);
    *css << temp;
  }

  if (this->CompositeDataSetType != -1)
  {
    *css << this->DataAssembly->SerializeToXML(vtkIndent());
    *css << this->Hierarchy->SerializeToXML(vtkIndent());
  }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromStream(const vtkClientServerStream* css)
{
  // argument counter.
  int argument = 0;

  if (!css->GetArgument(0, argument++, &this->DataSetType) ||
    !css->GetArgument(0, argument++, &this->CompositeDataSetType) ||
    !css->GetArgument(0, argument++, &this->FirstLeafCompositeIndex) ||
    !css->GetArgument(0, argument++, &this->NumberOfTrees) ||
    !css->GetArgument(0, argument++, &this->NumberOfLeaves) ||
    !css->GetArgument(0, argument++, &this->NumberOfAMRLevels) ||
    !css->GetArgument(0, argument++, &this->NumberOfDataSets) ||
    !css->GetArgument(0, argument++, &this->MemorySize) ||
    !css->GetArgument(0, argument++, this->Bounds, 6) ||
    !css->GetArgument(0, argument++, this->Extent, 6) ||
    !css->GetArgument(0, argument++, &this->HasTime) ||
    !css->GetArgument(0, argument++, &this->Time) ||
    !css->GetArgument(0, argument++, this->TimeRange, 2) ||
    !css->GetArgument(0, argument++, &this->TimeLabel) ||
    !css->GetArgument(0, argument++, &this->NumberOfTimeSteps) ||
    !css->GetArgument(
      0, argument++, this->NumberOfElements, vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES))
  {
    this->Initialize();
    vtkErrorMacro("Error parsing stream.");
    return;
  }

  // read UniqueBlockTypes.
  int uniqueBlockTypesLength;
  if (!css->GetArgument(0, argument++, &uniqueBlockTypesLength))
  {
    this->Initialize();
    vtkErrorMacro("Error parsing stream.");
    return;
  }
  this->UniqueBlockTypes.resize(uniqueBlockTypesLength);
  if (uniqueBlockTypesLength > 0 &&
    !css->GetArgument(0, argument++, &this->UniqueBlockTypes[0], uniqueBlockTypesLength))
  {
    this->Initialize();
    vtkErrorMacro("Error parsing stream.");
    return;
  }

  vtkClientServerStream temp;
  if (!css->GetArgument(0, argument++, &temp))
  {
    this->Initialize();
    vtkErrorMacro("Error parsing stream.");
    return;
  }
  this->PointArrayInformation->CopyFromStream(&temp);

  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++cc)
  {
    temp.Reset();
    if (!css->GetArgument(0, argument++, &temp))
    {
      this->Initialize();
      vtkErrorMacro("Error parsing stream.");
      return;
    }
    this->AttributeInformations[cc]->CopyFromStream(&temp);
  }

  if (this->CompositeDataSetType != -1)
  {
    std::string hierarchy, assembly;
    if (!css->GetArgument(0, argument++, &assembly) ||
      !this->DataAssembly->InitializeFromXML(assembly.c_str()) ||
      !css->GetArgument(0, argument++, &hierarchy) ||
      !this->Hierarchy->InitializeFromXML(hierarchy.c_str()))
    {
      this->Initialize();
      vtkErrorMacro("Error parsing stream.");
      return;
    }
  }
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVDataInformation::GetArrayInformation(
  const char* arrayname, int attribute_type) const
{
  vtkPVDataSetAttributesInformation* attrInfo = this->GetAttributeInformation(attribute_type);
  return attrInfo ? attrInfo->GetArrayInformation(arrayname) : nullptr;
}

//----------------------------------------------------------------------------
int vtkPVDataInformation::GetExtentType(int type)
{
  // types defined in vtkType.h
  switch (type)
  {
    case VTK_EXPLICIT_STRUCTURED_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_STRUCTURED_GRID:
    case VTK_RECTILINEAR_GRID:
    case VTK_IMAGE_DATA:
    case VTK_UNIFORM_GRID:
      return VTK_3D_EXTENT;

    case VTK_POLY_DATA:
    case VTK_UNSTRUCTURED_GRID:
    case VTK_PIECEWISE_FUNCTION:
    case VTK_DATA_OBJECT:
    case VTK_DATA_SET:
    case VTK_POINT_SET:
    case VTK_COMPOSITE_DATA_SET:
    case VTK_MULTIGROUP_DATA_SET:
    case VTK_MULTIBLOCK_DATA_SET:
    case VTK_HIERARCHICAL_DATA_SET:
    case VTK_HIERARCHICAL_BOX_DATA_SET:
    case VTK_GENERIC_DATA_SET:
    case VTK_HYPER_OCTREE:
    case VTK_TEMPORAL_DATA_SET:
    case VTK_TABLE:
    case VTK_GRAPH:
    case VTK_TREE:
    case VTK_SELECTION:
    case VTK_DIRECTED_GRAPH:
    case VTK_UNDIRECTED_GRAPH:
    case VTK_MULTIPIECE_DATA_SET:
    case VTK_DIRECTED_ACYCLIC_GRAPH:
    case VTK_ARRAY_DATA:
    case VTK_REEB_GRAPH:
    case VTK_UNIFORM_GRID_AMR:
    case VTK_NON_OVERLAPPING_AMR:
    case VTK_OVERLAPPING_AMR:
    case VTK_HYPER_TREE_GRID:
    case VTK_MOLECULE:
    case VTK_PISTON_DATA_OBJECT:
    case VTK_PATH:
    case VTK_UNSTRUCTURED_GRID_BASE:
    case VTK_PARTITIONED_DATA_SET:
    case VTK_PARTITIONED_DATA_SET_COLLECTION:
    case VTK_UNIFORM_HYPER_TREE_GRID:
      return VTK_PIECES_EXTENT;

    default:
      vtkLogF(ERROR, "Unknown type: %d", type);
      return VTK_PIECES_EXTENT;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkPVDataInformation::GetSubset(vtkDataObject* dobj) const
{
  auto cd = vtkCompositeDataSet::SafeDownCast(dobj);
  if (!this->SubsetSelector || cd == nullptr)
  {
    return dobj;
  }

  auto activeAssembly = vtkDataAssemblyUtilities::GetDataAssembly(this->SubsetAssemblyName, cd);
  if (!activeAssembly)
  {
    return nullptr;
  }

  const auto cids = vtkDataAssemblyUtilities::GetSelectedCompositeIds(
    { this->SubsetSelector }, activeAssembly, vtkPartitionedDataSetCollection::SafeDownCast(cd));
  if (cids.size() == 0)
  {
    return nullptr;
  }

  if (cids.size() == 1)
  {
    vtkSmartPointer<vtkPartitionedDataSetCollection> subset;
    auto iter = vtkSmartPointer<vtkCompositeDataIterator>::Take(cd->NewIterator());
    if (auto diter = vtkDataObjectTreeIterator::SafeDownCast(iter))
    {
      diter->VisitOnlyLeavesOff();
      diter->TraverseSubTreeOn();
    }
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (iter->GetCurrentFlatIndex() == cids.front())
      {
        return iter->GetCurrentDataObject();
      }
    }
    return nullptr;
  }
  else
  {
    vtkNew<vtkExtractBlockUsingDataAssembly> extractor;
    extractor->SetInputDataObject(dobj);
    extractor->SetAssemblyName(this->SubsetAssemblyName);
    extractor->SetSelector(this->SubsetSelector);
    extractor->Update();
    return extractor->GetOutputDataObject(0);
  }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::SetSubsetAssemblyNameToHierarchy()
{
  this->SetSubsetAssemblyName(vtkDataAssemblyUtilities::HierarchyName());
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkPVDataInformation::GetHierarchy() const
{
  return this->IsCompositeDataSet() ? this->Hierarchy.Get() : nullptr;
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkPVDataInformation::GetDataAssembly(const char* assemblyName) const
{
  if (assemblyName && strcmp(vtkDataAssemblyUtilities::HierarchyName(), assemblyName) == 0)
  {
    return this->GetHierarchy();
  }
  return this->GetDataAssembly();
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkPVDataInformation::GetDataAssembly() const
{
  return vtkDataObjectTypes::TypeIdIsA(
           this->CompositeDataSetType, VTK_PARTITIONED_DATA_SET_COLLECTION)
    ? this->DataAssembly.Get()
    : nullptr;
}

//----------------------------------------------------------------------------
int vtkPVDataInformation::GetUniqueBlockType(unsigned int index) const
{
  return index < this->GetNumberOfUniqueBlockTypes() ? this->UniqueBlockTypes[index] : -1;
}

//----------------------------------------------------------------------------
std::string vtkPVDataInformation::GetBlockName(vtkTypeUInt64 cid) const
{
  if (this->DataSetTypeIsA(VTK_MULTIBLOCK_DATA_SET))
  {
    auto hierarchy = this->GetHierarchy();
    const auto selector = vtkDataAssemblyUtilities::GetSelectorForCompositeId(cid, hierarchy);
    const auto nodes = hierarchy->SelectNodes({ selector });
    if (nodes.size() >= 1)
    {
      return hierarchy->GetAttributeOrDefault(nodes.front(), "label", "");
    }
  }
  return {};
}

//============================================================================
#if !defined(VTK_LEGACY_REMOVE)
vtkTypeUInt64 vtkPVDataInformation::GetPolygonCount()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVDataInformation::GetPolygonCount, "ParaView 5.10", vtkPVDataInformation::GetNumberOfCells);
  return this->GetNumberOfCells();
}

void* vtkPVDataInformation::GetCompositeDataInformation()
{
  VTK_LEGACY_BODY(vtkPVDataInformation::GetCompositeDataInformation, "ParaView 5.10");
  return nullptr;
}

vtkPVDataInformation* vtkPVDataInformation::GetDataInformationForCompositeIndex(int)
{
  VTK_LEGACY_BODY(vtkPVDataInformation::GetDataInformationForCompositeIndex, "ParaView 5.10");
  return nullptr;
}

unsigned int vtkPVDataInformation::GetNumberOfBlockLeafs(bool)
{
  VTK_LEGACY_BODY(vtkPVDataInformation::GetNumberOfBlockLeafs, "ParaView 5.10");
  return 0;
}

vtkPVDataInformation* vtkPVDataInformation::GetDataInformationForCompositeIndex(int*)
{
  VTK_LEGACY_BODY(vtkPVDataInformation::GetDataInformationForCompositeIndex, "ParaView 5.10");
  return nullptr;
}

double* vtkPVDataInformation::GetTimeSpan()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVDataInformation::GetTimeSpan, "ParaView 5.10", vtkPVDataInformation::GetTimeRange);
  return this->GetTimeRange();
}

void vtkPVDataInformation::GetTimeSpan(double& x, double& y)
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVDataInformation::GetTimeSpan, "ParaView 5.10", vtkPVDataInformation::GetTimeRange);
  this->GetTimeRange(x, y);
}

void vtkPVDataInformation::GetTimeSpan(double val[2])
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVDataInformation::GetTimeSpan, "ParaView 5.10", vtkPVDataInformation::GetTimeRange);
  this->GetTimeRange(val);
}

void vtkPVDataInformation::RegisterHelper(const char*, const char*)
{
  VTK_LEGACY_BODY(vtkPVDataInformation::RegisterHelper, "ParaView 5.10");
}

#endif
