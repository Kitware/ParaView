/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMultiResolutionGenericIOReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMultiResolutionGenericIOReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArraySelection.h"
#include "vtkFileSeriesReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPGenericIOMultiBlockReader.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtk_jsoncpp.h"

#include "vtksys/FStream.hxx"

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <string>

struct resolution_t
{
  std::string FileName;
  vtkSmartPointer<vtkFileSeriesReader> Reader;
};

class vtkPMultiResolutionGenericIOReader::vtkInternal
{
public:
  std::vector<resolution_t> Resolutions;
  int NumberOfBlocksPerLevel;

  void AddLevel(int level)
  {
    resolution_t newLevel;
    newLevel.Reader = vtkSmartPointer<vtkFileSeriesReader>::New();
    vtkNew<vtkPGenericIOMultiBlockReader> internalReader;
    newLevel.Reader->SetReader(internalReader.GetPointer());
    newLevel.Reader->SetFileNameMethod("SetFileName");
    this->Resolutions.insert(this->Resolutions.begin() + level, newLevel);
  }
#define JSON_READ_ERROR()                                                                          \
  std::cerr << "Error reading in JSON" << std::endl;                                               \
  return false

  vtkPGenericIOMultiBlockReader* GetReaderForLevel(int level)
  {
    return vtkPGenericIOMultiBlockReader::SafeDownCast(
      this->Resolutions[level].Reader->GetReader());
  }

  void Clear()
  {
    this->Resolutions.clear();
    this->NumberOfBlocksPerLevel = -1;
  }

  bool ParseJson(const std::string& parentDir, std::istream& jsonIn)
  {
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;

    Json::Value root;
    if (!parseFromStream(builder, jsonIn, &root, nullptr))
    {
      JSON_READ_ERROR();
    }
    if (!root.isMember("levels"))
    {
      JSON_READ_ERROR();
    }
    const Json::Value& levelsArray = root.get("levels", Json::Value::nullSingleton());
    if (!levelsArray.isArray())
    {
      JSON_READ_ERROR();
    }
    for (Json::Value::ArrayIndex i = 0; i < levelsArray.size(); ++i)
    {
      const Json::Value& level = levelsArray[i];
      if (!level.isMember("timesteps"))
      {
        JSON_READ_ERROR();
      }
      const Json::Value& timestepsArray = level.get("timesteps", Json::Value::nullSingleton());
      if (!timestepsArray.isArray())
      {
        JSON_READ_ERROR();
      }
      this->AddLevel(i);
      std::map<double, std::string> timeFiles;
      for (Json::Value::ArrayIndex j = 0; j < timestepsArray.size(); ++j)
      {
        const Json::Value& timestep = timestepsArray[j];
        if (!(timestep.isMember("time") && timestep.isMember("file")))
        {
          JSON_READ_ERROR();
        }
        const Json::Value& time = timestep.get("time", Json::Value::nullSingleton());
        const Json::Value& file = timestep.get("file", Json::Value::nullSingleton());
        if (!time.isNumeric())
        {
          JSON_READ_ERROR();
        }
        double t = time.asDouble();
        std::string f = file.asString();
        // allow paths relative to the config file
        if (f[0] != '/')
        {
          f = parentDir + f;
        }
        timeFiles.insert(std::pair<double, std::string>(t, f));
      }
      for (std::map<double, std::string>::iterator itr = timeFiles.begin(); itr != timeFiles.end();
           ++itr)
      {
        // vtkFileSeriesReader ignores time on inputs unless they already have
        // a time range, but iteration over the map ensures they are in order...
        // Eventually we may need to take into account the time in the JSON,
        // which is why it is there.
        this->Resolutions[i].Reader->AddFileName(itr->second.c_str());
      }
    }
    return true;
  }
};

