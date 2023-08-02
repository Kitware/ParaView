// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkParallelSerialWriter.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDataSet.h"
#include "vtkFileSeriesWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkReductionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <vtksys/SystemTools.hxx>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

namespace
{
bool vtkIsEmpty(vtkDataObject* dobj)
{
  for (int cc = 0; (dobj != nullptr) && (cc < vtkDataObject::NUMBER_OF_ASSOCIATIONS); ++cc)
  {
    if (dobj->GetNumberOfElements(cc) > 0)
    {
      return false;
    }
  }
  return true;
}
}

vtkStandardNewMacro(vtkParallelSerialWriter);
vtkCxxSetObjectMacro(vtkParallelSerialWriter, Writer, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkParallelSerialWriter, PreGatherHelper, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkParallelSerialWriter, PostGatherHelper, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkParallelSerialWriter, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkParallelSerialWriter::vtkParallelSerialWriter()
  : NumberOfIORanks(1)
  , RankAssignmentMode(vtkParallelSerialWriter::ASSIGNMENT_MODE_CONTIGUOUS)
  , Controller(nullptr)
  , SubController(nullptr)
{
  this->SetNumberOfOutputPorts(0);

  this->Writer = nullptr;

  this->FileNameMethod = nullptr;
  this->FileName = nullptr;
  this->FileNameSuffix = nullptr;

  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;

  this->PreGatherHelper = nullptr;
  this->PostGatherHelper = nullptr;

  this->WriteAllTimeSteps = 0;
  this->NumberOfTimeSteps = 0;
  this->CurrentTimeIndex = 0;

  this->Interpreter = nullptr;
  this->SetInterpreter(vtkClientServerInterpreterInitializer::GetGlobalInterpreter());
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkParallelSerialWriter::~vtkParallelSerialWriter()
{
  this->SetWriter(nullptr);
  this->SetFileNameMethod(nullptr);
  this->SetFileName(nullptr);
  this->SetFileNameSuffix(nullptr);
  this->SetPreGatherHelper(nullptr);
  this->SetPostGatherHelper(nullptr);
  this->SetInterpreter(nullptr);
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    vtkErrorMacro("No input provided!");
    return 0;
  }

  // always write even if the data hasn't changed
  this->Modified();

  this->Update();
  return 1;
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NumberOfPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->Piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);

  double* inTimes =
    inputVector[0]->GetInformationObject(0)->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->WriteAllTimeSteps)
  {
    double timeReq = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkParallelSerialWriter::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->Writer)
  {
    vtkErrorMacro("No internal writer specified. Cannot write.");
    return 0;
  }

  if (!this->Controller)
  {
    vtkErrorMacro("No controller specified!");
    return 0;
  }

  if (!this->FileName || this->FileName[0] == '\0')
  {
    vtkErrorMacro("Invalid filename specified!");
    return 0;
  }

  bool write_all = (this->WriteAllTimeSteps != 0 && this->NumberOfTimeSteps > 0);
  if (write_all)
  {
    if (this->CurrentTimeIndex == 0)
    {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
  }
  else
  {
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
  }

  const int num_ranks = this->Controller->GetNumberOfProcesses();
  int num_io_ranks = std::min(this->NumberOfIORanks, num_ranks);
  num_io_ranks = num_io_ranks <= 0 ? num_ranks : num_io_ranks;
  if (num_io_ranks == 1)
  {
    this->SubController = nullptr;
    this->SubControllerColor = -1;
  }
  else
  {
    const int myid = this->Controller->GetLocalProcessId();
    if (this->RankAssignmentMode == ASSIGNMENT_MODE_CONTIGUOUS)
    {
      const int div = num_ranks / num_io_ranks;
      const int mod = num_ranks % num_io_ranks;
      const int r = myid / (div + 1);
      if (r < mod)
      {
        this->SubControllerColor = r;
      }
      else
      {
        this->SubControllerColor = mod + (myid - (div + 1) * mod) / div;
      }
    }
    else
    {
      this->SubControllerColor = myid % num_io_ranks;
    }
    assert(this->SubControllerColor >= 0 && this->SubControllerColor < num_io_ranks);
    this->SubController.TakeReference(
      this->Controller->PartitionController(this->SubControllerColor, myid));
  }

  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);

  // PartitionedDataSet (PD)/PartitionedDataSetCollection (PDC) make it much easier
  // to deal with blocks and partitions esp. in distributed environments.
  if (vtkCompositeDataSet::SafeDownCast(inputDO) != nullptr &&
    vtkPartitionedDataSet::SafeDownCast(inputDO) == nullptr)
  {
    vtkNew<vtkConvertToPartitionedDataSetCollection> converter;
    converter->SetInputDataObject(inputDO);
    converter->Update();

    const std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
    const std::string fnameNoExt =
      vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
    const std::string ext = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);

    auto pdc = converter->GetOutput();
    const auto numBlocks = pdc->GetNumberOfPartitionedDataSets();
    const int precision = numBlocks > 0 ? static_cast<int>(std::log10(numBlocks)) + 1 : 1;

    for (unsigned int cc = 0, max = pdc->GetNumberOfPartitionedDataSets(); cc < max; ++cc)
    {
      // Create filename for the block.
      auto fname = fmt::format("{0}/{1}{2:{3}}{4}", path, fnameNoExt, cc, precision, ext);
      this->WriteATimestep(fname, pdc->GetPartitionedDataSet(cc));
    }
  }
  else
  {
    vtkNew<vtkConvertToPartitionedDataSetCollection> converter;
    converter->SetInputDataObject(inputDO);
    converter->Update();
    auto pdc = converter->GetOutput();
    assert(pdc->GetNumberOfPartitionedDataSets() == 1);
    this->WriteATimestep(this->FileName, pdc->GetPartitionedDataSet(0));
  }

  if (write_all)
  {
    this->CurrentTimeIndex++;
    if (this->CurrentTimeIndex >= this->NumberOfTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
    }
  }

  this->SubController = nullptr;

  // A barrier at end to just sync up. This just makes it easier to write tests
  // etc.
  this->Controller->Barrier();
  return 1;
}

