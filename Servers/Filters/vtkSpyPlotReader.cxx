/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpyPlotReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpyPlotReader.h"

#include "vtkObjectFactory.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkMultiProcessController.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkHierarchicalDataInformation.h"
#include "vtkUniformGrid.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <sys/stat.h>
#include <assert.h>

#include "spcth_interface.h"
#define vtkMIN(x, y) \
  (\
   ((x)<(y))?(x):(y)\
   )


vtkCxxRevisionMacro(vtkSpyPlotReader, "1.8");
vtkStandardNewMacro(vtkSpyPlotReader);
vtkCxxSetObjectMacro(vtkSpyPlotReader,Controller,vtkMultiProcessController);

//-----------------------------------------------------------------------------
//=============================================================================
class vtkSpyPlotReaderMap
{
public:
  typedef vtkstd::map<vtkstd::string, SPCTH *> MapOfStringToSPCTH;
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  MapOfStringToSPCTH Files;
  vtkstd::string MasterFileName;

  void Initialize(const char *file)
    {
    if ( !file || file != this->MasterFileName )
      {
      this->Clean();
      }
    }
  void Clean()
    {
    MapOfStringToSPCTH::iterator it=this->Files.begin();
    MapOfStringToSPCTH::iterator end=this->Files.end();
    while(it!=end)
      {
      spcth_finalize(it->second);
      it->second=0;
      ++it;
      }
    this->Files.erase(this->Files.begin(),end);
    }
};

//-----------------------------------------------------------------------------
class vtkBlockIterator
{
public:
  // Description:
  // Initialize the iterator with informations about processors,
  // files and timestep.
  virtual void Init(int numberOfProcessors,
                    int processorId,
                    vtkSpyPlotReaderMap *fileMap,
                    int currentTimeStep)
    {
      assert("pre: fileMap_exists" && fileMap!=0);
      
      this->NumberOfProcessors=numberOfProcessors;
      this->ProcessorId=processorId;
      this->FileMap=fileMap;
      this->CurrentTimeStep=currentTimeStep;
      this->NumberOfFiles=this->FileMap->Files.size();
    }
  
  // Description:
  // Go to first block if any.
  virtual void Start()=0;
  
  // Description:
  // Is there no block at current position?
  int IsOff()
    {
      return this->Off;
    }
  
  // Description:
  // Go to the next block if any
  // \pre not_is_off: !IsOff()
  void Next()
    {
      assert("pre: not_is_off" && !IsOff() );
      
      ++this->Block;
      if(this->Block>this->BlockEnd)
        {
        ++this->FileIterator;
        ++this->FileIndex;
        this->FindFirstBlockOfCurrentOrNextFile();
        }
    }
  
  // Description:
  // Return the block at current position.
  // \pre not_is_off: !IsOff()
  int GetBlock()
    {
      assert("pre: not_is_off" && !IsOff() );
      return this->Block;
    }
  
  // Description:
  // Return the number of fields at current position.
  // \pre not_is_off: !IsOff()
  int GetNumberOfFields()
    {
      assert("pre: not_is_off" && !IsOff() );
      return this->NumberOfFields;
    }
  
  // Description:
  // Return the SPCTH API handle at current position.
  // \pre not_is_off: !IsOff()
  SPCTH *GetSpcth()
    {
      assert("pre: not_is_off" && !IsOff() );
      return this->Spcth;
    }
  
protected:
  virtual void FindFirstBlockOfCurrentOrNextFile()=0;
  
  int NumberOfProcessors;
  int ProcessorId;
  vtkSpyPlotReaderMap *FileMap;
  int CurrentTimeStep;
  
  int NumberOfFiles;
  
  int Off;
  int Block;
  int NumberOfFields;
  SPCTH *Spcth;
  
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator FileIterator;
  int FileIndex;
  
  int BlockEnd;
};

class vtkBlockDistributionBlockIterator
  : public vtkBlockIterator
{
public:
  vtkBlockDistributionBlockIterator() {}
  virtual ~vtkBlockDistributionBlockIterator() {}
  
  virtual void Start()
    {
      this->FileIterator=this->FileMap->Files.begin();
      this->FileIndex=0;
      this->FindFirstBlockOfCurrentOrNextFile();
    }
  
protected:
  virtual void FindFirstBlockOfCurrentOrNextFile()
    {
      this->Off=this->FileIndex>=this->NumberOfFiles;
      int found=0;
      while(!this->Off && !found)
        {
        const char *fname=this->FileIterator->first.c_str();
        this->Spcth=this->FileIterator->second;
        spcth_openSpyFile(this->Spcth,fname);
        spcth_setTimeStep(this->Spcth,
                          spcth_getTimeStepValue(this->Spcth,
                                                 this->CurrentTimeStep));
        
        this->NumberOfFields=spcth_getNumberOfCellFields(this->Spcth);
        
        int numBlocks=spcth_getNumberOfDataBlocksForCurrentTime(this->Spcth);
        
        found=this->ProcessorId<numBlocks;
          
        if(found) // otherwise skip to the next file
          {
          int numBlocksPerProcess=numBlocks/this->NumberOfProcessors;
          int leftOverBlocks=numBlocks-
            (numBlocksPerProcess*this->NumberOfProcessors);
          
          int blockStart;
          
          if(this->ProcessorId<leftOverBlocks)
            {
            blockStart=(numBlocksPerProcess+1)*this->ProcessorId;
            this->BlockEnd=blockStart+(numBlocksPerProcess+1)-1;
            }
          else
            {
            blockStart=numBlocksPerProcess*this->ProcessorId+leftOverBlocks;
            this->BlockEnd=blockStart+numBlocksPerProcess-1;
            }
          this->Block=blockStart;
          found=this->Block<=this->BlockEnd;
          }
        if(!found)
          {
          ++this->FileIterator;
          ++this->FileIndex;
          this->Off=this->FileIndex>=this->NumberOfFiles;
          }
        }
    }
};

class vtkFileDistributionBlockIterator
  : public vtkBlockIterator
{
public:
  vtkFileDistributionBlockIterator() {}
  virtual ~vtkFileDistributionBlockIterator() {}
  virtual void Init(int numberOfProcessors,
                    int processorId,
                    vtkSpyPlotReaderMap *fileMap,
                    int currentTimeStep)
    {
      vtkBlockIterator::Init(numberOfProcessors,processorId,fileMap,
                             currentTimeStep);
      
      if(this->ProcessorId>=this->NumberOfFiles)
        {
        this->FileEnd=this->NumberOfFiles;
        this->FileStart=this->FileEnd+1;
        }
      else
        {
        int numFilesPerProcess=this->NumberOfFiles/this->NumberOfProcessors;
        int leftOverFiles=this->NumberOfFiles
          -(numFilesPerProcess*this->NumberOfProcessors);
        
        if (this->ProcessorId < leftOverFiles)
          {
          this->FileStart = (numFilesPerProcess+1) * this->ProcessorId;
          this->FileEnd = this->FileStart + (numFilesPerProcess+1) - 1;
          }
        else
          {
          this->FileStart=numFilesPerProcess*this->ProcessorId + leftOverFiles;
          this->FileEnd = this->FileStart + numFilesPerProcess - 1;
          }
        }
    }
  
  virtual void Start()
    {
      this->Off=this->ProcessorId>=this->NumberOfFiles; // processor not used
      if(!this->Off)
        {
        // skip the first files
        this->FileIndex=0;
        this->FileIterator=this->FileMap->Files.begin();
        while(this->FileIndex<this->FileStart)
          {
          ++this->FileIterator;
          ++this->FileIndex;
          }
        
        this->FindFirstBlockOfCurrentOrNextFile();
        }
    }
  
protected:
  virtual void FindFirstBlockOfCurrentOrNextFile()
    {
      this->Off=this->FileIndex>this->FileEnd;
      int found=0;
      while(!this->Off && !found)
        {
        const char *fname=this->FileIterator->first.c_str();
        this->Spcth=this->FileIterator->second;
//        vtkDebugMacro("Reading data from file: " << fname);
        
        spcth_openSpyFile(this->Spcth,fname);
        spcth_setTimeStep(this->Spcth,
                          spcth_getTimeStepValue(this->Spcth,
                                                 this->CurrentTimeStep));
        
        this->NumberOfFields=spcth_getNumberOfCellFields(this->Spcth);
        
        int numberOfBlocks;
        numberOfBlocks=spcth_getNumberOfDataBlocksForCurrentTime(this->Spcth);
        
        this->BlockEnd=numberOfBlocks-1;
        this->Block=0;
        found=this->Block<=this->BlockEnd;
        if(!found)
          {
          ++this->FileIterator;
          ++this->FileIndex;
          this->Off=this->FileIndex>this->FileEnd;
          }
        }
    }
  int FileStart;
  int FileEnd;
};

