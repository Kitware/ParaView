/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightMasterServerReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVEnSightMasterServerReader.h"

#include "vtkDataArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPVEnSightMasterServerTranslator.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkToolkits.h"
#include "vtkUnstructuredGrid.h"

#ifdef VTK_USE_MPI
# include "vtkMPIController.h"
#endif

#include <vtkstd/string>
#include <vtkstd/vector>

#include <ctype.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightMasterServerReader);
vtkCxxRevisionMacro(vtkPVEnSightMasterServerReader, "1.6.2.4");

#ifdef VTK_USE_MPI
vtkCxxSetObjectMacro(vtkPVEnSightMasterServerReader, Controller,
                     vtkMPIController);
vtkMultiProcessController* vtkPVEnSightMasterServerReader::GetController()
{
  return this->Controller;
}
#else
void vtkPVEnSightMasterServerReader::SetController(vtkMPIController*)
{
}
vtkMultiProcessController* vtkPVEnSightMasterServerReader::GetController()
{
  return 0;
}
#endif

//----------------------------------------------------------------------------
class vtkPVEnSightMasterServerReaderInternal
{
public:
  vtkstd::vector<vtkstd::string> PieceFileNames;
  int EnSightVersion;
  int NumberOfTimeSets;
  int NumberOfOutputs;
  vtkstd::vector<int> CumulativeTimeSetSizes;
  vtkstd::vector<float> TimeSetValues;
  vtkstd::vector<int> OutputTypes;
};

