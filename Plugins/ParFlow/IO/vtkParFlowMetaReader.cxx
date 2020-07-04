// See license.md for copyright information.
#include "vtkParFlowMetaReader.h"
#include "vtkVectorJSON.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTypes.h"
#include "vtkDoubleArray.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
// #include "vtkMultiBlockDataSet.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <sstream>

using json = nlohmann::json;

static constexpr std::streamoff headerSize = 6 * sizeof(double) + 4 * sizeof(int);
static constexpr std::streamoff subgridHeaderSize = 9 * sizeof(int);
static constexpr std::streamoff pfbEntrySize = sizeof(double);
static constexpr std::streamoff clmEntrySize =
  11 * sizeof(double); // TODO: This is just a starting point.

vtkStandardNewMacro(vtkParFlowMetaReader);

vtkParFlowMetaReader::vtkParFlowMetaReader()
  : FileName(nullptr)
  , Directory(nullptr)
  , NumberOfGhostLayers(0)
  , DuplicateNodes(1)
  , EnableSubsurfaceDomain(1)
  , EnableSurfaceDomain(1)
  , SubsurfaceVOI{ 0, -1, 0, -1, 0, -1 }
  , SurfaceAOI{ 0, -1, 0, -1 }
  , DeflectTerrain(0)
  , DeflectionScale(20.0)
  , TimeStep(0)
  , CacheTimeStep(-1)
  , SubsurfaceExtent{ 0, 0, 0, 0, 0, 0 }
  , SurfaceExtent{ 0, 0, 0, 0, 0, 0 }
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

vtkParFlowMetaReader::~vtkParFlowMetaReader()
{
  this->SetFileName(nullptr);
  this->SetDirectory(nullptr);
}

void vtkParFlowMetaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << "\n";
  os << indent << "Directory: " << (this->Directory ? this->Directory : "(null)") << "\n";
  os << indent << "SurfaceAOI: " << this->SurfaceAOI[0] << " " << this->SurfaceAOI[1] << " "
     << this->SurfaceAOI[2] << " " << this->SurfaceAOI[3] << "\n"
     << indent << "SubsurfaceVOI: " << this->SubsurfaceVOI[0] << " " << this->SubsurfaceVOI[1]
     << " " << this->SubsurfaceVOI[2] << " " << this->SubsurfaceVOI[3] << " "
     << this->SubsurfaceVOI[4] << " " << this->SubsurfaceVOI[5] << "\n";
  os << indent << "DeflectTerrain: " << this->DeflectTerrain << "\n";
  os << indent << "DeflectionScale: " << this->DeflectionScale << "\n";
  os << indent << "EnableSubsurfaceDomain: " << this->EnableSubsurfaceDomain << "\n";
  os << indent << "EnableSurfaceDomain: " << this->EnableSurfaceDomain << "\n";
  os << indent << "TimeStep: " << this->TimeStep << "\n";
  os << indent << "CacheTimeStep: " << this->CacheTimeStep << "\n";
  os << indent << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << "\n";
  os << indent << "DuplicateNodes: " << this->DuplicateNodes << "\n";
}

int vtkParFlowMetaReader::GetNumberOfSubsurfaceVariableArrays() const
{
  // std::cout << "Found " << this->SubsurfaceVariables.size() << " arrays\n";
  return static_cast<int>(this->SubsurfaceVariables.size());
}

std::string vtkParFlowMetaReader::GetSubsurfaceVariableArrayName(int idx) const
{
  if (idx < 0 || idx >= static_cast<int>(this->SubsurfaceVariables.size()))
  {
    return std::string();
  }

  // std::cout << "  Array " << idx;
  auto it = this->SubsurfaceVariables.begin();
  for (; idx > 0 && it != this->SubsurfaceVariables.end(); --idx)
  {
    ++it;
  }
  // std::cout << " " << it->first << "\n";
  return it->first;
}

int vtkParFlowMetaReader::GetSubsurfaceVariableArrayStatus(const std::string& array) const
{
  auto it = this->SubsurfaceVariables.find(array);
  if (it == this->SubsurfaceVariables.end())
  {
    return 0;
  }
  const json& entry(it->second);
  int status;
  if (entry.find("status") == entry.end())
  {
    status = 1;
  }
  else
  {
    status = entry["status"].get<int>();
  }
  return status;
}

bool vtkParFlowMetaReader::SetSubsurfaceVariableArrayStatus(const std::string& array, int status)
{
  try
  {
    auto it = this->SubsurfaceVariables.find(array);
    if (it == this->SubsurfaceVariables.end())
    {
      return false;
    }
    int currentStatus = 1;
    if (it->second.find("status") != it->second.end())
    {
      currentStatus = it->second["status"].get<int>();
    }
    if (currentStatus == status)
    {
      return false;
    }
    it->second["status"] = status;
  }
  catch (std::exception&)
  {
    return false;
  }
  this->Modified();
  return true;
}

int vtkParFlowMetaReader::GetNumberOfSurfaceVariableArrays() const
{
  // std::cout << "Found " << this->SurfaceVariables.size() << " arrays\n";
  return static_cast<int>(this->SurfaceVariables.size());
}

std::string vtkParFlowMetaReader::GetSurfaceVariableArrayName(int idx) const
{
  if (idx < 0 || idx >= static_cast<int>(this->SurfaceVariables.size()))
  {
    return std::string();
  }

  // std::cout << "  Array " << idx;
  auto it = this->SurfaceVariables.begin();
  for (; idx > 0 && it != this->SurfaceVariables.end(); --idx)
  {
    ++it;
  }
  // std::cout << " " << it->first << "\n";
  return it->first;
}

int vtkParFlowMetaReader::GetSurfaceVariableArrayStatus(const std::string& array) const
{
  auto it = this->SurfaceVariables.find(array);
  if (it == this->SurfaceVariables.end())
  {
    return 0;
  }
  const json& entry(it->second);
  int status;
  if (entry.find("status") == entry.end())
  {
    status = 1;
  }
  else
  {
    status = entry["status"].get<int>();
  }
  return status;
}

bool vtkParFlowMetaReader::SetSurfaceVariableArrayStatus(const std::string& array, int status)
{
  try
  {
    auto it = this->SurfaceVariables.find(array);
    if (it == this->SurfaceVariables.end())
    {
      return false;
    }
    int currentStatus = 1;
    if (it->second.find("status") != it->second.end())
    {
      currentStatus = it->second["status"].get<int>();
    }
    if (currentStatus == status)
    {
      return false;
    }
    it->second["status"] = status;
  }
  catch (std::exception&)
  {
    return false;
  }
  this->Modified();
  return true;
}

