// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-FileCopyrightText: Copyright (c) 2022 Verein zur Foerderung der Software openCFS
// SPDX-License-Identifier: BSD-3-Clause
#include <algorithm>
#include <cassert>
#include <iostream>

#include "hdf5Reader.h" // same directory
#include <vtkLogger.h>
#include <vtksys/SystemTools.hxx>

namespace H5CFS
{

//-----------------------------------------------------------------------------
Hdf5Reader::~Hdf5Reader()
{
  this->CloseFile();
}

//-----------------------------------------------------------------------------
void Hdf5Reader::CloseFile()
{
  if (this->MainFile != -1)
  {
    H5Gclose(this->MeshRoot);
    this->MeshRoot = -1;

    H5Gclose(this->MainRoot);
    this->MainRoot = -1;

    H5Fclose(this->MainFile);
    this->MainFile = -1;

    H5Pclose(this->FileProperties);
    this->FileProperties = -1;
  }
}

//-----------------------------------------------------------------------------
void Hdf5Reader::LoadFile(const std::string& fileName)
{
  this->CloseFile();

  this->FileName = vtksys::SystemTools::CollapseFullPath(fileName); // normalize
  this->BaseDir =
    vtksys::SystemTools::GetParentDirectory(this->FileName); // used to load external results

  this->FileProperties = H5Pcreate(H5P_FILE_ACCESS);
  if (this->FileProperties < 0)
  {
    throw std::runtime_error(std::string("cannot properly access ") + this->FileName);
  }

  // open file and store main group and mesh group
  this->MainFile = H5Fopen(this->FileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (this->MainFile < 0)
  {
    vtkLog(INFO, "Hdf5Reader::LoadFile: cannot load " + this->FileName);
    throw std::runtime_error(std::string("cannot open file ") + this->FileName);
  }
  vtkLog(INFO, "Hdf5Reader::LoadFile: successfully opened " + this->FileName);

  this->MainRoot = H5CFS::OpenGroup(this->MainFile, "/");
  this->MeshRoot = H5CFS::OpenGroup(this->MainRoot, "Mesh");

  // check if we have results or if this is pure mesh file (e.g. generated via cfs -g)
  // to check this, we may not read "Results/Mesh" but need to check layers manually
  bool PureGeometry = !H5CFS::TestGroupChild(this->MainRoot, "Results", "Mesh");
  vtkLog(INFO, "Hdf5Reader::LoadFile: PureGeometry=" + std::to_string(PureGeometry));

  // check for use of external files (uint to bool)
  this->HasExternalFiles = PureGeometry
    ? false
    : H5CFS::ReadAttribute<unsigned int>(this->MainRoot, "Results/Mesh", "ExternalFiles") != 0;

  // read general mesh information
  this->ReadMeshStatusInformations();
}

//-----------------------------------------------------------------------------
unsigned int Hdf5Reader::GetDimension() const
{
  return H5CFS::ReadAttribute<unsigned int>(this->MeshRoot, "Dimension");
}

//-----------------------------------------------------------------------------
unsigned int Hdf5Reader::GetGridOrder() const
{
  auto quadelems = H5CFS::ReadAttribute<unsigned int>(this->MeshRoot, "Elements", "QuadraticElems");
  return quadelems == 1 ? 2 : 1;
}

//-----------------------------------------------------------------------------
void Hdf5Reader::GetNodeCoordinates(std::vector<std::vector<double>>& coords) const
{
  hid_t nodeGroup = H5CFS::OpenGroup(this->MeshRoot, "Nodes");

  // read nodal coordinates
  std::vector<double> coordVec;
  H5CFS::ReadArray(nodeGroup, "Coordinates", coordVec);

  assert(coordVec.size() / 3 == H5CFS::GetArrayDims(nodeGroup, "Coordinates")[0]);
  assert(coordVec.size() % 3 == 0); // division by 3 has no rest

  const unsigned int numNodes = static_cast<unsigned int>(coordVec.size() / 3);
  coords.resize(numNodes);
  unsigned int index = 0;
  for (auto& coord : coords)
  {
    coord.resize(3);
    coord[0] = coordVec[index];
    coord[1] = coordVec[index + 1];
    coord[2] = coordVec[index + 2];
    index += 3;
  }

  H5Gclose(nodeGroup);
}

//-----------------------------------------------------------------------------
void Hdf5Reader::GetElements(
  std::vector<H5CFS::ElemType>& types, std::vector<std::vector<unsigned int>>& connect) const
{
  std::vector<unsigned int> dim = H5CFS::GetArrayDims(this->MeshRoot, "Elements/Connectivity");
  unsigned int numElems = dim[0];        // number of elements
  unsigned int numNodesPerElem = dim[1]; // maximum number of nodes per elements

  // read element types
  std::vector<int> elemTypes;
  H5CFS::ReadArray(this->MeshRoot, "Elements/Types", elemTypes);

  // read nodes per element
  std::vector<unsigned int> globConnect;
  H5CFS::ReadArray(this->MeshRoot, "Elements/Connectivity", globConnect);

  // add element definition
  types.resize(numElems);
  connect.resize(numElems);
  auto it1 = globConnect.begin();
  auto it2 = it1;
  for (unsigned int i = 0; i < numElems; i++)
  {
    it2 = it1 + NUM_ELEM_NODES[elemTypes[i]];
    connect[i] = std::vector<unsigned int>(it1, it2);
    types[i] = static_cast<H5CFS::ElemType>(elemTypes[i]);
    it1 += numNodesPerElem;
  }
}

//-----------------------------------------------------------------------------
const std::vector<unsigned int>& Hdf5Reader::GetElementsOfRegion(const std::string& regionName)
{
  if (std::find(this->RegionNames.begin(), this->RegionNames.end(), regionName) ==
    this->RegionNames.end())
  {
    throw std::runtime_error(std::string("no elements present for region ") + regionName);
  }

  return this->RegionElems[regionName];
}

//-----------------------------------------------------------------------------
const std::vector<unsigned int>& Hdf5Reader::GetNodesOfRegion(const std::string& regionName)
{
  if (std::find(this->RegionNames.begin(), this->RegionNames.end(), regionName) ==
    this->RegionNames.end())
  {
    throw std::runtime_error(std::string("no nodes present for region ") + regionName);
  }

  return this->RegionNodes[regionName];
}

//-----------------------------------------------------------------------------
const std::vector<unsigned int>& Hdf5Reader::GetNamedNodes(const std::string& name)
{
  if (std::find(this->NodeNames.begin(), this->NodeNames.end(), name) == this->NodeNames.end() &&
    std::find(this->ElementNames.begin(), this->ElementNames.end(), name) ==
      this->ElementNames.end())
  {
    throw std::runtime_error(std::string("no nodes present for named entity ") + name);
  }

  return this->EntityNodes[name];
}

//-----------------------------------------------------------------------------
const std::vector<unsigned int>& Hdf5Reader::GetNamedElements(const std::string& name)
{
  if (std::find(this->ElementNames.begin(), this->ElementNames.end(), name) ==
    this->ElementNames.end())
  {
    throw std::runtime_error(std::string("no elements present for named entity ") + name);
  }

  return this->EntityElems[name];
}

//-----------------------------------------------------------------------------
const std::vector<unsigned int>& Hdf5Reader::GetEntities(EntityType type, const std::string& name)
{
  bool isRegion = (std::find(this->RegionNames.begin(), this->RegionNames.end(), name) !=
    this->RegionNames.end());

  if (type == H5CFS::NODE)
  {
    return isRegion ? this->GetNodesOfRegion(name) : this->GetNamedNodes(name);
  }

  if (type == H5CFS::ELEMENT)
  {
    return isRegion ? this->GetElementsOfRegion(name) : this->GetNamedElements(name);
  }

  assert(type == H5CFS::SURF_ELEM);
  return this->GetNamedElements(name);
}

//-----------------------------------------------------------------------------
void Hdf5Reader::GetNumberOfMultiSequenceSteps(
  std::map<unsigned int, H5CFS::AnalysisType>& analysis,
  std::map<unsigned int, unsigned int>& numSteps, bool isHistory) const
{
  analysis.clear();
  numSteps.clear();

  // search for all MultiStep_n with n >= 1 and very often 1 in either
  // - /Results/History (history results are (scalar) region results
  // - /Results/Mesh for the common nodes and elements results (almost always present)

  // hdf5 indicates with OpenGroup() if a group does not exist, but then also prints output
  // this output makes ctest to trigger an error. Therefore we check before.
  std::string test = std::string(isHistory ? "History" : "Mesh");
  if (!H5CFS::TestGroupChild(this->MainRoot, "/Results", test))
  {
    // group does not exist, hence don't add anything.
    return;
  }

  hid_t baseGroup = H5CFS::OpenGroup(this->MainRoot, "/Results/" + test);

  // next count the MultiStep_n
  H5G_info_t info = H5CFS::GetInfo(baseGroup);
  std::set<unsigned int> msStepNumbers;
  for (hsize_t i = 0; i < info.nlinks; i++)
  {
    // get name
    std::string actName = H5CFS::GetObjNameByIdx(baseGroup, i);

    // cut away "MultiStep_"-substring and convert  into int
    // we don't want to use boost::erase_all and std has no nice replacement
    char* no_str = vtksys::SystemTools::RemoveChars(actName.c_str(), "MultiStep_");
    std::string numOnly(no_str);
    msStepNumbers.insert(std::stoi(numOnly));
    delete[] no_str;

    // try to find all single multisequence steps and related analysis strings
    for (auto it : msStepNumbers)
    {
      hid_t actMsGroup = H5CFS::GetMultiStepGroup(this->MainRoot, it, isHistory);

      // get analysis string
      std::string actAnalysisString = H5CFS::ReadAttribute<std::string>(actMsGroup, "AnalysisType");
      auto actMsNumSteps = H5CFS::ReadAttribute<unsigned int>(actMsGroup, "LastStepNum");
      H5CFS::AnalysisType actAnalysis = H5CFS::NO_ANALYSIS_TYPE;
      if (actAnalysisString == "static")
      {
        actAnalysis = H5CFS::STATIC;
      }
      else if (actAnalysisString == "transient")
      {
        actAnalysis = H5CFS::TRANSIENT;
      }
      else if (actAnalysisString == "harmonic" || actAnalysisString == "multiharmonic")
      {
        actAnalysis = H5CFS::HARMONIC;
      }
      else if (actAnalysisString == "eigenFrequency")
      {
        actAnalysis = H5CFS::EIGENFREQUENCY;
      }
      else if (actAnalysisString == "buckling")
      {
        actAnalysis = H5CFS::BUCKLING;
      }
      else if (actAnalysisString == "eigenValue")
      {
        actAnalysis = H5CFS::EIGENVALUE;
      }
      else
      {
        throw std::runtime_error("Unknown analysis type found in hdf file: " + actAnalysisString);
      }
      analysis[it] = actAnalysis;
      numSteps[it] = actMsNumSteps;

      H5Gclose(actMsGroup);
    }
  }
  H5Gclose(baseGroup);
}

//-----------------------------------------------------------------------------
void Hdf5Reader::GetStepValues(unsigned int sequenceStep, const std::string& infoName,
  std::map<unsigned int, double>& steps, bool isHistory) const
{
  // open corresponding multistep group
  hid_t msGroup = H5CFS::GetMultiStepGroup(this->MainRoot, sequenceStep, isHistory);

  // open result description
  std::string key = "ResultDescription/" + std::string(infoName);
  hid_t resGroup = H5CFS::OpenGroup(msGroup, key);

  // read stepValues and stepNumbers
  std::vector<double> values;
  std::vector<unsigned int> numbers;
  H5CFS::ReadArray(resGroup, "StepNumbers", numbers);
  H5CFS::ReadArray(resGroup, "StepValues", values);

  // sanity check: both vectors need to have the same dimension
  if (values.size() != numbers.size())
  {
    throw std::runtime_error("There are not as many stepnumbers as stepvalues");
  }

  // copy to steps-array
  // make it robust to handle old optimization files which are corrupt
  // stop reading if the value is not larger than the last
  steps.clear();
  for (size_t i = 0; i < numbers.size(); ++i)
  {
    steps[numbers[i]] = values[i];
  }

  H5Gclose(resGroup);
  H5Gclose(msGroup);
}

//-----------------------------------------------------------------------------
void Hdf5Reader::GetResultTypes(unsigned int sequenceStep,
  std::vector<std::shared_ptr<H5CFS::ResultInfo>>& infos, bool isHistory) const
{
  // open ms group and 'Result Description' subgroup
  hid_t msGroup = H5CFS::GetMultiStepGroup(this->MainRoot, sequenceStep, isHistory);
  hid_t resInfoGroup = H5CFS::OpenGroup(msGroup, "ResultDescription");
  H5G_info_t gi = H5CFS::GetInfo(resInfoGroup);

  infos.clear();
  for (hsize_t i = 0; i < gi.nlinks; i++)
  {
    // iterate over all entries and assemble the result info object

    // create new H5CFS::ResultInfo objects
    std::shared_ptr<H5CFS::ResultInfo> pi(new H5CFS::ResultInfo());
    pi->name = H5CFS::GetObjNameByIdx(resInfoGroup, i);
    hid_t resGroup = H5CFS::OpenGroup(resInfoGroup, pi->name);

    pi->unit = H5CFS::ReadDataSet<std::string>(resGroup, "Unit");
    pi->entryType = static_cast<EntryType>(H5CFS::ReadDataSet<unsigned int>(resGroup, "EntryType"));
    pi->listType = static_cast<EntityType>(H5CFS::ReadDataSet<unsigned int>(resGroup, "DefinedOn"));
    pi->isHistory = isHistory;
    H5CFS::ReadArray(resGroup, "DOFNames", pi->dofNames);

    // perform consistency check
    if (pi->name.empty())
    {
      throw std::runtime_error("Result has no proper name");
    }

    if (pi->entryType == H5CFS::UNKNOWN)
    {
      throw std::runtime_error("Result '" + pi->name + "' has no proper EntryType");
    }

    if (pi->dofNames.empty())
    {
      throw std::runtime_error("Result '" + pi->name + "' has no degrees of freedoms");
    }

    // now, run over all regions and create for each region a new result info
    std::vector<std::string> entities;
    H5CFS::ReadArray(resGroup, "EntityNames", entities);

    for (const std::string& region : entities)
    {
      std::shared_ptr<H5CFS::ResultInfo> actInfo(new H5CFS::ResultInfo(*pi));
      actInfo->listName = region;
      infos.push_back(actInfo);
    }
    H5Gclose(resGroup);
  }
  H5Gclose(resInfoGroup);
  H5Gclose(msGroup);
}

//-----------------------------------------------------------------------------
void Hdf5Reader::GetMeshResult(unsigned int sequenceStep, unsigned int stepNum, Result* result)
{
  // open step group, open specific result subgroup (or external file)
  hid_t stepGroup = H5CFS::GetStepGroup(this->MainRoot, sequenceStep, stepNum);

  // check, if results are stored at external file location
  hid_t extFile = 0;
  if (this->HasExternalFiles)
  {
    std::string extFileString = H5CFS::ReadAttribute<std::string>(stepGroup, "ExtHDF5FileName");
    // construct the path with BaseDir. Convert makes sure this is also nice for Windows
    std::string extFileNameComplete =
      vtksys::SystemTools::ConvertToOutputPath(BaseDir + "/" + extFileString);

    extFile = H5Fopen(extFileNameComplete.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    if (extFile < 0)
    {
      throw std::runtime_error(std::string("cannot open external file ") + extFileNameComplete);
    }

    // replace old step group by new one
    H5Gclose(stepGroup);
    stepGroup = H5CFS::OpenGroup(extFile, "/");
  }

  // determine region for this results
  std::string groupName = result->resultInfo->name + "/" + result->resultInfo->listName + "/";

  switch (result->resultInfo->listType)
  {
    case H5CFS::NODE:
      groupName += "Nodes";
      break;
    case H5CFS::ELEMENT:
    case H5CFS::SURF_ELEM:
      groupName += "Elements";
      break;
    default:
      throw std::runtime_error(
        "unknown mesh result type " + std::to_string(result->resultInfo->listType));
  }

  hid_t resGroup = H5CFS::OpenGroup(stepGroup, groupName);

  // read data array
  std::vector<double> realVals;
  H5CFS::ReadArray(resGroup, "Real", realVals);

  std::vector<unsigned int> idx;
  const unsigned int numDofs = static_cast<unsigned int>(result->resultInfo->dofNames.size());
  std::vector<unsigned int> entities =
    this->GetEntities(result->resultInfo->listType, result->resultInfo->listName);
  const unsigned int numEntities = static_cast<unsigned int>(entities.size());
  const unsigned int resVecSize = static_cast<unsigned int>(numEntities * numDofs);

  // copy data array to result object
  // REAL part
  std::vector<double>& resRealVec = result->realVals;
  resRealVec.resize(resVecSize);
  for (unsigned int i = 0; i < numEntities; i++)
  {
    for (unsigned int iDof = 0; iDof < numDofs; iDof++)
    {
      resRealVec[i * numDofs + iDof] = realVals[i * numDofs + iDof];
    }
  }

  // check if also imaginaryinary values are present
  H5G_info_t info = H5CFS::GetInfo(resGroup);
  if (info.nlinks > 1)
  {
    result->isComplex = true;
    std::vector<double> imaginaryVals;
    H5CFS::ReadArray(resGroup, "Imag", imaginaryVals);
    std::vector<double>& resImagVec = result->imaginaryVals;
    resImagVec.resize(resVecSize);
    for (unsigned int i = 0; i < numEntities; i++)
    {
      for (unsigned int iDof = 0; iDof < numDofs; iDof++)
      {
        resImagVec[i * numDofs + iDof] = imaginaryVals[i * numDofs + iDof];
      }
    }
  }
  else
  {
    result->isComplex = false;
  }

  H5Gclose(resGroup);
  H5Gclose(stepGroup);
  if (this->HasExternalFiles)
  {
    H5Fclose(extFile);
  }
}

//-----------------------------------------------------------------------------
void Hdf5Reader::GetHistoryResult(
  unsigned int sequenceStep, const std::string& entityId, Result* result) const
{
  // open multisequence group
  hid_t msGroup =
    H5CFS::GetMultiStepGroup(this->MainRoot, sequenceStep, true); // true for isHistory

  // open group for specific result
  hid_t resGroup = H5CFS::OpenGroup(msGroup, result->resultInfo->name);

  // determine from definedOn type the correct string representation of the subgroup
  H5CFS::EntityType definedOn = result->resultInfo->listType;

  std::string entityTypeString = H5CFS::MapUnknownTypeAsString(definedOn);
  hid_t entityGroup = H5CFS::OpenGroup(resGroup, entityTypeString);
  hid_t entGroup = H5CFS::OpenGroup(entityGroup, entityId);

  // read single part of array and set it in the result vector
  H5CFS::ReadArray(entGroup, "Real", result->realVals);
  H5G_info_t info = H5CFS::GetInfo(entGroup);
  if (info.nlinks > 1)
  {
    result->isComplex = true;
    H5CFS::ReadArray(entGroup, "Imag", result->imaginaryVals);
  }
  else
  {
    result->isComplex = false;
  }

  H5Gclose(entGroup);
  H5Gclose(entityGroup);
  H5Gclose(resGroup);
  H5Gclose(msGroup);
}

//-----------------------------------------------------------------------------
void Hdf5Reader::ReadMeshStatusInformations()
{
  H5CFS::ReadAttribute(this->MeshRoot, "Nodes", "NumNodes", this->NumNodes);
  H5CFS::ReadAttribute(this->MeshRoot, "Elements", "NumElems", this->NumElems);

  // read region names and dimensions
  hid_t regionGroup = H5CFS::OpenGroup(this->MeshRoot, "Regions");
  H5G_info_t info = H5CFS::GetInfo(regionGroup);

  this->RegionNames.clear();
  for (hsize_t i = 0; i < info.nlinks; i++)
  {
    // get name
    std::string actName = H5CFS::GetObjNameByIdx(regionGroup, i);
    this->RegionNames.push_back(actName);

    hid_t actRegion = H5CFS::OpenGroup(regionGroup, actName);

    RegionDims[actName] = H5CFS::ReadAttribute<unsigned int>(actRegion, "Dimension");

    // read elem numbers for this region
    H5CFS::ReadArray(actRegion, "Elements", this->RegionElems[actName]);

    // read node numbers for this region
    H5CFS::ReadArray(actRegion, "Nodes", this->RegionNodes[actName]);

    H5Gclose(actRegion);
  }
  H5Gclose(regionGroup);

  // groups are named nodes/elements
  // we are a little tolerant w.r.t. groups
  hid_t groups = H5Gopen(this->MeshRoot, "Groups", H5P_DEFAULT);
  if (groups >= 0)
  {
    H5G_info_t groupsInfo = H5CFS::GetInfo(groups);

    this->NodeNames.clear();
    this->ElementNames.clear();

    for (hsize_t ig = 0; ig < groupsInfo.nlinks; ig++)
    {
      // get name of group
      std::string actName = H5CFS::GetObjNameByIdx(groups, ig);

      // open entitygroup and get number of different entity types
      // (nodes, elements) it is defined on
      hid_t actEntityGroup = H5CFS::OpenGroup(groups, actName);

      H5G_info_t actEntityInfo = H5CFS::GetInfo(actEntityGroup);

      // check all children of the act group for "Elements"
      bool hasElems = false;
      for (hsize_t it = 0; it < actEntityInfo.nlinks && !hasElems; it++)
      {
        if (H5CFS::GetObjNameByIdx(actEntityGroup, it) == "Elements")
        {
          hasElems = true;
        }
      }

      if (hasElems)
      {
        this->ElementNames.push_back(actName);

        // read nodes and elements from file and add to grid
        H5CFS::ReadArray(actEntityGroup, "Nodes", this->EntityNodes[actName]);
        H5CFS::ReadArray(actEntityGroup, "Elements", this->EntityElems[actName]);
      }
      else
      {
        this->NodeNames.push_back(actName);

        // read nodes from file and add to grid
        H5CFS::ReadArray(actEntityGroup, "Nodes", this->EntityNodes[actName]);
      }
      H5Gclose(actEntityGroup);
    } // end loop over entity groups

    H5Gclose(groups);
  }
}

} // end of namespace H5CFS