//----------------------------------------------------------------------------
vtkPVEnSightMasterServerReader::vtkPVEnSightMasterServerReader()
{
  this->Internal = new vtkPVEnSightMasterServerReaderInternal;
  this->Controller = 0;
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
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << "\n";
  os << indent << "Error: " << this->InformationError << "\n";
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
template <class T>
int vtkPVEnSightMasterServerReaderSyncValues(T* data, int numValues,
                                             int numPieces,
                                             vtkMPIController* controller)
{
  // Compare values on all processes that will read real pieces.
  // Returns whether the values match.  If they match, all processes'
  // values are modified to match that of node 0.  This will leave the
  // values unchanged on processes that will read real data, but
  // inform the other processes of the proper values.
  
  if(!controller)
    {
    return VTK_ERROR;
    }
  
  vtkMPICommunicator* communicator = vtkMPICommunicator::SafeDownCast(
    controller->GetCommunicator());
  
  if(!communicator)
    {
    return VTK_ERROR;
    }
  
  int numProcs = controller->GetNumberOfProcesses();
  int myid = controller->GetLocalProcessId();
  
  // Collect all the values to node 0.
  T* values = new T[numProcs*numValues];
  communicator->Gather(data, values, numValues, 0);
  
  int result = VTK_OK;
  // Node 0 compares its values to those from other processes that
  // will actually be reading data.
  if(myid == 0)
    {
    for(int i=1; (result == VTK_OK) && (i < numPieces); i++)
      {
      for(int j=0; (result == VTK_OK) && (j < numValues); j++)
        {
        if(values[i*numValues+j] != values[j])
          {
          result = VTK_ERROR;
          }
        }
      }
    }
  
  // Free buffer where values were collected.
  delete [] values;
  
  // Broadcast result of comparison to all processes.
  communicator->Broadcast(&result, 1, 0);
  
  // If the results were okay, broadcast the correct values to all
  // processes so that those that will not read can have the correct
  // values.
  if(result == VTK_OK)
    {
    communicator->Broadcast(data, numValues, 0);
    }
  
  return result;
}
#endif

//----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkPVEnSightMasterServerReader::ExecuteInformation()
{
  int i;
  
  // Assume no error will occur.
  this->InformationError = 0;
  
  // Make sure we have a controller.
  if(!this->Controller)
    {
    vtkErrorMacro("ExecuteInformation requires a Controller.");
    this->InformationError = 1;
    return;
    }
  
  // Parse the input file.  This will set NumberOfPieces.
  int parseResults[2];
  parseResults[0] = this->ParseMasterServerFile();
  parseResults[1] = (parseResults[0] == VTK_OK)? this->NumberOfPieces : -1;
  
  // Make sure all nodes could parse the file and agree on the number
  // of pieces.
  if((vtkPVEnSightMasterServerReaderSyncValues(
        parseResults, 2, this->Controller->GetNumberOfProcesses(),
        this->Controller) != VTK_OK) ||
     (parseResults[0] != VTK_OK))
    {
    vtkErrorMacro("Error parsing the master server file.");
    this->InformationError = 1;
    return;
    }
  
  // Compare case file versions.
  this->Internal->EnSightVersion = -1;
  int piece = this->Controller->GetLocalProcessId();
  if(piece < this->NumberOfPieces)
    {
    // Let the superclass read the file information on this node.
    this->SuperclassExecuteInformation();
    this->Internal->EnSightVersion = this->EnSightVersion;
    }  
  if(vtkPVEnSightMasterServerReaderSyncValues(&this->Internal->EnSightVersion,
                                              1, this->NumberOfPieces,
                                              this->Controller) != VTK_OK)
    {
    vtkErrorMacro("EnSight version mismatch across nodes.");
    this->InformationError = 1;
    return;
    }
  
  // Make sure all nodes could read their files.
  if(this->Internal->EnSightVersion < 0)
    {
    vtkErrorMacro("Error reading case file on at least one node.");
    this->InformationError = 1;
    return;
    }
  
  // Compare number of time sets.
  vtkDataArrayCollection* timeSets = this->GetTimeSets();
  this->Internal->NumberOfTimeSets = timeSets? timeSets->GetNumberOfItems():0;
  if(vtkPVEnSightMasterServerReaderSyncValues(&this->Internal->NumberOfTimeSets,
                                              1, this->NumberOfPieces,
                                              this->Controller) != VTK_OK)
    {
    vtkErrorMacro("Number of time sets not equal on all nodes.");
    this->InformationError = 1;
    return;
    }
  
  // Compare the time sets' sizes.
  this->Internal->CumulativeTimeSetSizes.resize(
    this->Internal->NumberOfTimeSets+1);
  if(piece < this->NumberOfPieces)
    {
    this->Internal->CumulativeTimeSetSizes[0] = 0;
    for(i=0; i < this->Internal->NumberOfTimeSets; ++i)
      {
      this->Internal->CumulativeTimeSetSizes[i+1] =
        (this->Internal->CumulativeTimeSetSizes[i] +
         timeSets->GetItem(i)->GetNumberOfTuples());
      }
    }
  if(vtkPVEnSightMasterServerReaderSyncValues(
       &*this->Internal->CumulativeTimeSetSizes.begin(),
       this->Internal->NumberOfTimeSets+1, this->NumberOfPieces,
       this->Controller) != VTK_OK)
    {
    vtkErrorMacro("Time set sizes not equal on all nodes.");
    this->InformationError = 1;
    return;
    }
  
  // Compare time set values.
  this->Internal->TimeSetValues.clear();
  if(piece < this->NumberOfPieces)
    {
    for(i=0; i < this->Internal->NumberOfTimeSets; ++i)
      {
      vtkDataArray* array = timeSets->GetItem(i);
      int numValues = array->GetNumberOfTuples();
      for(int j=0; j < numValues; ++j)
        {
        this->Internal->TimeSetValues.push_back(array->GetTuple1(j));
        }
      }
    }
  if(vtkPVEnSightMasterServerReaderSyncValues(
       &*this->Internal->TimeSetValues.begin(),
       this->Internal->TimeSetValues.size(), this->NumberOfPieces,
       this->Controller) != VTK_OK)
    {
    vtkErrorMacro("Time set values do not match on all nodes.");
    this->InformationError = 1;
    return;
    }
}
#else
void vtkPVEnSightMasterServerReader::ExecuteInformation()
{
}
#endif

//----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkPVEnSightMasterServerReader::Execute()
{
  int i;
  
  // Do not execute if ExecuteInformation failed.
  if(this->InformationError)
    {
    this->ExecuteError();
    return;
    }
  
  //cout << "In execute " << endl;

  // Compare the number of outputs.
  int piece = this->Controller->GetLocalProcessId();
  if(piece < this->NumberOfPieces)
    {
    // Let the superclass read the data and setup the outputs.
    this->SuperclassExecuteData();
    this->Internal->NumberOfOutputs = this->GetNumberOfOutputs();
    }
  if(vtkPVEnSightMasterServerReaderSyncValues(
       &this->Internal->NumberOfOutputs, 1, this->NumberOfPieces,
       this->Controller) != VTK_OK)
    {
    vtkErrorMacro("Number of outputs does not match on all nodes.");
    this->ExecuteError();
    return;
    }
  
  // Compare the output types.
  this->Internal->OutputTypes.resize(this->Internal->NumberOfOutputs);
  if(piece < this->NumberOfPieces)
    {
    for(i=0; i < this->Internal->NumberOfOutputs; ++i)
      {
      if(vtkDataSet* output = this->GetOutput(i))
        {
        this->Internal->OutputTypes[i] = output->GetDataObjectType();
        }
      else
        {
        this->Internal->OutputTypes[i] = -1;
        }
      }
    }
  if(vtkPVEnSightMasterServerReaderSyncValues(
       &*this->Internal->OutputTypes.begin(),
       this->Internal->NumberOfOutputs, this->NumberOfPieces,
       this->Controller) != VTK_OK)
    {
    vtkErrorMacro("Output types do not match on all nodes.");
    this->ExecuteError();
    return;
    }  
  
  // If we are on a node that did not read real data, create empty
  // outputs of the right type.
  if(piece >= this->NumberOfPieces)
    {
    for(i=0; i < this->Internal->NumberOfOutputs; ++i)
      {
      vtkDataObject* output = 0;
      switch (this->Internal->OutputTypes[i])
        {
        case VTK_IMAGE_DATA:
        case VTK_STRUCTURED_POINTS:
          if(!(this->GetOutput(i) && this->GetOutput(i)->IsA("vtkImageData")))
            {
            output = vtkImageData::New();
            }
          break;
        case VTK_STRUCTURED_GRID:
          if(!(this->GetOutput(i) && this->GetOutput(i)->IsA("vtkStructuredGrid")))
            {
            output = vtkStructuredGrid::New();
            }
          break;
        case VTK_RECTILINEAR_GRID:
          if(!(this->GetOutput(i) && this->GetOutput(i)->IsA("vtkRectilinearGrid")))
            {
            output = vtkRectilinearGrid::New();
            }
          break;
        case VTK_UNSTRUCTURED_GRID:
          if(!(this->GetOutput(i) && this->GetOutput(i)->IsA("vtkUnstructuredGrid")))
            {
            output = vtkUnstructuredGrid::New();
            }
          break;
        case VTK_POLY_DATA:
          if(!(this->GetOutput(i) && this->GetOutput(i)->IsA("vtkPolyData")))
            {
            output = vtkPolyData::New();
            }
          break;
        }
      if(output)
        {
        this->SetNthOutput(i, output);
        output->Initialize();
        output->Delete();
        }
      }
    }
  
  // Set the extent translator on the outputs.
  this->ExtentTranslator->SetProcessId(piece);
  for(i=0; i < this->Internal->NumberOfOutputs; ++i)
    {
    this->GetOutput(i)->SetExtentTranslator(this->ExtentTranslator);
    }
}
#else
void vtkPVEnSightMasterServerReader::Execute()
{
  // Without MPI, all we can do is produce an empty output.
  this->ExecuteError();
}
#endif

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerReader::ExecuteError()
{
  // An error has occurred reading the file.  Just produce empty
  // output.
  this->SetNumberOfOutputs(1);
  if(!(this->GetOutput(0) && this->GetOutput(0)->IsA("vtkPolyData")))
    {
    vtkPolyData* output = vtkPolyData::New();
    this->SetNthOutput(0, output);
    output->Delete();
    }
  this->GetOutput(0)->Initialize();
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkPVEnSightMasterServerReader::SuperclassExecuteInformation()
{
  // Fake the filename when the superclass uses it.
  int piece = this->Controller->GetLocalProcessId();
  char* temp = this->CaseFileName;
  this->CaseFileName =
    const_cast<char*>(this->Internal->PieceFileNames[piece].c_str());
  this->Superclass::ExecuteInformation();
  this->CaseFileName = temp;
}
#else
void vtkPVEnSightMasterServerReader::SuperclassExecuteInformation()
{
}
#endif

//----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkPVEnSightMasterServerReader::SuperclassExecuteData()
{
  // Fake the filename when the superclass uses it.
  int piece = this->Controller->GetLocalProcessId();
  char* temp = this->CaseFileName;
  this->CaseFileName =
    const_cast<char*>(this->Internal->PieceFileNames[piece].c_str());
  this->Superclass::Execute();
  this->CaseFileName = temp;
}
#else
void vtkPVEnSightMasterServerReader::SuperclassExecuteData()
{
}
#endif

#ifdef VTK_USE_MPI
//----------------------------------------------------------------------------
static int vtkPVEnSightMasterServerReaderIsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
static int vtkPVEnSightMasterServerReaderStartsWith(const char* str1,
                                                    const char* str2)
{
  if ( !str1 || !str2 || strlen(str1) < strlen(str2) )
    {
    return 0;
    }
  return !strncmp(str1, str2, strlen(str2));  
}

//----------------------------------------------------------------------------
// Due to a buggy stream library on the HP and another on Mac OSX, we
// need this very carefully written version of getline.  Returns true
// if any data were read before the end-of-file was reached.
static int vtkPVEnSightMasterServerReaderGetLineFromStream(
  istream& is, vtkstd::string& line)
{
  const int bufferSize = 1024;
  char buffer[bufferSize];
  line = "";
  int haveData = 0;

  // If no characters are read from the stream, the end of file has
  // been reached.
  while((is.getline(buffer, bufferSize), is.gcount() > 0))
    {
    haveData = 1;
    line.append(buffer);

    // If newline character was read, the gcount includes the
    // character, but the buffer does not.  The end of line has been
    // reached.
    if(strlen(buffer) < static_cast<size_t>(is.gcount()))
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
  vtkstd::string sfilename;
  if(!this->CaseFileName)
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
  if(!fin)
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
  vtkstd::string line;
  while(vtkPVEnSightMasterServerReaderGetLineFromStream(fin, line))
    {
    // This section determines the type of file: case of SOS.
    if(!readingFormat && (line == "FORMAT"))
      {
      // The FORMAT section starts here.
      readingFormat = 1;
      }
    else if(readingFormat && !numServers &&
            vtkPVEnSightMasterServerReaderStartsWith(line.c_str(),
                                                     "type:"))
      {
      // Remove the leading "type:" to get the remaining value.
      const char* p = line.c_str();
      p += strlen("type:");
      // Remove leading spaces.
      while(*p && vtkPVEnSightMasterServerReaderIsSpace(*p))
        {
        ++p;
        }
      if(!*p)
        {
        vtkErrorMacro("Error parsing file type from: "
                      << line.c_str());
        return VTK_ERROR;
        }
      // These are the special cases (case files).
      if (strcmp(p,"ensight") == 0 || strcmp(p,"ensight gold")==0)
        {
        // Handle the case file line an sos file with one server.
        numServers = 1;
        this->Internal->PieceFileNames.push_back(this->CaseFileName);
        // We could exit the state machine here 
        // because we have nothing else to do from this state.
        // We assume the line "SERVERS" will not appear in any case file.
        }
      else if (strcmp(p,"master_server gold") != 0)
        { // We might as well have this sanity check here.
        vtkErrorMacro("Unexpected file type: '"
                      << p << "'");
        return VTK_ERROR;
        }
      readingFormat = 0; 
      }

    // The rest of this stuff is for picking the number of servers
    // and case file names from the SOS file.
    else if(!readingServers && (line == "SERVERS"))
      {
      // The SERVERS section starts here.
      readingServers = 1;
      }
    else if(readingServers && !numServers &&
            vtkPVEnSightMasterServerReaderStartsWith(line.c_str(),
                                                     "number of servers:"))
      {
      // Found the number of servers line.
      if(sscanf(line.c_str(), "number of servers: %i", &numServers) < 1)
        {
        vtkErrorMacro("Error parsing number of servers from: "
                      << line.c_str());
        return VTK_ERROR;
        }
      }
    else if(readingServers && 
            vtkPVEnSightMasterServerReaderStartsWith(line.c_str(),
                                                     "casefile:"))
      {
      // Found a casefile line.  Record the name.
      const char* p = line.c_str();
      p += strlen("casefile:");
      while(*p && vtkPVEnSightMasterServerReaderIsSpace(*p))
        {
        ++p;
        }
      if(!*p)
        {
        vtkErrorMacro("Error parsing case file name from: "
                      << line.c_str());
        return VTK_ERROR;
        }
      this->Internal->PieceFileNames.push_back(p);
      }
    }
  
  // Make sure we got the expected number of servers (pieces).
  if(static_cast<int>(this->Internal->PieceFileNames.size()) != numServers)
    {
    vtkErrorMacro("Number of servers specified by file does not match "
                  "actual number of servers provided by file.");
    return VTK_ERROR;
    }
  
  // The number of pieces is minimum of the number of EnSight servers
  // specified in the file and the number of processes to read the
  // data.
  int numProcs = this->Controller->GetNumberOfProcesses();
  
  
  // Make sure we have enoght processes to read all the pieces.
  if (numProcs < numServers)
    {
    vtkErrorMacro("Not enough processes (" << numProcs
                  << ") to read all Ensight server files ("
                  << numServers << ")");
    }  
  
  this->NumberOfPieces = (numServers < numProcs)? numServers:numProcs;

  for (int i = 0; i < this->GetNumberOfOutputs(); i++)
    {
    vtkDataObject* output = this->GetOutput(i);

    output->SetMaximumNumberOfPieces(this->NumberOfPieces);
    }
  
  return VTK_OK;
}
#else
int vtkPVEnSightMasterServerReader::ParseMasterServerFile()
{
  return VTK_ERROR;
}
#endif
