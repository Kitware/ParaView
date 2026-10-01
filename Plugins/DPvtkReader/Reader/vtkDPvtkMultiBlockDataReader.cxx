// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDPvtkMultiBlockDataReader.h"

#include "vtkCommunicator.h"
#include "vtkDPvz.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkSetGet.h"
#ifdef DPVZ_MPI
#include "vtkMPIController.h"
#endif
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtk_fmt.h"
#include <vtksys/SystemTools.hxx>
// Keep clang-format from adding spaces around the '/' path separator:
// clang-format off
#include VTK_FMT(fmt/args.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
// clang-format on

#include <DPvzTocEntry.h>
#include <DPvzVtk.h>
#include <DPvzVtkData.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
struct RankDataType
{
  /**
   * Rank at writing
   */
  int Rank = -1;
  /**
   * storage for VtkData
   */
  std::string RawData;
  /**
   * array of DPvzVtkData  (file name, size and data)
   * stored in RawData for a time step and rank
   */
  DPvzVtkData* VtkData = nullptr;
  /**
   * How many VTK files are in VtkData.
   */
  int VtkDataCount = 0;

  RankDataType() = default;
  RankDataType(const RankDataType&) = delete;
  RankDataType& operator=(const RankDataType&) = delete;
  ~RankDataType() { this->FreeVtkData(); }

  /**
   * Releases the VtkData array allocated by DPvzVtkData::extract.
   */
  void FreeVtkData()
  {
    if (this->VtkData)
    {
      DPvzVtkData::free(this->VtkDataCount, this->VtkData);
    }
    this->VtkData = nullptr;
    this->VtkDataCount = 0;
  }
  void Clear()
  {
    this->FreeVtkData();
    this->Rank = -1;
    this->RawData.clear();
  }
};
}

vtkStandardNewMacro(vtkDPvtkMultiBlockDataReader);

class vtkDPvtkMultiBlockDataReader::vtkInternals
{
public:
  vtkDPvtkMultiBlockDataReader* Object = nullptr;
  vtkMultiProcessController* Controller = nullptr;
  std::unique_ptr<DPvzVtk> File;
  // Table of contents for the whole file. One entry for each time step.
  std::vector<DPvzTocEntry> Toc;
  // All time values
  std::vector<double> Times;

  // Table of contents for the current time step. One entry for each rank.
  std::vector<DPvzRankToc> StepToc;
  // Current time value
  double Time = 0;
  // Current time step
  size_t Step = 0;
  // true if StepToc, VtmData, FileNameToRankIndex and RankData are loaded for Step
  bool StepLoaded = false;
  // .vtm for current time step
  std::string VtmData;
  // map (file name -> (rank, index inside of rank data)) for current time step
  std::map<std::string, std::pair<int, int>> FileNameToRankIndex;
  // Rank data for current time step
  RankDataType RankData;

  vtkInternals(vtkDPvtkMultiBlockDataReader* object)
    : Object(object)
  {
  }
  /**
   * Opens the DPvtk file, reads Toc
   */
  void Open(const char* filename);
  void Close();
  bool IsOpen() { return File && !this->File->failed(); }
  /**
   * Returns the time step index for the UPDATE_TIME_STEP in outInfo
   * or 0 if there is no UPDATE_TIME_STEP.
   */
  size_t GetRequestedStep(vtkInformation* outInfo);
  /**
   * Reads StepToc, VtmData and FileNameToRankIndex for 'step' if that step is not
   * already loaded. Sets Time and Step. Returns true if a new step was loaded.
   * This is a collective operation.
   */
  bool LoadStep(size_t step);
  /**
   * Reads StepToc for this->Step from the file.
   * This is a local operation, called by ReadPartitionInformation.
   */
  void ReadStepToc();
  /**
   * Reads StepToc and all RankData on all nodes, and builds FileNameToRankIndex.
   * All ranks agree on success before communicating, so an error on one rank
   * is reported on all ranks. This is a collective operation.
   */
  void ReadPartitionInformation();
  bool ReaderSetData(vtkXMLReader* reader, const char* fileName);
};

