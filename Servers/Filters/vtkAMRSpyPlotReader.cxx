/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRSpyPlotReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRSpyPlotReader.h"

#include "vtkObjectFactory.h"
#include "vtkCTHData.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkMultiProcessController.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <sys/stat.h>

#include "spcth_interface.h"

vtkCxxRevisionMacro(vtkAMRSpyPlotReader, "1.6");
vtkStandardNewMacro(vtkAMRSpyPlotReader);
vtkCxxSetObjectMacro(vtkAMRSpyPlotReader,Controller,vtkMultiProcessController);

//------------------------------------------------------------------------------
//==============================================================================
class vtkAMRSpyPlotReaderInternal
{
public:
  typedef vtkstd::map<vtkstd::string, SPCTH*> MapOfStringToSPCTH;
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  MapOfStringToSPCTH Files;
  vtkstd::string MasterFileName;

  void Initialize(const char* file)
    {
    if ( !file || file != this->MasterFileName )
      {
      this->Clean();
      }
    }
  void Clean()
    {
    MapOfStringToSPCTH::iterator it;
    for ( it = this->Files.begin(); it != this->Files.end(); ++ it )
      {
      spcth_finalize(it->second);
      it->second = 0;
      }
    this->Files.erase(this->Files.begin(), this->Files.end());
    }
};
//==============================================================================
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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


//------------------------------------------------------------------------------
vtkAMRSpyPlotReader::vtkAMRSpyPlotReader()
{
  this->Internals = new vtkAMRSpyPlotReaderInternal;
  this->FileName = 0;
  this->CurrentFileName = 0;
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
    &vtkAMRSpyPlotReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                            this->SelectionObserver);
  this->TimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkAMRSpyPlotReader::~vtkAMRSpyPlotReader()
{
  this->SetFileName(0);
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();
  this->Internals->Clean();
  delete this->Internals;
  this->Internals = 0;
  this->SetController(0);
}

//------------------------------------------------------------------------------
void vtkAMRSpyPlotReader::ExecuteInformation()
{
  struct stat fs;
  if(stat(this->FileName, &fs) != 0)
    {
    vtkErrorMacro("Cannot find file " << this->FileName);
    return;
    }

  ifstream ifs(this->FileName);
  if (!ifs )
    {
    vtkErrorMacro("Error opening file " << this->FileName);
    return;
    }
  char buffer[8];
  if ( !ifs.read(buffer, 7) )
    {
    vtkErrorMacro("Problem reading header of file: " << this->FileName);
    return;
    }
  buffer[7] = 0;
  ifs.close();
  if ( strcmp(buffer, "spydata") == 0 )
    {
    this->UpdateMetaData(this->FileName);
    this->GetOutput()->SetMaximumNumberOfPieces(1);
    }
  else if ( strcmp(buffer, "spycase") == 0 )
    {
    this->UpdateCaseFile(this->FileName);
    this->GetOutput()->SetMaximumNumberOfPieces(-1);
    }
  else
    {
    vtkErrorMacro("Not a SpyData file");
    return;
    }
}

//------------------------------------------------------------------------------
void vtkAMRSpyPlotReader::UpdateCaseFile(const char* fname)
{

  SPCTH* spcth;
  
  // Check to see if this is a different filename than before.
  // If we do have a difference file name we reset the 
  // filename map and start over/
  if (!this->GetCurrentFileName() || strcmp(fname,this->GetCurrentFileName()))
    {
    this->SetCurrentFileName(fname);
    this->Internals->Clean();
    }

  // Probably don't need to go through all of this everytime
  // but inside UpdateMetaData we set the timestep which 
  // obviously needs to happen
  ifstream ifs(this->FileName);
  if (!ifs )
    {
    vtkErrorMacro("Error opening file " << fname);
    return;
    }
  vtkstd::string line;
  if ( !::GetLineFromStream(ifs, line) )
    {
    vtkErrorMacro("Syntax error in case file " << fname);
    }

  while ( ::GetLineFromStream(ifs, line) )
    {
    // Check for blank lines
    if (line.length() == 0) 
      {
      continue;
      }
    vtkstd::string::size_type stp, etp;
    stp = line.find_first_not_of(" \n\t\r");
    etp = line.find_last_not_of(" \n\t\r");
    vtkstd::string f(line, stp, etp-stp+1);
    if ( f[0] == '#' )
      {
      continue;
      }
    if ( !::FileIsFullPath(f.c_str()) )
      {
      f = GetFilenamePath(this->FileName) + "/" + f;
      }
      
    // Store all the file names and spcth structures in our internal 'file map'
    spcth = spcth_initialize();
    this->Internals->Files[f.c_str()] = spcth;
    
    }
    
    // Okay now open just the first file to get meta data
    // See CAVEATS in the documentation
    vtkAMRSpyPlotReaderInternal::MapOfStringToSPCTH::iterator it;
    it = this->Internals->Files.begin();
    this->UpdateMetaData(it->first.c_str());
    vtkDebugMacro("Reading Meta Data in UpdateCaseFile(ExecuteInformation) from file: " << it->first.c_str());
}