//=============================================================================
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Return whether the path represents a full path (not relative). 
static bool FileIsFullPath(const char* in_name)
{
  vtkstd::string name = in_name;
#if defined(_WIN32) || defined(__CYGWIN__)
  // On Windows, the name must be at least two characters long.
  if(name.length() < 2)
    {
    return false;
    }
  if(name[1] == ':')
    {
    return true;
    }
  if(name[0] == '\\')
    {
    return true;
    }
#else
  // On UNIX, the name must be at least one character long.
  if(name.length() < 1)
    {
    return false;
    }
#endif
  // On UNIX, the name must begin in a '/'.
  // On Windows, if the name begins in a '/', then it is a full
  // network path.
  if(name[0] == '/')
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
// replace replace with with as many times as it shows up in source.
// write the result into source.
static void ReplaceString(vtkstd::string& source,
                          const char* replace,
                          const char* with)
{
  const char *src = source.c_str();
  char *searchPos = const_cast<char *>(strstr(src,replace));

  // get out quick if string is not found
  if (!searchPos)
    {
    return;
    }

  // perform replacements until done
  size_t replaceSize = strlen(replace);
  char *orig = strdup(src);
  char *currentPos = orig;
  searchPos = searchPos - src + orig;

  // initialize the result
  source.erase(source.begin(),source.end());
  do
    {
    *searchPos = '\0';
    source += currentPos;
    currentPos = searchPos + replaceSize;
    // replace
    source += with;
    searchPos = strstr(currentPos,replace);
    }
  while (searchPos);

  // copy any trailing text
  source += currentPos;
  free(orig);
}

//-----------------------------------------------------------------------------
// convert windows slashes to unix slashes 
//
static void ConvertToUnixSlashes(vtkstd::string& path)
{
  vtkstd::string::size_type pos = 0;
  while((pos = path.find('\\', pos)) != vtkstd::string::npos)
    {
    // make sure we don't convert an escaped space to a unix slash
    if(pos < path.size()-1)
      {
      if(path[pos+1] != ' ')
        {
        path[pos] = '/';
        }
      }
    pos++;
    }
  // Remove all // from the path just like most unix shells
  int start_find;

#ifdef _WIN32
  // However, on windows if the first characters are both slashes,
  // then keep them that way, so that network paths can be handled.
  start_find = 1;
#else
  start_find = 0;
#endif

  while((pos = path.find("//", start_find)) != vtkstd::string::npos)
    {
    ::ReplaceString(path, "//", "/");
    }

  // remove any trailing slash
  if(path.size() > 1 && path[path.size()-1] == '/')
    {
    path = path.substr(0, path.size()-1);
    }

  // if there is a tilda ~ then replace it with HOME
  if(path.find("~") == 0)
    {
    if (getenv("HOME"))
      {
      path = vtkstd::string(getenv("HOME")) + path.substr(1);
      }
    }
}

//-----------------------------------------------------------------------------
// Return path of a full filename (no trailing slashes).
// Warning: returned path is converted to Unix slashes format.
//
static vtkstd::string GetFilenamePath(const vtkstd::string& filename)
{
  vtkstd::string fn = filename;
  ::ConvertToUnixSlashes(fn);

  vtkstd::string::size_type slash_pos = fn.rfind("/");
  if(slash_pos != vtkstd::string::npos)
    {
    return fn.substr(0, slash_pos);
    }
  else
    {
    return "";
    }
}

//-----------------------------------------------------------------------------
// Due to a buggy stream library on the HP and another on Mac OSX, we
// need this very carefully written version of getline.  Returns true
// if any data were read before the end-of-file was reached.
// 
static bool GetLineFromStream(istream& is,
  vtkstd::string& line, bool *has_newline = 0)
{
  const int bufferSize = 1024;
  char buffer[bufferSize];
  line = "";
  bool haveData = false;
  if ( has_newline )
    {
    *has_newline = false;
    }

  // If no characters are read from the stream, the end of file has
  // been reached.
  while((is.getline(buffer, bufferSize), is.gcount() > 0))
    {
    haveData = true;
    line.append(buffer);

    // If newline character was read, the gcount includes the
    // character, but the buffer does not.  The end of line has been
    // reached.
    if(strlen(buffer) < static_cast<size_t>(is.gcount()))
      {
      if ( has_newline )
        {
        *has_newline = true;
        }
      break;
      }

    // The fail bit may be set.  Clear it.
    is.clear(is.rdstate() & ~ios::failbit);
    }
  return haveData;
}


//-----------------------------------------------------------------------------
vtkSpyPlotReader::vtkSpyPlotReader()
{
  this->SetNumberOfInputPorts(0);
  
  this->Map = new vtkSpyPlotReaderMap;
  this->FileName = 0;
  this->CurrentFileName = 0;
  this->CellDataArraySelection = vtkDataArraySelection::New();
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
    &vtkSpyPlotReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
    this->SelectionObserver);
  this->TimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  
  this->DistributeFiles=0; // by default, distribute blocks, not files.
}

//-----------------------------------------------------------------------------
vtkSpyPlotReader::~vtkSpyPlotReader()
{
  this->SetFileName(0);
  this->SetCurrentFileName(0);
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->Map->Clean();
  delete this->Map;
  this->Map = 0;
  this->SetController(0);
}

