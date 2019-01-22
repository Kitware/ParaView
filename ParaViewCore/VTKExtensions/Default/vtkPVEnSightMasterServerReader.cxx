/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightMasterServerReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEnSightMasterServerReader.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVEnSightMasterServerTranslator.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkTransmitPolyDataPiece.h"
#include "vtkTransmitUnstructuredGridPiece.h"
#include "vtkUnstructuredGrid.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPICommunicator.h"
#endif

#include <string>
#include <vector>

#include <ctype.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightMasterServerReader);

vtkCxxSetObjectMacro(vtkPVEnSightMasterServerReader, Controller, vtkMultiProcessController);
vtkMultiProcessController* vtkPVEnSightMasterServerReader::GetController()
{
  return this->Controller;
}

//----------------------------------------------------------------------------
class vtkPVEnSightMasterServerReaderInternal
{
public:
  std::vector<std::string> PieceFileNames;
  int EnSightVersion;
  int NumberOfTimeSets;
  int NumberOfOutputs;
  std::vector<int> CumulativeTimeSetSizes;
  std::vector<float> TimeSetValues;
};

//----------------------------------------------------------------------------
vtkPVEnSightMasterServerReader::vtkPVEnSightMasterServerReader()
{
  this->Internal = new vtkPVEnSightMasterServerReaderInternal;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->InformationError = 0;
  this->ExtentTranslator = vtkPVEnSightMasterServerTranslator::New();
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
vtkPVEnSightMasterServerReader::~vtkPVEnSightMasterServerReader()
{
  this->SetController(0);
  delete this->Internal;
  this->ExtentTranslator->Delete();
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << "\n";
  os << indent << "Error: " << this->InformationError << "\n";
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
}

//----------------------------------------------------------------------------
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
template <class T>
int vtkPVEnSightMasterServerReaderSyncValues(
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
    return VTK_ERROR;
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
int vtkPVEnSightMasterServerReaderSyncValues(T*, int, int, vtkMultiProcessController*)
{
  return VTK_OK;
}
#endif

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
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
  if ((vtkPVEnSightMasterServerReaderSyncValues(
         parseResults, 2, this->Controller->GetNumberOfProcesses(), this->Controller) != VTK_OK) ||
    (parseResults[0] != VTK_OK))
  {
    vtkErrorMacro("Error parsing the master server file.");
    this->InformationError = 1;
    return 0;
  }

  // Compare case file versions.
  this->Internal->EnSightVersion = -1;
  int piece = this->Controller->GetLocalProcessId();
  if (piece < this->NumberOfPieces)
  {
    // Let the superclass read the file information on this node.
    this->SuperclassExecuteInformation(request, inputVector, outputVector);
    this->Internal->EnSightVersion = this->EnSightVersion;
  }
  if (vtkPVEnSightMasterServerReaderSyncValues(
        &this->Internal->EnSightVersion, 1, this->NumberOfPieces, this->Controller) != VTK_OK)
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
  vtkDataArrayCollection* timeSets = this->GetTimeSets();
  this->Internal->NumberOfTimeSets = timeSets ? timeSets->GetNumberOfItems() : 0;
  if (vtkPVEnSightMasterServerReaderSyncValues(
        &this->Internal->NumberOfTimeSets, 1, this->NumberOfPieces, this->Controller) != VTK_OK)
  {
    vtkErrorMacro("Number of time sets not equal on all nodes.");
    this->InformationError = 1;
    return 0;
  }

  // Compare the time sets' sizes.
  this->Internal->CumulativeTimeSetSizes.resize(this->Internal->NumberOfTimeSets + 1);
  if (piece < this->NumberOfPieces)
  {
    this->Internal->CumulativeTimeSetSizes[0] = 0;
    for (i = 0; i < this->Internal->NumberOfTimeSets; ++i)
    {
      this->Internal->CumulativeTimeSetSizes[i + 1] =
        (this->Internal->CumulativeTimeSetSizes[i] + timeSets->GetItem(i)->GetNumberOfTuples());
    }
  }
  if (vtkPVEnSightMasterServerReaderSyncValues(&*this->Internal->CumulativeTimeSetSizes.begin(),
        this->Internal->NumberOfTimeSets + 1, this->NumberOfPieces, this->Controller) != VTK_OK)
  {
    vtkErrorMacro("Time set sizes not equal on all nodes.");
    this->InformationError = 1;
    return 0;
  }

  // All time sets have the correct size.  Now compare the values.
  // The CumulativeTimeSetSizes has the sizes on all nodes, even those
  // beyond the number of pieces.

  // Compare time set values.
  this->Internal->TimeSetValues.clear();
  if (piece < this->NumberOfPieces)
  {
    // This is a real piece, fill in the time set values.
    for (i = 0; i < this->Internal->NumberOfTimeSets; ++i)
    {
      vtkDataArray* array = timeSets->GetItem(i);
      int numValues = array->GetNumberOfTuples();
      for (int j = 0; j < numValues; ++j)
      {
        this->Internal->TimeSetValues.push_back(array->GetTuple1(j));
      }
    }
  }
  else
  {
    // This is not a piece, allocate memory to receive the time set values.
    this->Internal->TimeSetValues.resize(
      this->Internal->CumulativeTimeSetSizes[this->Internal->NumberOfTimeSets]);
  }
  if (vtkPVEnSightMasterServerReaderSyncValues(&*this->Internal->TimeSetValues.begin(),
        static_cast<int>(this->Internal->TimeSetValues.size()), this->NumberOfPieces,
        this->Controller) != VTK_OK)
  {
    vtkErrorMacro("Time set values do not match on all nodes.");
    this->InformationError = 1;
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Do not execute if ExecuteInformation failed.
  if (this->InformationError)
  {
    return 0;
  }

  // Compare the number of outputs.
  int piece = this->Controller->GetLocalProcessId();
  if (piece < this->NumberOfPieces)
  {
    // Let the superclass read the data and setup the outputs.
    this->SuperclassExecuteData(request, inputVector, outputVector);
  }
  if (vtkPVEnSightMasterServerReaderSyncValues(
        &this->Internal->NumberOfOutputs, 1, this->NumberOfPieces, this->Controller) != VTK_OK)
  {
    vtkErrorMacro("Number of outputs does not match on all nodes.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader::SuperclassExecuteInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Fake the filename when the superclass uses it.
  int piece = this->Controller->GetLocalProcessId();
  char* temp = this->CaseFileName;
  this->CaseFileName = const_cast<char*>(this->Internal->PieceFileNames[piece].c_str());
  this->Superclass::RequestInformation(request, inputVector, outputVector);
  this->CaseFileName = temp;
}

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader::SuperclassExecuteData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Fake the filename when the superclass uses it.
  int piece = this->Controller->GetLocalProcessId();
  char* temp = this->CaseFileName;
  this->CaseFileName = const_cast<char*>(this->Internal->PieceFileNames[piece].c_str());
  this->Superclass::RequestData(request, inputVector, outputVector);
  this->CaseFileName = temp;
}

//----------------------------------------------------------------------------
static int vtkPVEnSightMasterServerReaderIsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
static int vtkPVEnSightMasterServerReaderStartsWith(const char* str1, const char* str2)
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
static int vtkPVEnSightMasterServerReaderGetLineFromStream(istream& is, std::string& line)
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
int vtkPVEnSightMasterServerReader::ParseMasterServerFile()
{
  // Clear old list of pieces.
  this->Internal->PieceFileNames.clear();

  // Construct the file name to open.
  std::string sfilename;
  if (!this->CaseFileName)
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
  ifstream fin(sfilename.c_str(), ios::in);
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
  while (vtkPVEnSightMasterServerReaderGetLineFromStream(fin, line))
  {
    // This section determines the type of file: case of SOS.
    if (!readingFormat && vtkPVEnSightMasterServerReaderStartsWith(line.c_str(), "FORMAT"))
    {
      // The FORMAT section starts here.
      readingFormat = 1;
    }
    else if (readingFormat && !numServers &&
      vtkPVEnSightMasterServerReaderStartsWith(line.c_str(), "type:"))
    {
      // Remove the leading "type:" to get the remaining value.
      const char* p = line.c_str();
      p += strlen("type:");
      // Remove leading spaces.
      while (*p && vtkPVEnSightMasterServerReaderIsSpace(*p))
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
    else if (!readingServers && vtkPVEnSightMasterServerReaderStartsWith(line.c_str(), "SERVERS"))
    {
      // The SERVERS section starts here.
      readingServers = 1;
    }
    else if (readingServers && !numServers &&
      vtkPVEnSightMasterServerReaderStartsWith(line.c_str(), "number of servers:"))
    {
      // Found the number of servers line.
      if (sscanf(line.c_str(), "number of servers: %i", &numServers) < 1)
      {
        vtkErrorMacro("Error parsing number of servers from: " << line.c_str());
        return VTK_ERROR;
      }
    }
    else if (readingServers && vtkPVEnSightMasterServerReaderStartsWith(line.c_str(), "casefile:"))
    {
      // Found a casefile line.  Record the name.
      const char* p = line.c_str();
      p += strlen("casefile:");
      while (*p && vtkPVEnSightMasterServerReaderIsSpace(*p))
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

  // The number of pieces is minimum of the number of EnSight servers
  // specified in the file and the number of processes to read the
  // data.
  int numProcs = this->Controller->GetNumberOfProcesses();

  // Make sure we have enough processes to read all the pieces.
  if (numProcs < numServers)
  {
    vtkErrorMacro("Not enough processes (" << numProcs << ") to read all Ensight server files ("
                                           << numServers << ")");
  }

  this->NumberOfPieces = (numServers < numProcs) ? numServers : numProcs;

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerReader::CanReadFile(const char* fname)
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