//------------------------------------------------------------------------------
void vtkAMRSpyPlotReader::UpdateMetaData(const char* fname)
{
  int i, x, y, z, field;
  int num_time_steps, num_blocks, num_cell_fields;

  struct stat fs;
  if(stat(this->FileName, &fs) != 0)
    {
    vtkErrorMacro("Cannot find file " << this->FileName);
    return;
    }

  SPCTH* spcth = 0;
  vtkAMRSpyPlotReaderInternal::MapOfStringToSPCTH::iterator it = this->Internals->Files.find(fname);
  if ( it == this->Internals->Files.end() )
    {
    vtkErrorMacro("Could not find file " << fname << " in the internal file map!");
    }
  else
    {
    spcth = it->second;
    }

  // Open the file and get # time steps
  spcth_openSpyFile(spcth, fname);
  num_time_steps = spcth_getNumTimeSteps(spcth);
  this->TimeStepRange[1] = num_time_steps-1;
  int timestep = this->TimeStep;
  if ( this->TimeStep < 0 || this->TimeStep >= num_time_steps )
    {
    vtkErrorMacro("TimeStep set to outside of the range 0 - " << (num_time_steps-1));
    timestep = 0;
    }

  // Set the reader to read the first
  spcth_setTimeStep(spcth, spcth_getTimeStepValue(spcth, timestep));

  // Print info
  vtkDebugMacro("File has " << num_time_steps << " timesteps");
  vtkDebugMacro("Timestep values:")
  for(i=0; i< num_time_steps; ++i)
    {
    vtkDebugMacro(<< i << ": " << spcth_getTimeStepValue(spcth, i));
    }

  // Block Stuff
  num_blocks = spcth_getNumberOfDataBlocksForCurrentTime(spcth);
  vtkDebugMacro("Number of Blocks: " << num_blocks);

  // Dimensions for each block
  for(i=0; i< num_blocks; ++i)
    {
    spcth_getDataBlockDimensions(spcth, i, &x, &y, &z);
    vtkDebugMacro("Block Dimensions(" << i << "): " <<  x << " " << y << " " << z);
    vtkDebugMacro("Block level(" << i << "): " << spcth_getDataBlockLevel(spcth, i));
    }


  // Cell Field stuff
  num_cell_fields = spcth_getNumberOfCellFields(spcth);
  vtkDebugMacro("Number of Cell Fields: " << num_cell_fields);
  for(field=0; field< num_cell_fields; ++field)
    {
    const char* fieldName = spcth_getCellFieldDescription(spcth, field);
    vtkDebugMacro("Cell Field Name -- Description(" << field << "): " << fieldName << " -- " << spcth_getCellFieldDescription(spcth, field));
    if ( !this->CellDataArraySelection->ArrayExists(fieldName) )
      {
      this->CellDataArraySelection->AddArray(fieldName);
      }
    }
}

#define vtkMIN(x, y) (((x)<(y))?(x):(y))

