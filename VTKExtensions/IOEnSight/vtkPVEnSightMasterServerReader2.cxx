/*=========================================================================

 Program:   ParaView
 Module:    vtkPVEnSightMasterServerReader2.cxx

 Copyright (c) Kitware, Inc.
 All rights reserved.
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPVEnSightMasterServerReader2.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPICommunicator.h"
#endif

#include "vtksys/FStream.hxx"

#include <string>
#include <vector>

#include <ctype.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightMasterServerReader2);

vtkCxxSetObjectMacro(vtkPVEnSightMasterServerReader2, Controller, vtkMultiProcessController);
vtkMultiProcessController* vtkPVEnSightMasterServerReader2::GetController()
{
  return this->Controller;
}

//----------------------------------------------------------------------------
class vtkPVEnSightMasterServerReader2Internal
{
public:
  std::vector<std::string> PieceFileNames;
  int EnSightVersion;
  int NumberOfTimeSets;
  int NumberOfOutputs;
  std::vector<int> CumulativeTimeSetSizes;
  std::vector<float> TimeSetValues;
  std::vector<vtkPGenericEnSightReader*> RealReaders;
};

//----------------------------------------------------------------------------
vtkPVEnSightMasterServerReader2::vtkPVEnSightMasterServerReader2()
{
  this->Internal = new vtkPVEnSightMasterServerReader2Internal;
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->InformationError = 0;
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
vtkPVEnSightMasterServerReader2::~vtkPVEnSightMasterServerReader2()
{
  int rIdx;
  this->SetController(nullptr);
  for (rIdx = static_cast<int>(this->Internal->RealReaders.size() - 1); rIdx >= 0; rIdx--)
  {
    this->Internal->RealReaders[rIdx]->Delete();
    this->Internal->RealReaders.pop_back();
  }
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << "\n";
  os << indent << "Error: " << this->InformationError << "\n";
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
}

//----------------------------------------------------------------------------
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
template <class T>
int vtkPVEnSightMasterServerReader2SyncValues(
  T* data, int numValues, int numPieces, vtkMultiProcessController* controller)
{
  // Compare values on all processes that will read real pieces.
  // Returns whether the values match.  If they match, all processes'
  // values are modified to match that of node 0.  This will leave the
  // values unchanged on processes that will read real data, but
  // inform the other processes of the proper values.

  if (!controller)
  {
    return VTK_ERROR;
  }

  vtkMPICommunicator* communicator =
    vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());

  if (!communicator)
  {
    if (controller->GetNumberOfProcesses() == 1)
    {
      return VTK_OK;
    }
    else
    {
      return VTK_ERROR;
    }
  }

  int numProcs = controller->GetNumberOfProcesses();
  int myid = controller->GetLocalProcessId();

  // Collect all the values to node 0.
  T* values = new T[numProcs * numValues];
  communicator->Gather(data, values, numValues, 0);

  int result = VTK_OK;
  // Node 0 compares its values to those from other processes that
  // will actually be reading data.
  if (myid == 0)
  {
    for (int i = 1; (result == VTK_OK) && (i < numPieces); i++)
    {
      for (int j = 0; (result == VTK_OK) && (j < numValues); j++)
      {
        if (values[i * numValues + j] != values[j])
        {
          result = VTK_ERROR;
        }
      }
    }
  }

  // Free buffer where values were collected.
  delete[] values;

  // Broadcast result of comparison to all processes.
  communicator->Broadcast(&result, 1, 0);

  // If the results were okay, broadcast the correct values to all
  // processes so that those that will not read can have the correct
  // values.
  if (result == VTK_OK)
  {
    communicator->Broadcast(data, numValues, 0);
  }

  return result;
}
#else
template <class T>
int vtkPVEnSightMasterServerReader2SyncValues(T*, int, int, vtkMultiProcessController*)
{
  return VTK_OK;
}
#endif

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  int i;

  // Assume no error will occur.
  this->InformationError = 0;

  // Make sure we have a controller.
  if (!this->Controller)
  {
    vtkErrorMacro("ExecuteInformation requires a Controller.");
    this->InformationError = 1;
    return 0;
  }

  // Parse the input file.  This will set NumberOfPieces.
  int parseResults[2];
  parseResults[0] = this->ParseMasterServerFile();
  parseResults[1] = (parseResults[0] == VTK_OK) ? this->NumberOfPieces : -1;

  // Make sure all nodes could parse the file and agree on the number
  // of pieces.
  if ((vtkPVEnSightMasterServerReader2SyncValues(
         parseResults, 2, this->Controller->GetNumberOfProcesses(), this->Controller) != VTK_OK) ||
    (parseResults[0] != VTK_OK))
  {
    vtkErrorMacro("Error parsing the master server file.");
    this->InformationError = 1;
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  for (unsigned int rIdx = 0; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    this->Internal->RealReaders[rIdx]->SetReadAllVariables(this->ReadAllVariables);
    this->Internal->RealReaders[rIdx]->SetFilePath(this->GetFilePath());
    this->Internal->RealReaders[rIdx]->SetByteOrder(this->ByteOrder);
    this->Internal->RealReaders[rIdx]->UpdateInformation();
  }

  // Compare case file versions.
  // Across pieces.
  for (unsigned int rIdx = 1; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    if (this->Internal->RealReaders[0]->GetEnSightVersion() !=
      this->Internal->RealReaders[rIdx]->GetEnSightVersion())
    {
      vtkErrorMacro("EnSight version mismatch across pieces.");
      this->InformationError = 1;
      return 0;
    }
  }
  // Across nodes.
  this->Internal->EnSightVersion = this->Internal->RealReaders[0]->GetEnSightVersion();

  if (vtkPVEnSightMasterServerReader2SyncValues(&this->Internal->EnSightVersion, 1,
        this->Controller->GetNumberOfProcesses(), this->Controller) != VTK_OK)
  {
    vtkErrorMacro("EnSight version mismatch across nodes.");
    this->InformationError = 1;
    return 0;
  }

  // Make sure all nodes could read their files.
  if (this->Internal->EnSightVersion < 0)
  {
    vtkErrorMacro("Error reading case file on at least one node.");
    this->InformationError = 1;
    return 0;
  }

  // Compare number of time sets.
  // Across pieces.
  vtkDataArrayCollection* timeSets = this->Internal->RealReaders[0]->GetTimeSets();
  vtkDataArrayCollection* pTimeSets;
  for (unsigned int rIdx = 1; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    pTimeSets = this->Internal->RealReaders[rIdx]->GetTimeSets();
    if ((timeSets ? timeSets->GetNumberOfItems() : 0) !=
      (pTimeSets ? pTimeSets->GetNumberOfItems() : 0))
    {
      vtkErrorMacro("Number of time sets not equal across pieces.");
      this->InformationError = 1;
      return 0;
    }
  }
  // Across nodes.
  this->Internal->NumberOfTimeSets = (timeSets ? timeSets->GetNumberOfItems() : 0);
  if (vtkPVEnSightMasterServerReader2SyncValues(&this->Internal->NumberOfTimeSets, 1,
        this->Controller->GetNumberOfProcesses(), this->Controller) != VTK_OK)
  {
    this->InformationError = 1;
    return 0;
  }

  // Compare the time sets' sizes.
  // Across pieces.
  for (i = 0; i < this->Internal->NumberOfTimeSets; ++i)
  {
    for (unsigned int rIdx = 1; rIdx < this->Internal->RealReaders.size(); rIdx++)
    {
      pTimeSets = this->Internal->RealReaders[rIdx]->GetTimeSets();
      if (timeSets->GetItem(i)->GetNumberOfTuples() != pTimeSets->GetItem(i)->GetNumberOfTuples())
      {
        vtkErrorMacro("Time set sizes not equal across pieces.");
        this->InformationError = 1;
        return 0;
      }
    }
  }
  // Across nodes.
  this->Internal->CumulativeTimeSetSizes.resize(this->Internal->NumberOfTimeSets + 1);
  this->Internal->CumulativeTimeSetSizes[0] = 0;
  for (i = 0; i < this->Internal->NumberOfTimeSets; ++i)
  {
    this->Internal->CumulativeTimeSetSizes[i + 1] =
      (this->Internal->CumulativeTimeSetSizes[i] + timeSets->GetItem(i)->GetNumberOfTuples());
  }

  if (vtkPVEnSightMasterServerReader2SyncValues(&this->Internal->CumulativeTimeSetSizes.at(0),
        this->Internal->NumberOfTimeSets + 1, this->Controller->GetNumberOfProcesses(),
        this->Controller) != VTK_OK)
  {
    vtkErrorMacro("Time set sizes not equal on all nodes.");
    this->InformationError = 1;
    return 0;
  }

  // All time sets have the correct size.  Now compare the values.
  // The CumulativeTimeSetSizes has the sizes on all nodes.

  // Compare time set values.
  // Across pieces.
  for (i = 0; i < this->Internal->NumberOfTimeSets; ++i)
  {
    vtkDataArray* array = timeSets->GetItem(i);
    vtkIdType numValues = array->GetNumberOfTuples();
    for (unsigned int rIdx = 1; rIdx < this->Internal->RealReaders.size(); rIdx++)
    {
      pTimeSets = this->Internal->RealReaders[rIdx]->GetTimeSets();
      vtkDataArray* pArray = pTimeSets->GetItem(i);
      for (vtkIdType j = 0; j < numValues; ++j)
      {
        if (array->GetTuple1(j) != pArray->GetTuple1(j))
        {
          vtkErrorMacro("Time set values do not match on across pieces.");
          this->InformationError = 1;
          return 0;
        }
      }
    }
  }
  // Across nodes.
  this->Internal->TimeSetValues.clear();
  // Fill in the time set values.
  for (i = 0; i < this->Internal->NumberOfTimeSets; ++i)
  {
    vtkDataArray* array = timeSets->GetItem(i);
    vtkIdType numValues = array->GetNumberOfTuples();
    for (vtkIdType j = 0; j < numValues; ++j)
    {
      this->Internal->TimeSetValues.push_back(array->GetTuple1(j));
    }
  }

  if (this->Internal->TimeSetValues.size() >= 1)
  {
    if (vtkPVEnSightMasterServerReader2SyncValues(&this->Internal->TimeSetValues.at(0),
          static_cast<int>(this->Internal->TimeSetValues.size()),
          this->Controller->GetNumberOfProcesses(), this->Controller) != VTK_OK)
    {
      vtkErrorMacro("Time set values do not match on all nodes.");
      this->InformationError = 1;
      return 0;
    }

    int nTimes = static_cast<int>(this->Internal->TimeSetValues.size());
    double timeRange[2];
    double* timeSteps = new double[nTimes];
    timeRange[0] = this->Internal->TimeSetValues[0];
    timeRange[1] = this->Internal->TimeSetValues[nTimes - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    for (i = 0; i < nTimes; ++i)
    {
      timeSteps[i] = this->Internal->TimeSetValues[i];
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeSteps[0], nTimes);
    delete[] timeSteps;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Do not execute if ExecuteInformation failed.
  if (this->InformationError)
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int tsLength = 0;
  double* steps = nullptr;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    tsLength = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) && steps != nullptr &&
    tsLength > 0)
  {
    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < tsLength - 1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    this->SetTimeValue(steps[cnt]);
  }

  output->Initialize();
  output->SetNumberOfBlocks(this->NumberOfPieces);

  for (unsigned int rIdx = 0; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    this->Internal->RealReaders[rIdx]->SetTimeValue(this->GetTimeValue());
    vtkMultiBlockDataSet* rout = this->Internal->RealReaders[rIdx]->GetOutput();
    this->Internal->RealReaders[rIdx]->UpdateInformation();
    this->Internal->RealReaders[rIdx]->Update();
    output->SetBlock(rIdx, rout);
  }

  return 1;
}

//----------------------------------------------------------------------------
static int vtkPVEnSightMasterServerReader2IsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
static int vtkPVEnSightMasterServerReader2StartsWith(const char* str1, const char* str2)
{
  if (!str1 || !str2 || strlen(str1) < strlen(str2))
  {
    return 0;
  }
  return !strncmp(str1, str2, strlen(str2));
}

//----------------------------------------------------------------------------
// Due to a buggy stream library on the HP and another on Mac OSX, we
// need this very carefully written version of getline.  Returns true
// if any data were read before the end-of-file was reached.
static int vtkPVEnSightMasterServerReader2GetLineFromStream(istream& is, std::string& line)
{
  const int bufferSize = 1024;
  char buffer[bufferSize];
  line = "";
  int haveData = 0;

  // If no characters are read from the stream, the end of file has
  // been reached.
  while ((is.getline(buffer, bufferSize), is.gcount() > 0))
  {
    char* p;
    int ppos, numCharsRead = is.gcount();

    haveData = 1;

    // Get rid of endline character.
    p = buffer;
    ppos = 0;
    while (ppos < bufferSize && ppos < numCharsRead && *p != '\0')
    {
      if (*p == '\r')
      {
        // Change existing method as little as possible.
        *p = '\0';
        line.append(buffer);
        return haveData;
      }
      ++p;
      ++ppos;
    }

    line.append(buffer);

    // If newline character was read, the gcount includes the
    // character, but the buffer does not.  The end of line has been
    // reached.
    if (strlen(buffer) < static_cast<size_t>(is.gcount()))
    {
      break;
    }

    // The fail bit may be set.  Clear it.
    is.clear(is.rdstate() & ~ios::failbit);
  }
  return haveData;
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::ParseMasterServerFile()
{
  // Clear old list of pieces.
  this->Internal->PieceFileNames.clear();
  // Construct the file name to open.
  std::string sfilename;
  if (!this->CaseFileName || this->CaseFileName[0] == 0)
  {
    vtkErrorMacro("A case file name must be specified.");
    return VTK_ERROR;
  }
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    sfilename += this->CaseFileName;
    vtkDebugMacro("Full path to file: " << sfilename.c_str());
  }
  else
  {
    sfilename = this->CaseFileName;
  }

  // Open the file for reading.
  vtksys::ifstream fin(sfilename.c_str(), ios::in);
  if (!fin)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return VTK_ERROR;
  }

  // A little state machine to read a sequence of lines.
  // First: Determine what kind of file this is.
  // We treat a case file like a degenerate sos file with one server.
  int readingFormat = 0;
  // Only case files have the SERVER tag.
  int readingServers = 0;
  int numServers = 0;
  // Read all data lines in the file.
  std::string line;
  while (vtkPVEnSightMasterServerReader2GetLineFromStream(fin, line))
  {
    // This section determines the type of file: case of SOS.
    if (!readingFormat && vtkPVEnSightMasterServerReader2StartsWith(line.c_str(), "FORMAT"))
    {
      // The FORMAT section starts here.
      readingFormat = 1;
    }
    else if (readingFormat && !numServers &&
      vtkPVEnSightMasterServerReader2StartsWith(line.c_str(), "type:"))
    {
      // Remove the leading "type:" to get the remaining value.
      const char* p = line.c_str();
      p += strlen("type:");
      // Remove leading spaces.
      while (*p && vtkPVEnSightMasterServerReader2IsSpace(*p))
      {
        ++p;
      }
      if (!*p)
      {
        vtkErrorMacro("Error parsing file type from: " << line.c_str());
        return VTK_ERROR;
      }
      // These are the special cases (case files).
      if (strncmp(p, "ensight", 7) == 0 || strncmp(p, "ensight gold", 12) == 0)
      {
        // Handle the case file line an sos file with one server.
        numServers = 1;
        this->Internal->PieceFileNames.push_back(this->CaseFileName);
        // We could exit the state machine here
        // because we have nothing else to do from this state.
        // We assume the line "SERVERS" will not appear in any case file.
      }
      else if (strncmp(p, "master_server gold", 18) != 0)
      { // We might as well have this sanity check here.
        vtkErrorMacro("Unexpected file type: '" << p << "'");
        return VTK_ERROR;
      }
      readingFormat = 0;
    }

    // The rest of this stuff is for picking the number of servers
    // and case file names from the SOS file.
    else if (!readingServers && vtkPVEnSightMasterServerReader2StartsWith(line.c_str(), "SERVERS"))
    {
      // The SERVERS section starts here.
      readingServers = 1;
    }
    else if (readingServers && !numServers &&
      vtkPVEnSightMasterServerReader2StartsWith(line.c_str(), "number of servers:"))
    {
      // Found the number of servers line.
      if (sscanf(line.c_str(), "number of servers: %i", &numServers) < 1)
      {
        vtkErrorMacro("Error parsing number of servers from: " << line.c_str());
        return VTK_ERROR;
      }
      // Check the number of servers is > 0.
      if (numServers < 1)
      {
        vtkErrorMacro("Number of servers specified by file is 0.");
        return VTK_ERROR;
      }
    }
    else if (readingServers && vtkPVEnSightMasterServerReader2StartsWith(line.c_str(), "casefile:"))
    {
      // Found a casefile line.  Record the name.
      const char* p = line.c_str();
      p += strlen("casefile:");
      while (*p && vtkPVEnSightMasterServerReader2IsSpace(*p))
      {
        ++p;
      }
      if (!*p)
      {
        vtkErrorMacro("Error parsing case file name from: " << line.c_str());
        return VTK_ERROR;
      }
      this->Internal->PieceFileNames.push_back(p);
    }
  }

  // Make sure we got the expected number of servers (pieces).
  if (static_cast<int>(this->Internal->PieceFileNames.size()) != numServers)
  {
    vtkErrorMacro("Number of servers specified by file does not match "
                  "actual number of servers provided by file.");
    return VTK_ERROR;
  }

  this->NumberOfPieces = numServers;

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::CanReadFile(const char* fname)
{
  // We may have to read quite a few lines of the file to do this test
  // for real.  Just check the extension.
  size_t len = strlen(fname);
  if ((len >= 4) && (strcmp(fname + len - 4, ".sos") == 0))
  {
    return 1;
  }
  else if ((len >= 5) && (strcmp(fname + len - 5, ".case") == 0))
  {
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader2::SetCaseFileName(const char* fileName)
{
  // If the CaseFileName is already set with the same file, do nothing.
  if (this->CaseFileName && fileName && (!strcmp(this->CaseFileName, fileName)))
  {
    return;
  }

  int rIdx;
  this->Superclass::SetCaseFileName(fileName);
  for (rIdx = static_cast<int>(this->Internal->RealReaders.size() - 1); rIdx >= 0; rIdx--)
  {
    this->Internal->RealReaders[rIdx]->Delete();
    this->Internal->RealReaders.pop_back();
  }
  if (this->ParseMasterServerFile() != VTK_OK)
  {
    vtkErrorMacro("Unable to parse master file");
    return;
  }
  for (rIdx = 0; rIdx < this->NumberOfPieces; rIdx++)
  {
    vtkPGenericEnSightReader* aReader = vtkPGenericEnSightReader::New();
    aReader->SetFilePath(this->GetFilePath());
    aReader->SetCaseFileName(this->Internal->PieceFileNames[rIdx].c_str());
    this->Internal->RealReaders.push_back(aReader);
  }
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::GetNumberOfPointArrays()
{
  return this->Internal->RealReaders.size() == 0
    ? 0
    : this->Internal->RealReaders[0]->GetNumberOfPointArrays();
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::GetNumberOfCellArrays()
{
  return this->Internal->RealReaders.size() == 0
    ? 0
    : this->Internal->RealReaders[0]->GetNumberOfCellArrays();
}

//----------------------------------------------------------------------------
const char* vtkPVEnSightMasterServerReader2::GetPointArrayName(int index)
{
  return this->Internal->RealReaders.size() == 0
    ? nullptr
    : this->Internal->RealReaders[0]->GetPointArrayName(index);
}

//----------------------------------------------------------------------------
const char* vtkPVEnSightMasterServerReader2::GetCellArrayName(int index)
{
  return this->Internal->RealReaders.size() == 0
    ? nullptr
    : this->Internal->RealReaders[0]->GetCellArrayName(index);
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::GetPointArrayStatus(const char* name)
{
  return this->Internal->RealReaders.size() == 0
    ? 0
    : this->Internal->RealReaders[0]->GetPointArrayStatus(name);
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::GetCellArrayStatus(const char* name)
{
  return this->Internal->RealReaders.size() == 0
    ? 0
    : this->Internal->RealReaders[0]->GetCellArrayStatus(name);
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader2::SetPointArrayStatus(const char* name, int status)
{
  for (unsigned int rIdx = 0; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    this->Internal->RealReaders[rIdx]->SetPointArrayStatus(name, status);
    this->Internal->RealReaders[rIdx]->Modified();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader2::SetCellArrayStatus(const char* name, int status)
{
  for (unsigned int rIdx = 0; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    this->Internal->RealReaders[rIdx]->SetCellArrayStatus(name, status);
    this->Internal->RealReaders[rIdx]->Modified();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader2::SetByteOrderToBigEndian()
{
  for (unsigned int rIdx = 0; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    this->Internal->RealReaders[rIdx]->SetByteOrderToBigEndian();
    this->Internal->RealReaders[rIdx]->Modified();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader2::SetByteOrderToLittleEndian()
{
  for (unsigned int rIdx = 0; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    this->Internal->RealReaders[rIdx]->SetByteOrderToLittleEndian();
    this->Internal->RealReaders[rIdx]->Modified();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader2::SetByteOrder(int byteOrder)
{
  for (unsigned int rIdx = 0; rIdx < this->Internal->RealReaders.size(); rIdx++)
  {
    this->Internal->RealReaders[rIdx]->SetByteOrder(byteOrder);
    this->Internal->RealReaders[rIdx]->Modified();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader2::GetByteOrder()
{
  return this->Internal->RealReaders.size() == 0 ? vtkPGenericEnSightReader::FILE_UNKNOWN_ENDIAN
                                                 : this->Internal->RealReaders[0]->GetByteOrder();
}

//----------------------------------------------------------------------------
const char* vtkPVEnSightMasterServerReader2::GetByteOrderAsString()
{
  return this->Internal->RealReaders.size() == 0
    ? nullptr
    : this->Internal->RealReaders[0]->GetByteOrderAsString();
}