//-----------------------------------------------------------------------------
// Read the case file and the first binary file do get meta
// informations (number of files, number of fields, number of timestep).
int vtkSpyPlotReader::RequestInformation(vtkInformation *request, 
                                         vtkInformationVector **inputVector, 
                                         vtkInformationVector *outputVector)
{
  if(this->Controller==0)
    {
    vtkErrorMacro("Controller not specified. This reader requires controller to be set.");
    return 0;
    }
  if(!this->Superclass::RequestInformation(request,inputVector,outputVector))
    {
    return 0;
    }
  
  vtkHierarchicalDataInformation *compInfo
    =vtkHierarchicalDataInformation::New();

  vtkInformation *info=outputVector->GetInformationObject(0);
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(),compInfo);

  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  compInfo->Delete();

  struct stat fs;
  if(stat(this->FileName,&fs)!=0)
    {
    vtkErrorMacro("Cannot find file " << this->FileName);
    return 0;
    }

  ifstream ifs(this->FileName);
  if(!ifs)
    {
    vtkErrorMacro("Error opening file " << this->FileName);
    return 0;
    }
  char buffer[8];
  if(!ifs.read(buffer,7))
    {
    vtkErrorMacro("Problem reading header of file: " << this->FileName);
    return 0;
    }
  buffer[7] = 0;
  ifs.close();
  if(strcmp(buffer,"spydata")==0)
    {
    this->SetCurrentFileName(this->FileName);
    this->Map->Clean();
    this->Map->Files[this->FileName]=spcth_initialize();
    return this->UpdateMetaData();
    }
  else
    {
    if(strcmp(buffer,"spycase")==0)
      {
      return this->UpdateCaseFile(this->FileName);
      }
    else
      {
      vtkErrorMacro("Not a SpyData file");
      return 0;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateCaseFile(const char *fname)
{
  // Check to see if this is the same filename as before
  // if so then I already have the meta info, so just return
  if(this->GetCurrentFileName()!=0 &&
     !strcmp(fname,this->GetCurrentFileName()))
    {
    return 0;
    }

  // Set case file name and clean/initialize file map
  this->SetCurrentFileName(fname);
  this->Map->Clean();

  // Setup the filemap and spcth structures
  ifstream ifs(this->FileName);
  if (!ifs )
    {
    vtkErrorMacro("Error opening file " << fname);
    return 0;
    }
  
  vtkstd::string line;
  if(!::GetLineFromStream(ifs,line)) // eat spycase line
    {
    vtkErrorMacro("Syntax error in case file " << fname);
    return 0;
    }

  while(::GetLineFromStream(ifs, line))
    {
    if(line.length()!=0)  // Skip blank lines
      {
      vtkstd::string::size_type stp = line.find_first_not_of(" \n\t\r");
      vtkstd::string::size_type etp = line.find_last_not_of(" \n\t\r");
      vtkstd::string f(line, stp, etp-stp+1);
      if(f[0]!='#') // skip comment
        {
        if(!::FileIsFullPath(f.c_str()))
          {
          f=::GetFilenamePath(this->FileName)+"/"+f;
          }
        this->Map->Files[f.c_str()]=spcth_initialize();
        }
      }
    }

  // Okay now open just the first file to get meta data
  vtkDebugMacro("Reading Meta Data in UpdateCaseFile(ExecuteInformation) from file: " << this->Map->Files.begin()->first.c_str());
  return this->UpdateMetaData();
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateMetaData()
{
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator it;
  it=this->Map->Files.begin();
  
  const char *fname=0;
  SPCTH *spcth=0;
  
  if(it==this->Map->Files.end())
    {
    vtkErrorMacro("The internal file map is empty!");
    return 0;
    }
  else
    {
    fname=it->first.c_str();
    spcth=it->second;
    }
  
  int i;
  int num_time_steps;
  
  // Open the file and get the number time steps
  spcth_openSpyFile(spcth,fname);
  num_time_steps=spcth_getNumTimeSteps(spcth);
  this->TimeStepRange[1]=num_time_steps-1;
  
  this->CurrentTimeStep=this->TimeStep;
  if(this->CurrentTimeStep<0||this->CurrentTimeStep>=num_time_steps)
    {
    vtkErrorMacro("TimeStep set to " << this->CurrentTimeStep << " outside of the range 0 - " << (num_time_steps-1) << ". Use 0.");
    this->CurrentTimeStep=0;
    }

  // Set the reader to read the first time step.
  spcth_setTimeStep(spcth,spcth_getTimeStepValue(spcth,this->CurrentTimeStep));

  // Print info
  vtkDebugMacro("File has " << num_time_steps << " timesteps");
  vtkDebugMacro("Timestep values:");
  for(i=0; i< num_time_steps; ++i)
    {
    vtkDebugMacro(<< i << ": " << spcth_getTimeStepValue(spcth, i));
    }

  // AMR (hierarchy of uniform grids) or flat mesh (set of rectilinear grids)
  this->IsAMR=spcth_isAMR(spcth);
  
  // Fields
  int fieldsCount = spcth_getNumberOfCellFields(spcth);
  vtkDebugMacro("Number of fields: " << fieldsCount);
  
  
  vtkstd::set<vtkstd::string> fileFields;
  
  int field;
  for(field=0; field<fieldsCount; ++field)
    {
    const char*fieldName=spcth_getCellFieldName(spcth,field);
    vtkDebugMacro("Field #" << field << ": " << fieldName << " -- " << spcth_getCellFieldDescription(spcth, field));
    fileFields.insert(fieldName);
    
    if(!this->CellDataArraySelection->ArrayExists(fieldName))
      {
      // the array may exists from a previous read and have to be kept
      // if the array has be enabled/disables by the user.
      this->CellDataArraySelection->AddArray(fieldName);
      }
    }
  // Now remove the exising array that were not found in the file.
  field=0;
  // the trick is that GetNumberOfArrays() may change at each step.
  while(field<this->CellDataArraySelection->GetNumberOfArrays())
    {
    if(fileFields.find(this->CellDataArraySelection->GetArrayName(field))==fileFields.end())
      {
      this->CellDataArraySelection->RemoveArrayFromIndex(field);
      }
    else
      {
      ++field;
      }
    }
  return 1;
}

// Magic number that encode the message ids for parallel communication
enum
{
  VTK_MSG_SPY_READER_HAS_BOUNDS=288302,
  VTK_MSG_SPY_READER_LOCAL_BOUNDS,
  VTK_MSG_SPY_READER_GLOBAL_BOUNDS,
  VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS,
  VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS,
  VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS,
  VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS,
  VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX
};

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  SPCTH* spcth = 0;
  const char *fname=0;

  vtkInformation *info=outputVector->GetInformationObject(0);
  vtkDataObject *doOutput=info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkHierarchicalDataSet *hb=vtkHierarchicalDataSet::SafeDownCast(doOutput);  
  if(!hb)
    {
    vtkErrorMacro("The output is not a HierarchicalDataSet");
    return 0;
    }
  if (!info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) ||
    !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    vtkErrorMacro("Expected information not found. "
                  "Cannot provide update extent.");
    return 0;
    }


  vtkHierarchicalDataInformation *compInfo=
    vtkHierarchicalDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

  hb->Initialize(); // remove all previous blocks
  hb->SetHierarchicalDataInformation(compInfo);
  
  int numFiles = this->Map->Files.size();

  // By setting SetMaximumNumberOfPieces(-1) 
  // then GetUpdateNumberOfPieces() should always return the number
  // of processors in the parallel job and GetUpdatePiece() should
  // return the specific process number
  int processNumber = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numProcessors =info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  
  int block;
  int field;
  vtkBlockIterator *blockIterator;
  if(this->DistributeFiles)
    {
    cout<<"Distribute files"<<endl;
    blockIterator=new vtkFileDistributionBlockIterator;
    }
  else
    {
    cout<<"Distribute blocks"<<endl;
    blockIterator=new vtkBlockDistributionBlockIterator;
    }
  
  blockIterator->Init(numProcessors,processNumber,this->Map,this->CurrentTimeStep);

#if 1 // skip loading for valgrind test
  
  double dsBounds[6]; // only for removing wrong ghost cells
  int firstBlock=1;
  
  blockIterator->Start();
  while(!blockIterator->IsOff())
    {
    block=blockIterator->GetBlock();
    spcth=blockIterator->GetSpcth();
    int dims[3];
    spcth_getDataBlockDimensions(spcth, block, dims, dims+1, dims+2);
    double realBounds[6];
    
    // Compute real bounds for the current block
    if(this->IsAMR)
      {
      double bounds[6];
      spcth_getDataBlockBounds(spcth, block, bounds);
      double spacing;
      int cc=0;
      while(cc<3)
        {
        // as the file includes ghost cells,
        // the bounding box also includes ghost cells
        // there is then two extras endpoints in each axis
        if(dims[cc]>1)
          {
          spacing=(bounds[2*cc+1]-bounds[2*cc])/dims[cc];
          realBounds[2*cc]=bounds[2*cc]+spacing;
          realBounds[2*cc+1]=bounds[2*cc+1]-spacing;
          }
        else
          {
          realBounds[2*cc]=0;
          realBounds[2*cc+1]=0;
          }
        ++cc;
        }
      }
    else
      {
      double *rawvector[3];
      spcth_getDataBlockVectors(spcth,block,&(rawvector[0]),
                                &(rawvector[1]),&(rawvector[2]));
      int cc=0;
      while(cc<3)
        {
        if(dims[cc]>1)
          {
          realBounds[2*cc]=rawvector[cc][1];
          realBounds[2*cc+1]=rawvector[cc][dims[cc]-1];
          }
        else
          {
          realBounds[2*cc]=0;
          realBounds[2*cc+1]=0;
          }
        ++cc;
        }
      }
    // Update the whole bounds
    // we will be use only for removing wrong ghost cells
    if(firstBlock)
      {
      int cc=0;
      while(cc<6)
        {
        dsBounds[cc]=realBounds[cc];
        ++cc;
        }
      firstBlock=0;
      }
    else
      {
      int cc=0;
      while(cc<3)
        {
        if(realBounds[2*cc]<dsBounds[2*cc])
          {
          dsBounds[2*cc]=realBounds[2*cc];
          }
        if(realBounds[2*cc+1]>dsBounds[2*cc+1])
          {
          dsBounds[2*cc+1]=realBounds[2*cc+1];
          }
        ++cc;
        }
      }
    blockIterator->Next();
    } // while
  
  int parent;
  int left=GetLeftChildProcessor(processNumber);
  int right=left+1;
  if(processNumber>0) // not root (nothing to do if root)
    {
    parent=this->GetParentProcessor(processNumber);
    }
  else
    {
    parent=0; // just to remove warnings, never used
    }
  
  double otherBounds[6];
  int leftHasBounds;
  int rightHasBounds;
  
  if(left<numProcessors)
    {
    // TODO WARNING if the child is empty the bounds are not initialized!
    // Grab the bounds from left child
    this->Controller->Receive(&leftHasBounds, 1, left,
                              VTK_MSG_SPY_READER_HAS_BOUNDS);
    
    if(leftHasBounds)
      {
      this->Controller->Receive(otherBounds, 6, left,
                                VTK_MSG_SPY_READER_LOCAL_BOUNDS);
      
      if(firstBlock) // impossible the current processor is not a leaf
        {
        int cc=0;
        while(cc<6)
          {
          dsBounds[cc]=otherBounds[cc];
          ++cc;
          }
        firstBlock=0;
        }
      else
        {
        int cc=0;
        while(cc<3)
          {
          if(otherBounds[2*cc]<dsBounds[2*cc])
            {
            dsBounds[2*cc]=otherBounds[2*cc];
            }
          if(otherBounds[2*cc+1]>dsBounds[2*cc+1])
            {
            dsBounds[2*cc+1]=otherBounds[2*cc+1];
            }
          ++cc;
          }
        }
      }
    
    if(right<numProcessors)
      {
      // Grab the bounds from right child
      this->Controller->Receive(&rightHasBounds, 1, right,
                                VTK_MSG_SPY_READER_HAS_BOUNDS);
      if(rightHasBounds)
        {
        this->Controller->Receive(otherBounds, 6, right,
                                  VTK_MSG_SPY_READER_LOCAL_BOUNDS);
        if(firstBlock)// impossible the current processor is not a leaf
          {
          int cc=0;
          while(cc<6)
            {
            dsBounds[cc]=otherBounds[cc];
            ++cc;
            }
          firstBlock=0;
          }
        else
          {
          int cc=0;
          while(cc<3)
            {
            if(otherBounds[2*cc]<dsBounds[2*cc])
              {
              dsBounds[2*cc]=otherBounds[2*cc];
              }
            if(otherBounds[2*cc+1]>dsBounds[2*cc+1])
              {
              dsBounds[2*cc+1]=otherBounds[2*cc+1];
              }
            ++cc;
            }
          }
        }
      }
    }
  
  // Send local to parent, Receive global from the parent.
  if(processNumber>0) // not root (nothing to do if root)
    {
    int hasBounds=!firstBlock;
    this->Controller->Send(&hasBounds, 1, parent,
                           VTK_MSG_SPY_READER_HAS_BOUNDS);
    if(hasBounds)
      {
      this->Controller->Send(dsBounds, 6, parent,
                             VTK_MSG_SPY_READER_LOCAL_BOUNDS);
      
      this->Controller->Receive(dsBounds, 6, parent,
                                VTK_MSG_SPY_READER_GLOBAL_BOUNDS);
      }
    }
  
  if(firstBlock) // empty, no bounds, nothing to do
    {
    delete blockIterator;
    return 1;
    }
  
  // Send it to children.
  if(left<numProcessors)
    {
    if(leftHasBounds)
      {
      this->Controller->Send(dsBounds, 6, left,
                             VTK_MSG_SPY_READER_GLOBAL_BOUNDS);
      }
    if(right<numProcessors)
      {
      if(rightHasBounds)
        {
        this->Controller->Send(dsBounds, 6, right,
                               VTK_MSG_SPY_READER_GLOBAL_BOUNDS);
        }
      }
    }
  
  // At this point, the global bounds is set in each processor.
  
  cout<<processNumber<<" bounds="<<dsBounds[0]<<"; "<<dsBounds[1]<<"; "<<dsBounds[2]<<"; "<<dsBounds[3]<<"; "<<dsBounds[4]<<"; "<<dsBounds[5]<<endl;
  
  // Read all files
  
  cout<<"there is (are) "<<numFiles<<" file(s)"<<endl;
    // Read only the part of the file for this processNumber.
//    for ( block = startBlock; block <= endBlock; ++ block )
  
  blockIterator->Start();
  while(!blockIterator->IsOff())
    {
    block=blockIterator->GetBlock();
    int numFields=blockIterator->GetNumberOfFields();
    spcth=blockIterator->GetSpcth();
      
    int cc;
    int dims[3];
    spcth_getDataBlockDimensions(spcth, block, dims, dims+1, dims+2);
    
    int extents[6];
    cc=0;
    while(cc<3)
      {
      extents[2*cc]=0;
      if(dims[cc]==1)
        {
        extents[2*cc+1]=0;
        }
        else
          {
          extents[2*cc+1]=dims[cc];
          }
      ++cc;
      }
    vtkCellData* cd;
    double bounds[6];
      
    int hasBadGhostCells;
    int realExtents[6];
    int realDims[3];
      
    if(this->IsAMR)
      {
      int level = spcth_getDataBlockLevel(spcth, block);
      spcth_getDataBlockBounds(spcth, block, bounds);
      
      // add at the end of the level list.
      vtkUniformGrid* ug = vtkUniformGrid::New();
//      vtkImageData* ug = vtkImageData::New();
      hb->SetDataSet(level, hb->GetNumberOfDataSets(level), ug);
      ug->Delete();
      
      double origin[3];
      double spacing[3];
      cc=0;
      while(cc<3)
        {
        // as the file includes ghost cells,
        // the bounding box also includes ghost cells
        // there is then two extras endpoints in each axis
        spacing[cc]=(bounds[2*cc+1]-bounds[2*cc])/dims[cc];
        // skip the first cell, which is a ghost cell
        // otherwise it should be just bounds[2*cc]
        origin[cc]=bounds[2*cc]; //+spacing[cc];
        ++cc;
        }
      ug->SetSpacing(spacing);
      
      hasBadGhostCells=0;
      cc=0;
      while(cc<3)
        {
        if(dims[cc]>1)
          {
          if(bounds[2*cc]<dsBounds[2*cc])
            {
            realExtents[2*cc]=1;
            --extents[2*cc+1];
            origin[cc]+=spacing[cc];
            hasBadGhostCells=1;
            }
          else
            {
            realExtents[2*cc]=0;
            }
          if(bounds[2*cc+1]>dsBounds[2*cc+1])
            {
            realExtents[2*cc+1]=dims[cc]-1;
            --extents[2*cc+1];
            hasBadGhostCells=1;
            }
          else
            {
            realExtents[2*cc+1]=dims[cc];
            }
          realDims[cc]=realExtents[2*cc+1]-realExtents[2*cc];
//            if(realDims[cc]==0)
//              {
//              realDims[cc]=1;
//              }
          }
        else
          {
          realExtents[2*cc]=0;
          realExtents[2*cc+1]=1;
          realDims[cc]=1;
          }
        ++cc;
        }
      
      ug->SetExtent(extents);
      ug->SetOrigin(origin);
      
      cd = ug->GetCellData();
      }
    else
      {
      vtkRectilinearGrid *rg=vtkRectilinearGrid::New();
      vtkDoubleArray *coordinates[3];
      double *rawvector[3];
      
      spcth_getDataBlockVectors(spcth,block,&(rawvector[0]),&(rawvector[1]),
                                &(rawvector[2]));
      
      // compute real bounds (removing the ghostcells)
      // we will be use only for removing wrong ghost cells
      cc=0;
      while(cc<3)
        {
        if(dims[cc]>1)
          {
          bounds[2*cc]=rawvector[cc][0]; // coordinates[cc]->GetValue(0);
          bounds[2*cc+1]=rawvector[cc][dims[cc]]; // coordinates[cc]->GetValue(dims[cc]);
          }
        else
          {
          bounds[2*cc]=0;
          bounds[2*cc+1]=0;
          }
        ++cc;
        }
      
      hasBadGhostCells=0;
      cc=0;
      while(cc<3)
        {
        if(dims[cc]>1)
          {
          if(bounds[2*cc]<dsBounds[2*cc])
            {
            realExtents[2*cc]=1;
            --extents[2*cc+1];
            hasBadGhostCells=1;
            }
          else
            {
            realExtents[2*cc]=0;
            }
          if(bounds[2*cc+1]>dsBounds[2*cc+1])
            {
            realExtents[2*cc+1]=dims[cc]-1;
            --extents[2*cc+1];
            hasBadGhostCells=1;
            }
          else
            {
            realExtents[2*cc+1]=dims[cc];
            }
          realDims[cc]=realExtents[2*cc+1]-realExtents[2*cc];
//            if(realDims[cc]==0)
//              {
//              realDims[cc]=1;
//              }
          }
        else
          {
          realExtents[2*cc]=0;
          realExtents[2*cc+1]=1;
          realDims[cc]=1;
          }
        ++cc;
        }
      
      rg->SetExtent(extents);
      
      cc=0;
      while(cc<3)
        {
        if(dims[cc]==1)
          {
          coordinates[cc]=0;
          }
        else
          {
          coordinates[cc]=vtkDoubleArray::New();
          coordinates[cc]->SetNumberOfTuples(realDims[cc]+1);
          memcpy(coordinates[cc]->GetVoidPointer(0),
                 rawvector[cc]+realExtents[2*cc],
                 (realDims[cc]+1)*sizeof(double));
          }
        ++cc;
        }
      
      if(dims[0]>1)
        {
        rg->SetXCoordinates(coordinates[0]);
        }
      if(dims[1]>1)
        {
        rg->SetYCoordinates(coordinates[1]);
        }
      if(dims[2]>1)
        {
        rg->SetZCoordinates(coordinates[2]);
        }
      cc=0;
      while(cc<3)
        {
        if(dims[cc]>1)
          {
          coordinates[cc]->Delete();
          }
        ++cc;
        }
      hb->SetDataSet(0, hb->GetNumberOfDataSets(0), rg);
      rg->Delete();
      
      cd = rg->GetCellData();
      }
    vtkDebugMacro("Executing block: " << block);
    if(!hasBadGhostCells)
      {
      for ( field = 0; field < numFields; field ++ )
        {
        fname = spcth_getCellFieldName(spcth, field);
        if (this->CellDataArraySelection->ArrayIsEnabled(fname))
          {
          vtkDataArray *array=cd->GetArray(fname);
          if(array!=0)
            {
            cd->RemoveArray(fname); // if this is not the first step,
            // make sure we have a clean array
            }
          
          array = vtkDoubleArray::New();
          cd->AddArray(array);
          array->Delete();
          
          array->SetName(fname);
          array->SetNumberOfComponents(1);
          array->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
          
          if (!spcth_getCellFieldData(spcth, block,field, static_cast<double*>(array->GetVoidPointer(0)))) 
            {
            vtkErrorMacro("Problem reading block: " << block << ", field: " << field << endl);
            }
          }
        }
      // Mark the bounding cells as ghost cells of level 1.
      vtkUnsignedCharArray *ghostArray=vtkUnsignedCharArray::New();
      ghostArray->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
      ghostArray->SetName("vtkGhostLevels"); //("vtkGhostLevels");
      cd->AddArray(ghostArray);
      ghostArray->Delete();
      unsigned char *ptr =static_cast<unsigned char*>(ghostArray->GetVoidPointer(0));
      
      int k=0;
      while(k<dims[2])
        {
        int kIsGhost;
        if(dims[2]==1)
          {
          kIsGhost=0;
          }
        else
          {
          kIsGhost=k==0 || (k==dims[2]-1);
          }
        
        int j=0;
        while(j<dims[1])
          {
          int jIsGhost=kIsGhost;
          if(!jIsGhost)
            {
            if(dims[1]>1)
              {
              jIsGhost=j==0 || (j==dims[1]-1);
              }
            }
          
          int i=0;
          while(i<dims[0])
            {
            int isGhost=jIsGhost;
            if(!isGhost)
              {
              if(dims[0]>1)
                {
                isGhost=i==0 || (i==dims[0]-1);
                }
              }
            if(isGhost)
              {
              (*ptr)=1;
              }
            else
              {
              (*ptr)=0;
              }
            ++ptr;
            ++i;
            }
          ++j;
          }
        ++k;
        }
      }
    else // some bad ghost cells
      {
      // reuse this memory array for each field.
      double *tmp=new double [dims[0]*dims[1]*dims[2]];
      
      int realPtDims[3];
      int ptDims[3];
      
      cc=0;
      while(cc<3)
        {
        realPtDims[cc]=realDims[cc]+1;
        ptDims[cc]=dims[cc]+1;
        ++cc;
        }
      
      for ( field = 0; field < numFields; field ++ )
        {
        fname = spcth_getCellFieldName(spcth, field);
        if (this->CellDataArraySelection->ArrayIsEnabled(fname))
          {
          vtkDataArray *array=cd->GetArray(fname);
          if(array!=0)
            {
            cd->RemoveArray(fname); // if this is not the first step,
            // make sure we have a clean array
            }
          
          array = vtkDoubleArray::New();
          cd->AddArray(array);
          array->Delete();
          
          array->SetName(fname);
          array->SetNumberOfComponents(1);
          array->SetNumberOfTuples(realDims[0]*realDims[1]*realDims[2]);
          
          if (!spcth_getCellFieldData(spcth, block,field, tmp)) 
            {
            vtkErrorMacro("Problem reading block: " << block << ", field: " << field << endl);
            }
          
          // skip some cell data.
          int xyz[3];
          int destXyz[3];
          xyz[2]=realExtents[4];
          destXyz[2]=0;
          while(xyz[2]<realExtents[5])
            {
            destXyz[1]=0;
            xyz[1]=realExtents[2];
            while(xyz[1]<realExtents[3])
              {
              destXyz[0]=0;
              xyz[0]=realExtents[0];
              while(xyz[0]<realExtents[1])
                {
                static_cast<double *>(array->GetVoidPointer(0))[vtkStructuredData::ComputeCellId(realPtDims,destXyz)]=tmp[vtkStructuredData::ComputeCellId(ptDims,xyz)];
                ++xyz[0];
                ++destXyz[0];
                }
              ++xyz[1];
              ++destXyz[1];
              }
            ++xyz[2];
            ++destXyz[2];
            }
          }
        }
      
      delete[] tmp;
      
      // Mark the remains ghost cell as real ghost cells of level 1.
      vtkUnsignedCharArray *ghostArray=vtkUnsignedCharArray::New();
      ghostArray->SetNumberOfTuples(realDims[0]*realDims[1]*realDims[2]);
      ghostArray->SetName("vtkGhostLevels"); //("vtkGhostLevels");
      cd->AddArray(ghostArray);
      ghostArray->Delete();
      unsigned char *ptr =static_cast<unsigned char*>(ghostArray->GetVoidPointer(0));
      
      int k=0;
      while(k<realDims[2])
        {
        int kIsGhost;
        if(realDims[2]==1)
          {
          kIsGhost=0;
          }
        else
          {
          kIsGhost=(k==0 && realExtents[4]==0) || (k==realDims[2]-1 && realExtents[5]==dims[2]);
          }
        
        int j=0;
        while(j<realDims[1])
          {
          int jIsGhost=kIsGhost;
          if(!jIsGhost)
            {
            if(realDims[1]>1)
              {
              jIsGhost=(j==0 && realExtents[2]==0) || (j==realDims[1]-1 && realExtents[3]==dims[1]);
              }
            }
          
          int i=0;
          while(i<realDims[0])
            {
            int isGhost=jIsGhost;
            if(!isGhost)
              {
              if(realDims[0]>1)
                {
                isGhost=(i==0 && realExtents[0]==0 ) || (i==realDims[0]-1 && realExtents[1]==dims[0]);
                }
              }
            if(isGhost)
              {
              (*ptr)=1;
              }
            else
              {
              (*ptr)=0;
              }
            ++ptr;
            ++i;
            }
          ++j;
          }
        ++k;
        }
      }
    //this->MergeVectors(cd);
    blockIterator->Next();
    } // while
  delete blockIterator;
  
#endif //  if 0 skip loading for valgrind test
  // All files seem to have 1 ghost level.
//  this->AddGhostLevelArray(1);
 
  // At this point, each processor has its own blocks
  // They have to exchange the blocks they have get a unique id for
  // each block over the all dataset.
  
 #if 0
  // Remove the bad ghost cells from each block
  // Bad ghost cells are ghost cells out of the bounding box of the dataset.
  
  unsigned int numberOfLevels=hb->GetNumberOfLevels();
  unsigned int level=0;
  while(level<numberOfLevels)
    {
    int numberOfDataSets=hb->GetNumberOfDataSets(level);
    int i=0;
    while(i<numberOfDataSets)
      {
      vtkDataObject *do=hb->GetDataSet(level,i);
      vtkUniformGrid *ug=dynamic_cast<vtkUniformGrid *>(do);
      if(ug!=0)
        {
        // Mark the bounding cells as ghost cells of level 1.
        vtkUnsignedCharArray *ghostArray=vtkUnsignedCharArray::New();
        ghostArray->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
        ghostArray->SetName("vtkGhostLevels"); //("vtkGhostLevels");
        cd->AddArray(ghostArray);
        ghostArray->Delete();
        unsigned char *ptr =static_cast<unsigned char*>(ghostArray->GetVoidPointer(0));
        
        int k=0;
        while(k<dims[2])
          {
          int kIsGhost;
          if(dims[2]==1)
            {
            kIsGhost=0;
            }
          else
            {
            kIsGhost=k==0 || (k==dims[2]-1);
            }
          
          int j=0;
          while(j<dims[1])
            {
            int jIsGhost=kIsGhost;
            if(!jIsGhost)
              {
              if(dims[1]>1)
                {
                jIsGhost=j==0 || (j==dims[1]-1);
                }
              }
            
            int i=0;
            while(i<dims[0])
              {
              int isGhost=jIsGhost;
              if(!isGhost)
                {
                if(dims[0]>1)
                  {
                  isGhost=i==0 || (i==dims[0]-1);
                  }
                }
              if(isGhost)
                {
                (*ptr)=1;
                }
              else
                {
                (*ptr)=0;
                }
              ++ptr;
              ++i;
              }
            ++j;
            }
          ++k;
          }
        }
      else // rectilinear grid case
        {
        vtkRectilinearGrid *rg=dynamic_cast<vtkRectilinearGrid *>(ds);
        }
      ++i;
      }
    ++level;
    }
#endif
  
  // Update the number of levels.
  unsigned int numberOfLevels=hb->GetNumberOfLevels();
  
  if(this->IsAMR)
    {
    unsigned long ulintMsgValue;
    // Update it from the children
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        // Grab the number of levels from left child
        this->Controller->Receive(&ulintMsgValue, 1, left,
                                  VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS);
        if(numberOfLevels<ulintMsgValue)
          {
          numberOfLevels=ulintMsgValue;
          }
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          // Grab the number of levels from right child
          this->Controller->Receive(&ulintMsgValue, 1, right,
                                    VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS);
          if(numberOfLevels<ulintMsgValue)
            {
            numberOfLevels=ulintMsgValue;
            }
          }
        }
      }
    
    ulintMsgValue=numberOfLevels;
    
    // Send local to parent, Receive global from the parent.
    if(processNumber>0) // not root (nothing to do if root)
      {
//      parent=this->GetParentProcessor(processNumber);
      this->Controller->Send(&ulintMsgValue, 1, parent,
                             VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS);
      this->Controller->Receive(&ulintMsgValue, 1, parent,
                                VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
      numberOfLevels=ulintMsgValue;
      }
    
    // Send it to children.
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        this->Controller->Send(&ulintMsgValue, 1, left,
                               VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          this->Controller->Send(&ulintMsgValue, 1, right,
                                 VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
          }
        }
      }
    
    if(numberOfLevels>hb->GetNumberOfLevels())
      {
      hb->SetNumberOfLevels(numberOfLevels);
      }
    } //  if(this->IsAMR)
  // At this point, the global number of levels is set in each processor.
  
  // Update each level
  unsigned int level=0;
  int intMsgValue;
  while(level<numberOfLevels)
    {
    int numberOfDataSets=hb->GetNumberOfDataSets(level);
    int totalNumberOfDataSets=numberOfDataSets;
    int leftNumberOfDataSets=0;
    int rightNumberOfDataSets=0;
    // Get number of dataset of each child
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        // Grab info the number of datasets from left child
        this->Controller->Receive(&intMsgValue, 1, left,
                                  VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS);
        leftNumberOfDataSets=intMsgValue;
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          // Grab info the number of datasets from right child
          this->Controller->Receive(&intMsgValue, 1, right,
                                    VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS);
          rightNumberOfDataSets=intMsgValue;
          }
        }
      }

    int globalIndex;
    if(processNumber==0) // root
      {
      totalNumberOfDataSets=numberOfDataSets+leftNumberOfDataSets
        +rightNumberOfDataSets;
      globalIndex=0;
      }
    else
      {
      // Send local to parent, Receive global from the parent.
      intMsgValue=numberOfDataSets+leftNumberOfDataSets+rightNumberOfDataSets;
      this->Controller->Send(&intMsgValue, 1, parent,
                             VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS);
      this->Controller->Receive(&intMsgValue, 1, parent,
                                VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
      totalNumberOfDataSets=intMsgValue;
      this->Controller->Receive(&intMsgValue, 1, parent,
                                VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
      globalIndex=intMsgValue;
      }
    
    // Send it to children.
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        intMsgValue=totalNumberOfDataSets;
        this->Controller->Send(&intMsgValue, 1, left,
                               VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
        intMsgValue=globalIndex+numberOfDataSets;
        this->Controller->Send(&intMsgValue, 1, left,
                               VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          intMsgValue=totalNumberOfDataSets;
          this->Controller->Send(&intMsgValue, 1, right,
                                 VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
          intMsgValue=globalIndex+numberOfDataSets+leftNumberOfDataSets;
          this->Controller->Send(&intMsgValue, 1, right,
                                 VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
          }
        }
      }
    
    // Update the level.
    if(totalNumberOfDataSets>numberOfDataSets)
      {
      hb->SetNumberOfDataSets(level,totalNumberOfDataSets);
      int i;
      if(globalIndex>0)
        {
        // move the datasets to the right indices
        i=0;
        int j=globalIndex;
        while(i<numberOfDataSets)
          {
          hb->SetDataSet(level,j,hb->GetDataSet(level,i));
          ++i;
          ++j;
          }
        // add null pointers at the beginning
        i=0;
        while(i<globalIndex)
          {
           hb->SetDataSet(level,i,0);
          ++i;
          }
        }
      // add null pointers at the end
      i=globalIndex+numberOfDataSets;
      while(i<totalNumberOfDataSets)
        {
        hb->SetDataSet(level,i,0);
        ++i;
        }
      }
    ++level;
    }
#if 0
  //  Display the block list for each level
  
  level=0;
  numberOfLevels=hb->GetNumberOfLevels();
  while(level<numberOfLevels)
    {
    cout<<processNumber<<" level="<<level<<"/"<<numberOfLevels<<endl;
    int totalNumberOfDataSets=hb->GetNumberOfDataSets(level);
    int i=0;
    while(i<totalNumberOfDataSets)
      {
      cout<<processNumber<<" dataset="<<i<<"/"<<totalNumberOfDataSets;
      if(hb->GetDataSet(level,i)==0)
        {
        cout<<" Void"<<endl;
        }
      else
        {
        cout<<" Exists"<<endl;
        }
      ++i;
      }
    ++level;
    }
#endif
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::SetUpdateBlocks(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // See vtkCompositeDataPipeline class doc for explanations.
  
  vtkInformation *info=outputVector->GetInformationObject(0);
  vtkHierarchicalDataInformation *compositeInfo=
    vtkHierarchicalDataInformation::SafeDownCast(info->Get(
      vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));
  
  if (!compositeInfo)
    {
    vtkErrorMacro("Composite information not found. ");
    return 0;
    }

  vtkHierarchicalDataInformation *updateInfo=
    vtkHierarchicalDataInformation::New();
  info->Set(vtkCompositeDataPipeline::UPDATE_BLOCKS(), updateInfo);
  updateInfo->Delete();
  
  int numLevels=compositeInfo->GetNumberOfLevels();
  
  vtkDataObject *doOutput=info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkHierarchicalDataSet *hb=vtkHierarchicalDataSet::SafeDownCast(doOutput); 
  
  updateInfo->SetNumberOfLevels(numLevels);
  int level=0;
  while(level<numLevels)
    {
    int numBlocks=compositeInfo->GetNumberOfDataSets(level);
    updateInfo->SetNumberOfDataSets(level,numBlocks);
    
    // find the first index
    int i=0;
    int found=0;
    while(!found && i<numBlocks)
      {
      found=hb->GetDataSet(level,i)!=0;
      ++i;
      }
    if(found)
      {
      --i;
      int done=0;
      while(!done && i<numBlocks)
        {
        done=hb->GetDataSet(level,i)==0;
        if(!done)
          {
          updateInfo->GetInformation(level,i)->Set(vtkCompositeDataPipeline::MARKED_FOR_UPDATE(),1);
          }
        ++i;
        }
      }
    ++level;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::AddGhostLevelArray(int numLevels)
{
  (void)numLevels;
  /*
  int numCells = output->GetNumberOfCells();
  int numBlocks = output->GetNumberOfBlocks();
  vtkUnsignedCharArray* array = vtkUnsignedCharArray::New();
  int blockId;
  int dims[3];
  int i, j, k;
  unsigned char* ptr;
  int iLevel, jLevel, kLevel, tmp;

  output->SetNumberOfGhostLevels(numLevels);
  array->SetNumberOfTuples(numCells);
  ptr = (unsigned char*)(array->GetVoidPointer(0));


  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    output->GetBlockCellDimensions(blockId, dims);    
    for (k = 0; k < dims[2]; ++k)
      {
      kLevel = numLevels - k;
      tmp = k - dims[2] + 1 + numLevels;
      if (tmp > kLevel) { kLevel = tmp;}
      if (dims[2] == 1) {kLevel = 0;}
      for (j = 0; j < dims[1]; ++j)
        {
        jLevel = kLevel;
        tmp = numLevels - j;
        if (tmp > jLevel) { jLevel = tmp;}
        tmp = j - dims[1] + 1 + numLevels;
        if (tmp > jLevel) { jLevel = tmp;}
        if (dims[1] == 1) {jLevel = 0;}
        for (i = 0; i < dims[0]; ++i)
          {
          iLevel = jLevel;
          tmp = numLevels - i;
          if (tmp > iLevel) { iLevel = tmp;}
          tmp = i - dims[0] + 1 + numLevels;
          if (tmp > iLevel) { iLevel = tmp;}
          if (dims[0] == 1) {iLevel = 0;}
          if (iLevel <= 0)
            {
            *ptr = 0;
            }
          else
            {
            *ptr = iLevel;
            }
          ++ptr;
          }
        }
      }
    }

  //array->SetName("Test");
  array->SetName("vtkGhostLevels");
  output->GetCellData()->AddArray(array);
  array->Delete();
  */
}

//----------------------------------------------------------------------------
void vtkSpyPlotReader::SelectionModifiedCallback(vtkObject*, unsigned long,
  void* clientdata, void*)
{
  static_cast<vtkSpyPlotReader*>(clientdata)->Modified();
}
//-----------------------------------------------------------------------------
int vtkSpyPlotReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkSpyPlotReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::SetCellArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->CellDataArraySelection->EnableArray(name);
    }
  else
    {
    this->CellDataArraySelection->DisableArray(name);
    }
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::MergeVectors(vtkDataSetAttributes* da)
{
  int numArrays = da->GetNumberOfArrays();
  int idx;
  vtkDataArray *a1, *a2, *a3;
  int flag = 1;

  // Loop merging arrays.
  // Since we are modifying the list of arrays that we are traversing,
  // merge one set of arrays at a time.
  while (flag)
    {
    flag = 0;  
    for (idx = 0; idx < numArrays-1 && !flag; ++idx)
      {
      a1 = da->GetArray(idx);
      a2 = da->GetArray(idx+1);
      if (idx+2 < numArrays)
        {
        a3 = da->GetArray(idx+2);
        if (this->MergeVectors(da, a1, a2, a3))
          {
          flag = 1;
          continue;
          }    
        if (this->MergeVectors(da, a3, a2, a1))
          {
          flag = 1;
          continue;
          }
        }
      if (this->MergeVectors(da, a1, a2))
        {
        flag = 1;
        continue;
        }
      if (this->MergeVectors(da, a2, a1))
        {
        flag = 1;
        continue;
        }
      }
    }
}

//-----------------------------------------------------------------------------
// The templated execute function handles all the data types.
  template <class T>
void vtkMergeVectorComponents(vtkIdType length,
  T *p1, T *p2, T *p3, T *po)
{
  vtkIdType idx;
  if (p3)
    {
    for (idx = 0; idx < length; ++idx)
      {
      *po++ = *p1++;
      *po++ = *p2++;
      *po++ = *p3++;
      }
    }
  else
    {
    for (idx = 0; idx < length; ++idx)
      {
      *po++ = *p1++;
      *po++ = *p2++;
      *po++ = (T)0;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::MergeVectors(vtkDataSetAttributes* da, 
  vtkDataArray * a1, vtkDataArray * a2, vtkDataArray * a3)
{
  int prefixFlag = 0;

  if (a1 == 0 || a2 == 0 || a3 == 0)
    {
    return 0;
    }
  if(a1->GetNumberOfTuples() != a2->GetNumberOfTuples() ||
    a1->GetNumberOfTuples() != a3->GetNumberOfTuples())
    { // Sanity check.  Should never happen.
    return 0;
    }
  if(a1->GetDataType()!=a2->GetDataType()||a1->GetDataType()!=a3->GetDataType())
    {
    return 0;
    }
  if(a1->GetNumberOfComponents()!=1 || a2->GetNumberOfComponents()!=1 ||
    a3->GetNumberOfComponents()!=1)
    {
    return 0;
    }
  const char *n1, *n2, *n3;
  int e1, e2, e3;
  n1 = a1->GetName();
  n2 = a2->GetName();
  n3 = a3->GetName();
  if (n1 == 0 || n2 == 0 || n3 == 0)
    {
    return 0;
    }  
  e1 = strlen(n1)-1;
  e2 = strlen(n2)-1;
  e3 = strlen(n3)-1;
  if(e1!=e2 || e1 != e3)
    {
    return 0;
    }
  if (strncmp(n1+1,n2+1,e1)==0 && strncmp(n1+1,n3+1,e1)==0)
    { // Trailing characters are the same. Check for prefix XYZ.
    if ((n1[0]!='X' || n2[0]!='Y' || n3[0]!='Z') &&
      (n1[0]!='x' || n2[0]!='x' || n3[0]!='x'))
      { // Since last characters are the same, there is no way
      // the names can have postfix XYZ.
      return 0;
      }
    prefixFlag = 1;
    }
  else
    { // Check for postfix case.
    if (strncmp(n1,n2,e1)!=0 || strncmp(n1,n3,e1)!=0)
      { // Not pre or postfix.
      return 0;
      }
    if ((n1[e1]!='X' || n2[e2]!='Y' || n3[e3]!='Z') &&
      (n1[e1]!='x' || n2[e2]!='x' || n3[e3]!='x'))
      { // Tails are the same, but postfix not XYZ.
      return 0;
      }
    }
  // Merge the arrays.
  vtkDataArray* newArray = a1->NewInstance();
  newArray->SetNumberOfComponents(3);
  vtkIdType length = a1->GetNumberOfTuples();
  newArray->SetNumberOfTuples(length);
  void *p1 = a1->GetVoidPointer(0);
  void *p2 = a2->GetVoidPointer(0);
  void *p3 = a3->GetVoidPointer(0);
  void *pn = newArray->GetVoidPointer(0);
  switch (a1->GetDataType())
    {
    vtkTemplateMacro5(vtkMergeVectorComponents, length, 
      (VTK_TT*)p1,(VTK_TT*)p2,(VTK_TT*)p3,(VTK_TT*)pn);
  default:
    vtkErrorMacro(<< "Execute: Unknown ScalarType");
    return 0;
    }
  if (prefixFlag)
    {
    newArray->SetName(n1+1);
    }
  else
    {
    char* name = new char[e1+2];
    strncpy(name,n1,e1);
    name[e1] = '\0';
    newArray->SetName(name);
    delete [] name;
    }
  da->RemoveArray(n1);    
  da->RemoveArray(n2);    
  da->RemoveArray(n3);
  da->AddArray(newArray);
  newArray->Delete();    
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::MergeVectors(vtkDataSetAttributes* da, 
  vtkDataArray * a1, vtkDataArray * a2)
{
  int prefixFlag = 0;

  if (a1 == 0 || a2 == 0)
    {
    return 0;
    }
  if (a1->GetNumberOfTuples() != a2->GetNumberOfTuples())
    { // Sanity check.  Should never happen.
    return 0;
    }
  if (a1->GetDataType() != a2->GetDataType())
    {
    return 0;
    }
  if (a1->GetNumberOfComponents() != 1 || a2->GetNumberOfComponents() != 1)
    {
    return 0;
    }
  const char *n1, *n2;
  int e1, e2;
  n1 = a1->GetName();
  n2 = a2->GetName();
  if (n1 == 0 || n2 == 0)
    {
    return 0;
    }  
  e1 = strlen(n1)-1;
  e2 = strlen(n2)-1;
  if (e1 != e2 )
    {
    return 0;
    }
  if ( strncmp(n1+1,n2+1,e1) == 0)
    { // Trailing characters are the same. Check for prefix XYZ.
    if ((n1[0]!='X' || n2[0]!='Y') && (n1[0]!='x' || n2[0]!='x'))
      { // Since last characters are the same, there is no way
      // the names can have postfix XYZ.
      return 0;
      }
    prefixFlag = 1;
    }
  else
    { // Check for postfix case.
    if (strncmp(n1,n2,e1) != 0)
      { // Not pre or postfix.
      return 0;
      }
    if ((n1[e1]!='X' || n2[e2]!='Y') && (n1[e1]!='x' || n2[e2]!='x'))
      { // Tails are the same, but postfix not XYZ.
      return 0;
      }
    }
  // Merge the arrays.
  vtkDataArray* newArray = a1->NewInstance();
  // Creae the third componnt and set to 0.
  newArray->SetNumberOfComponents(3);
  vtkIdType length = a1->GetNumberOfTuples();
  newArray->SetNumberOfTuples(length);
  void *p1 = a1->GetVoidPointer(0);
  void *p2 = a2->GetVoidPointer(0);
  void *pn = newArray->GetVoidPointer(0);
  switch (a1->GetDataType())
    {
    vtkTemplateMacro5(vtkMergeVectorComponents, length, 
      (VTK_TT*)p1,(VTK_TT*)p2,(VTK_TT*)0,(VTK_TT*)pn);
  default:
    vtkErrorMacro(<< "Execute: Unknown ScalarType");
    return 0;
    }
  if (prefixFlag)
    {
    newArray->SetName(n1+1);
    }
  else
    {
    char* name = new char[e1+2];
    strncpy(name,n1,e1);
    name[e1] = '\0';
    newArray->SetName(name);
    delete [] name;
    }
  da->RemoveArray(n1);    
  da->RemoveArray(n2);    
  da->AddArray(newArray);
  newArray->Delete();    
  return 1;
}

// The processors are views as a heap tree. The root is the processor of
// id 0.
//-----------------------------------------------------------------------------
int vtkSpyPlotReader::GetParentProcessor(int proc)
{
  int result;
  if(proc%2==1)
    {
    result=proc>>1; // /2
    }
  else
    {
    result=(proc-1)>>1; // /2
    }
  return result;
}

int vtkSpyPlotReader::GetLeftChildProcessor(int proc)
{
  return (proc<<1)+1; // *2+1
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "FileName: " << (this->FileName?this->FileName:"(none)") << endl;
  os << "TimeStep: " << this->TimeStep << endl;
  os << "TimeStepRange: " << this->TimeStepRange[0] << " " << this->TimeStepRange[1] << endl;
  if ( this->CellDataArraySelection )
    {
    os << "CellDataArraySelection:" << endl;
    this->CellDataArraySelection->PrintSelf(os, indent.GetNextIndent());
    }
  if ( this->Controller)
    {
    os << "Controller:" << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
    }
}