//----------------------------------------------------------------------------
void vtkParallelSerialWriter::WriteATimestep(const std::string& fname, vtkPartitionedDataSet* input)
{
  assert(input != nullptr);

  auto inputDO = vtk::MakeSmartPointer(vtkDataObject::SafeDownCast(input));
  auto controller = this->SubController ? this->SubController.GetPointer() : this->Controller;

  if (this->PreGatherHelper)
  {
    this->PreGatherHelper->SetInputDataObject(inputDO);
    this->PreGatherHelper->Update();
    inputDO = this->PreGatherHelper->GetOutputDataObject(0);
    this->PreGatherHelper->RemoveAllInputConnections(0);
  }

  // gather data to "root"; note this can be the root of the subcontroller.
  std::vector<vtkSmartPointer<vtkDataObject>> gatheredDataSets;
  controller->Gather(inputDO, gatheredDataSets, 0);
  if (controller->GetLocalProcessId() != 0)
  {
    // done.
    return;
  }
  assert(!gatheredDataSets.empty());

  // flatten the datasets.
  std::vector<vtkSmartPointer<vtkDataObject>> allDataSets;
  for (auto& dobj : gatheredDataSets)
  {
    const auto pieces = vtkCompositeDataSet::GetDataSets<vtkDataObject>(dobj);
    allDataSets.insert(allDataSets.end(), pieces.begin(), pieces.end());
  }
  gatheredDataSets.clear();

  // purge empty datasets from allDataSets.
  allDataSets.erase(std::remove_if(allDataSets.begin(), allDataSets.end(),
                      [](vtkDataObject* dobj) { return vtkIsEmpty(dobj); }),
    allDataSets.end());
  if (allDataSets.empty())
  {
    return;
  }

  if (this->PostGatherHelper)
  {
    for (auto piece : allDataSets)
    {
      this->PostGatherHelper->AddInputDataObject(piece);
    }
    this->PostGatherHelper->Update();
    inputDO = this->PostGatherHelper->GetOutputDataObject(0);
    this->PostGatherHelper->RemoveAllInputConnections(0);
  }
  else if (allDataSets.size() > 1)
  {
    vtkErrorMacro("PostGatherHelper was not specified. Unclear how to 'merge' partitions."
                  "Only 1st partition will be written out");
    inputDO = allDataSets.front();
  }
  else
  {
    inputDO = allDataSets.front();
  }

  // release memory.
  allDataSets.clear();
  this->WriteAFile(fname, inputDO);
}

//----------------------------------------------------------------------------
void vtkParallelSerialWriter::WriteAFile(const std::string& filename_arg, vtkDataObject* input)
{
  std::string filename = this->GetPartitionFileName(filename_arg);
  if (this->WriteAllTimeSteps)
  {
    std::string path = vtksys::SystemTools::GetFilenamePath(filename);
    std::string fnamenoext = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
    if (this->FileNameSuffix && vtkFileSeriesWriter::SuffixValidation(this->FileNameSuffix))
    {
      // Print this->CurrentTimeIndex to a string using this->FileNameSuffix as format
      char suffix[100];
      snprintf(suffix, 100, this->FileNameSuffix, this->CurrentTimeIndex);
      filename = fmt::format("{0}/{1}{2}{3}", path, fnamenoext, suffix, ext);
    }
    else
    {
      filename = fmt::format("{0}/{1}.{2}{3}", path, fnamenoext, this->CurrentTimeIndex, ext);
    }
  }

  this->Writer->SetInputDataObject(input);
  this->SetWriterFileName(filename.c_str());
  this->WriteInternal();
  this->Writer->RemoveAllInputConnections(0);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the internal reader is
// modified, then this object is modified as well.
vtkMTimeType vtkParallelSerialWriter::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  vtkMTimeType readerMTime;

  if (this->Writer)
  {
    readerMTime = this->Writer->GetMTime();
    mTime = (readerMTime > mTime ? readerMTime : mTime);
  }

  return mTime;
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::WriteInternal()
{
  if (this->Writer && this->FileNameMethod)
  {
    // Get the local process interpreter.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->Writer << "Write"
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }
}

//-----------------------------------------------------------------------------
std::string vtkParallelSerialWriter::GetPartitionFileName(const std::string& fname)
{
  if (this->SubController != nullptr && this->SubControllerColor >= 0)
  {
    std::string path = vtksys::SystemTools::GetFilenamePath(fname);
    std::string fnamenoext = vtksys::SystemTools::GetFilenameWithoutLastExtension(fname);
    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(fname);
    return path + "/" + fnamenoext + "-" + std::to_string(this->SubControllerColor) + ext;
  }
  return fname;
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::SetWriterFileName(const char* fname)
{
  if (this->Writer && this->FileName && this->FileNameMethod)
  {
    // Get the local process interpreter.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->Writer << this->FileNameMethod << fname
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }
}

//-----------------------------------------------------------------------------
void vtkParallelSerialWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