void vtkDPvtkMultiBlockDataReader::vtkInternals::Open(const char* filename)
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
#ifdef DPVZ_MPI
  // DPvz must use the same processes as this->Controller, which is used to
  // distribute the reading.
  auto* controller = vtkMPIController::SafeDownCast(this->Controller);
  auto* communicator =
    controller ? vtkMPICommunicator::SafeDownCast(controller->GetCommunicator()) : nullptr;
  if (!communicator)
  {
    throw std::runtime_error("DPvtk needs that vtkMultiProcessController::GetGlobalController() "
                             "returns a vtkMPIController.");
  }
  MPI_Comm rawComm = *(communicator->GetMPIComm()->GetHandle());
  this->File = std::make_unique<DPvzVtk>(filename, DPvzMode::DPvzReadOnly, rawComm);
#else
  this->File = std::make_unique<DPvzVtk>(filename, DPvzMode::DPvzReadOnly);
#endif
  if (this->File->failed())
  {
    throw std::runtime_error(std::string("Cannot open ") + filename);
  }
  int64_t steps = this->File->get_steps();
  if (steps <= 0)
  {
    throw std::runtime_error(std::string("DPvtkFile::get_steps returned ") + std::to_string(steps));
  }
  this->Toc.resize(steps);
  vtkDebugWithObjectMacro(this->Object, "Number of steps: " << steps);
  if (this->File->get_map(this->Toc.data()))
  {
    throw std::runtime_error("DPvtkFile::get_map failed");
  }
  this->Times.resize(this->Toc.size());
  std::transform(this->Toc.begin(), this->Toc.end(), this->Times.begin(),
    [](DPvzTocEntry& value) { return value.time; });
}

void vtkDPvtkMultiBlockDataReader::vtkInternals::Close()
{
  this->File.reset();
  this->Toc.clear();
  this->Times.clear();
  this->StepToc.clear();
  this->VtmData.clear();
  this->FileNameToRankIndex.clear();
  this->RankData.Clear();
  this->StepLoaded = false;
}

size_t vtkDPvtkMultiBlockDataReader::vtkInternals::GetRequestedStep(vtkInformation* outInfo)
{
  if (this->Times.empty())
  {
    // we already checked for this
    throw std::runtime_error("No times information has been read from the DPvtkFile");
  }
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    return 0;
  }
  double requestedValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  // last time step with time <= requestedValue, clamped to the first step
  auto it = std::upper_bound(this->Times.begin(), this->Times.end(), requestedValue);
  return it == this->Times.begin() ? 0 : std::distance(this->Times.begin(), it) - 1;
}

bool vtkDPvtkMultiBlockDataReader::vtkInternals::LoadStep(size_t step)
{
  if (this->StepLoaded && this->Step == step)
  {
    return false;
  }
  this->StepLoaded = false;
  this->Step = step;
  this->Time = this->Times[this->Step];
  this->ReadPartitionInformation();
  this->StepLoaded = true;
  return true;
}

void vtkDPvtkMultiBlockDataReader::vtkInternals::ReadStepToc()
{
  int32_t ranks = this->Toc[this->Step].ranks;
  vtkDebugWithObjectMacro(
    this->Object, << fmt::format("Number of ranks for time step {}: {} ", this->Step, ranks));
  this->StepToc.resize(ranks);
  if (this->File->get_step_toc(this->Toc[this->Step], this->StepToc.data()))
  {
    throw std::runtime_error("DPvtkFile::get_time_toc failed");
  }
}