bool vtkParFlowMetaReader::BroadcastMetadata(std::vector<uint8_t>& metadata)
{
  // When run in parallel, we choose a range of blocks
  // to load from those available.
  int rank = 0;
  int jbsz = 1;
  int status = 1;
  auto mpc = vtkMultiProcessController::GetGlobalController();
  if (mpc)
  {
    rank = mpc->GetLocalProcessId();
    jbsz = mpc->GetNumberOfProcesses();
    std::size_t msz = metadata.size();
    status = mpc->Broadcast(&msz, 1, 0);
    if (rank > 0)
    {
      metadata.resize(msz);
    }
    status &= mpc->Broadcast(&metadata[0], metadata.size(), 0);
  }
  if (!status || metadata.empty())
  {
    if (rank == 0)
    {
      vtkErrorMacro("Could not broadcast metadata");
    }
    return false;
  }

  try
  {
    this->Metadata = json::parse(metadata);
  }
  catch (std::exception&)
  {
    if (rank == 0)
    {
      vtkErrorMacro("Could not parse JSON");
    }
    return false;
  }

  return true;
}

bool vtkParFlowMetaReader::IngestMetadata()
{
  this->TimeSteps.clear();

  try
  {
    json inputs = this->Metadata["inputs"];
    json outputs = this->Metadata["outputs"];
    json domains = this->Metadata["domains"];

    // Get the grid extents
    auto sse = domains["subsurface"]["cell-extent"].get<vtkVector3i>();
    this->SubsurfaceOrigin = domains["subsurface"]["origin"].get<vtkVector3d>();
    this->SubsurfaceSpacing = domains["subsurface"]["spacing"].get<vtkVector3d>();

    // Now, note that ParFlow assumes the origin is at the surface (k=sse[2])
    // but the k=0 layer of the grid is VTK's origin. Translate the origin as
    // required. Note the 1.025 scale factor prevents z-fighting between the
    // surface and subsurface geometry.
    this->SubsurfaceOrigin[2] -= 1.025 * this->SubsurfaceSpacing[2] * sse[2];

    auto se = domains["surface"]["cell-extent"].get<vtkVector2i>();
    this->SurfaceOrigin = domains["surface"]["origin"].get<vtkVector3d>();
    // NB: SurfaceSpacing is a 3-d vector to make life easy on GenerateDistributedMesh.
    this->SurfaceSpacing = domains["surface"]["spacing"].get<vtkVector3d>();
    for (int ii = 0; ii < 3; ++ii)
    {
      this->SubsurfaceExtent[2 * ii] = 0;
      this->SurfaceExtent[2 * ii] = 0;
      this->SubsurfaceExtent[2 * ii + 1] = sse[ii];
      this->SurfaceExtent[2 * ii + 1] = ii < 2 ? se[ii] : 0;
      this->IJKDivs[Domain::Subsurface][ii] =
        domains["subsurface"]["subgrid-divisions"][ii].get<std::vector<int> >();
      if (ii < 2)
      {
        this->IJKDivs[Domain::Surface][ii] =
          domains["surface"]["subgrid-divisions"][ii].get<std::vector<int> >();
      }
      else
      {
        this->IJKDivs[Domain::Surface][ii] = { { 0, 0 } };
      }
    }

    for (auto& entry : inputs.items())
    {
      this->IngestMetadataItem("inputs", entry.key(), entry.value());
    }
    for (auto& entry : outputs.items())
    {
      this->IngestMetadataItem("outputs", entry.key(), entry.value());
    }
    // Accept older metadata that segregated solution variables from other output.
    if (outputs.find("solution") != outputs.end())
    {
      json solution = outputs["solution"];
      for (auto& entry : solution.items())
      {
        this->IngestMetadataItem("solution", entry.key(), entry.value());
      }
    }
  }
  catch (std::exception&)
  {
    return false;
  }

  return true;
}

bool vtkParFlowMetaReader::IngestMetadataItem(
  const std::string& section, const std::string& key, const json& value)
{
  (void)section;
  std::string type;
  try
  {
    type = value.at("type").get<std::string>();
  }
  catch (std::exception&)
  {
    return false;
  }
  if (type == "pfb")
  {
    bool subsurface;
    try
    {
      subsurface = value.at("domain").get<std::string>() == "subsurface";
    }
    catch (std::exception&)
    {
      subsurface = true;
    }
    this->ReplaceVariable(
      subsurface ? this->SubsurfaceVariables : this->SurfaceVariables, key, value);
  }
  else if (type == "pfb 2d timeseries")
  {
    this->ReplaceVariable(this->SurfaceVariables, key, value);
  }

  return true;
}

void vtkParFlowMetaReader::ReplaceVariable(
  std::map<std::string, json>& vmap, const std::string& key, const json& value)
{
  int status = -1;
  auto it = vmap.find(key);
  if (it != vmap.end())
  {
    try
    {
      status = it->second.at("status").get<int>();
    }
    catch (std::exception&)
    {
    }
  }
  vmap[key] = value;
  if (status >= 0)
  {
    vmap[key]["status"] = status;
  }

  // Now add timesteps indicated by files.
  try
  {
    auto data = value.at("data");
    for (auto& entry : data)
    {
      std::vector<int> timeSpec;
      if (entry.find("times-between") != entry.end())
      {
        timeSpec = entry.at("times-between").get<std::vector<int> >();
        // Now, since times-between is inclusive while time-range is not,
        // we increment the end time.
        if (timeSpec.size() >= 2)
        {
          timeSpec[0] -= 1;
        }
        // Also, the final entry is not a delta, but the number of entries per file.
        // Overwrite with a delta of 1.
        if (timeSpec.size() > 2)
        {
          timeSpec[2] = 1;
        }
      }
      else if (entry.find("time-range") != entry.end())
      {
        timeSpec = entry.at("time-range").get<std::vector<int> >();
      }
      if (timeSpec.size() >= 2)
      {
        int delta = timeSpec.size() > 2 ? timeSpec.back() : 1;
        for (int ts = timeSpec[0]; ts < timeSpec[1]; ts += delta)
        {
          this->TimeSteps.insert(ts);
        }
      }
      else if (entry.find("times") != entry.end())
      {
        timeSpec = entry.at("times").get<std::vector<int> >();
        for (auto ts : timeSpec)
        {
          this->TimeSteps.insert(ts);
        }
      }
    }
  }
  catch (std::exception&)
  {
  }
}