vtkStandardNewMacro(vtkPMultiResolutionGenericIOReader);
//----------------------------------------------------------------------------
vtkPMultiResolutionGenericIOReader::vtkPMultiResolutionGenericIOReader()
{
  this->Internal = new vtkInternal();
  this->FileName = nullptr;
  this->Internal->NumberOfBlocksPerLevel = -1;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->XAxisVariableName = nullptr;
  this->YAxisVariableName = nullptr;
  this->ZAxisVariableName = nullptr;

  this->SetXAxisVariableName("x");
  this->SetYAxisVariableName("y");
  this->SetZAxisVariableName("z");

  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
    &vtkPMultiResolutionGenericIOReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
}

//----------------------------------------------------------------------------
vtkPMultiResolutionGenericIOReader::~vtkPMultiResolutionGenericIOReader()
{
  this->SetFileName(nullptr);
  delete this->Internal;
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->PointDataArraySelection->Delete();
}

//----------------------------------------------------------------------------
void vtkPMultiResolutionGenericIOReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPMultiResolutionGenericIOReader::CanReadFile(const char* fName)
{
  std::string filename(fName);
  std::string::size_type it = filename.find_last_of(".");
  std::string extension(filename, it + 1, filename.length());
  return extension == "gios";
}

//----------------------------------------------------------------------------
static inline bool SetStringProperty(char*& destStr, const char* newStr)
{
  if (destStr == nullptr && newStr == nullptr)
    return false;
  if (destStr && newStr && strcmp(destStr, newStr) == 0)
    return false;
  delete[] destStr;
  if (newStr)
  {
    size_t n = strlen(newStr) + 1;
    char* cp1 = new char[n];
    const char* cp2 = newStr;
    destStr = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
  else
  {
    destStr = nullptr;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPMultiResolutionGenericIOReader::SetFileName(const char* fname)
{
  this->Internal->Clear();
  if (!SetStringProperty(this->FileName, fname))
    return;
  if (fname)
  {
    vtksys::ifstream fis(this->FileName);
    std::string filename(this->FileName);
    std::string::size_type idx = filename.find_last_of("/");
    std::string parentDir(filename, 0, idx + 1);
    if (!this->Internal->ParseJson(parentDir, fis))
    {
      // clear any partially read data if reading failed for some reason
      this->Internal->Resolutions.clear();
    }
    else
    {
      // sync selected data arrays
      this->PointDataArraySelection->CopySelections(
        this->Internal->GetReaderForLevel(0)->GetPointDataArraySelection());
      // sync x,y,z axes
      for (int i = 0; i < this->GetNumberOfLevels(); ++i)
      {
        vtkPGenericIOMultiBlockReader* r = this->Internal->GetReaderForLevel(i);
        r->SetXAxisVariableName(this->XAxisVariableName);
        r->SetYAxisVariableName(this->YAxisVariableName);
        r->SetZAxisVariableName(this->ZAxisVariableName);
      }
    }
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPMultiResolutionGenericIOReader::SetXAxisVariableName(const char* arg)
{
  if (SetStringProperty(this->XAxisVariableName, arg))
  {
    this->Modified();
    for (unsigned i = 0; i < this->Internal->Resolutions.size(); ++i)
    {
      this->Internal->GetReaderForLevel(i)->SetXAxisVariableName(arg);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPMultiResolutionGenericIOReader::SetYAxisVariableName(const char* arg)
{
  if (SetStringProperty(this->YAxisVariableName, arg))
  {
    this->Modified();
    for (unsigned i = 0; i < this->Internal->Resolutions.size(); ++i)
    {
      this->Internal->GetReaderForLevel(i)->SetYAxisVariableName(arg);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPMultiResolutionGenericIOReader::SetZAxisVariableName(const char* arg)
{
  if (SetStringProperty(this->ZAxisVariableName, arg))
  {
    this->Modified();
    for (unsigned i = 0; i < this->Internal->Resolutions.size(); ++i)
    {
      this->Internal->GetReaderForLevel(i)->SetZAxisVariableName(arg);
    }
  }
}

//----------------------------------------------------------------------------
vtkStringArray* vtkPMultiResolutionGenericIOReader::GetArrayList()
{
  if (this->GetNumberOfLevels() > 0)
  {
    return this->Internal->GetReaderForLevel(0)->GetArrayList();
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
bool vtkPMultiResolutionGenericIOReader::InsertLevel(const char* fileName, int level)
{
  if (fileName == nullptr)
  {
    return false;
  }
  assert(this->Internal != nullptr);
  if (level < 0 || static_cast<unsigned>(level) > this->Internal->Resolutions.size())
  {
    vtkErrorMacro(<< "Level is out of range.");
    return false;
  }
  // this->Internal->AddLevel(level,fileName);

  if (this->GetNumberOfLevels() != 0)
  {
    this->Internal->GetReaderForLevel(level)->GetPointDataArraySelection()->CopySelections(
      this->PointDataArraySelection);
  }
  else
  {
    this->PointDataArraySelection->CopySelections(
      this->Internal->GetReaderForLevel(level)->GetPointDataArraySelection());
  }
  this->Modified();
  return true;
}

//----------------------------------------------------------------------------
int vtkPMultiResolutionGenericIOReader::GetNumberOfLevels() const
{
  assert(this->Internal != nullptr);
  return this->Internal->Resolutions.size();
}

//----------------------------------------------------------------------------
const char* vtkPMultiResolutionGenericIOReader::GetFileNameForLevel(int level) const
{
  assert(this->Internal != nullptr);
  if (level < 0 || static_cast<unsigned>(level) >= this->Internal->Resolutions.size())
  {
    return (const char*)nullptr;
  }
  return this->Internal->Resolutions[level].FileName.c_str();
}

//----------------------------------------------------------------------------
void vtkPMultiResolutionGenericIOReader::RemoveAllLevels()
{
  this->Internal->Resolutions.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPMultiResolutionGenericIOReader::SelectionModifiedCallback(vtkObject* vtkNotUsed(caller),
  unsigned long vtkNotUsed(eid), void* clientdata, void* vtkNotUsed(calldata))
{
  assert(clientdata != nullptr);
  vtkPMultiResolutionGenericIOReader* reader =
    static_cast<vtkPMultiResolutionGenericIOReader*>(clientdata);
  for (int i = 0; i < reader->GetNumberOfLevels(); ++i)
  {
    reader->Internal->GetReaderForLevel(i)->GetPointDataArraySelection()->CopySelections(
      reader->PointDataArraySelection);
  }
  reader->Modified();
}

//------------------------------------------------------------------------------
int vtkPMultiResolutionGenericIOReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkPMultiResolutionGenericIOReader::GetPointArrayName(int i)
{
  assert("pre: array index is out-of-bounds!" && (i >= 0) && (i < this->GetNumberOfPointArrays()));
  return this->PointDataArraySelection->GetArrayName(i);
}

//------------------------------------------------------------------------------
int vtkPMultiResolutionGenericIOReader::GetPointArrayStatus(const char* name)
{
  assert(this->PointDataArraySelection->ArrayExists(name));
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkPMultiResolutionGenericIOReader::SetPointArrayStatus(const char* name, int status)
{
  if (!this->PointDataArraySelection->ArrayExists(name))
  {
    assert(this->Internal->Resolutions.size() > 1);
    this->PointDataArraySelection->CopySelections(
      this->Internal->GetReaderForLevel(0)->GetPointDataArraySelection());
    vtkPMultiResolutionGenericIOReader::SelectionModifiedCallback(
      nullptr, 0l, (void*)this, nullptr);
    assert(this->PointDataArraySelection->ArrayExists(name));
  }
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
  for (int i = 0; i < this->GetNumberOfLevels(); ++i)
  {
    if (this->Internal->GetReaderForLevel(i)->GetArrayList()->LookupValue(name) >= 0)
    {
      this->Internal->GetReaderForLevel(i)->SetPointArrayStatus(name, status);
    }
  }
}

void printBoundsOfBlockInfo(vtkMultiBlockDataSet* ds, int offset)
{
  for (unsigned i = 0; i < ds->GetNumberOfBlocks(); ++i)
  {
    vtkInformation* info = ds->GetMetaData(i);
    std::cerr << "Bounds for block " << (i + offset) << ": ";
    if (info->Has(vtkCompositeDataPipeline::BOUNDS()))
    {
      double bb[6];
      info->Get(vtkCompositeDataPipeline::BOUNDS(), bb);
      std::cerr << "[ ";
      for (int j = 0; j < 6; ++j)
      {
        std::cerr << bb[j] << ", ";
      }
      std::cerr << "]" << std::endl;
    }
  }
}

//----------------------------------------------------------------------------
int vtkPMultiResolutionGenericIOReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // call the superclass's method
  this->Superclass::RequestInformation(request, inputVector, outputVector);

  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  assert(outInfo != nullptr);

  // this filter does handle streaming pieces of the dataset
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  // create a dummy dataset that will have the right structure
  vtkSmartPointer<vtkMultiBlockDataSet> infoSet = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  infoSet->SetNumberOfBlocks(this->GetNumberOfLevels());
  this->Internal->NumberOfBlocksPerLevel = -1;

  // request the information from each internal reader and put them
  // into the information object for this reader as blocks
  for (unsigned i = 0; i < this->Internal->Resolutions.size(); ++i)
  {
    vtkMultiBlockDataSet* dataSet;
    this->Internal->Resolutions[i].Reader->ProcessRequest(request, inputVector, outputVector);
    if (i == 0)
    {
      this->PointDataArraySelection->CopySelections(
        this->Internal->GetReaderForLevel(0)->GetPointDataArraySelection());
    }
    dataSet = vtkMultiBlockDataSet::SafeDownCast(
      outInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));
    // also make sure that they have the same number of blocks
    // (important for this kind of data)
    if (this->Internal->NumberOfBlocksPerLevel == -1)
    {
      this->Internal->NumberOfBlocksPerLevel = dataSet->GetNumberOfBlocks();
    }
    else if ((unsigned)(this->Internal->NumberOfBlocksPerLevel) != dataSet->GetNumberOfBlocks())
    {
      vtkErrorMacro(<< "Wrong number of blocks in level " << i);
      return 0;
    }

    //    printBoundsOfBlockInfo(dataSet,i * this->Internal->NumberOfBlocksPerLevel);
    infoSet->SetBlock(i, dataSet);
    outInfo->Remove(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA());
  }
  // We assume all datasets have blocks with the same bounds.  The rest of the pipeline
  // does not know this, so this loop first finds which resolution has bounds and copies
  // those bounds to all the other resolutions for each block
  for (int i = 0; i < this->Internal->NumberOfBlocksPerLevel; ++i) // for each block
  {
    double bounds[6] = { 0, 0, 0, 0, 0, 0 };
    // find the resolution with nonzero bounds (zero is default from GenericIO reader
    for (unsigned j = 0; j < this->Internal->Resolutions.size(); ++j)
    {
      if (bounds[0] == 0 && bounds[1] == 0)
      {
        static_cast<vtkMultiBlockDataSet*>(infoSet->GetBlock(j))
          ->GetMetaData(i)
          ->Get(vtkStreamingDemandDrivenPipeline::BOUNDS(), bounds);
      }
    }
    // copy bounds to all resolutions
    for (unsigned j = 0; j < this->Internal->Resolutions.size(); ++j)
    {
      static_cast<vtkMultiBlockDataSet*>(infoSet->GetBlock(j))
        ->GetMetaData(i)
        ->Set(vtkStreamingDemandDrivenPipeline::BOUNDS(), bounds, 6);
    }
  }

  std::cerr.flush();
  // return the computed object as the composite data metadata
  outInfo->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), infoSet.GetPointer());
  return 1;
}

//----------------------------------------------------------------------------
// This function is a binary search that returns the index of where the item
// should be inserted if it is not already present.  More precisely, it
// returns the minimum index i such that vec[i] <= value && value < vec[i+1].
// If the item is greater than every item in the array, it returns the size of
// the array and if the item is less than every item in the array it returns 0.
// NOTE: as per binary search, vec should be a sorted array of ints
int binSearch(int* vec, int size, int value)
{
  // handle boundary conditions
  if (vec[0] > value)
  {
    return 0;
  }
  if (vec[size - 1] < value)
  {
    return size;
  }
  // loop invariants:
  // vec[min] is always <= value
  // vec[max] is always > value
  int min = 0, max = size;
  while (min < max - 1)
  {
    int mid = (min + max) / 2;
    if (vec[mid] <= value)
    {
      min = mid;
    }
    else
    {
      max = mid;
    }
  }
  return min;
}

int vtkPMultiResolutionGenericIOReader::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // The vtkFileSeriesReader needs this to be called to update its timestep
  for (unsigned i = 0; i < this->Internal->Resolutions.size(); ++i)
  {
    this->Internal->Resolutions[i].Reader->ProcessRequest(request, inputVector, outputVector);
    this->Internal->GetReaderForLevel(i)->GetPointDataArraySelection()->CopySelections(
      this->PointDataArraySelection);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPMultiResolutionGenericIOReader::RequestData(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Get the output dataset
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  assert(outInfo != nullptr);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  assert(output != nullptr);
  // set the number of blocks
  output->SetNumberOfBlocks(this->GetNumberOfLevels());

  int size;
  std::vector<int> idVector;
  // find out which parts of the dataset we need to load
  if (outInfo->Has(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS()))
  {
    int* ids;
    size = outInfo->Length(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    ids = outInfo->Get(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    idVector.resize(size);
    std::copy(ids, ids + size, idVector.begin());
  }
  // default to loading all of the lowest level of detail
  else
  {
    for (int j = 0; j < this->Internal->NumberOfBlocksPerLevel; ++j)
    {
      idVector.push_back(j);
    }
    size = idVector.size();
  }

  // sort the requested blocks
  std::sort(idVector.begin(), idVector.end());
  // compute the block ids relative to the file (the internal reader needs these)
  std::vector<int> localIdVector;
  for (unsigned i = 0; i < idVector.size(); ++i)
  {
    localIdVector.push_back(idVector[i] % this->Internal->NumberOfBlocksPerLevel);
  }
  // these mark the beginning and end of the local ids for the internal reader
  // for the current itertion of the loop
  int lBound = 0, uBound;
  for (int i = 0; i < this->GetNumberOfLevels(); ++i)
  {
    // compute new bounds for current reader's blocks
    uBound =
      binSearch(&idVector[0], idVector.size(), this->Internal->NumberOfBlocksPerLevel * (i + 1));
    int levelSize = uBound - lBound;

    vtkNew<vtkMultiBlockDataSet> dataset;
    if (levelSize > 0)
    {
      // create a new request
      vtkNew<vtkInformation> internalRequestData;
      internalRequestData->Set(vtkDemandDrivenPipeline::REQUEST_DATA());
      internalRequestData->Copy(request);

      // create a new output info
      vtkNew<vtkInformation> internalOutInfo;
      internalOutInfo->CopyEntry(outInfo, vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      internalOutInfo->CopyEntry(outInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      internalOutInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));
      internalOutInfo->Set(vtkDataObject::DATA_OBJECT(), dataset.GetPointer());
      internalOutInfo->Set(
        vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), &localIdVector[lBound], levelSize);
      internalOutInfo->Set(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1);

      // put the output info in a vector
      vtkNew<vtkInformationVector> internalOutVector;
      internalOutVector->Append(internalOutInfo.GetPointer());

      // ask the internal reader for its data
      this->Internal->Resolutions[i].Reader->ProcessRequest(internalRequestData.GetPointer(),
        (vtkInformationVector**)nullptr, internalOutVector.GetPointer());
      // set the block in our output to the output of the internal reader
    }
    else
    {
      dataset->SetNumberOfBlocks(this->Internal->NumberOfBlocksPerLevel);
    }
    output->SetBlock(i, dataset.GetPointer());
    // the next reader's blocks will start where this one's blocks ended
    lBound = uBound;
    if (i == 0)
    {
      output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),
        dataset->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP()));
    }
  }
  return 1;
}