void vtkDPvtkMultiBlockDataReader::vtkInternals::ReadPartitionInformation()
{
  assert(this->Controller);
  // discard data from the previously loaded time step
  this->StepToc.clear();
  this->VtmData.clear();
  this->FileNameToRankIndex.clear();
  this->RankData.Clear();
  int numRanks = this->Controller->GetNumberOfProcesses();
  int rank = this->Controller->GetLocalProcessId();

  // read the file names on all ranks
  // unfortunately we also read the file data in the same time.
  std::vector<std::string> localFileName;
  std::vector<int> localRank;
  std::vector<int> localIndex;
  // Errors are not thrown right away: all ranks must agree on success
  // before the collective operations below, otherwise the other ranks would hang.
  std::string localError;
  try
  {
    this->ReadStepToc();
    if (this->StepToc.empty())
    {
      throw std::runtime_error("Cannot read partition information without time step information");
    }
    // Process 'rank' reads the writer ranks i with i % numRanks == rank.
    // We traverse backward so that the data of writer rank 'rank' stays cached.
    for (int i = static_cast<int>(this->StepToc.size()) - 1; i >= 0; --i)
    {
      if (i % numRanks == rank)
      {
        this->RankData.FreeVtkData();
        this->RankData.Rank = i;
        this->RankData.RawData.resize(this->StepToc[i].inflated_size);
        vtkDebugWithObjectMacro(
          this->Object, << fmt::format("ReadPartitionInformation, DPvtkFile::get_data rank={}", i));
        if (this->File->get_data(this->StepToc[i], this->RankData.RawData.data()))
        {
          throw std::runtime_error(fmt::format("DPvtkFile::get_data failed for rank {}", i));
        }
        if (DPvzVtkData::extract(this->RankData.RawData.data(), this->StepToc[i].inflated_size,
              this->RankData.VtkDataCount, this->RankData.VtkData))
        {
          throw std::runtime_error("DPvzVtkData::extract failed");
        }

        for (int j = 0; j < this->RankData.VtkDataCount; ++j)
        {
          std::string fileName = this->RankData.VtkData[j].file;
          if (i == 0)
          {
            std::string ext = vtksys::SystemTools::GetFilenameExtension(fileName);
            if (ext == ".pvd")
            {
              continue;
            }
            else if (ext == ".vtm")
            {
              this->VtmData.resize(this->RankData.VtkData[j].size);
              std::copy(this->RankData.VtkData[j].data,
                this->RankData.VtkData[j].data + this->RankData.VtkData[j].size,
                this->VtmData.data());
              continue;
            }
          }
          localFileName.emplace_back(fileName);
          localRank.emplace_back(i);
          localIndex.emplace_back(j);
        }
      }
    }
    if (rank == 0 && this->VtmData.empty())
    {
      throw std::runtime_error("Cannot find a .vtm file");
    }
  }
  catch (std::exception& e)
  {
    localError = e.what();
    // do not keep partially read data
    this->StepToc.clear();
    this->RankData.Clear();
  }
  int localFailed = localError.empty() ? 0 : 1;
  int anyFailed = 0;
  this->Controller->AllReduce(&localFailed, &anyFailed, 1, vtkCommunicator::MAX_OP);
  if (anyFailed)
  {
    throw std::runtime_error(localFailed
        ? localError
        : std::string("Reading partition information failed on another rank"));
  }

  // build a stream because strings can have different sizes
  vtkMultiProcessStream stream;
  stream << static_cast<int>(localFileName.size());
  for (size_t i = 0; i < localFileName.size(); ++i)
  {
    stream << localFileName[i];
    stream << localRank[i];
    stream << localIndex[i];
  }
  std::vector<vtkMultiProcessStream> streams(numRanks);

  // send the streams to all ranks
  this->Controller->AllGather(stream, streams);
  // send the vtm file to all ranks
  stream.Reset();
  if (rank == 0)
  {
    stream << this->VtmData;
  }
  this->Controller->Broadcast(stream, 0);
  if (rank != 0)
  {
    stream >> this->VtmData;
  }

  for (int i = 0; i < numRanks; ++i)
  {
    int streamISize = 0;
    streams[i] >> streamISize;
    for (int j = 0; j < streamISize; ++j)
    {
      std::string fileName;
      int fileNameRank;
      int fileNameIndex;
      streams[i] >> fileName;
      streams[i] >> fileNameRank;
      streams[i] >> fileNameIndex;
      // All ranks build the same map, so all ranks throw together.
      auto inserted =
        this->FileNameToRankIndex.emplace(fileName, std::make_pair(fileNameRank, fileNameIndex));
      if (!inserted.second)
      {
        throw std::runtime_error(
          fmt::format("Duplicate file name {} stored at rank {} index {} and at rank {} index {}",
            fileName, inserted.first->second.first, inserted.first->second.second, fileNameRank,
            fileNameIndex));
      }
    }
  }
  vtkDebugWithObjectMacro(this->Object, << fmt::format("Number of partitions for time step {}: {} ",
                                          this->Step, this->FileNameToRankIndex.size()));
}

