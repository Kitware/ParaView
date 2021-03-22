/*=========================================================================

  Program:   ParaView
  Module:    vtkGmshReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGmshReader.h"
#include "gmshCommon.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include "gmsh.h"

#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

//-----------------------------------------------------------------------------
template <typename T>
using Array2D = std::vector<std::vector<T> >;

//-----------------------------------------------------------------------------
struct DataArray
{
  std::string Type;
  std::vector<vtkSmartPointer<vtkDoubleArray> > VtkArrays; // [timestep]
  std::vector<double> Times;                               // [timestep]
};

//-----------------------------------------------------------------------------
struct PhysicalGroup
{
  int Dimension = -1;
  int Tag = -1;
  std::string Name = "";

  vtkSmartPointer<vtkIntArray> EntityIds;
  std::unordered_map<std::size_t, std::size_t> CellCorrespondence;

  // Elements by type ------------
  gmsh::vectorpair ElemType;       // [typeIndex]
  Array2D<std::size_t> ElemTags;   // [typeIndex]{tags}
  Array2D<vtkIdType> ElemNodeTags; // [typeIndex]{nodeTags}
  // -------------------- Elements

  std::vector<DataArray> Data; // [arrayIdx]
};

//-----------------------------------------------------------------------------
struct GmshReaderInternal
{
  vtkSmartPointer<vtkPoints> NodesCoords;
  vtkSmartPointer<vtkIntArray> NodeIds;
  std::vector<std::size_t> NodeCorrespondence;

  std::vector<PhysicalGroup> Groups;
  bool IsLoaded = false;
  std::set<double> Timesteps;

  // Map a GmshPrimitive to a pair (vtkType, numberOfVertices)
  static const std::unordered_map<GmshPrimitive, std::pair<int, int>, std::hash<GmshPrimitive> >
    TRANSLATE_CELL;

  static constexpr short NUM_DIM = 4;
  static constexpr const char* DIM_LABEL[NUM_DIM] = { "0-Points", "1-Curves", "2-Surfaces",
    "3-Volumes" };
};

constexpr short GmshReaderInternal::NUM_DIM;
constexpr const char* GmshReaderInternal::DIM_LABEL[GmshReaderInternal::NUM_DIM];

const std::unordered_map<GmshPrimitive, std::pair<int, int>, std::hash<GmshPrimitive> >
  GmshReaderInternal::TRANSLATE_CELL = { { GmshPrimitive::POINT, { VTK_VERTEX, 1 } },
    { GmshPrimitive::LINE, { VTK_LINE, 2 } }, { GmshPrimitive::TRIANGLE, { VTK_TRIANGLE, 3 } },
    { GmshPrimitive::QUAD, { VTK_QUAD, 4 } }, { GmshPrimitive::TETRA, { VTK_TETRA, 4 } },
    { GmshPrimitive::HEXA, { VTK_HEXAHEDRON, 8 } }, { GmshPrimitive::PRISM, { VTK_WEDGE, 6 } },
    { GmshPrimitive::PYRAMID, { VTK_PYRAMID, 5 } } };

//-----------------------------------------------------------------------------
namespace
{
void GetElements(gmsh::vectorpair& elemTypes, Array2D<std::size_t>& elemTags,
  Array2D<vtkIdType>& nodeTags, const std::vector<std::size_t>& correspondence, int dim, int entity)
{
  elemTypes.clear();
  elemTags.clear();
  nodeTags.clear();
  std::vector<int> gmshTypes;

  {
    Array2D<std::size_t> nodeTagsUL;
    gmsh::model::mesh::getElements(gmshTypes, elemTags, nodeTagsUL, dim, entity);
    // Convert std::size_t nodeTagsUL 2Dvector to a vtkIdType 2Dvector
    nodeTags.resize(nodeTagsUL.size());
    for (std::size_t i = 0; i < nodeTagsUL.size(); ++i)
    {
      nodeTags[i].resize(nodeTagsUL[i].size());
      std::copy(nodeTagsUL[i].begin(), nodeTagsUL[i].end(), nodeTags[i].begin());
    }
  }

  const int nbTypes = gmshTypes.size();
  elemTypes.resize(nbTypes);
  for (int i = nbTypes - 1; i >= 0; --i)
  {
    const int type = gmshTypes[i];
    auto vtkTypeAndDimIt = GmshReaderInternal::TRANSLATE_CELL.find(GmshPrimitive(type));
    if (vtkTypeAndDimIt != GmshReaderInternal::TRANSLATE_CELL.end())
    {
      elemTypes[i] = vtkTypeAndDimIt->second;
      for (std::size_t j = 0; j < nodeTags[i].size(); ++j)
      {
        nodeTags[i][j] = correspondence[nodeTags[i][j]];
      }
    }
    else
    {
      vtkGenericWarningMacro("Unsupported Gmsh cell type " << type);
      elemTypes.erase(elemTypes.begin() + i);
      elemTags.erase(elemTags.begin() + i);
      nodeTags.erase(nodeTags.begin() + i);
    }
  }
}
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGmshReader);

//-----------------------------------------------------------------------------
vtkGmshReader::vtkGmshReader()
  : Internal(new GmshReaderInternal)
{
  this->SetNumberOfInputPorts(0);
}

//-----------------------------------------------------------------------------
vtkGmshReader::~vtkGmshReader()
{
  this->SetFileName(nullptr);
  delete this->Internal;
}

//-----------------------------------------------------------------------------
int vtkGmshReader::LoadNodes()
{
  std::vector<std::size_t> gmshTags;
  std::vector<double> gmshCoords, _dummy;
  gmsh::model::mesh::getNodes(gmshTags, gmshCoords, _dummy, -1, -1, false, false);
  const std::size_t nbNodes = gmshTags.size();

  if (nbNodes == 0)
  {
    vtkErrorMacro("To be a valid msh file we need at least the $Nodes$ and $Elements$ section.");
    return 0;
  }

  // For simplicity of code and for handling in an efficient way most cases,
  // we assume tags are contiguous in their distribution
  const auto max = std::max_element(gmshTags.begin(), gmshTags.end());
  if ((*max) > (2 * nbNodes))
  {
    vtkWarningMacro("Node correspondence is not optimal in your Gmsh file and may increase \
    the memory usage of this reader. You may want to check you Gmsh file generation process");
  }

  this->Internal->NodeIds = vtkSmartPointer<vtkIntArray>::New();
  this->Internal->NodeIds->SetName("gmshNodeID");
  this->Internal->NodeIds->SetNumberOfComponents(1);
  this->Internal->NodeIds->SetNumberOfTuples(nbNodes);

  this->Internal->NodesCoords = vtkSmartPointer<vtkPoints>::New();
  this->Internal->NodesCoords->SetNumberOfPoints(nbNodes);

  this->Internal->NodeCorrespondence.resize((*max) + 1);

  for (std::size_t i = 0; i < nbNodes; ++i)
  {
    this->Internal->NodeIds->SetValue(i, gmshTags[i]);
    this->Internal->NodeCorrespondence[gmshTags[i]] = i;
    this->Internal->NodesCoords->SetPoint(
      i, gmshCoords[i * 3 + 0], gmshCoords[i * 3 + 1], gmshCoords[i * 3 + 2]);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkGmshReader::LoadPhysicalGroups()
{
  std::vector<PhysicalGroup>& groups = this->Internal->Groups;
  groups.clear();

  gmsh::vectorpair dimTag;
  gmsh::model::getPhysicalGroups(dimTag);

  // If no physical group we create 1 default group per dimension
  // Tag=-1 to get every elements and nodes when requesting data using gmsh API
  if (dimTag.empty())
  {
    for (int dim = 0; dim < GmshReaderInternal::NUM_DIM; ++dim)
    {
      PhysicalGroup currentGrp;
      currentGrp.Dimension = dim;
      currentGrp.Tag = -1;
      currentGrp.Name = GmshReaderInternal::DIM_LABEL[dim];
      this->FillGroupElements(currentGrp);
      this->FillGroupEntities(currentGrp);
      groups.emplace_back(currentGrp);
    }
  }
  else
  {
    for (const auto& grp : dimTag)
    {
      std::string name;
      gmsh::model::getPhysicalName(grp.first, grp.second, name);
      if (name.empty())
      {
        name = "PhysicalGroup" + std::to_string(grp.second);
      }

      PhysicalGroup currentGrp;
      currentGrp.Dimension = grp.first;
      currentGrp.Tag = grp.second;
      currentGrp.Name = name;
      this->FillGroupElements(currentGrp);
      this->FillGroupEntities(currentGrp);
      groups.emplace_back(currentGrp);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkGmshReader::FillGroupElements(PhysicalGroup& group) const
{
  if (group.Tag >= 0)
  {
    std::vector<int> entities;
    gmsh::model::getEntitiesForPhysicalGroup(group.Dimension, group.Tag, entities);
    gmsh::vectorpair tmpElmTypes;
    Array2D<std::size_t> tmpElmTags;
    Array2D<vtkIdType> tmpNodeTags;

    for (int entity : entities)
    {
      ::GetElements(tmpElmTypes, tmpElmTags, tmpNodeTags, this->Internal->NodeCorrespondence,
        group.Dimension, entity);
      group.ElemType.insert(group.ElemType.end(), tmpElmTypes.begin(), tmpElmTypes.end());
      group.ElemTags.insert(group.ElemTags.end(), tmpElmTags.begin(), tmpElmTags.end());
      group.ElemNodeTags.insert(group.ElemNodeTags.end(), tmpNodeTags.begin(), tmpNodeTags.end());
    }
  }
  else
  {
    ::GetElements(group.ElemType, group.ElemTags, group.ElemNodeTags,
      this->Internal->NodeCorrespondence, group.Dimension, -1);
  }

  group.CellCorrespondence.clear();
  std::size_t counter = 0;
  for (const auto& tags : group.ElemTags)
  {
    for (std::size_t tag : tags)
    {
      group.CellCorrespondence[tag] = counter;
      ++counter;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkGmshReader::FillGroupEntities(PhysicalGroup& group) const
{
  std::vector<int> tags;
  if (group.Tag != -1)
  {
    gmsh::model::getEntitiesForPhysicalGroup(group.Dimension, group.Tag, tags);
  }
  else
  {
    gmsh::vectorpair dimTags;
    gmsh::model::getEntities(dimTags, group.Dimension);
    tags.resize(dimTags.size());
    for (std::size_t i = 0; i < dimTags.size(); ++i)
    {
      tags[i] = dimTags[i].second;
    }
  }
  const std::size_t nbCells = group.CellCorrespondence.size();
  group.EntityIds = vtkSmartPointer<vtkIntArray>::New();
  group.EntityIds->SetName("gmshEntityId");
  group.EntityIds->SetNumberOfComponents(1);
  group.EntityIds->SetNumberOfTuples(nbCells);

  std::vector<int> _dummy;
  Array2D<std::size_t> elemTags, _dummyy;
  for (const auto& entityTag : tags)
  {
    gmsh::model::mesh::getElements(_dummy, elemTags, _dummyy, group.Dimension, entityTag);
    for (const auto& tagsByType : elemTags)
    {
      for (std::size_t elmTag : tagsByType)
      {
        group.EntityIds->SetValue(group.CellCorrespondence[elmTag], entityTag);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkGmshReader::FillSubDataArray(int viewTag, int viewIdx, int step)
{
  std::string dataType, name;
  std::vector<std::size_t> tags;
  std::vector<double> data;
  int nbOfComponents;
  double time;
  gmsh::view::getHomogeneousModelData(viewTag, step, dataType, tags, data, time, nbOfComponents);

  const int idxView = gmsh::view::getIndex(viewTag);
  gmsh::option::getString("View[" + std::to_string(idxView) + "].Name", name);
  if (name.empty())
  {
    name = "DataArray" + std::to_string(viewTag);
  }

  if (dataType == "NodeData")
  {
    const int nbOfTuples = tags.size();

    // Translate node tags into vtk indexes
    for (std::size_t& t : tags)
    {
      t = this->Internal->NodeCorrespondence[t];
    }

    // Translate information for each physical group
    for (PhysicalGroup& group : this->Internal->Groups)
    {
      auto vtkData = vtkSmartPointer<vtkDoubleArray>::New();
      vtkData->SetName(name.c_str());
      vtkData->SetNumberOfComponents(nbOfComponents);
      vtkData->SetNumberOfTuples(nbOfTuples);
      for (int i = 0; i < nbOfTuples; ++i)
      {
        vtkData->SetTuple(tags[i], &data[i * nbOfComponents]);
      }

      group.Data[viewIdx].Type = dataType;
      group.Data[viewIdx].Times[step] = time;
      group.Data[viewIdx].VtkArrays[step] = vtkData;
    }
  }
  else if (dataType == "ElementData")
  {
    for (PhysicalGroup& group : this->Internal->Groups)
    {
      const std::size_t nbOfCells = group.CellCorrespondence.size();

      vtkSmartPointer<vtkDoubleArray> vtkData = vtkSmartPointer<vtkDoubleArray>::New();
      vtkData->SetName(name.c_str());
      vtkData->SetNumberOfComponents(nbOfComponents);
      vtkData->SetNumberOfTuples(nbOfCells);

      // Set actual values for vtkData
      for (std::size_t tagIdx = 0; tagIdx < tags.size(); ++tagIdx)
      {
        const std::size_t& cellTag = tags[tagIdx];
        const auto findTagIt = group.CellCorrespondence.find(cellTag);

        if (findTagIt != group.CellCorrespondence.end())
        {
          const std::size_t newTag = group.CellCorrespondence[cellTag];
          vtkData->SetTuple(newTag, &data[tagIdx * nbOfComponents]);
        }
      }

      group.Data[viewIdx].Type = dataType;
      group.Data[viewIdx].Times[step] = time;
      group.Data[viewIdx].VtkArrays[step] = vtkData;
    }
  }
  else
  {
    vtkWarningMacro(
      "Array " << viewTag << " is not supported by Paraview (bad Data Type" << dataType << ").");
  }
}

//-----------------------------------------------------------------------------
void vtkGmshReader::LoadPhysicalGroupsData()
{
  std::vector<int> tags;
  gmsh::view::getTags(tags);
  const int nbViews = tags.size();
  for (PhysicalGroup& group : this->Internal->Groups)
  {
    group.Data.resize(nbViews);
  }

  for (int i = 0; i < nbViews; ++i)
  {
    const int viewTag = tags[i];
    double nbTimeStepsDbl;

    std::string viewStr = "View[" + std::to_string(gmsh::view::getIndex(viewTag));
    std::string timeStepStr = viewStr + "].TimeStep";
    gmsh::option::getNumber(viewStr + "].NbTimeStep", nbTimeStepsDbl);
    const int nbTimeSteps = static_cast<int>(nbTimeStepsDbl);

    for (PhysicalGroup& group : this->Internal->Groups)
    {
      group.Data[i].Times.resize(nbTimeSteps);
      group.Data[i].VtkArrays.resize(nbTimeSteps);
    }

    for (int step = 0; step < nbTimeSteps; ++step)
    {
      gmsh::option::setNumber(timeStepStr, step);
      this->FillSubDataArray(viewTag, i, step);
    }
  }
}

//-----------------------------------------------------------------------------
int vtkGmshReader::FetchData()
{
  if (!this->Internal->IsLoaded)
  {
    gmsh::initialize();
    gmsh::option::setNumber("General.Verbosity", 1);
    gmsh::open(this->FileName);

    if (!this->LoadNodes())
    {
      return 0;
    }

    this->LoadPhysicalGroups();
    this->LoadPhysicalGroupsData();

    // Get every time values in a flat array
    this->Internal->Timesteps.clear();
    for (const PhysicalGroup& group : this->Internal->Groups)
    {
      for (const DataArray& data : group.Data)
      {
        this->Internal->Timesteps.insert(data.Times.begin(), data.Times.end());
      }
    }
    if (this->Internal->Timesteps.empty())
    {
      this->Internal->Timesteps = { 0.0 };
    }

    this->Internal->IsLoaded = true;
    gmsh::clear();
    gmsh::finalize();
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkGmshReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified.");
    return 0;
  }

  if (this->Internal->IsLoaded)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    this->FillOutputTimeInformation(outInfo);
  }

  return 1;
}

//-----------------------------------------------------------------------------
double vtkGmshReader::GetActualTime(vtkInformation* outInfo) const
{
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    const double uTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    const auto& dTimeIt = this->Internal->Timesteps.lower_bound(uTime);

    return (dTimeIt != this->Internal->Timesteps.end()) ? *(dTimeIt)
                                                        : *(this->Internal->Timesteps.rbegin());
  }
  else
  {
    return *(this->Internal->Timesteps.begin());
  }
}

//-----------------------------------------------------------------------------
void vtkGmshReader::FillGrid(vtkUnstructuredGrid* grid, int groupIdx, double time) const
{
  grid->SetPoints(this->Internal->NodesCoords);

  // Preallocate the destination grid for cells
  const PhysicalGroup& group = this->Internal->Groups[groupIdx];
  const std::size_t nbCells = group.CellCorrespondence.size();
  grid->AllocateEstimate(nbCells, 8);

  vtkNew<vtkIntArray> cellIds;
  cellIds->SetName("gmshCellID");
  cellIds->SetNumberOfComponents(1);
  cellIds->SetNumberOfTuples(nbCells);

  for (std::size_t elmIdx = 0, counter = 0; elmIdx < group.ElemType.size(); ++elmIdx)
  {
    const int vtkType = group.ElemType[elmIdx].first;
    const int nbOfComponents = group.ElemType[elmIdx].second;
    for (std::size_t i = 0; i < group.ElemTags[elmIdx].size(); ++i)
    {
      grid->InsertNextCell(
        vtkType, nbOfComponents, &group.ElemNodeTags[elmIdx][i * nbOfComponents]);
      cellIds->SetValue(counter, group.ElemTags[elmIdx][i]);
      ++counter;
    }
  }

  if (this->CreateGmshNodeIDArray)
  {
    grid->GetPointData()->AddArray(this->Internal->NodeIds);
  }
  if (this->CreateGmshCellIDArray)
  {
    grid->GetCellData()->AddArray(cellIds);
  }
  if (this->CreateGmshEntityIDArray)
  {
    grid->GetCellData()->AddArray(group.EntityIds);
  }

  for (const DataArray& data : group.Data)
  {
    int arrayIndex = -1;
    if (time < 0 && !data.VtkArrays.empty())
    {
      arrayIndex = 0;
    }
    else
    {
      for (std::size_t i = 0; i < data.Times.size(); ++i)
      {
        if (data.Times[i] == time)
        {
          arrayIndex = i;
          break;
        }
      }
    }

    if (arrayIndex < 0)
    {
      continue;
    }
    else if (data.Type == "NodeData")
    {
      grid->GetPointData()->AddArray(data.VtkArrays[arrayIndex]);
    }
    else if (data.Type == "ElementData")
    {
      grid->GetCellData()->AddArray(data.VtkArrays[arrayIndex]);
    }
  }

  grid->Squeeze();
}

//-----------------------------------------------------------------------------
void vtkGmshReader::FillOutputTimeInformation(vtkInformation* outInfo) const
{
  const std::set<double>& timesteps = this->Internal->Timesteps;
  if (!timesteps.empty())
  {
    std::vector<double> inlineTimes(timesteps.size());
    double timeRange[2] = { inlineTimes.front(), inlineTimes.back() };

    std::copy(timesteps.begin(), timesteps.end(), inlineTimes.begin());
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &inlineTimes[0],
      static_cast<int>(inlineTimes.size()));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
}

//-----------------------------------------------------------------------------
int vtkGmshReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->FetchData())
  {
    return 0;
  }

  this->FillOutputTimeInformation(outInfo);
  const double actualTime = this->GetActualTime(outInfo);
  const int nbOfPhysGroups = this->Internal->Groups.size();
  vtkMultiBlockDataSet* root =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->GroupByDimension)
  {
    std::array<std::vector<std::size_t>, GmshReaderInternal::NUM_DIM> groups;
    for (std::size_t i = 0; i < this->Internal->Groups.size(); ++i)
    {
      const int dim = this->Internal->Groups[i].Dimension;
      if (dim > 0 && dim < GmshReaderInternal::NUM_DIM)
      {
        groups[dim].emplace_back(i);
      }
    }

    std::array<vtkNew<vtkMultiBlockDataSet>, GmshReaderInternal::NUM_DIM> dimBlocks;
    root->SetNumberOfBlocks(GmshReaderInternal::NUM_DIM);
    for (int dim = 0; dim < GmshReaderInternal::NUM_DIM; ++dim)
    {
      root->SetBlock(dim, dimBlocks[dim]);
      root->GetMetaData(dim)->Set(vtkCompositeDataSet::NAME(), GmshReaderInternal::DIM_LABEL[dim]);

      dimBlocks[dim]->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), actualTime);
      dimBlocks[dim]->SetNumberOfBlocks(groups[dim].size());
      int localLeafIndex = 0;
      for (const std::size_t& grpIdx : groups[dim])
      {
        vtkNew<vtkUnstructuredGrid> leaf;
        this->FillGrid(leaf, grpIdx, actualTime);

        if (this->GetCreateGmshDimensionArray())
        {
          vtkNew<vtkIntArray> dimInfoArray;
          dimInfoArray->SetName("gmshDimension");
          dimInfoArray->SetNumberOfComponents(1);
          dimInfoArray->SetNumberOfTuples(1);
          dimInfoArray->SetValue(0, dim);
          leaf->GetFieldData()->AddArray(dimInfoArray);
        }

        dimBlocks[dim]->SetBlock(localLeafIndex, leaf);
        leaf->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), actualTime);
        dimBlocks[dim]
          ->GetMetaData(localLeafIndex)
          ->Set(vtkCompositeDataSet::NAME(), this->Internal->Groups[grpIdx].Name);
        localLeafIndex++;
      }
    }
  }
  else
  {
    root->SetNumberOfBlocks(nbOfPhysGroups);
    root->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), actualTime);
    for (int grpIdx = 0; grpIdx < nbOfPhysGroups; ++grpIdx)
    {
      vtkNew<vtkUnstructuredGrid> leaf;
      this->FillGrid(leaf, grpIdx, actualTime);

      if (this->GetCreateGmshDimensionArray())
      {
        vtkNew<vtkIntArray> dimInfoArray;
        dimInfoArray->SetName("gmshDimension");
        dimInfoArray->SetNumberOfComponents(1);
        dimInfoArray->SetNumberOfTuples(1);
        dimInfoArray->SetValue(0, this->Internal->Groups[grpIdx].Dimension);
        leaf->GetFieldData()->AddArray(dimInfoArray);
      }

      root->SetBlock(grpIdx, leaf);
      leaf->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), actualTime);
      root->GetMetaData(grpIdx)->Set(
        vtkCompositeDataSet::NAME(), this->Internal->Groups[grpIdx].Name);
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkGmshReader::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Filename:" << this->FileName << std::endl;
}