int vtkParFlowMetaReader::ClosestTimeStep(double time) const
{
  if (this->TimeSteps.empty())
  {
    return 0;
  }
  int floored = static_cast<int>(floor(time));
  auto it = this->TimeSteps.lower_bound(floored);
  if (it == this->TimeSteps.end())
  {
    // The earliest file time is past the requested time.
    // Just return the earliest we have.
    return *this->TimeSteps.begin();
  }
  return *it;
}

void vtkParFlowMetaReader::GenerateDistributedMesh(
  vtkDataSet* data, int extent[6], vtkVector3d& origin, vtkVector3d& spacing)
{
  unsigned int rank = 1;
  int jbsz = 1;
  auto mpc = vtkMultiProcessController::GetGlobalController();
  if (mpc)
  {
    rank = mpc->GetLocalProcessId();
    jbsz = mpc->GetNumberOfProcesses();
  }

  // This code is horked from VTK's vtkRectilinearGridPartitioner.
  vtkNew<vtkExtentRCBPartitioner> extentPartitioner;
  extentPartitioner->SetGlobalExtent(extent);
  extentPartitioner->SetNumberOfPartitions(jbsz);
  extentPartitioner->SetNumberOfGhostLayers(this->NumberOfGhostLayers);
  if (this->DuplicateNodes)
  {
    extentPartitioner->DuplicateNodesOn();
  }
  else
  {
    extentPartitioner->DuplicateNodesOff();
  }

  // STEP 4: Partition
  extentPartitioner->Partition();

  // STEP 5: Extract partition in a multi-block
  // data->SetNumberOfBlocks(extentPartitioner->GetNumExtents());

  // Set the whole extent of the grid
  data->GetInformation()->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  int subext[6];
  extentPartitioner->GetPartitionExtent(rank, subext);
  data->GetInformation()->Set(vtkDataObject::PIECE_EXTENT(), subext, 6);

  bool didSetUpGrid = false;
  if (this->DeflectTerrain)
  {
    auto grid = vtkExplicitStructuredGrid::SafeDownCast(data);
    if (grid)
    {
      didSetUpGrid = true;
      grid->SetExtent(subext);
      vtkNew<vtkPoints> pts;
      vtkIdType numPoints = subext[5] - subext[4] + 1; // k-axis faces will share points
      // i- and j-axis faces do not share points since elevation is constant over each column:
      for (int ii = 0; ii < 2; ++ii)
      {
        int numPointsThisAxis = 2 * (subext[2 * ii + 1] - subext[2 * ii]);
        if (numPointsThisAxis > 0)
        {
          numPoints *= numPointsThisAxis;
        }
        else
        {
          numPoints *= 2;
        }
      }
      grid->SetPoints(pts);
      pts->SetNumberOfPoints(numPoints);
      // Now generate the points
      std::vector<double> dz;
      vtkDataArray* def = nullptr;
      auto it = this->SurfaceVariables.find("elevation");
      if (it != this->SurfaceVariables.end())
      {
        vtkNew<vtkImageData> dummy;
        int dummyExtent[] = { subext[0], subext[1], subext[2], subext[3], 0, 0 };
        dummy->SetExtent(dummyExtent);
        dummy->GetInformation()->Set(vtkDataObject::PIECE_EXTENT(), dummyExtent, 6);
        this->LoadVariable(dummy, it->first, it->second, /* assume elevation is constant */ 0);
        def = dummy->GetCellData()->GetArray("elevation");
        def->Register(this);
      }
      try
      {
        dz.clear();
        auto cfg = this->Metadata["inputs"]["configuration"]["data"];
        auto vdz = cfg["Solver.Nonlinear.VariableDz"];
        auto val = vdz.get<std::string>();
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        if (val == "true" || val == "on" || val == "1")
        { // Have variable dz... fetch multipliers
          double zaccum = 0.0;
          dz.push_back(zaccum);
          for (int kk = subext[4]; kk < subext[5]; ++kk)
          {
            std::ostringstream name;
            name << "Cell." << kk << ".dzScale.Value";
            std::istringstream multiplier(cfg[name.str()].get<std::string>());
            double dzv;
            multiplier >> dzv;
            zaccum += spacing[2] * dzv;
            dz.push_back(zaccum);
          }
        }
      }
      catch (std::exception&)
      {
        dz.clear();
      }
      if (dz.empty())
      {
        if (!def)
        {
          static bool once = false;
          // No variable dz, no elevation. Warn user this is inefficient.
          if (!once)
          {
            once = true;
            vtkWarningMacro("No variable dz and no elevation. "
                            "It would be more efficient to turn DeflectTerrain off.");
          }
        }
        for (int kk = subext[4]; kk <= subext[5]; ++kk)
        {
          dz.push_back(1.0 * spacing[2] * kk);
        }
      }
      vtkIdType pid = 0;
      double deflectionScale = this->DeflectionScale <= 0.0 ? 1.0 : this->DeflectionScale;
      vtkNew<vtkCellArray> conn;
      conn->AllocateExact(grid->GetNumberOfCells(), pts->GetNumberOfPoints());
      // Insert points in i-j columns
      for (int jj = subext[2]; jj < subext[3]; ++jj)
      {
        for (int ii = subext[0]; ii < subext[1]; ++ii)
        {
          double elev = def
            ? def->GetTuple1((ii == subext[1] ? ii - 1 : ii) - subext[0] +
                (subext[1] - subext[0]) * ((jj == subext[3] ? jj - 1 : jj) - subext[2]))
            : 0.0;
          for (int kk = subext[4]; kk <= subext[5]; ++kk)
          {
            double zz = deflectionScale * (dz[kk - subext[4]] + elev);
            for (int vv = 0; vv < 4; ++vv, ++pid)
            {
              vtkVector3d pt = origin +
                vtkVector3d((ii + vv % 2) * spacing[0], (jj + (vv / 2) % 2) * spacing[1], zz);
              pts->SetPoint(pid, pt.GetData());
            }
          }
        }
      }
      // Insert cells with i varying fastest to match file's array-order
      // so we don't have to reshuffle them, esp. time-varying arrays.
      vtkIdType nk = subext[5] - subext[4] + 1; // number of cells per i-j column
      for (int kk = subext[4]; kk < subext[5]; ++kk)
      {
        for (int jj = subext[2]; jj < subext[3]; ++jj)
        {
          for (int ii = subext[0]; ii < subext[1]; ++ii)
          {
            vtkIdType offset = 4 *
              (kk - subext[4] + nk * (ii - subext[0] + (subext[1] - subext[0]) * (jj - subext[2])));
            std::array<vtkIdType, 8> hexConn{ offset + 0, offset + 1, offset + 3, offset + 2,
              offset + 4, offset + 5, offset + 7, offset + 6 };
            conn->InsertNextCell(8, hexConn.data());
          }
        }
      }

      grid->SetCells(conn);
      grid->ComputeFacesConnectivityFlagsArray();
      if (def)
      {
        def->Delete();
      }
    }
  }
  if (!didSetUpGrid)
  {
    // TODO: Also handle variable dz with a vtkRectlinearGrid
    auto grid = vtkImageData::SafeDownCast(data);
    grid->SetExtent(subext);
    grid->SetOrigin(origin.GetData());
    grid->SetSpacing(spacing.GetData());
  }
}