//------------------------------------------------------------------------------
void vtkAMRSpyPlotReader::Execute()
{
  SPCTH* spcth = 0;
  double blockSpacing[3];
  double toplevel_origin[3] = { 0, 0, 0 };
  double toplevel_spacing[3] = { 1, 1, 1 };
  vtkCTHData* output = this->GetOutput();

  vtkIdType total_cells = 0;

  int numFiles = static_cast<int>(this->Internals->Files.size());

  // My understanding is that by setting SetMaximumNumberOfPieces(-1) 
  // then GetUpdateNumberOfPieces() should always return the number
  // of processors in the parallel job and GetUpdatePiece() should
  // return the specific process number
  int processNumber  = output->GetUpdatePiece();
  int numProcessors  = output->GetUpdateNumberOfPieces();

  // Right now reader does not have support for more processors than files
  if ( numProcessors > numFiles)
    {
    vtkErrorMacro("AMRSpy Reader does not currently work with more processors than files" << endl);
    return;
    }

  int start,end;
  int numFilesPerProcess = numFiles / numProcessors;
  int leftOverFiles = numFiles - (numFilesPerProcess*numProcessors);
  if (processNumber < leftOverFiles)
    {
    start = (numFilesPerProcess+1) * processNumber;
    end = start + (numFilesPerProcess+1) - 1;
    }
  else
    {
    start = numFilesPerProcess * processNumber + leftOverFiles;
    end = start + numFilesPerProcess - 1;
    }
  cout << processNumber << ": (out of "<< numProcessors << ") start: " << start << " end:" << end << endl;


  vtkAMRSpyPlotReaderInternal::MapOfStringToSPCTH::iterator it;

  // Loop until I hit my start file
  int index=0;
  for(it = this->Internals->Files.begin(); index<start; ++it,++index);

  // Now loop and read until I hit my end file
  for(; index<=end; ++it,++index)
    {
    cout << processNumber << " cf: " << it->first.c_str() << endl;
    }

  cout << "-----------------------------------------------------------" << endl;

  int block, field;

  // Loop until I hit my start file
  index=0;
  for(it = this->Internals->Files.begin(); index<start; ++it,++index);

  // Now loop and read until I hit my end file
  for(; index<=end; ++it,++index)
    {
    // Because only the first file got 'UpdataMetaData'
    // called during ExecuteInformation we need to make
    // sure that every file we use first has it's
    // spcth data structure properly filled in
    this->UpdateMetaData(it->first.c_str());
    vtkDebugMacro("Reading Meta Data in Execute from file: " << it->first.c_str());
    
    vtkDebugMacro("Processing File: " << it->first.c_str());
    spcth = it->second;
    int number_of_blocks = spcth_getNumberOfDataBlocksForCurrentTime(spcth);
    for ( block = 0; block < number_of_blocks; ++ block )
      {
      int cc;
      int dims[3];
      double bounds[6];
      spcth_getDataBlockDimensions(spcth, block, dims, dims+1, dims+2);
      int level = spcth_getDataBlockLevel(spcth, block);
      int b = spcth_getDataBlockBounds(spcth, block, bounds);
      vtkDebugMacro("Level: " << level);
      vtkDebugMacro("Dimensions: " << dims[0] << ", " << dims[1] << ", " << dims[2]);
      vtkDebugMacro("Bounds[" << block << "] " << b << " "
        << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
        << bounds[3] << ", " << bounds[4] << ", " << bounds[5])
      for ( cc = 0; cc < 3; cc ++ )
        {
        // spacing for this block
        blockSpacing[cc] = (bounds[cc*2+1] - bounds[cc*2])/dims[cc];
        // Ignore ghost levels when computing the origin.
        toplevel_origin[cc] = vtkMIN(toplevel_origin[cc],bounds[cc*2]+blockSpacing[cc]);
        // We only really need to do this once ...
        toplevel_spacing[cc] = blockSpacing[cc]
          * pow(static_cast<double>(2), static_cast<double>(level));
        }
      vtkDebugMacro("Spacing: " << toplevel_spacing[0] << " " << toplevel_spacing[1] << " " << toplevel_spacing[2]);
      int dim = 0;
      for ( cc = 0; cc <3; cc ++ )
        {
        if ( dims[cc] > 1 )
          {
          dim = cc;
          }
        }
      for ( cc = 0; cc < 3; cc ++ )
        {
        if ( dims[cc] == 1 )
          {
          toplevel_spacing[cc] = toplevel_spacing[dim];
          }
        }
      vtkDebugMacro("Origin: " << toplevel_origin[0] << " " << toplevel_origin[1] << " " << toplevel_origin[2]);
      vtkDebugMacro("Fixed Spacing: " << toplevel_spacing[0] << " " << toplevel_spacing[1] << " " << toplevel_spacing[2]);
        total_cells += dims[0] * dims[1] * dims[2];
      }
    }

  // Compute a common origin among processes.
  if (this->Controller)
    {
    int idx;
    double otherOrigin[3];
    int numProcs = this->Controller->GetNumberOfProcesses();
    int myId = this->Controller->GetLocalProcessId();
    if (myId == 0)
      {
      // Get all origins to find the smallest.
      for (idx = 1; idx < numProcs; ++idx)
        {
        this->Controller->Receive(otherOrigin, 3, idx, 288300);
        toplevel_origin[0] = vtkMIN(toplevel_origin[0],otherOrigin[0]);
        toplevel_origin[1] = vtkMIN(toplevel_origin[1],otherOrigin[1]);
        toplevel_origin[2] = vtkMIN(toplevel_origin[2],otherOrigin[2]);
        }
      // Send it back to all processes.
      for (idx = 1; idx < numProcs; ++idx)
        {
        this->Controller->Send(toplevel_origin, 3, idx, 288301);
        }
      }
    else
      {
      this->Controller->Send(toplevel_origin, 3, 0, 288300);
      this->Controller->Receive(toplevel_origin, 3, 0, 288301);
      }
    }

  output->SetTopLevelOrigin(toplevel_origin);
  output->SetTopLevelSpacing(toplevel_spacing);
  vtkCellData* cd = output->GetCellData();

  int numFields = spcth_getNumberOfCellFields(spcth);
  for ( field = 0; field < numFields; ++ field )
    {
    const char* fname = spcth_getCellFieldDescription(spcth, field);
    if ( !this->CellDataArraySelection->ArrayIsEnabled(fname) )
      {
      continue;
      }
    vtkDoubleArray* array = vtkDoubleArray::New();
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(total_cells);
    array->SetName(fname);
    cd->RemoveArray(fname);
    cd->AddArray(array);
    array->Delete();
    }

  //cout << processNumber << " ---------------------------------------------------" << endl;

  vtkIdType current_index = 0;

  // Loop until I hit my start file
  index=0;
  for(it = this->Internals->Files.begin(); index<start; ++it,++index);

  // Now loop and read until I hit my end file
  for(; index<=end; ++it,++index)
    {
    vtkDebugMacro("Reading data from file: " << it->first.c_str());
    spcth = it->second;
    for ( block = 0; block < spcth_getNumberOfDataBlocksForCurrentTime(spcth); ++ block )
      {
      int cc;
      vtkIdType global_block = output->InsertNextBlock();
      int dims[3];
      double bounds[6];
      spcth_getDataBlockDimensions(spcth, block, dims, dims+1, dims+2);
      int level = spcth_getDataBlockLevel(spcth, block);
      spcth_getDataBlockBounds(spcth, block, bounds);

      int extents[6];
      for ( cc = 0; cc < 3; cc ++ )
        {
        double spacing = toplevel_spacing[cc]
          / pow(static_cast<double>(2), static_cast<double>(level));
        int minext = (int)(floor((bounds[cc*2] - toplevel_origin[cc]) / spacing + .5));
          extents[cc * 2] = minext;
          extents[cc*2+1] = minext + dims[cc] -1;
          }

        output->SetBlockCellExtent(global_block, level,
          extents[0], extents[1], extents[2], extents[3], extents[4], extents[5]);
        vtkDebugMacro("SetBlockCellExtent: " << block << "/" << global_block << " " << level
          << " " << extents[0] << " " << extents[1] << " " << extents[2]
          << " " << extents[3] << " " << extents[4] << " " << extents[5]);


        vtkDebugMacro("Executing block: " << block);
        for ( field = 0; field < numFields; field ++ )
          {
          const char* fname = spcth_getCellFieldDescription(spcth, field);
          if ( !this->CellDataArraySelection->ArrayIsEnabled(fname) )
            {
            continue;
            }
          vtkDataArray* array = cd->GetArray(fname);
          vtkDebugMacro("Field " << fname << " (" << field << ") ");
          // Now actually go and get the data for this block
          if (!spcth_getCellFieldData(spcth, block,field, static_cast<double*>(array->GetVoidPointer(current_index)))) 
            {
            vtkErrorMacro("Problem reading block: " << block << ", field: " << field << endl);
            }
          }
        current_index += dims[0] * dims[1] * dims[2];
        }
    }
  
  // Merge vector arrays.
  this->MergeVectors(output->GetCellData());
  
  // All files seem to have 1 ghost level.
  this->AddGhostLevelArray(1);
  output->CheckAttributes();
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}



