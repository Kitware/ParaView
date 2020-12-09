/*=========================================================================

  Program:   ParaView
  Module:    vtkGmshWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGmshWriter.h"
#include "gmshCommon.h"

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkType.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include "gmsh.h"

#include <numeric>
#include <set>
#include <unordered_map>
#include <vector>

//-----------------------------------------------------------------------------
struct GmshWriterInternal
{
  int Dimension = 3;
  int ModelTag = -1;
  std::vector<std::string> NodeViews;
  std::vector<std::string> CellViews;
  std::string ModelName;

  std::vector<std::size_t> CellDataIndex;

  double* TimeSteps = nullptr;
  unsigned int NumberOfTimeSteps = 0;
  unsigned int CurrentTimeStep = 0;
  double CurrentTime = 0.0;

  vtkUnstructuredGrid* Input = nullptr;

  static const std::unordered_map<unsigned char, GmshPrimitive> TRANSLATE_CELLS_TYPE;
  static constexpr unsigned char MAX_TAG = 15u;
};

const std::unordered_map<unsigned char, GmshPrimitive> GmshWriterInternal::TRANSLATE_CELLS_TYPE = {
  { VTK_VERTEX, GmshPrimitive::POINT }, { VTK_LINE, GmshPrimitive::LINE },
  { VTK_POLY_LINE, GmshPrimitive::Unsupported }, { VTK_TRIANGLE, GmshPrimitive::TRIANGLE },
  { VTK_TRIANGLE_STRIP, GmshPrimitive::Unsupported }, { VTK_POLYGON, GmshPrimitive::Unsupported },
  { VTK_PIXEL, GmshPrimitive::QUAD }, { VTK_QUAD, GmshPrimitive::QUAD },
  { VTK_TETRA, GmshPrimitive::TETRA }, { VTK_VOXEL, GmshPrimitive::HEXA },
  { VTK_HEXAHEDRON, GmshPrimitive::HEXA }, { VTK_WEDGE, GmshPrimitive::PRISM },
  { VTK_PYRAMID, GmshPrimitive::PYRAMID }
};

//-----------------------------------------------------------------------------
namespace
{
// Reorder voxel vertices as hexahedron vertices to support it with Gmsh.
// `cellArray` contains a lot a cell vertices, while `idx` is the index of the
// first vertex for the current voxel
void OrderVoxel(std::vector<std::size_t>& cellArray, std::size_t idx)
{
  std::swap(cellArray[idx + 2], cellArray[idx + 3]);
  std::swap(cellArray[idx + 6], cellArray[idx + 7]);
}

// Reorder pixel vertices as quad vertices to support it with Gmsh.
// `cellArray` contains a lot a cell vertices, while `idx` is the index of the
// first vertex for the current pixel
void OrderPixel(std::vector<std::size_t>& cellArray, std::size_t idx)
{
  std::swap(cellArray[idx + 2], cellArray[idx + 3]);
}

// Transform VTK types not supported by Gmsh (like triange strips or polylines)
// as simpler primitives.
void AddTriangulatedCell(std::vector<std::size_t>& nodeTags,
  std::vector<std::size_t>& newTypeIndexes, const std::vector<std::size_t>& oldTypeIndexes,
  GmshWriterInternal* internal, vtkIdType& cellCounterId, const int dim)
{
  for (std::size_t idx : oldTypeIndexes)
  {
    const std::size_t vtkIdx = idx - 1u;
    vtkCell* cell = internal->Input->GetCell(vtkIdx);
    vtkNew<vtkIdList> ptIds;
    vtkNew<vtkPoints> points;
    cell->Triangulate(0, ptIds, points);
    const vtkIdType numIds = ptIds->GetNumberOfIds();

    for (vtkIdType i = 0; i < numIds; ++i)
    {
      nodeTags.push_back(ptIds->GetId(i) + 1);
    }
    for (int i = 0; i < (numIds / dim); ++i)
    {
      newTypeIndexes.push_back(cellCounterId);
      internal->CellDataIndex.push_back(vtkIdx);
      ++cellCounterId;
    }
  }
}

// Add every supported VTK cells in the gmsh model by type.
void FillCells(const int modelTag, GmshWriterInternal* internal,
  std::vector<std::size_t> idxPerType[], vtkUnstructuredGrid* input, vtkDataArray* offsets,
  vtkDataArray* connectivity)
{
  vtkIdType cellCounterId = 1;
  internal->CellDataIndex.clear();
  internal->CellDataIndex.reserve(input->GetNumberOfCells());

  for (unsigned char currentType = 1; currentType < GmshWriterInternal::MAX_TAG; ++currentType)
  {
    std::vector<std::size_t>& indexes = idxPerType[currentType];
    if (indexes.empty())
    {
      continue;
    }

    char gmshType =
      static_cast<char>(GmshWriterInternal::TRANSLATE_CELLS_TYPE.find(currentType)->second);
    // If this type is not natively supported, it will be transleted in either lines or triangles
    if (gmshType < 0)
    {
      continue;
    }

    // Add cells for this vtk type
    std::vector<std::size_t> gmshNodeTags;
    for (std::size_t idx : indexes)
    {
      const std::size_t vtkIdx = idx - 1;
      const int beginCell = gmshNodeTags.size();
      long offsetBegin = static_cast<long>(*offsets->GetTuple(vtkIdx));
      long offsetEnd = static_cast<long>(*offsets->GetTuple(vtkIdx + 1));
      for (unsigned int j = offsetBegin; j < offsetEnd; ++j)
      {
        gmshNodeTags.push_back(static_cast<long>(*connectivity->GetTuple(j)) + 1u);
      }

      // Ordering if needed
      if (currentType == VTK_PIXEL)
      {
        ::OrderPixel(gmshNodeTags, beginCell);
      }
      else if (currentType == VTK_VOXEL)
      {
        ::OrderVoxel(gmshNodeTags, beginCell);
      }

      // Add data index
      internal->CellDataIndex.push_back(vtkIdx);
    }

    // Generate gmsh ids
    std::vector<std::size_t> gmshIds(indexes.size());
    std::iota(gmshIds.begin(), gmshIds.end(), cellCounterId);
    cellCounterId += gmshIds.size();

    // Translate non-supported type into supported one
    if (currentType == VTK_LINE)
    {
      ::AddTriangulatedCell(
        gmshNodeTags, gmshIds, idxPerType[VTK_POLY_LINE], internal, cellCounterId, 2);
    }
    else if (currentType == VTK_TRIANGLE)
    {
      ::AddTriangulatedCell(
        gmshNodeTags, gmshIds, idxPerType[VTK_TRIANGLE_STRIP], internal, cellCounterId, 3);
      ::AddTriangulatedCell(
        gmshNodeTags, gmshIds, idxPerType[VTK_POLYGON], internal, cellCounterId, 3);
    }

    gmsh::model::mesh::addElementsByType(modelTag, gmshType, gmshIds, gmshNodeTags);
  }
}
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGmshWriter);

//-----------------------------------------------------------------------------
vtkGmshWriter::vtkGmshWriter()
  : Internal(new GmshWriterInternal)
{
  this->SetNumberOfInputPorts(1);
}

//-----------------------------------------------------------------------------
vtkGmshWriter::~vtkGmshWriter()
{
  this->SetFileName(nullptr);
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkGmshWriter::LoadNodes()
{
  vtkUnstructuredGrid* input = this->Internal->Input;

  vtkDataArray* points = input->GetPoints()->GetData();

  const vtkIdType numTuples = points->GetNumberOfTuples();
  const vtkIdType numCompnt = points->GetNumberOfComponents();
  std::vector<double> inlineCoords(numTuples * numCompnt);
  // Store point coordinates in a structure Gmsh can understand
  for (vtkIdType i = 0, counter = 0; i < numTuples; ++i)
  {
    for (vtkIdType j = 0; j < numCompnt; ++j)
    {
      inlineCoords[counter] = *(points->GetTuple(i) + j);
      ++counter;
    }
  }

  std::vector<std::size_t> gmshTags(numTuples);
  std::iota(gmshTags.begin(), gmshTags.end(), 1u);

  gmsh::model::mesh::addNodes(
    this->Internal->Dimension, this->Internal->ModelTag, gmshTags, inlineCoords);
}

//-----------------------------------------------------------------------------
void vtkGmshWriter::LoadCells()
{
  vtkUnstructuredGrid* input = this->Internal->Input;

  // Build list of gmsh indexes per type
  vtkCellArray* cells = input->GetCells();
  vtkUnsignedCharArray* cellType = input->GetCellTypesArray();
  std::vector<std::size_t> indexesPerTypes[GmshWriterInternal::MAX_TAG];

  for (vtkIdType i = 0; i < cells->GetNumberOfCells(); ++i)
  {
    const unsigned char vtkCellType = cellType->GetValue(i);
    if (!GmshWriterInternal::TRANSLATE_CELLS_TYPE.count(vtkCellType))
    {
      continue;
    }
    indexesPerTypes[vtkCellType].push_back(i + 1);
  }

  // Add these cells to gmsh::model
  ::FillCells(this->Internal->ModelTag, this->Internal, indexesPerTypes, input,
    cells->GetOffsetsArray(), cells->GetConnectivityArray());
}

//-----------------------------------------------------------------------------
void vtkGmshWriter::LoadNodeData()
{
  vtkDataSetAttributes* pointData =
    vtkDataSetAttributes::SafeDownCast(this->Internal->Input->GetPointData());
  const int numVtkArrays = this->Internal->NodeViews.size();
  if (numVtkArrays == 0)
  {
    return;
  }

  const vtkIdType numTuples =
    pointData->GetAbstractArray(pointData->GetArrayName(0))->GetNumberOfTuples();
  // Generate Gmsh tags
  std::vector<std::size_t> tags(numTuples);
  std::iota(tags.begin(), tags.end(), 1);

  for (int arrayId = 0; arrayId < numVtkArrays; ++arrayId)
  {
    // Get VTK data (we can safely safedowncast because we already checked in InitViews())
    const std::string arrayName = this->Internal->NodeViews[arrayId];
    vtkDataArray* vtkArray =
      vtkDataArray::SafeDownCast(pointData->GetAbstractArray(arrayName.c_str()));
    const int numComponents = vtkArray->GetNumberOfComponents();

    // Store it in a structure Gmsh can understand
    std::vector<double> gmshData(numTuples * numComponents);
    gmshData.resize(numTuples * numComponents);
    vtkIdType counter = 0;
    for (vtkIdType i = 0; i < numTuples; ++i)
    {
      for (int j = 0; j < numComponents; ++j)
      {
        gmshData[counter] = *(vtkArray->GetTuple(i) + j);
        ++counter;
      }
    }

    // Finally add it to gmsh
    gmsh::view::addHomogeneousModelData(arrayId, this->Internal->CurrentTimeStep,
      this->Internal->ModelName, "NodeData", tags, gmshData, this->Internal->CurrentTime,
      numComponents);
  }
}

//-----------------------------------------------------------------------------
void vtkGmshWriter::LoadCellData()
{
  vtkDataSetAttributes* cellData =
    vtkDataSetAttributes::SafeDownCast(this->Internal->Input->GetCellData());
  const int numVtkArrays = this->Internal->CellViews.size();
  if (numVtkArrays == 0)
  {
    return;
  }

  const int vtkArrayOffset = this->Internal->NodeViews.size();
  // Generate Gmsh tags
  std::vector<std::size_t> tags(this->Internal->CellDataIndex.size());
  std::iota(tags.begin(), tags.end(), 1);

  for (int arrayId = 0; arrayId < numVtkArrays; ++arrayId)
  {
    // Get VTK data (we can safely safedowncast because we already checked in InitViews())
    std::string arrayName = this->Internal->CellViews[arrayId];
    vtkDataArray* vtkArray =
      vtkDataArray::SafeDownCast(cellData->GetAbstractArray(arrayName.c_str()));
    const int numComponents = vtkArray->GetNumberOfComponents();

    // Store it in a structure Gmsh can understand
    std::vector<double> gmshData(this->Internal->CellDataIndex.size() * numComponents);
    vtkIdType counter = 0;
    for (std::size_t idx : this->Internal->CellDataIndex)
    {
      for (int j = 0; j < numComponents; ++j)
      {
        gmshData[counter] = *(vtkArray->GetTuple(idx) + j);
        ++counter;
      }
    }

    // Finally add it to gmsh
    gmsh::view::addHomogeneousModelData(arrayId + vtkArrayOffset, this->Internal->CurrentTimeStep,
      this->Internal->ModelName, "ElementData", tags, gmshData, this->Internal->CurrentTime,
      numComponents);
  }
}

//-----------------------------------------------------------------------------
int vtkGmshWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkGmshWriter::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->WriteAllTimeSteps && inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->Internal->NumberOfTimeSteps =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->Internal->NumberOfTimeSteps = 0;
  }
  this->Internal->CurrentTimeStep = 0;

  return 1;
}

//-----------------------------------------------------------------------------
int vtkGmshWriter::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->WriteAllTimeSteps && inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double* timeSteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->Internal->CurrentTime = timeSteps[this->Internal->CurrentTimeStep];
    inputVector[0]->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->Internal->CurrentTime);
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkGmshWriter::InitViews()
{
  vtkDataSetAttributes* pointsData =
    vtkDataSetAttributes::SafeDownCast(this->Internal->Input->GetPointData());

  int arrayCounter = 0;
  std::string name;
  const int nbPointArrays = pointsData->GetNumberOfArrays();
  for (int i = 0; i < nbPointArrays; ++i)
  {
    name = pointsData->GetArrayName(i);
    if (name.rfind("gmsh", 0) == 0 && !this->WriteGmshSpecificArray)
    {
      continue;
    }
    vtkDataArray* vtkArray = vtkDataArray::SafeDownCast(pointsData->GetAbstractArray(name.c_str()));
    if (!vtkArray)
    {
      continue;
    }
    this->Internal->NodeViews.push_back(name);
    gmsh::view::add(name, arrayCounter);
    ++arrayCounter;
  }

  vtkDataSetAttributes* cellsData =
    vtkDataSetAttributes::SafeDownCast(this->Internal->Input->GetCellData());
  const int nbCellArrays = cellsData->GetNumberOfArrays();
  for (int i = 0; i < nbCellArrays; ++i)
  {
    name = cellsData->GetArrayName(i);
    if (name.rfind("gmsh", 0) == 0 && !this->WriteGmshSpecificArray)
    {
      continue;
    }
    vtkDataArray* vtkArray = vtkDataArray::SafeDownCast(cellsData->GetAbstractArray(name.c_str()));
    if (!vtkArray)
    {
      continue;
    }
    this->Internal->CellViews.push_back(name);
    gmsh::view::add(name, arrayCounter);
    ++arrayCounter;
  }
}

//-----------------------------------------------------------------------------
int vtkGmshWriter::RequestData(
  vtkInformation* request, vtkInformationVector**, vtkInformationVector*)
{
  // make sure the user specified a FileName
  if (!this->FileName)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return 0;
  }

  this->Internal->Input = this->GetInput();

  // If this is the first request
  if (this->Internal->CurrentTimeStep == 0)
  {
    std::string file(this->FileName);
    gmsh::initialize();
    gmsh::option::setNumber("General.Verbosity", 1);
    gmsh::option::setNumber("PostProcessing.SaveMesh", 0);
    gmsh::model::add(this->Internal->ModelName.c_str());
    gmsh::model::addDiscreteEntity(0);
    gmsh::model::addDiscreteEntity(1);
    gmsh::model::addDiscreteEntity(2);
    gmsh::model::addDiscreteEntity(3);
    this->Internal->Dimension = 3;
    // Get tag of the current model
    {
      gmsh::vectorpair tmp;
      gmsh::model::getEntities(tmp);
      this->Internal->ModelTag = tmp[0].second;
    }

    // Load and save topology
    this->LoadNodes();
    this->LoadCells();
    gmsh::write(this->FileName);

    this->InitViews();
    if (this->WriteAllTimeSteps)
    {
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
  }

  this->LoadNodeData();
  this->LoadCellData();
  this->Internal->CurrentTimeStep++;

  const int localContinue = request->Get(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  if (this->Internal->CurrentTimeStep >= this->Internal->NumberOfTimeSteps || !localContinue)
  {
    // Tell the pipeline to stop looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 0);
    this->WriteData();
    gmsh::clear();
    gmsh::finalize();
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkGmshWriter::WriteData()
{
  const int numViews = this->Internal->CellViews.size() + this->Internal->NodeViews.size();
  for (int tag = 0; tag < numViews; ++tag)
  {
    gmsh::view::write(tag, this->FileName, true);
  }
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkGmshWriter::GetInput()
{
  return vtkUnstructuredGrid::SafeDownCast(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkGmshWriter::GetInput(int port)
{
  return vtkUnstructuredGrid::SafeDownCast(this->Superclass::GetInput(port));
}

//-----------------------------------------------------------------------------
int vtkGmshWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkGmshWriter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->GetFileName() ? this->GetFileName() : "(none)") << indent
     << ", WriteAllTimeSteps: " << this->WriteAllTimeSteps << indent
     << ", WriteGmshSpecificArray: " << this->WriteGmshSpecificArray << std::endl;
}