int vtkParFlowMetaReader::FindPFBFiles(std::vector<std::string>& filesToLoad,
  const json& variableMetadata, int& timestep, int& slice) const
{
  int status = 1;
  filesToLoad.clear();
  auto type = variableMetadata.at("type").get<std::string>();
  auto tvIt = variableMetadata.find("time-varying");
  bool timeVarying = (type == "pfb 2d timeseries") ||
    ((tvIt != variableMetadata.end()) && (tvIt->get<bool>() == true));
  int currentTime = -1;
  try
  {
    json entries = variableMetadata["data"];
    for (auto fileMeta = entries.begin(); fileMeta != entries.end(); ++fileMeta)
    {
      auto compIt = fileMeta->find("component");
      if (!timeVarying)
      {
        if ((entries.size() == 1 || compIt != fileMeta->end()))
        {
          // This assumes that components are listed in the "entries" array
          // ordered by increasing component.
          auto fileIt = fileMeta->find("file");
          if (fileIt != fileMeta->end())
          {
            filesToLoad.push_back(fileIt->get<std::string>());
          }
          else
          {
            vtkErrorMacro("Missing time-constant file entry " << fileMeta->dump());
            status = 0;
          }
        }
        else
        {
          vtkErrorMacro("Time-constant file with invalid metadata entry " << fileMeta->dump());
          status = 0;
        }
      }
      else if (timeVarying)
      {
        // We keep adding files for acceptable times, then clearing the list
        // when we encounter a better time. Files must be listed in the array
        // ordered by increasing time(s). If a variable is both vector-valued
        // and time-varying, files must be listed with consecutive components
        // at time t_0, then repeated for time t_1, and so forth.
        auto timeRange = fileMeta->find("time-range");
        auto timesBetween = fileMeta->find("times-between");
        auto fileSeries = fileMeta->find("file-series");
        if (fileSeries == fileMeta->end())
        {
          vtkErrorMacro("No file-series specified.");
          status = 0;
        }
        else
        {
          std::string filePattern = fileSeries->get<std::string>();
          if (timeRange != fileMeta->end())
          {
            auto times = timeRange->get<std::vector<int> >();
            if (times.size() < 2)
            {
              vtkErrorMacro("File entry \"" << fileMeta->dump() << "\" has a bad time range.");
              status = 0;
              continue;
            }
            else if ((currentTime < times[0] && times[0] <= timestep) ||
              (times[0] <= currentTime && currentTime <= times[1]))
            {
              if (currentTime < times[0])
              {
                filesToLoad.clear();
              }
              int delta = times.size() > 2 ? times[2] : 1;
              if (delta <= 0)
              {
                delta = 1;
              }
              int frac = (timestep - times[0]) / delta;
              currentTime = times[0] + delta * frac;
              // Clamp currentTime to never exceed the upper limit of this file-series entry.
              if (currentTime > times[1])
              {
                frac = (times[1] - times[0]) / delta;
                currentTime = times[0] + delta * frac;
              }
              size_t maxLen = filePattern.size() + 20;
              char* curFile = new char[maxLen];
              std::snprintf(curFile, maxLen - 1, filePattern.c_str(), currentTime);
              curFile[maxLen - 1] = '\0';
              filesToLoad.push_back(curFile);
              delete[] curFile;
            }
            else if (times[0] > timestep)
            {
              break; // no need to loop over future file-series entries.
            }
          }
          else if (timesBetween != fileMeta->end())
          {
            int ts1 = timestep + 1;
            auto times = timesBetween->get<std::vector<int> >();
            if (times.size() != 3)
            {
              vtkErrorMacro("File entry \"" << fileMeta->dump() << "\" has a bad times-between.");
              status = 0;
              continue;
            }
            else if ((currentTime < times[0] && times[0] <= ts1) ||
              (times[0] <= currentTime && currentTime <= times[1]))
            {
              if (currentTime < times[0])
              {
                filesToLoad.clear();
              }
              currentTime = times[1] > ts1 ? ts1 : times[1];
              int timesPerStack = times[2];
              int frac = (currentTime - times[0]) / timesPerStack;
              int tbLo = times[0] + timesPerStack * frac;
              int tbHi = tbLo + timesPerStack - 1;
              size_t maxLen = filePattern.size() + 40;
              slice = currentTime - tbLo;
              char* curFile = new char[maxLen];
              std::snprintf(curFile, maxLen - 1, filePattern.c_str(), tbLo, tbHi);
              curFile[maxLen - 1] = '\0';
              filesToLoad.push_back(curFile);
              delete[] curFile;
            }
          }
          else
          {
            vtkErrorMacro("Unhandled time-varying file list.");
            status = 0;
          }
        }
      }
    }
  }
  catch (std::exception&)
  {
    status = 0;
  }

  if (currentTime >= 0)
  {
    timestep = currentTime;
  }
  return status;
}

std::streamoff vtkParFlowMetaReader::GetBlockOffset(Domain dom, int blockId, int nz) const
{
  int gridTopo[3] = { static_cast<int>(this->IJKDivs[dom][0].size() - 1),
    static_cast<int>(this->IJKDivs[dom][1].size() - 1),
    static_cast<int>(this->IJKDivs[dom][2].size() - 1) };
  vtkVector3i blockIJK(blockId % gridTopo[0], (blockId / gridTopo[0]) % gridTopo[1],
    blockId / gridTopo[0] / gridTopo[1]);

  return this->GetBlockOffset(dom, blockIJK, nz);
}

