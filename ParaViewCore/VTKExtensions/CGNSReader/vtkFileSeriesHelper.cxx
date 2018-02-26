/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFileSeriesHelper.h"

#include "vtkAlgorithm.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSubsetInclusionLattice.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <cassert>
#include <iterator> // std::back_inserter
#include <vtksys/SystemTools.hxx>

//============================================================================
// class: vtkFileSeriesHelper::vtkTimeInformation
//============================================================================

//----------------------------------------------------------------------------
vtkFileSeriesHelper::vtkTimeInformation::vtkTimeInformation()
  : TimeRange(0, 0)
  , TimeRangeValid(false)
  , TimeStepsValid(false)
{
}

//----------------------------------------------------------------------------
vtkFileSeriesHelper::vtkTimeInformation::vtkTimeInformation(double time)
  : TimeRange(time, time)
  , TimeRangeValid(true)
  , TimeStepsValid(true)
{
  this->TimeSteps.push_back(time);
}

//----------------------------------------------------------------------------
vtkFileSeriesHelper::vtkTimeInformation::vtkTimeInformation(vtkInformation* info)
  : TimeRange(0, 0)
  , TimeRangeValid(false)
  , TimeStepsValid(false)
{
  if (info->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    this->TimeRangeValid = true;

    const double* range = info->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    this->TimeRange.first = range[0];
    this->TimeRange.second = range[1];
  }

  if (info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->TimeStepsValid = true;
    this->TimeSteps.resize(info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));
    if (this->TimeSteps.size() > 0)
    {
      info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->TimeSteps[0]);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkFileSeriesHelper::vtkTimeInformation::operator==(
  const vtkFileSeriesHelper::vtkTimeInformation& other) const
{
  if (this->TimeStepsValid != other.TimeStepsValid || this->TimeRangeValid != other.TimeRangeValid)
  {
    return false;
  }
  if (this->TimeStepsValid && this->TimeSteps != other.TimeSteps)
  {
    return false;
  }
  if (this->TimeRangeValid && this->TimeRange != other.TimeRange)
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::vtkTimeInformation::Save(vtkMultiProcessStream& stream) const
{
  stream << this->TimeRangeValid << this->TimeRange.first << this->TimeRange.second
         << this->TimeStepsValid << static_cast<unsigned int>(this->TimeSteps.size());
  for (size_t cc = 0; cc < this->TimeSteps.size(); ++cc)
  {
    stream << this->TimeSteps[cc];
  }
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::vtkTimeInformation::Load(vtkMultiProcessStream& stream)
{
  unsigned int count = 0;
  stream >> this->TimeRangeValid >> this->TimeRange.first >> this->TimeRange.second >>
    this->TimeStepsValid >> count;
  this->TimeSteps.resize(count);
  for (unsigned int cc = 0; cc < count; ++cc)
  {
    stream >> this->TimeSteps[cc];
  }
}

//============================================================================
// class: vtkFileSeriesHelper
//============================================================================
namespace vtkFileSeriesHelperNS
{
int GetTimeStepIndex(const double time, const double* timesteps, int num_timesteps)
{
  if (timesteps == NULL || num_timesteps <= 0)
  {
    return -1;
  }

  const double* lbptr = std::lower_bound(timesteps, timesteps + num_timesteps, time);
  int index = static_cast<int>(lbptr - timesteps);
  assert(index >= 0 && index <= num_timesteps);
  if (index == num_timesteps)
  {
    // if time > last_timestep, just give the last timestep.
    --index;
  }
  return index;
}
}

vtkStandardNewMacro(vtkFileSeriesHelper);
vtkCxxSetObjectMacro(vtkFileSeriesHelper, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkFileSeriesHelper::vtkFileSeriesHelper()
  : Controller(NULL)
  , FileNames()
  , IgnoreReaderTime(false)
  , PartitionedFiles(false)
  , Information()
  , AggregatedTimeSteps()
  , AggregatedTimeRangeValid(false)
  , AggregatedTimeRange(0, 0)
  , SIL(nullptr)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkFileSeriesHelper::~vtkFileSeriesHelper()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::RemoveAllFileNames()
{
  if (this->FileNames.size() > 0)
  {
    this->FileNames.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::AddFileName(const char* fname)
{
  if (fname != NULL && fname[0] != '\0')
  {
    const std::string fname_str(fname);
    if (std::find(this->FileNames.begin(), this->FileNames.end(), fname_str) ==
      this->FileNames.end())
    {
      this->FileNames.push_back(fname_str);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::SetFileNames(const std::vector<std::string>& filenames)
{
  if (this->FileNames != filenames)
  {
    this->FileNames = filenames;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkFileSeriesHelper::ReadMetaFile(const char* metafilename)
{
  if (metafilename == NULL || metafilename[0] == '\0')
  {
    // vtkErrorMacro("Invalid 'metafilename' passed to `ReadMetaFile`.");
    return false;
  }

  // Open the metafile.
  ifstream metafile(metafilename);
  if (metafile.bad())
  {
    // vtkErrorMacro("Failed to open meta-file: " << metafilename);
    return false;
  }

  std::string metaFileDir = vtksys::SystemTools::GetFilenamePath(metafilename);

  std::vector<std::string> fnames;

  // Iterate over all files pointed to by the metafile.
  while (metafile.good() && !metafile.eof())
  {
    std::string fname;
    metafile >> fname;
    for (size_t cc = 0; cc < fname.size(); cc++)
    {
      int ch = fname[cc];
      // to avoid assert() in debug MSVC, we need to ensure 0 <= ch < 256.
      if (static_cast<unsigned int>(ch) >= 256 || !isprint(ch))
      {
        // must not be an ASCII file.
        return false;
      }
    }
    fnames.push_back(vtksys::SystemTools::CollapseFullPath(fname, metaFileDir));
  }

  if (this->FileNames != fnames)
  {
    this->FileNames = fnames;
    this->Modified();
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkFileSeriesHelper::UpdateInformation(
  vtkAlgorithm* reader, const vtkFileSeriesHelper::FileNameFunctorType& setFileName)
{
  if (reader == nullptr)
  {
    return false;
  }

  if (this->GetMTime() < this->UpdateInformationTime)
  {
    // nothing of significance has changed that'd
    return true;
  }

  // Reader must not be a sink.
  assert(reader->GetNumberOfOutputPorts() > 0);

  // Clear time information since we're starting afresh.
  this->Information.clear();
  this->AggregatedTimeSteps.clear();
  this->AggregatedTimeRangeValid = false;
  this->AggregatedTimeRange = std::pair<double, double>(0, 0);

  if (this->FileNames.size() == 0)
  {
    // Having no files is not an error since the internal reader may have the
    // filename optional.
    return true;
  }

  if (this->Controller == NULL || this->Controller->GetLocalProcessId() == 0)
  {
    // Update information about timesteps.
    bool ignoreReaderTime = this->IgnoreReaderTime;
    if (!ignoreReaderTime)
    {
      assert(this->FileNames.size() > 0);

      // check if the reader is providing any time information. If not, we'll
      // still need to ignore the reader time.
      setFileName(reader, this->FileNames[0]);
      reader->UpdateInformation();
      vtkTimeInformation tinfo(reader->GetOutputInformation(0));
      if (tinfo.GetTimeStepsValid() || tinfo.GetTimeRangeValid())
      {
        this->Information.push_back(tinfo);
      }
      else
      {
        ignoreReaderTime = true;
      }
    }

    if (ignoreReaderTime)
    {
      for (size_t cc = 0; cc < this->FileNames.size(); ++cc)
      {
        this->Information.push_back(vtkTimeInformation(static_cast<double>(cc)));
      }
    }
    else
    {
      for (size_t cc = 1, fmax = this->FileNames.size(); cc < fmax; ++cc)
      {
        setFileName(reader, this->FileNames[cc]);
        reader->UpdateInformation();
        const vtkTimeInformation tinfo(reader->GetOutputInformation(0));
        this->Information.push_back(tinfo);

        if (cc == 1 && this->Information[0] == this->Information[1])
        {
          // If there's no time difference between the first two, then we can
          // assume that all files have the same time info and avoid reading all
          // the files.
          for (cc = cc + 1; cc < fmax; ++cc)
          {
            this->Information.push_back(tinfo);
          }
        }
      }
    }
  }

  this->Broadcast(0);

  // Let's determine if the file series is a temporal series, a spatial series
  // Currently we don't support a combination of both -- we should look into
  // adding support for that in the future.

  // If the time information are identical for 1st and 2nd files, then we
  // assume it's a collection of partitioned files.
  this->PartitionedFiles =
    (this->Information.size() >= 2 && this->Information[0] == this->Information[1]);

  if (this->PartitionedFiles)
  {
    this->AggregatedTimeSteps = this->Information[0].GetTimeSteps();
    this->AggregatedTimeRangeValid = this->Information[0].GetTimeRangeValid();
    this->AggregatedTimeRange = this->Information[0].GetTimeRange();
  }
  else
  {
    this->AggregatedTimeRange =
      std::pair<double, double>(vtkTypeTraits<double>::Max(), vtkTypeTraits<double>::Min());
    for (size_t cc = 0; cc < this->Information.size(); ++cc)
    {
      const vtkTimeInformation& cur = this->Information[cc];
      std::copy(cur.GetTimeSteps().begin(), cur.GetTimeSteps().end(),
        std::back_inserter(this->AggregatedTimeSteps));
      if (cur.GetTimeRangeValid())
      {
        this->AggregatedTimeRangeValid = true;
        this->AggregatedTimeRange.first =
          std::min(this->AggregatedTimeRange.first, cur.GetTimeRange().first);
        this->AggregatedTimeRange.second =
          std::max(this->AggregatedTimeRange.second, cur.GetTimeRange().second);
      }
    }
  }

  // let's check if we need to do something about SILs.
  this->UpdateSIL(reader, setFileName);

  this->UpdateInformationTime.Modified();
  return true;
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::Broadcast(int srcRank)
{
  if (this->Controller == NULL || this->Controller->GetNumberOfProcesses() <= 1)
  {
    return;
  }

  // Broadcast this->Information.
  if (this->Controller->GetLocalProcessId() == srcRank)
  {
    vtkMultiProcessStream stream;
    stream << static_cast<unsigned int>(this->Information.size());
    for (size_t cc = 0; cc < this->Information.size(); ++cc)
    {
      this->Information[cc].Save(stream);
    }
    this->Controller->Broadcast(stream, srcRank);
  }
  else
  {
    vtkMultiProcessStream stream;
    this->Controller->Broadcast(stream, srcRank);
    unsigned int count;
    stream >> count;
    this->Information.resize(count);
    for (unsigned int cc = 0; cc < count; ++cc)
    {
      this->Information[cc].Load(stream);
    }
  }
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::FillTimeInformation(vtkInformation* info) const
{
  if (this->AggregatedTimeRangeValid)
  {
    double trange[2] = { this->AggregatedTimeRange.first, this->AggregatedTimeRange.second };
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), trange, 2);
  }
  else
  {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  if (this->AggregatedTimeSteps.size() > 0)
  {
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->AggregatedTimeSteps[0],
      static_cast<int>(this->AggregatedTimeSteps.size()));
  }
  else
  {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkFileSeriesHelper::GetActiveFiles(vtkInformation* outInfo) const
{
  std::vector<std::string> activeFiles;

  typedef vtkStreamingDemandDrivenPipeline SDDP;

  bool hasTime = outInfo->Has(SDDP::UPDATE_TIME_STEP()) != 0;
  double time = hasTime ? outInfo->Get(SDDP::UPDATE_TIME_STEP()) : 0.0;
  int tindex = 0;

  if (hasTime)
  {
    // let's clamp the time to available timesteps.
    if (this->AggregatedTimeSteps.size() > 0)
    {
      tindex = vtkFileSeriesHelperNS::GetTimeStepIndex(
        time, &this->AggregatedTimeSteps[0], static_cast<int>(this->AggregatedTimeSteps.size()));
    }
  }

  if (tindex < 0 || tindex >= static_cast<int>(this->AggregatedTimeSteps.size()))
  {
    return activeFiles;
  }

  // clamp time to actual timestep available in the files.
  time = this->AggregatedTimeSteps[tindex];

  // build up activeFiles to match files that provide the requested timestep.
  for (size_t cc = 0; cc < this->Information.size(); ++cc)
  {
    const vtkTimeInformation& tinfo = this->Information[cc];
    if (tinfo.GetTimeStepsValid() &&
      std::find(tinfo.GetTimeSteps().begin(), tinfo.GetTimeSteps().end(), time) !=
        tinfo.GetTimeSteps().end())
    {
      activeFiles.push_back(this->FileNames[cc]);
    }
    else if (tinfo.GetTimeRangeValid() && tinfo.GetTimeRange().first <= time &&
      time <= tinfo.GetTimeRange().second)
    {
      activeFiles.push_back(this->FileNames[cc]);
    }
  }

  if (this->PartitionedFiles == false)
  {
    // This means the file series is a temporal file series. If so,
    // all files providing the timestep requested are processed by the current
    // rank.
    return activeFiles;
  }

  // activeFiles now tells us all the files that provide the timestep of
  // interest. If this->PartitionedFiles, we distribute the files among ranks.
  int piece = 0, numPieces = 1;
  if (this->Controller)
  {
    piece = this->Controller->GetLocalProcessId();
    numPieces = this->Controller->GetNumberOfProcesses();
  }

  if (outInfo->Has(SDDP::UPDATE_PIECE_NUMBER()) && outInfo->Has(SDDP::UPDATE_NUMBER_OF_PIECES()))
  {
    piece = outInfo->Get(SDDP::UPDATE_PIECE_NUMBER());
    numPieces = outInfo->Get(SDDP::UPDATE_NUMBER_OF_PIECES());
  }

  return this->SplitFiles(activeFiles, piece, numPieces);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkFileSeriesHelper::SplitFiles(
  const std::vector<std::string>& activeFiles, int piece, int numPieces) const
{
  if (!this->PartitionedFiles || numPieces <= 1)
  {
    return activeFiles;
  }

  int numFiles = static_cast<int>(activeFiles.size());
  if (numFiles > numPieces)
  {
    int filesPerRank = numFiles / numPieces;
    int leftover = numFiles % numPieces;

    int index = piece * filesPerRank + ((piece < leftover) ? piece : leftover);
    int count = filesPerRank + ((piece < leftover) ? 1 : 0);

    assert(index + count <= numFiles);

    std::vector<std::string> myfiles(count);
    for (int cc = 0; cc < count; ++cc)
    {
      myfiles[cc] = activeFiles[index + cc];
    }
    return myfiles;
  }
  else
  {
    std::vector<std::string> retval;
    if (piece < numFiles)
    {
      retval.push_back(activeFiles[piece]);
    }
    return retval;
  }
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::UpdateSIL(
  vtkAlgorithm* reader, const vtkFileSeriesHelper::FileNameFunctorType& setFileName)
{
  if (this->SIL == nullptr || reader == nullptr)
  {
    return;
  }

  // capture current selection
  auto currentSelection = this->SIL->GetSelection();
  this->SIL->Initialize();

  // SIL needs special handling for file series.

  //--------------------------------------------------------------------------
  // For temporal file series, we assume that the SIL from the 1st file on the
  // root node is good enough. Let's obtain that and share it with everyone.
  if (this->PartitionedFiles == false)
  {
    if (this->Controller == nullptr || this->Controller->GetLocalProcessId() == 0)
    {
      setFileName(reader, this->FileNames[0]);
      reader->UpdateInformation();
      this->SIL->DeepCopy(vtkSubsetInclusionLattice::GetSIL(reader->GetOutputInformation(0)));
    }
    this->Broadcast(this->SIL, 0);
  }

  //--------------------------------------------------------------------------
  // For partitioned files, since the structure need not be identical in all
  // partitioned files, we need to build a combined structure ourselves. We do
  // that by making each rank read the file-set it's intended to read and then
  // share the sil with each other.
  if (this->PartitionedFiles == true)
  {
    auto activeFiles = this->Controller != nullptr
      ? this->SplitFiles(this->FileNames, this->Controller->GetLocalProcessId(),
          this->Controller->GetNumberOfProcesses())
      : this->FileNames;

    for (auto fname : activeFiles)
    {
      setFileName(reader, fname);
      reader->UpdateInformation();
      this->SIL->Merge(vtkSubsetInclusionLattice::GetSIL(reader->GetOutputInformation(0)));
    }
    this->AllGather(this->SIL);
  }

  // restore selection state.
  if (!currentSelection.empty())
  {
    this->SIL->SetSelection(currentSelection);
  }
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::Broadcast(vtkSubsetInclusionLattice* sil, int srcRank)
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1 && sil)
  {
    if (this->Controller->GetLocalProcessId() == srcRank)
    {
      std::string data = sil->Serialize();
      vtkIdType len = static_cast<vtkIdType>(data.size() + 1);
      this->Controller->Broadcast(&len, 1, srcRank);
      this->Controller->Broadcast(const_cast<char*>(data.c_str()), len, srcRank);
    }
    if (this->Controller->GetLocalProcessId() > 0)
    {
      vtkIdType len = 0;
      this->Controller->Broadcast(&len, 1, srcRank);
      char* data = new char[len];
      this->Controller->Broadcast(data, len, srcRank);
      sil->Deserialize(data);
      delete[] data;
    }
  }
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::AllGather(vtkSubsetInclusionLattice* sil)
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1 && sil)
  {
    int numRanks = this->Controller->GetNumberOfProcesses();

    std::string data = sil->Serialize();
    const vtkIdType len = static_cast<vtkIdType>(data.size() + 1);

    std::vector<vtkIdType> lengths(numRanks);
    this->Controller->AllGather(&len, &lengths[0], 1);

    std::vector<vtkIdType> offsets(numRanks);
    vtkIdType totalLen = 0;
    for (int rank = 0; rank < numRanks; ++rank)
    {
      offsets[rank] = totalLen;
      totalLen += lengths[rank];
    }

    std::vector<char> all_data(totalLen);
    this->Controller->AllGatherV(data.c_str(), &all_data[0], len, &lengths[0], &offsets[0]);

    for (int rank = 0; rank < numRanks; ++rank)
    {
      vtkNew<vtkSubsetInclusionLattice> tmp;
      tmp->Deserialize(&all_data[offsets[rank]]);
      sil->Merge(tmp);
    }
  }
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::SetSIL(vtkSubsetInclusionLattice* asil)
{
  if (this->SIL != asil)
  {
    this->SIL = asil;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice* vtkFileSeriesHelper::GetSIL() const
{
  return this->SIL;
}

//----------------------------------------------------------------------------
void vtkFileSeriesHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "IgnoreReaderTime: " << this->IgnoreReaderTime << endl;
}