bool vtkDPvtkMultiBlockDataReader::vtkInternals::ReaderSetData(
  vtkXMLReader* reader, const char* fileName)
{
  try
  {
    int rank = this->Controller->GetLocalProcessId();
    vtkDebugWithObjectMacro(this->Object,
      "vtkDPvtkMultiBlockDataReader::vtkInternals::ReaderSetData rank: " << rank
                                                                         << " file: " << fileName);
    auto it = this->FileNameToRankIndex.find(fileName);
    if (it == this->FileNameToRankIndex.end())
    {
      throw std::runtime_error(fmt::format("Cannot find {}", fileName));
    }
    int requestedRank = it->second.first;
    int requestedIndex = it->second.second;
    if (this->RankData.Rank != requestedRank)
    {
      // RankData is not cached, read it
      this->RankData.Clear();
      if (requestedRank < 0 || static_cast<size_t>(requestedRank) >= this->StepToc.size())
      {
        throw std::runtime_error(
          fmt::format("Requested rank {} outside of the range of ranks [0, {})", requestedRank,
            this->StepToc.size()));
      }
      this->RankData.RawData.resize(this->StepToc[requestedRank].inflated_size);
      vtkDebugWithObjectMacro(
        this->Object, << fmt::format("ReaderSetData, DPvtkFile::get_data rank={}", requestedRank));
      if (this->File->get_data(this->StepToc[requestedRank], this->RankData.RawData.data()))
      {
        throw std::runtime_error(
          fmt::format("DPvtkFile::get_data failed for rank {}", requestedRank));
      }
      if (DPvzVtkData::extract(this->RankData.RawData.data(),
            this->StepToc[requestedRank].inflated_size, this->RankData.VtkDataCount,
            this->RankData.VtkData))
      {
        throw std::runtime_error("DPvzVtkData::extract failed");
      }
      this->RankData.Rank = requestedRank;
    }
    if (requestedIndex >= this->RankData.VtkDataCount)
    {
      throw std::runtime_error(
        fmt::format("Requested index {} greater or equal with number of VTK datasets {}",
          requestedIndex, this->RankData.VtkDataCount));
    }
    reader->ReadFromInputStringOn();
    reader->SetInputString(
      this->RankData.VtkData[requestedIndex].data, this->RankData.VtkData[requestedIndex].size);
  }
  catch (std::exception& e)
  {
    vtkErrorWithObjectMacro(this->Object, << e.what());
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
vtkDPvtkMultiBlockDataReader::vtkDPvtkMultiBlockDataReader()
  : Internals(new vtkDPvtkMultiBlockDataReader::vtkInternals(this))
{
}

//------------------------------------------------------------------------------
vtkDPvtkMultiBlockDataReader::~vtkDPvtkMultiBlockDataReader() = default;

int vtkDPvtkMultiBlockDataReader::CanReadFile(const char* name)
{
  try
  {
    vtkDebugMacro("vtkDPvtkMultiBlockDataReader::CanReadFile");
#ifdef DPVZ_MPI
    // CanReadFile is only called on node 0, so DPvzFile constructor with MPI_COMM_WORLD will
    // hang as it is a collective operation. Open the file with MPI_COMM_SELF instead
    std::unique_ptr<DPvzVtk> file =
      std::make_unique<DPvzVtk>(name, DPvzMode::DPvzReadOnly, MPI_COMM_SELF);
    if (file->failed())
    {
      throw std::runtime_error(std::string("Cannot open ") + name);
    }
    return 1;
#else

    if (this->Internals->IsOpen())
    {
      if (this->Internals->File->file_name != name)
      {
        this->Internals->Close();
      }
      else
      {
        return 1;
      }
    }

    // File is not Open at this point
    this->Internals->Open(name);
    return 1;
#endif
  }
  catch (std::exception&)
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkDPvtkMultiBlockDataReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro("vtkDPvtkMultiBlockDataReader::RequestInformation");
  try
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    // open the DPvtk file and read the Toc
    if (this->Internals->IsOpen())
    {
      if (this->Internals->File->file_name != this->FileName)
      {
        this->Internals->Close();
      }
    }

    if (!this->Internals->IsOpen())
    {
      this->Internals->Open(this->FileName);
    }
    // UPDATE_TIME_STEP is not yet set for this request, so we load a time step
    // only to provide the .vtm structure. RequestData loads the requested time step.
    if (this->Internals->LoadStep(this->Internals->GetRequestedStep(outInfo)))
    {
      // make sure the .vtm for the loaded step is parsed
      this->ReadMTime = vtkTimeStamp();
    }

    if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
      throw std::runtime_error("Cannot read the .vtm information");
    }
    // set TIME_STEPS and TIME_RANGE
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->Internals->Times.data(),
      static_cast<int>(this->Internals->Times.size()));
    double timeRange[2];
    timeRange[0] = this->Internals->Times[0];
    timeRange[1] = this->Internals->Times[this->Internals->Times.size() - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  catch (std::exception& e)
  {
    vtkErrorMacro("Error: " << e.what());
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkDPvtkMultiBlockDataReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro("vtkDPvtkMultiBlockDataReader::RequestData");
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  try
  {
    if (this->Internals->LoadStep(this->Internals->GetRequestedStep(outInfo)))
    {
      // A different time step than the one used in RequestInformation was loaded.
      // Force parsing of the new .vtm. We do not call Modified() as that
      // would re-execute the pipeline.
      this->ReadMTime = vtkTimeStamp();
      if (!this->ReadXMLInformation())
      {
        throw std::runtime_error("Cannot parse the .vtm file");
      }
      this->PrepareToCreateMetaData(this->GetPrimaryElement());
      this->CreateMetaData(this->GetPrimaryElement());
    }
  }
  catch (std::exception& e)
  {
    vtkErrorMacro("Error: " << e.what());
    return 0;
  }
  int ret = this->Superclass::RequestData(request, inputVector, outputVector);
  // the superclass sets the time to the first time step, as it does not know about DPvtk
  // time steps.
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->Internals->Time);
  return ret;
}

//------------------------------------------------------------------------------
int vtkDPvtkMultiBlockDataReader::ReadXMLInformation()
{
  vtkDebugMacro("vtkDPvtkMultiBlockDataReader::ReadXMLInformation");
  // Set the members directly: SetInputString() and ReadFromInputStringOn()
  // call Modified(), which would re-execute the pipeline.
  this->ReadFromInputString = 1;
  this->InputString = this->Internals->VtmData;
  return this->Superclass::ReadXMLInformation();
}

//------------------------------------------------------------------------------
bool vtkDPvtkMultiBlockDataReader::ReaderSetFileNameOrData(
  vtkXMLReader* reader, const char* fullFileName)
{
  vtkDebugMacro("vtkDPvtkMultiBlockDataReader::ReaderSetFileNameOrData");
  std::string fileName = vtksys::SystemTools::GetFilenameName(fullFileName);
  return this->Internals->ReaderSetData(reader, fileName.c_str());
}

//------------------------------------------------------------------------------
void vtkDPvtkMultiBlockDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "vtkDPvtkMultiBlockDataReader" << std::endl;
  Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