std::streamoff vtkParFlowMetaReader::GetBlockOffset(
  Domain dom, const vtkVector3i& blockIJK, int nz) const
{
  (void)clmEntrySize;
  std::streamoff offset;
  std::streamoff entrySize = pfbEntrySize;
  int ni = static_cast<int>(this->IJKDivs[dom][0].size()) - 1;
  int nj = static_cast<int>(this->IJKDivs[dom][1].size()) - 1;
  int blockId = blockIJK[0] + ni * (blockIJK[1] + nj * blockIJK[2]);

  // Account for file and subgrid headers:
  offset = headerSize + subgridHeaderSize * blockId;

  // Account for points in all whole layers of subgrids "beneath" this location:
  offset += entrySize * this->IJKDivs[dom][2][blockIJK[2]] * this->IJKDivs[dom][1].back() *
    this->IJKDivs[dom][0].back();

  // Account for points in all whole i-rows of subgrids "beneath" this location:
  int dk = this->IJKDivs[dom][2][blockIJK[2] + 1] - this->IJKDivs[dom][2][blockIJK[2]];
  if (dk == 0)
  {
    dk = nz <= 0 ? 1 : nz;
  }
  offset += entrySize * dk * this->IJKDivs[dom][1][blockIJK[1]] * this->IJKDivs[dom][0].back();

  // Account for points in the current i-row of subgrids "beneath" this location:
  int dj = this->IJKDivs[dom][1][blockIJK[1] + 1] - this->IJKDivs[dom][1][blockIJK[1]];
  offset += entrySize * dk * dj * this->IJKDivs[dom][0][blockIJK[0]];

  return offset;
}

std::streamoff vtkParFlowMetaReader::GetEndOffset(Domain dom) const
{
  std::streamoff offset;
  std::streamoff entrySize = pfbEntrySize;
  int ni = static_cast<int>(this->IJKDivs[dom][0].size()) - 1;
  int nj = static_cast<int>(this->IJKDivs[dom][1].size()) - 1;
  int nk = static_cast<int>(this->IJKDivs[dom][2].size()) - 1;
  int numBlocks = ni * nj * nk;

  // Account for file and subgrid headers:
  offset = headerSize + subgridHeaderSize * numBlocks;

  // Account for all entries in all blocks
  offset += entrySize * this->IJKDivs[dom][0].back() * this->IJKDivs[dom][1].back() *
    this->IJKDivs[dom][2].back();

  return offset;
}

bool vtkParFlowMetaReader::ReadSubgridHeader(
  istream& pfb, vtkVector3i& si, vtkVector3i& sn, vtkVector3i& sr)
{
  pfb.read(reinterpret_cast<char*>(si.GetData()), 3 * sizeof(int));
  pfb.read(reinterpret_cast<char*>(sn.GetData()), 3 * sizeof(int));
  pfb.read(reinterpret_cast<char*>(sr.GetData()), 3 * sizeof(int));

  // Swap bytes as required knowing that we started with big-endian data:
  vtkByteSwap::SwapBERange(si.GetData(), 3);
  vtkByteSwap::SwapBERange(sn.GetData(), 3);
  vtkByteSwap::SwapBERange(sr.GetData(), 3);

  return pfb.good() && !pfb.eof();
}

bool vtkParFlowMetaReader::ReadComponentSubgridOverlap(istream& pfb, const vtkVector3i& si,
  const vtkVector3i& sn, const int extent[6], int component, vtkDoubleArray* variable)
{
  vtkIdType subgridSize = sn[0] * sn[1];
  if (sn[2] > 0)
  {
    subgridSize *= sn[2];
  }
  if (variable->GetNumberOfComponents() == 1 && extent[0] == si[0] && extent[2] == si[1] &&
    extent[4] == si[2] && extent[1] == si[0] + sn[0] && extent[3] == si[1] + sn[1] &&
    extent[5] == si[2] + sn[2])
  {
    // Fast path the case where we can read directly into the array.
    pfb.read(reinterpret_cast<char*>(variable->GetVoidPointer(0)), sizeof(double) * subgridSize);
    vtkByteSwap::SwapBERange(reinterpret_cast<double*>(variable->GetVoidPointer(0)), subgridSize);
    return true;
  }

  // Read in the entire subgrid so threads can access what's needed
  // without creating new ifstream objects:
  double* dest = variable->GetPointer(0);
  std::vector<double> buffer;
  buffer.resize(subgridSize);
  pfb.read(reinterpret_cast<char*>(&buffer[0]), sizeof(double) * subgridSize);

  vtkVector3i oLo;
  vtkVector3i oHi;
  for (int ii = 0; ii < 3; ++ii)
  {
    oLo[ii] = extent[2 * ii] < si[ii] ? si[ii] : extent[2 * ii];
    int mx = si[ii] + sn[ii];
    oHi[ii] = extent[2 * ii + 1] > mx ? mx : extent[2 * ii + 1];
  }
  if (oLo[2] == oHi[2])
  {
    ++oHi[2];
  }
  int lineSize = oHi[0] - oLo[0];

  // This is a functor used by vtkSMPTools below to
  // thread the reorganization of data from its on-disk
  // structure into an in-memory partitioning.
  struct FileBufferToDataArray
  {
    FileBufferToDataArray(int irange, int j0, int j1, int st, const double* buf, double* dst,
      const int* ext, const vtkVector3i& wantLow, const vtkVector3i& szi, const vtkVector3i& szn)
      : m_id(irange)
      , m_ji(j0)
      , m_jf(j1)
      , m_nc(st)
      , m_buffer(buf)
      , m_darray(dst)
      , m_ext(ext)
      , m_lo(wantLow)
      , m_si(szi)
      , m_sn(szn)
    {
    }

    void operator()(vtkIdType klo, vtkIdType khi)
    {
      for (int kk = klo; kk < khi; ++kk)
      {
        for (int jj = m_ji; jj < m_jf; ++jj)
        {
          std::streamoff fileOffset =
            m_lo[0] - m_si[0] + m_sn[0] * (jj - m_si[1] + m_sn[1] * (kk - m_si[2]));
          vtkIdType arrayOffset = m_lo[0] - m_ext[0] +
            (m_ext[1] - m_ext[0]) * (jj - m_ext[2] + (m_ext[3] - m_ext[2]) * (kk - m_ext[4]));
          if (m_nc == 1)
          {
            std::copy(m_buffer + fileOffset, m_buffer + fileOffset + m_id, m_darray + arrayOffset);
            vtkByteSwap::SwapBERange(m_darray + arrayOffset, m_id);
          }
          else
          {
            arrayOffset *= m_nc;
            const double* from = m_buffer + fileOffset;
            double* to = m_darray + arrayOffset;
            for (int ii = 0; ii < m_id; ++ii, ++from, to += m_nc)
            {
              *to = *from;
              vtkByteSwap::SwapBERange(to, 1);
            }
          }
        }
      }
    }

    int m_id;                // span of i-axis indices to copy
    int m_ji;                // initial j-axis index from which to copy
    int m_jf;                // final j-axis index from which to copy
    int m_nc;                // number of interleaved components in the destination
    const double* m_buffer;  // raw values read from storage (may need byte-swap)
    double* m_darray;        // pointer to destination vtkDoubleArray
    const int* m_ext;        // subset of whole file that should be copied to m_darray
    const vtkVector3i& m_lo; // lower corner index values trimmed to subgrid extent
    const vtkVector3i& m_si; // lower corner indices of subgrid
    const vtkVector3i& m_sn; // subgrid size
  };
  FileBufferToDataArray translator(lineSize, oLo[1], oHi[1], variable->GetNumberOfComponents(),
    &buffer[0], dest + component, extent, oLo, si, sn);
  vtkSMPTools::For(oLo[2], oHi[2], translator);
  // variable->FillComponent(component, 0.0);
  return true;
}