//----------------------------------------------------------------------------
void vtkAMRSpyPlotReader::AddGhostLevelArray(int numLevels)
{
  vtkCTHData* output = this->GetOutput();
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
}

//----------------------------------------------------------------------------
void vtkAMRSpyPlotReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                             void* clientdata, void*)
{
  static_cast<vtkAMRSpyPlotReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
int vtkAMRSpyPlotReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkAMRSpyPlotReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkAMRSpyPlotReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkAMRSpyPlotReader::SetCellArrayStatus(const char* name, int status)
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

//------------------------------------------------------------------------------
void vtkAMRSpyPlotReader::MergeVectors(vtkDataSetAttributes* da)
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

//----------------------------------------------------------------------------
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
 
//------------------------------------------------------------------------------
int vtkAMRSpyPlotReader::MergeVectors(vtkDataSetAttributes* da, 
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

//------------------------------------------------------------------------------
int vtkAMRSpyPlotReader::MergeVectors(vtkDataSetAttributes* da, 
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

//------------------------------------------------------------------------------
void vtkAMRSpyPlotReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "FileName: " << (this->FileName?this->FileName:"(none)") << endl;
  os << "TimeStep: " << this->TimeStep << endl;
  os << "TimeStepRange: " << this->TimeStepRange[0] << " " << this->TimeStepRange[1] << endl;
  if ( this->PointDataArraySelection )
    {
    os << "PointDataArraySelection:" << endl;
    this->PointDataArraySelection->PrintSelf(os, indent.GetNextIndent());
    }
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