int vtkParFlowMetaReader::LoadPFBComponent(Domain dom, vtkDoubleArray* variable,
  const std::string& filename, int component, const int extent[6]) const
{
  vtksys::ifstream pfb(filename.c_str(), std::ios::binary);
  if (!pfb.good() && !vtksys::SystemTools::FileIsFullPath(filename))
  {
    pfb.clear();
    std::string fullPath =
      vtksys::SystemTools::CollapseFullPath(filename, std::string(this->Directory));
    pfb.open(fullPath.c_str(), std::ios::binary);
  }

  if (!pfb.good())
  {
    vtkErrorMacro("Unable to open file \"" << filename << "\"");
    return 0;
  }

  int rank = 1;
  int jbsz = 1;
  auto mpc = vtkMultiProcessController::GetGlobalController();
  if (mpc)
  {
    rank = mpc->GetLocalProcessId();
    jbsz = mpc->GetNumberOfProcesses();
  }
  vtkVector3d xx;
  vtkVector3i nn;
  vtkVector3d dx;
  int numSubGrids;

  pfb.seekg(0);
  pfb.read(reinterpret_cast<char*>(xx.GetData()), 3 * sizeof(double));
  pfb.read(reinterpret_cast<char*>(nn.GetData()), 3 * sizeof(int));
  pfb.read(reinterpret_cast<char*>(dx.GetData()), 3 * sizeof(double));
  pfb.read(reinterpret_cast<char*>(&numSubGrids), sizeof(int));

  // Swap bytes as required knowing that we started with big-endian data:
  vtkByteSwap::SwapBERange(xx.GetData(), 3);
  vtkByteSwap::SwapBERange(nn.GetData(), 3);
  vtkByteSwap::SwapBERange(dx.GetData(), 3);
  vtkByteSwap::SwapBE(&numSubGrids);

  std::streamoff measuredHeaderSize = pfb.tellg();
  if (measuredHeaderSize != headerSize)
  {
    vtkErrorMacro("Header size mismatch");
    return 0;
  }

  vtkIdType expectedSubgrids =
    (this->IJKDivs[dom][0].size() - 1) * (this->IJKDivs[dom][1].size() - 1);
  vtkIdType zsg = this->IJKDivs[dom][2].size() - 1;
  if (zsg > 0)
    expectedSubgrids *= zsg;
  if (numSubGrids != expectedSubgrids)
  {
    vtkErrorMacro("File for \"" << variable->GetName() << "\"(" << component
                                << ") has a different distribution (" << numSubGrids << " vs "
                                << expectedSubgrids << "). Skipping.");
    return 0;
  }

  // TODO: Handle funky surface-domain cases:
  //       (1) .C.pfb files will have nn[2] > 1
  //       (2) 2d pfb timeseries will have nn[2] > 1
  if (nn[0] != (this->IJKDivs[dom][0].back() - this->IJKDivs[dom][0].front()) ||
    nn[1] != (this->IJKDivs[dom][1].back() - this->IJKDivs[dom][1].front()) ||
    (dom == Domain::Subsurface &&
        nn[2] != (this->IJKDivs[dom][2].back() - this->IJKDivs[dom][2].front())))
  {
    vtkErrorMacro("File for \"" << variable->GetName() << "\"(" << component
                                << ") has a different size " << nn << " than expected. Skipping.");
    return 0;
  }

  vtkVector3i lo;
  vtkVector3i hi;
  for (int ii = 0; ii < (dom == Domain::Surface ? 2 : 3); ++ii)
  {
    const std::vector<int>& divisions(this->IJKDivs[dom][ii]);
    int nd = static_cast<int>(divisions.size()) - 1;
    lo[ii] = nd;
    hi[ii] = 0;
    for (int dd = 0; dd < nd; ++dd)
    {
      if (extent[2 * ii] <= divisions[dd + 1] && extent[2 * ii + 1] >= divisions[dd])
      {
        if (lo[ii] > dd)
        {
          lo[ii] = dd;
        }
        if (hi[ii] < dd)
        {
          hi[ii] = dd;
        }
      }
    }
  }
  if (dom == Domain::Surface)
  {
    // Handle 2d pfb timeseries
    lo[2] = 0; // extent[4];
    hi[2] = 0; // extent[5];
  }
  // std::cout << "        Read subgrids " << lo << " -- " << hi << " in " << filename << "\n";
  for (int rr = lo[2]; rr <= hi[2]; ++rr)
  {
    for (int qq = lo[1]; qq <= hi[1]; ++qq)
    {
      for (int pp = lo[0]; pp <= hi[0]; ++pp)
      {
        int isizem1 = static_cast<int>(this->IJKDivs[dom][0].size() - 1);
        int jsizem1 = static_cast<int>(this->IJKDivs[dom][1].size() - 1);
        int subgrid = pp + isizem1 * (qq + jsizem1 * rr);
        vtkVector3i subgridIdx(pp, qq, rr);
        std::streamoff off = this->GetBlockOffset(dom, subgridIdx, nn[2]);
        // std::cout << variable->GetName() << " comp " << component << " subgrid " << subgrid << "
        // offset " << off << "\n";
        vtkVector3i si;
        vtkVector3i sn;
        vtkVector3i sr;
        pfb.seekg(off);
        if (!this->ReadSubgridHeader(pfb, si, sn, sr))
        {
          vtkErrorMacro("Could not read \"" << filename << "\" subgrid header, block " << subgrid);
          return 0;
        }
        if (dom == Domain::Surface)
        {
          if (sn[2] == 1)
          {
            // 2-D PFB files (non-timeseries) report as if they have cells with a z extent of 1,
            // which will confuse ReadComponentSubgridOverlap() below.
            sn[2] = 0;
          }
        }
        if (!this->ReadComponentSubgridOverlap(pfb, si, sn, extent, component, variable))
        {
          vtkErrorMacro("Could not read \"" << filename << "\" subgrid data, block " << subgrid);
          return 0;
        }
        /*
        std::cout
          << "           Subgrid " << pp << " " << qq << " " << rr
          << " (" << subgrid << ") offset " << off
          << " si " << si << " sn " << sn << " sr " << sr
          << "\n";
          */
      }
    }
  }

  return 1;
}

int vtkParFlowMetaReader::LoadPFB(vtkDataSet* data, const int extent[6],
  const std::vector<std::string>& fileList, const std::string& variableName,
  const json& variableMetadata) const
{
  int status = 1;
  // Fetch domain info on subgrid distribution.
  Domain dom;
  try
  {
    dom = variableMetadata.at("domain").get<std::string>() == "surface" ? Domain::Surface
                                                                        : Domain::Subsurface;
  }
  catch (std::exception&)
  {
    vtkErrorMacro("No domain in metadata " << variableMetadata.dump());
    return 0;
  }

  // Now fileList specifies 1 entry for each component of the variable
  // and we have extents and (via variableMetadata) the data placement
  // (i.e., cell- or point-centered). So we can allocate an array.
  int numComps = static_cast<int>(fileList.size());
  try
  {
    std::string place = variableMetadata.at("place");
    if (place == "cell")
    {
      vtkIdType numTuples = (extent[1] - extent[0]) * (extent[3] - extent[2]);
      if (extent[5] > extent[4])
      {
        numTuples *= extent[5] - extent[4];
      }
      vtkNew<vtkDoubleArray> array;
      array->SetName(variableName.c_str());
      array->SetNumberOfComponents(numComps);
      array->SetNumberOfTuples(numTuples);
      // std::cout << "Array " << variableName << " " << numTuples << " ( " << extent[1] << " " <<
      // extent[3] << " " << extent[5] << " )\n";
      data->GetCellData()->AddArray(array);
      int comp = 0;
      for (auto file : fileList)
      {
        status &= this->LoadPFBComponent(dom, array, file, comp, extent);
        ++comp;
      }
    }
    else
    {
      vtkErrorMacro("Cannot load " << place << "-centered data yet.");
      status = 0;
    }
  }
  catch (std::exception& e)
  {
    vtkErrorMacro("Bad JSON " << e.what());
    status = 0;
  }

  /*
  std::cout << "    Need to load:\n";
  for (auto file : fileList)
  {
    std::cout << "      " << file << "\n";
  }
  std::cout
    << "    for " << variableName
    << "[" << extent[0] << "-" << extent[1]
    << ", " << extent[2] << "-" << extent[3]
    << ", " << extent[4] << "-" << extent[5]
    << "]\n";
    */
  return status;
}

int vtkParFlowMetaReader::LoadVariable(
  vtkDataSet* grid, const std::string& variableName, const json& variableMetadata, int timestep)
{
  int status = 1;
  // std::cout << "  Loading " << variableName << " @ t" << timestep << "\n";
  auto type = variableMetadata["type"].get<std::string>();
  std::vector<std::string> filesToLoad;
  if (type == "pfb" || type == "pfb 2d timeseries")
  {
    // Find file(s) corresponding to current timestep
    int slice = (type == "pfb 2d timeseries" ? -2 : -1);
    status &= this->FindPFBFiles(filesToLoad, variableMetadata, timestep, slice);

    {
      auto block = grid;

      int extent[6];
      auto info = grid->GetInformation();
      info->Get(vtkDataObject::PIECE_EXTENT(), extent);

      // For pfb 2d timeseries, encode the time slice number in the extent:
      if (slice > 0 && extent[4] == 0 && extent[5] == 0)
      {
        extent[4] = (extent[5] = slice);
      }

      status &= this->LoadPFB(block, extent, filesToLoad, variableName, variableMetadata);
    }
  }
  else
  {
    vtkErrorMacro("Cannot load variable " << variableName << " of unknown type " << type);
    status = 0;
  }
  return status;
}

int vtkParFlowMetaReader::LoadSubsurfaceData(vtkDataSet* mbds, int timestep)
{
  int status = 1;
  int extent[6];
  this->ClampRequestToData<3>(extent, this->SubsurfaceVOI, this->SubsurfaceExtent);
  // this->CellExtentToPointExtent<3>(extent);
  this->GenerateDistributedMesh(mbds, extent, this->SubsurfaceOrigin, this->SubsurfaceSpacing);

  // Now loop over each variable and load it
  for (auto varIt = this->SubsurfaceVariables.begin(); varIt != this->SubsurfaceVariables.end();
       ++varIt)
  {
    if (this->GetSubsurfaceVariableArrayStatus(varIt->first))
    {
      status &= this->LoadVariable(mbds, varIt->first, varIt->second, timestep);
    }
  }
  return status;
}

int vtkParFlowMetaReader::LoadSurfaceData(vtkDataSet* mbds, int timestep)
{
  int status = 1;
  int extent[6];
  this->ClampRequestToData<2>(extent, this->SurfaceAOI, this->SurfaceExtent);
  // this->CellExtentToPointExtent<2>(extent);
  this->GenerateDistributedMesh(mbds, extent, this->SurfaceOrigin, this->SurfaceSpacing);

  for (auto varIt = this->SurfaceVariables.begin(); varIt != this->SurfaceVariables.end(); ++varIt)
  {
    if (this->GetSurfaceVariableArrayStatus(varIt->first))
    {
      status &= this->LoadVariable(mbds, varIt->first, varIt->second, timestep);
    }
  }
  return status;
}

int vtkParFlowMetaReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

int vtkParFlowMetaReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  if (!this->FileName || !this->FileName[0])
  {
    vtkErrorMacro("No filename provided.");
    return 0;
  }

  this->SetDirectory(vtksys::SystemTools::GetParentDirectory(this->FileName).c_str());

  // When run in parallel, we choose a range of blocks
  // to load from those available.
  int rank = 0;
  int jbsz = 1;
  auto mpc = vtkMultiProcessController::GetGlobalController();
  if (mpc)
  {
    rank = mpc->GetLocalProcessId();
    jbsz = mpc->GetNumberOfProcesses();
  }

  std::vector<uint8_t> contents;
  if (rank == 0)
  {
    vtksys::ifstream pfb(this->FileName, std::ios::binary);
    if (pfb.good())
    {
      pfb.seekg(0, std::ios::end);
      contents.resize(pfb.tellg());
      pfb.seekg(0, std::ios::beg);
      pfb.read(reinterpret_cast<char*>(&contents[0]), contents.size());
      pfb.close();
    }
    else
    {
      vtkErrorMacro("Unable to read metadata \"" << this->FileName << "\"");
    }
  }

  if (!this->BroadcastMetadata(contents))
  {
    return 0;
  }

  if (!this->IngestMetadata())
  {
    return 0;
  }

  auto subsurfaceInfo = outInfo->GetInformationObject(Domain::Subsurface);
  subsurfaceInfo->Set(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);
  int subsurfaceAvailable[6];
  this->ClampRequestToData<3>(subsurfaceAvailable, this->SubsurfaceVOI, this->SubsurfaceExtent);
  // this->CellExtentToPointExtent<3>(subsurfaceAvailable);
  subsurfaceInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), subsurfaceAvailable, 6);

  auto surfaceInfo = outInfo->GetInformationObject(Domain::Surface);
  surfaceInfo->Set(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);
  int surfaceAvailable[6];
  this->ClampRequestToData<2>(surfaceAvailable, this->SurfaceAOI, this->SurfaceExtent);
  // this->CellExtentToPointExtent<2>(surfaceAvailable);
  surfaceInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), surfaceAvailable, 6);

  // For now, both grids share the same set of time steps.
  // In the future we could track this separately.
  if (!this->TimeSteps.empty())
  {
    std::vector<double> times(this->TimeSteps.begin(), this->TimeSteps.end());
    double timeRange[2];
    timeRange[0] = times.front();
    timeRange[1] = times.back();
    for (int ii = 0; ii < 2; ++ii)
    {
      auto info = outInfo->GetInformationObject(ii);
      info->Set(
        vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], static_cast<int>(times.size()));
      info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  }
  else
  {
    for (int ii = 0; ii < 2; ++ii)
    {
      auto info = outInfo->GetInformationObject(ii);
      info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }
  }

  return this->Superclass::RequestInformation(request, inInfo, outInfo);
}

int vtkParFlowMetaReader::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inInfo), vtkInformationVector* outputVector)
{
  for (int port = 0; port < this->GetNumberOfOutputPorts(); ++port)
  {
    // Use a vtkExplicitStructuredGrid for the subsurface when deflecting by elevation.
    // Use vtkImageData in all other cases.
    int outputDataSetType = this->DeflectTerrain && port == Domain::Subsurface
      ? VTK_EXPLICIT_STRUCTURED_GRID
      : VTK_IMAGE_DATA;
    const char* outTypeStr = vtkDataObjectTypes::GetClassNameFromTypeId(outputDataSetType);

    vtkInformation* info = outputVector->GetInformationObject(port);
    vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
    if (!output || !output->IsA(outTypeStr))
    {
      vtkDataObject* newOutput = vtkDataObjectTypes::NewDataObject(outputDataSetType);
      if (!newOutput)
      {
        vtkErrorMacro("Could not create chosen output data type: " << outTypeStr);
        return 0;
      }
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      newOutput->Delete();
    }
  }
  return 1;
}

int vtkParFlowMetaReader::RequestData(
  vtkInformation* request, vtkInformationVector** vtkNotUsed(inInfo), vtkInformationVector* outInfo)
{
  auto subsurface = vtkDataSet::GetData(outInfo, 0);
  auto subsurfaceInfo = outInfo->GetInformationObject(0);
  if (!subsurface)
  {
    vtkErrorMacro("No subsurface data object created.");
    return 0;
  }

  auto surface = vtkDataSet::GetData(outInfo, 1);
  auto surfaceInfo = outInfo->GetInformationObject(1);
  if (!surface)
  {
    vtkErrorMacro("No surface data object created.");
    return 0;
  }

  int timeStep = this->TimeStep;
  int requestPort = request->Has(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    ? request->Get(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    : 0;
  auto reqPortOutInfo = requestPort == 0 ? subsurfaceInfo : surfaceInfo;
  // Fetch the pipeline-requested timestep and use it for both outputs, regardless
  // of which port's downstream filters initiated the request.
  if (reqPortOutInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double ts = reqPortOutInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    timeStep = this->ClosestTimeStep(ts);
  }

  // A simple cache for now. We can get fancier with an LRU for caching
  // multiple timesteps and fields later.
  if (this->SubsurfaceCache && this->GetMTime() < this->SubsurfaceCache->GetMTime() &&
    this->CacheTimeStep == timeStep)
  {
    // std::cout << "  Used cache\n";
    subsurface->ShallowCopy(this->SubsurfaceCache);
    subsurface->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeStep);
    surface->ShallowCopy(this->SurfaceCache);
    surface->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeStep);
    return 1;
  }

#if 0
  // For debugging
  std::cout
    << "  Not using cache " << this->SubsurfaceCache
    << " " << (this->SubsurfaceCache ? this->SubsurfaceCache->GetMTime() : -1) << " vs " << this->GetMTime()
    << " " << this->CacheTimeStep << " vs " << timeStep << "\n";
#endif

  this->CacheTimeStep = timeStep;
  if (this->EnableSubsurfaceDomain)
  {
    this->LoadSubsurfaceData(subsurface, timeStep);
    subsurface->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeStep);
  }
  else
  {
    subsurface->Initialize();
  }
  this->SubsurfaceCache = vtkDataObjectTypes::NewDataObject(subsurface->GetDataObjectType());
  this->SubsurfaceCache->ShallowCopy(subsurface);

  if (this->EnableSurfaceDomain)
  {
    this->LoadSurfaceData(surface, timeStep);
    surface->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeStep);
  }
  else
  {
    surface->Initialize();
  }
  this->SurfaceCache = vtkDataObjectTypes::NewDataObject(surface->GetDataObjectType());
  this->SurfaceCache->ShallowCopy(surface);

#if 0
  int gridLo = (rank * numSubGrids) / jbsz;
  int gridHi = ((rank + 1) * numSubGrids) / jbsz;
  // std::cout << "Rank " << rank << " owns subgrids " << gridLo << " -- " << gridHi << "\n";

  output->SetNumberOfBlocks(numSubGrids);
  for (int ni = gridLo; ni < gridHi; ++ni)
  {
    this->ReadBlock(pfb, output, xx, dx, arrayName, ni);
  }
#endif

  return 1;
}
