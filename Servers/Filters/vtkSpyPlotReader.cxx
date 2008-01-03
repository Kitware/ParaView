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

#include "vtkBoundingBox.h"
#include "vtkByteSwap.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArraySelection.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkExtractCTHPart.h" // for the BOUNDS key
#include "vtkFloatArray.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedCharArray.h"

#include "vtkSpyPlotReaderMap.h"
#include "vtkSpyPlotUniReader.h"
#include "vtkSpyPlotBlock.h"
#include "vtkSpyPlotBlockIterator.h"
#include "vtkSpyPlotIStream.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <sys/stat.h>
#include <vtksys/SystemTools.hxx>
#include <assert.h>
#include <cctype>


#define vtkMIN(x, y) \
  (\
   ((x)<(y))?(x):(y)\
   )

#define coutVector6(x) (x)[0] << " " << (x)[1] << " " << (x)[2] << " " << (x)[3] << " " << (x)[4] << " " << (x)[5]
#define coutVector3(x) (x)[0] << " " << (x)[1] << " " << (x)[2]

vtkCxxRevisionMacro(vtkSpyPlotReader, "1.55");
vtkStandardNewMacro(vtkSpyPlotReader);
vtkCxxSetObjectMacro(vtkSpyPlotReader,Controller,vtkMultiProcessController);

void createSpyPlotLevelArray(vtkCellData *cd, int size, int level);

//=============================================================================



//-----------------------------------------------------------------------------
vtkSpyPlotReader::vtkSpyPlotReader()
{
  this->SetNumberOfInputPorts(0);
  
  this->Map = new vtkSpyPlotReaderMap;
  this->Bounds = new vtkBoundingBox;
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
  this->DownConvertVolumeFraction = 1;

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  
  this->DistributeFiles=0; // by default, distribute blocks, not files.
  this->GenerateLevelArray=0; // by default, do not generate level array.
  this->GenerateBlockIdArray=0; // by default, do not generate block id array.
  this->GenerateActiveBlockArray = 0; // by default do not generate active array

  this->TimeRequestedFromPipeline = false;
}

//-----------------------------------------------------------------------------
vtkSpyPlotReader::~vtkSpyPlotReader()
{
  this->SetFileName(0);
  this->SetCurrentFileName(0);
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->Map->Clean(0);
  delete this->Map;
  delete this->Bounds;
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
    //return 0;
    }
  if(!this->Superclass::RequestInformation(request,inputVector,outputVector))
    {
    return 0;
    }
  
  vtkMultiGroupDataInformation *compInfo
    =vtkMultiGroupDataInformation::New();

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
    return this->UpdateSpyDataFile(request, outputVector);
    }
  else if(strcmp(buffer,"spycase")==0)
    {
    return this->UpdateCaseFile(this->FileName, request, outputVector);
    }
  else
    {
    vtkErrorMacro("Not a SpyData file");
    return 0;
    }
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateSpyDataFile(vtkInformation* request, 
                                        vtkInformationVector* outputVector)
{
  // See if this is part of a series
  vtkstd::string extension = 
    vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  int currentNum, esize, isASeries=0;
  esize = extension.size();
  if (esize > 0 )
    {
    // See if this is a pure numerical extension - hence it's part 
    // of a series - if the first char is a . we need to exclude it
    // from the check
    const char *a = extension.c_str();
    char *ep;
    int b = 0;
    if (a[0] == '.')
      {
      b = 1;
      }
    
    if ((esize > b) && isdigit(a[b]))
      {
      currentNum = static_cast<int>(strtol(&(a[b]), &ep, 10));
      if (ep[0] == '\0')
        {
        isASeries = 1;
        }
      }
    }
  
  if (!isASeries)
    {
    this->SetCurrentFileName(this->FileName);

    vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator mapIt = 
      this->Map->Files.find(this->FileName);
    
    vtkSpyPlotUniReader* oldReader = 0;
    if ( mapIt != this->Map->Files.end() )
      {
      oldReader = this->Map->GetReader(mapIt, this);
      oldReader->Register(this);
      oldReader->Print(cout);
      }
    this->Map->Clean(oldReader);
    if ( oldReader )
      {
      this->Map->Files[this->FileName]=oldReader;
      oldReader->UnRegister(this);
      }
    else
      {
      this->Map->Files[this->FileName]= 0;
      vtkDebugMacro( << __LINE__ << " Create new uni reader: " 
                     << this->Map->Files[this->FileName] );
      }
    return this->UpdateMetaData(request, outputVector);
    }

  // Check to see if this is the same filename as before
  // if so then I already have the meta info, so just return
  if(this->GetCurrentFileName()!=0 &&
     strcmp(this->FileName,this->GetCurrentFileName())==0)
    {
    return 1;
    }
  this->SetCurrentFileName(this->FileName);
  this->Map->Clean(0);
  vtkstd::string fileNoExt = 
    vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
  vtkstd::string filePath = 
    vtksys::SystemTools::GetFilenamePath(this->FileName);

  // Now find all the files that make up the series that this file is part
  // of
  int idx = currentNum - 100;
  int last = currentNum;
  char buffer[1024];
  int found = currentNum;
  int minimum = currentNum;
  int maximum = currentNum;
  while ( 1 )
    {
    sprintf(buffer, "%s/%s.%d",filePath.c_str(), fileNoExt.c_str(), idx);
    if ( !vtksys::SystemTools::FileExists(buffer) )
      {
      int next = idx;
      for ( idx = last; idx > next; idx -- )
        {
        sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
        if ( !vtksys::SystemTools::FileExists(buffer) )
          {
          break;
          }
        else
          {
          found = idx;
          }
        }
      break;
      }
    last = idx;
    idx -= 100;
    }
  minimum = found;
  idx = currentNum + 100;
  last = currentNum;
  found = currentNum;
  while ( 1 )
    {
    sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
    if ( !vtksys::SystemTools::FileExists(buffer) )
      {
      int next = idx;
      for ( idx = last; idx < next; idx ++ )
        {
        sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
        if ( !vtksys::SystemTools::FileExists(buffer) )
          {
          break;
          }
        else
          {
          found = idx;
          }
        }
      break;
      }
    last = idx;
    idx += 100;
    }
  maximum = found;
  for ( idx = minimum; idx <= maximum; ++ idx )
    {
    sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
    this->Map->Files[buffer]=0;
    vtkDebugMacro( << __LINE__ << " Create new uni reader: " 
                   << this->Map->Files[buffer] );
    }
  // Okay now open just the first file to get meta data
  vtkDebugMacro("Reading Meta Data in UpdateCaseFile(ExecuteInformation) from file: " << this->Map->Files.begin()->first.c_str());
  return this->UpdateMetaData(request, outputVector);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateCaseFile(const char *fname,
                                     vtkInformation* request, 
                                     vtkInformationVector* outputVector)
{
  // Check to see if this is the same filename as before
  // if so then I already have the meta info, so just return
  if(this->GetCurrentFileName()!=0 &&
     strcmp(fname,this->GetCurrentFileName())==0)
    {
    return 1;
    }

  // Set case file name and clean/initialize file map
  this->SetCurrentFileName(fname);
  this->Map->Clean(0);

  // Setup the filemap and spcth structures
  ifstream ifs(this->FileName);
  if (!ifs )
    {
    vtkErrorMacro("Error opening file " << fname);
    return 0;
    }
  
  vtkstd::string line;
  if(!vtksys::SystemTools::GetLineFromStream(ifs,line)) // eat spycase line
    {
    vtkErrorMacro("Syntax error in case file " << fname);
    return 0;
    }

  while(vtksys::SystemTools::GetLineFromStream(ifs, line))
    {
    if(line.length()!=0)  // Skip blank lines
      {
      vtkstd::string::size_type stp = line.find_first_not_of(" \n\t\r");
      vtkstd::string::size_type etp = line.find_last_not_of(" \n\t\r");
      vtkstd::string f(line, stp, etp-stp+1);
      if(f[0]!='#') // skip comment
        {
        if(!vtksys::SystemTools::FileIsFullPath(f.c_str()))
          {
          f = vtksys::SystemTools::GetFilenamePath(this->FileName)+"/"+f;
          }
        this->Map->Files[f.c_str()]=0;
        vtkDebugMacro( << __LINE__ << " Create new uni reader: " << this->Map->Files[f.c_str()] );
        }
      }
    }

  // Okay now open just the first file to get meta data
  vtkDebugMacro("Reading Meta Data in UpdateCaseFile(ExecuteInformation) from file: " << this->Map->Files.begin()->first.c_str());
  return this->UpdateMetaData(request, outputVector);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateMetaData(vtkInformation* request,
                                     vtkInformationVector* outputVector)
{
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator it;
  it=this->Map->Files.begin();
  
  const char *fname=0;
  vtkSpyPlotUniReader *uniReader=0;
  
  if(it==this->Map->Files.end())
    {
    vtkErrorMacro("The internal file map is empty!");
    return 0;
    }

  fname=it->first.c_str();
  uniReader=this->Map->GetReader(it, this);

  int i;
  int num_time_steps;
  
  // Open the file and get the number time steps
  uniReader->SetFileName(fname);
  uniReader->ReadInformation();
  int* timeStepRange = uniReader->GetTimeStepRange();
  num_time_steps=timeStepRange[1] + 1;
  this->TimeStepRange[1]=timeStepRange[1];
  
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    double* timeArray = uniReader->GetTimeArray();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
                 timeArray,
                 num_time_steps);
    double timeRange[2];
    timeRange[0] = timeArray[0];
    timeRange[1] = timeArray[num_time_steps-1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), 
                 timeRange, 2);
    }

  if (!this->TimeRequestedFromPipeline)
    {
    this->CurrentTimeStep=this->TimeStep;
    }
  if(this->CurrentTimeStep<0||this->CurrentTimeStep>=num_time_steps)
    {
    vtkErrorMacro("TimeStep set to " << this->CurrentTimeStep << " outside of the range 0 - " << (num_time_steps-1) << ". Use 0.");
    this->CurrentTimeStep=0;
    }

  // Set the reader to read the first time step.
  uniReader->SetCurrentTimeStep(this->CurrentTimeStep);

  // Print info
  vtkDebugMacro("File has " << num_time_steps << " timesteps");
  vtkDebugMacro("Timestep values:");
  for(i=0; i< num_time_steps; ++i)
    {
    vtkDebugMacro(<< i << ": " << uniReader->GetTimeFromTimeStep(i));
    }

  // AMR (hierarchy of uniform grids) or flat mesh (set of rectilinear grids)
  this->IsAMR=uniReader->IsAMR();

  // Fields
  int fieldsCount = uniReader->GetNumberOfCellFields();
  vtkDebugMacro("Number of fields: " << fieldsCount);
  
  
  vtkstd::set<vtkstd::string> fileFields;
  
  int field;
  for(field=0; field<fieldsCount; ++field)
    {
    const char*fieldName=this->CellDataArraySelection->GetArrayName(field);
    vtkDebugMacro("Field #" << field << ": " << fieldName);
    fileFields.insert(fieldName);
    }
  // Now remove the existing array that were not found in the file.
  field=0;
  // the trick is that GetNumberOfArrays() may change at each step.
  while(field<this->CellDataArraySelection->GetNumberOfArrays())
    {
    if(fileFields.find(this->CellDataArraySelection->GetArrayName(field))==fileFields.end())
      {
      this->CellDataArraySelection->RemoveArrayByIndex(field);
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

template<class DataType>
int vtkSpyPlotRemoveBadGhostCells(DataType* dataType, vtkDataArray* dataArray, int realExtents[6], int realDims[3], int ptDims[3], int realPtDims[3])
{
  /*
    vtkDebugMacro( "//////////////////////////////////////////////////////////////////////////////////" );
    vtkDebugMacro( << dataArray << " vtkSpyPlotRemoveBadGhostCells(" << dataArray->GetName() << ")" );
    vtkDebugMacro( "DataArray: ");
    dataArray->Print(cout);
    vtkDebugMacro( "Real Extents: " << coutVector6(realExtents) );
    vtkDebugMacro( "Real Dims:    " << coutVector3(realDims) );
    vtkDebugMacro( "PT Dims:      " << coutVector3(ptDims) );
    vtkDebugMacro( "Real PT Dims: " << coutVector3(realPtDims) );
    vtkDebugMacro( "//////////////////////////////////////////////////////////////////////////////////" );
  */
  (void)* dataType;
  // skip some cell data.
  int xyz[3];
  int destXyz[3];
  DataType* dataPtr = static_cast<DataType*>(dataArray->GetVoidPointer(0));
  for (xyz[2] = realExtents[4], destXyz[2] = 0; 
       xyz[2] < realExtents[5]; ++xyz[2], ++destXyz[2])
    {
    for (xyz[1] = realExtents[2], destXyz[1] = 0;
         xyz[1] < realExtents[3]; ++xyz[1], ++destXyz[1])
      {
      for (xyz[0] = realExtents[0], destXyz[0] = 0;
           xyz[0] < realExtents[1]; ++xyz[0], ++destXyz[0])
        {
        dataPtr[vtkStructuredData::ComputeCellId(realPtDims,destXyz)] =
          dataPtr[vtkStructuredData::ComputeCellId(ptDims,xyz)];
        }
      }
    }
  dataArray->SetNumberOfTuples(realDims[0]*realDims[1]*realDims[2]);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::RequestData(
  vtkInformation *request,
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro( "--------------------------- Request Data --------------------------------" );
  vtkSpyPlotUniReader* uniReader = 0;

  vtkstd::vector<vtkRectilinearGrid*> grids;

  vtkInformation *info=outputVector->GetInformationObject(0);
  vtkDataObject *doOutput=info->Get(vtkDataObject::DATA_OBJECT());
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

  int tsLength =
    info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps =
    info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  vtkMultiGroupDataInformation *compInfo=
    vtkMultiGroupDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

  hb->Initialize(); // remove all previous blocks
  hb->SetMultiGroupDataInformation(compInfo);
  //int numFiles = this->Map->Files.size();

  // By setting SetMaximumNumberOfPieces(-1) 
  // then GetUpdateNumberOfPieces() should always return the number
  // of processors in the parallel job and GetUpdatePiece() should
  // return the specific process number
  int processNumber = 
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numProcessors =
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  
  // Update the timestep.
  if(info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    // Get the requested time step. We only supprt requests of a single time
    // step in this reader right now
    double *requestedTimeSteps = 
      info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    double timeValue = requestedTimeSteps[0];
    
    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < tsLength-1 && steps[cnt] < timeValue)
      {
      cnt++;
      }
    this->CurrentTimeStep = cnt;
    
    this->TimeRequestedFromPipeline = true;
    this->UpdateMetaData(request, outputVector);
    this->TimeRequestedFromPipeline = false;
    }
  else
    {
    this->UpdateMetaData(request, outputVector);
    }

  hb->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(),
                            steps+this->CurrentTimeStep, 1);  
  
  // Tell all of the unireaders that they need to make to check to see
  // if they are current
  this->Map->TellReadersToCheck(this);
  
  vtkSpyPlotBlock *block;
  vtkSpyPlotBlockIterator *blockIterator;
  if(this->DistributeFiles)
    {
    vtkDebugMacro("Distribute files");
    blockIterator=new vtkSpyPlotFileDistributionBlockIterator;
    }
  else
    {
    vtkDebugMacro("Distribute blocks");
    blockIterator=new vtkSpyPlotBlockDistributionBlockIterator;
    }
  
  blockIterator->Init(numProcessors,processNumber,this,this->Map,
                      this->CurrentTimeStep);

#if 1 // skip loading for valgrind test
  
  int total_num_of_blocks = blockIterator->GetNumberOfBlocksToProcess();
  int progressInterval = total_num_of_blocks / 10 + 1;
  int rightHasBounds = 0;
  int leftHasBounds = 0;
  // Note that in the process of getting the bounds 
  // all of the readers will get updated appropriately  
  this->SetGlobalBounds(blockIterator, total_num_of_blocks,
                        progressInterval, &rightHasBounds,
                        &leftHasBounds);
  // Set the global bounds of the reader
  double b[6];
  this->Bounds->GetBounds(b);
  info->Set(vtkExtractCTHPart::BOUNDS(), b, 6);
  if (!this->Bounds->IsValid())
    {
    // There were no bounds set - reader must have no blocks
    delete blockIterator;
    return 1;
    }
  
  // Read all files
  
  //vtkDebugMacro("there is (are) "<<numFiles<<" file(s)");
  // Read only the part of the file for this processNumber.
  int current_block_number;
 for(blockIterator->Start(), current_block_number=1; 
      blockIterator->IsActive();
      blockIterator->Next(), current_block_number++)
    {
    if ( !(current_block_number % progressInterval) )
      {
      this->UpdateProgress(
        0.6 + 
        0.4 * static_cast<double>(current_block_number)/total_num_of_blocks);
      }
    block=blockIterator->GetBlock();
    int numFields=blockIterator->GetNumberOfFields();
    uniReader=blockIterator->GetUniReader();

    int dims[3];
    int blockID = blockIterator->GetBlockID();
    int level = 0;
    int hasBadGhostCells;
    int realExtents[6];
    int realDims[3];
    int extents[6];
    vtkCellData* cd;
   
    // Get the dimensions of the block
    block->GetDimensions(dims);
    if (this->IsAMR)
      {
      hasBadGhostCells = this->PrepareAMRData(hb, block, &level, 
                                              extents, 
                                              realExtents, realDims, &cd);
      }
    else
      {
      vtkRectilinearGrid *rg;
      vtkDebugMacro( "Preparing Block: " << blockID << " " 
                     << uniReader->GetFileName() );
      hasBadGhostCells = this->PrepareData(hb, block, &rg, extents, realExtents,
                                           realDims, &cd);
      grids.push_back(rg);
      }

    vtkDebugMacro("Executing block: " << blockID);
    // uniReader->PrintMemoryUsage();

    // Now we need to deal with the field data and mapout
    // where the true ghost cells are
    if(!hasBadGhostCells)
      {
      this->UpdateFieldData(numFields, dims, level, blockID,
                            uniReader, cd);
      }
    else // we have some bad ghost cells
      {
      this->UpdateBadGhostFieldData(numFields, dims, realDims,
                                    realExtents, level, blockID,
                                    uniReader, cd);
      }
    // Add active block array, for debugging
    if (this->GenerateActiveBlockArray)
      {
      vtkUnsignedCharArray* activeArray = vtkUnsignedCharArray::New();
      activeArray->SetName("ActiveBlock");
      vtkIdType numCells=realDims[0]*realDims[1]*realDims[2];
      activeArray->SetNumberOfTuples(numCells);
      for (vtkIdType myIdx=0; myIdx<numCells; myIdx++)
        {
        activeArray->SetValue(myIdx, block->IsActive());
        }
      cd->AddArray(activeArray);
      activeArray->Delete();
      }

    //this->MergeVectors(cd);
    } // for
  delete blockIterator;
  
#endif //  if 0 skip loading for valgrind test
  // All files seem to have 1 ghost level.
//  this->AddGhostLevelArray(1);
 
  // At this point, each processor has its own blocks
  // They have to exchange the blocks they have get a unique id for
  // each block over the all dataset.
  

  // Update the number of levels.  
  if(this->Controller)
    {
    this->SetGlobalLevels(hb, processNumber, numProcessors,
                          rightHasBounds, leftHasBounds);
    }
  
  unsigned int numberOfLevels=hb->GetNumberOfLevels();
  unsigned int level;
 
  
  // Set the unique block id cell data
  if(this->GenerateBlockIdArray)
    {
    int blockId=0, k, i;
    numberOfLevels=hb->GetNumberOfLevels();
    for (blockId = 0, level = 0; level<numberOfLevels; level++)
      {
      int totalNumberOfDataSets=hb->GetNumberOfDataSets(level);
      for (i = 0; i < totalNumberOfDataSets; blockId++, i++)
        {
        vtkDataObject *dataObject=hb->GetDataSet(level,i);
        if(dataObject!=0)
          {
          vtkDataSet *ds=vtkDataSet::SafeDownCast(dataObject);
          assert("check: ds_exists" && ds!=0);
          vtkCellData *cd=ds->GetCellData();
          // Add the block id cell data array
          
          vtkDataArray *array=cd->GetArray("blockId");
          if(array!=0)
            {
            cd->RemoveArray("blockId"); // if this is not the first step,
            // make sure we have a clean array
            }
          
          array = vtkIntArray::New();
          cd->AddArray(array);
          array->Delete();
        
          array->SetName("blockId");
          array->SetNumberOfComponents(1);
          int c=ds->GetNumberOfCells();
          array->SetNumberOfTuples(c);
          int *ptr=static_cast<int *>(array->GetVoidPointer(0));
          for(k=0; k<c; k++)
            {
            ptr[k]=blockId;
            }
          }
        }
      }
    }
  
#if 0
  //  Display the block list for each level
  numberOfLevels=hb->GetNumberOfLevels();
  for (level = 0; level<numberOfLevels; level++)
    {
    cout<<processNumber<<" level="<<level<<"/"<<numberOfLevels<<endl;
    int totalNumberOfDataSets=hb->GetNumberOfDataSets(level);
    int i;
    for (i = 0; i<totalNumberOfDataSets; i++)
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
      }
    }
#endif
  
  /*
    vtkstd::vector<vtkRectilinearGrid*>::iterator it;
    for ( it = grids.begin(); it != grids.end(); ++ it )
    {
    (*it)->Print(cout);
    int cc;
    for ( cc = 0; cc < (*it)->GetCellData()->GetNumberOfArrays(); ++ cc )
    {
    (*it)->GetCellData()->GetArray(cc)->Print(cout);
    }
    }
  */
  
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
        (n1[0]!='x' || n2[0]!='y' || n3[0]!='z'))
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
        (n1[e1]!='x' || n2[e2]!='y' || n3[e3]!='z'))
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
    vtkTemplateMacro(
      vtkMergeVectorComponents( length, 
                                (VTK_TT*)p1,
                                (VTK_TT*)p2,
                                (VTK_TT*)p3,
                                (VTK_TT*)pn));
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
    if ((n1[0]!='X' || n2[0]!='Y') && (n1[0]!='x' || n2[0]!='y'))
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
    if ((n1[e1]!='X' || n2[e2]!='Y') && (n1[e1]!='x' || n2[e2]!='y'))
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
    vtkTemplateMacro(
      vtkMergeVectorComponents( length, 
                                (VTK_TT*)p1,
                                (VTK_TT*)p2,
                                (VTK_TT*)0,
                                (VTK_TT*)pn));
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

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::CanReadFile(const char* fname)
{
  ifstream ifs(fname, ios::binary|ios::in);
  if ( !ifs )
    {
    return 0;
    }
  vtkSpyPlotIStream spis;
  spis.SetStream(&ifs);
  char magic[8];
  if ( !spis.ReadString(magic, 8) )
    {
    vtkErrorMacro( "Cannot read magic" );
    return 0;
    }
  if ( strncmp(magic, "spydata", 7) != 0 &&
       strncmp(magic, "spycase", 7) != 0 )
    {
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::SetDownConvertVolumeFraction(int vf)
{
  if ( vf == this->DownConvertVolumeFraction )
    {
    return;
    }
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator mapIt;
  for ( mapIt = this->Map->Files.begin();
        mapIt != this->Map->Files.end();
        ++ mapIt )
    {
    this->Map->GetReader(mapIt, this)->SetDownConvertVolumeFraction(vf);
    }
  this->DownConvertVolumeFraction = vf;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "FileName: " << (this->FileName?this->FileName:"(none)") << endl;
  os << "DistributeFiles: ";
  if(this->DistributeFiles)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }
  
  os << "DownConvertVolumeFraction: ";
  if(this->DownConvertVolumeFraction)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }

  os << "GenerateLevelArray: ";
  if(this->GenerateLevelArray)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }
  
  os << "GenerateBlockIdArray: ";
  if(this->GenerateBlockIdArray)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }

  os << "GenerateActiveBlockArray: ";
  if(this->GenerateActiveBlockArray)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }
  
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

// This functions return 1 if the bounds have been set
void vtkSpyPlotReader::GetLocalBounds(vtkSpyPlotBlockIterator *biter,
                                      int nBlocks, int progressInterval)
{
  int i;
  double bounds[6];
  double progressFactor = 0.4 / static_cast<double>(nBlocks);
  vtkSpyPlotBlock *block;

  biter->Start();
  for (i = 0; biter->IsActive(); i++, biter->Next())
    {
    // See if we need to update progress
    if (i && !(i % progressInterval))
      {
      this->UpdateProgress(
        static_cast<double>(1.2 + i) * progressFactor);
      }
    // Make sure that the block is up to date
    biter->GetUniReader()->MakeCurrent();
    block = biter->GetBlock();
    block->GetRealBounds(bounds);
    this->Bounds->AddBounds(bounds);
    }
}
    
// Returns 1 if the bounds are valid else 0
void vtkSpyPlotReader::SetGlobalBounds(vtkSpyPlotBlockIterator *biter,
                                       int total_num_of_blocks, 
                                       int progressInterval,
                                       int *rightHasBounds,
                                       int *leftHasBounds)
{
  // Get the local bounds of this reader
  this->GetLocalBounds(biter, total_num_of_blocks,
                       progressInterval);
 
  // If we are not running in parallel then the local
  // bounds are the global bounds
  if (!this->Controller)
    {
    return;
    }
  vtkCommunicator *comm = this->Controller->GetCommunicator();
  if (!comm)
    {
    return;
    }

  int processNumber = biter->GetProcessorId();
  int numProcessors = biter->GetNumberOfProcessors();
  if (!comm->ComputeGlobalBounds(processNumber, numProcessors,
                                 this->Bounds, rightHasBounds,
                                 leftHasBounds, 
                                 VTK_MSG_SPY_READER_HAS_BOUNDS,
                                 VTK_MSG_SPY_READER_LOCAL_BOUNDS,
                                 VTK_MSG_SPY_READER_GLOBAL_BOUNDS))
    {
    vtkErrorMacro("Problem occurred getting the global bounds");
    }
 return;
}
  
int vtkSpyPlotReader::PrepareAMRData(vtkHierarchicalDataSet *hb,
                                     vtkSpyPlotBlock *block, 
                                     int *level,
                                     int extents[6],
                                     int realExtents[6],
                                     int realDims[3],
                                     vtkCellData **cd)
{
  double spacing[3];
  double origin[3];
  int needsFixing;

  needsFixing = block->GetAMRInformation(*(this->Bounds),
                                         level, spacing,
                                         origin, extents, 
                                         realExtents,
                                         realDims);
  //vtkImageData* ug = vtkImageData::New();
  vtkUniformGrid* ug = vtkUniformGrid::New();
  hb->SetDataSet(*level, hb->GetNumberOfDataSets(*level), ug);
  ug->SetSpacing(spacing);
  ug->SetExtent(extents);
  ug->SetOrigin(origin);
  *cd = ug->GetCellData();
  ug->Delete();
  return needsFixing;
}
                                  
int vtkSpyPlotReader::PrepareData(vtkHierarchicalDataSet *hb,
                                  vtkSpyPlotBlock *block,
                                  vtkRectilinearGrid **rg,
                                  int extents[6],
                                  int realExtents[6],
                                  int realDims[3],
                                  vtkCellData **cd)
{

  int needsFixing;
  vtkDataArray *coordinates[3];
  needsFixing = block->FixInformation(*(this->Bounds),
                                      extents, realExtents,
                                      realDims,coordinates);
  double bounds[6];
  this->Bounds->GetBounds(bounds);
  vtkDebugMacro( << __LINE__ << " Real dims:    " 
                 << coutVector3(realDims) );
  vtkDebugMacro( << __LINE__ << " Real Extents: " 
                 << coutVector6(realExtents) );
  vtkDebugMacro( << __LINE__ << " Extents:      " 
                 << coutVector6(extents) );
  vtkDebugMacro( << __LINE__ << " Global Bounds:" 
                 << coutVector6(bounds) );
  vtkDebugMacro( << " Rectilinear grid pointer: " << rg );
  *rg=vtkRectilinearGrid::New();
  (*rg)->SetExtent(extents);
  hb->SetDataSet(0, hb->GetNumberOfDataSets(0), *rg);
  if (coordinates[0])
    {
    (*rg)->SetXCoordinates(coordinates[0]);
    vtkDebugMacro( "NT: " << coordinates[0]->GetNumberOfTuples() );
    }
  if (coordinates[1])
    {
    (*rg)->SetYCoordinates(coordinates[1]);
    vtkDebugMacro( "NT: " << coordinates[1]->GetNumberOfTuples() );
    }
  if (coordinates[2])
    {
    (*rg)->SetZCoordinates(coordinates[2]);
    vtkDebugMacro( "NT: " << coordinates[2]->GetNumberOfTuples() );
    }

  vtkDebugMacro( "*******************" );
  vtkDebugMacro( "Coordinates: " );
  /*
    int cor;
    for ( cor = 0; cor < 3; cor ++ )
    {
    if (coordinates[cor])
    {
    coordinates[cor]->Print(cout);
    }
    else
    {
    vtkDebugMacro("No " << cor << "th coordinates");
    }
    vtkDebugMacro( "*******************" );
  */
  *cd = (*rg)->GetCellData();
  (*rg)->Delete();
  return needsFixing;
}

void vtkSpyPlotReader::UpdateFieldData(int numFields, int dims[3],
                                       int level, int blockID,
                                       vtkSpyPlotUniReader *uniReader,
                                       vtkCellData *cd)
{
  int field;
  int fixed= 0;
  int totalSize = dims[0]*dims[1]*dims[2];
  const char *fname;
  for ( field = 0; field < numFields; field ++ )
    {
    fname = uniReader->GetCellFieldName(field);
    if (this->CellDataArraySelection->ArrayIsEnabled(fname))
      {
      vtkDataArray *array=cd->GetArray(fname);
      if(array!=0)
        {
        cd->RemoveArray(fname); // if this is not the first step,
        // make sure we have a clean array
        }
      array = uniReader->GetCellFieldData(blockID, field, &fixed);
      //vtkDebugMacro( << __LINE__ << " Read data block: " 
      // << blockID << " " << field << "  [" << array->GetName() << "]" );
      cd->AddArray(array);
      }
    }
  
  // Add a level array, for debugging
  if(this->GenerateLevelArray)
    {
    createSpyPlotLevelArray(cd, totalSize, level);
    }
  
  // Mark the bounding cells as ghost cells of level 1.
  vtkUnsignedCharArray *ghostArray=vtkUnsignedCharArray::New();
  ghostArray->SetNumberOfTuples(totalSize);
  ghostArray->SetName("vtkGhostLevels"); //("vtkGhostLevels");
  cd->AddArray(ghostArray);
  ghostArray->Delete();
  int planeSize = dims[0]*dims[1];
  int j, k;
  unsigned char *ptr =
    static_cast<unsigned char*>(ghostArray->GetVoidPointer(0));
  for (k = 0; k < dims[2]; k++)
    {
    // Is the entire ij plane a set of ghosts
    if ((dims[2] != 1) && ((!k) || (k == dims[2]-1)))
      {
      memset(ptr, 1, planeSize);
      ptr += planeSize;
      continue;
      }
    for (j = 0; j < dims[1]; j++)
      {
      // Either the row is all non-ghosts except for the ends or 
      // its all ghosts
      // Is the entire row a set of ghosts
      if ((dims[1] != 1) && ((!j) || (j == dims[1] - 1)))
        {
        memset(ptr, 1, dims[0]);
        ptr+= dims[0];
        continue;
        } 
      memset(ptr, 0, dims[0]);
      if (dims[0] > 1)
        {
        ptr[0] = 1;
        ptr[dims[0]-1] = 1;
        }
      ptr+= dims[0];
      }
    }
}

void vtkSpyPlotReader::UpdateBadGhostFieldData(int numFields, int dims[3],
                                               int realDims[3],
                                               int realExtents[6],
                                               int level, int blockID,
                                               vtkSpyPlotUniReader *uniReader,
                                               vtkCellData *cd)
{
  int realPtDims[3];
  int ptDims[3];
  int totalSize = realDims[0]*realDims[1]*realDims[2];
  const char *fname;
  int cc, fixed = 0;;
  int field;
  for (cc = 0; cc < 3; cc++)
    {
    realPtDims[cc]=realDims[cc]+1;
    ptDims[cc]=dims[cc]+1;
    }
  
  for ( field = 0; field < numFields; field ++ )
    {
    fname = uniReader->GetCellFieldName(field);
    if (this->CellDataArraySelection->ArrayIsEnabled(fname))
      {
      vtkDataArray *array=cd->GetArray(fname);
      if(array!=0)
        {
        cd->RemoveArray(fname); // if this is not the first step,
        // make sure we have a clean array
        }
      
      array = uniReader->GetCellFieldData(blockID, field, &fixed);
      //vtkDebugMacro( << __LINE__ << " Read data block: " << blockID 
      // << " " << field << "  [" << array->GetName() << "]" );
      cd->AddArray(array);
      //array->Print(cout);
      
      if ( !fixed )
        {
        vtkDebugMacro( " Fix bad ghost cells for the array: " << blockID 
                       << " / " << field 
                       << " (" << uniReader->GetFileName() << ")" );
        switch(array->GetDataType())
          {
          vtkTemplateMacro(
            ::vtkSpyPlotRemoveBadGhostCells(static_cast<VTK_TT*>(0), array,
                                            realExtents, realDims, 
                                            ptDims, realPtDims));
          }
        uniReader->MarkCellFieldDataFixed(blockID, field);
      
        }
      else
        {
        vtkDebugMacro( " Bad ghost cells already fixed for the array" );
        }
      }
    }
  
  // Add a level array, for debugging
  if(this->GenerateLevelArray)
    {
    createSpyPlotLevelArray(cd, totalSize, level);
    }
  
  // Mark the remains ghost cell as real ghost cells of level 1.
  vtkUnsignedCharArray *ghostArray=vtkUnsignedCharArray::New();
  ghostArray->SetNumberOfTuples(totalSize);
  ghostArray->SetName("vtkGhostLevels"); //("vtkGhostLevels");
  cd->AddArray(ghostArray);
  ghostArray->Delete();
  unsigned char *ptr 
    =static_cast<unsigned char*>(ghostArray->GetVoidPointer(0));
  int k,j;
  int planeSize = realDims[0] * realDims[1];
  int checkILower = (realExtents[0]==0);
  int checkIUpper = (realExtents[1]==dims[0]);
  int checkJLower = (realExtents[2]==0);
  int checkJUpper = (realExtents[3]==dims[1]);
  int checkKLower = (realExtents[4]==0);
  int checkKUpper = (realExtents[5]==dims[2]);
  for (k = 0; k < realDims[2]; k++)
    {
    // Is the entire ij plane a set of ghosts
    if ((realDims[2] != 1) && ((checkKLower && (!k)) || 
                               (checkKUpper && (k == realDims[2]-1))))
      {
      memset(ptr, 1, planeSize);
      ptr += planeSize;
      continue;
      }
    for (j = 0; j < realDims[1]; j++)
      {
      // Either the row is all non-ghosts except for the ends or 
      // its all ghosts
      // Is the entire row a set of ghosts
      if ((realDims[1] != 1) && ((checkJLower && (!j)) || 
                                 (checkJUpper && (j == realDims[1] - 1))))
        {
        memset(ptr, 1, realDims[0]);
        ptr+= realDims[0];
        continue;
        } 
      memset(ptr, 0, realDims[0]);
      if (dims[0] > 1)
        {
        if (checkILower)
          {
          ptr[0] = 1;
          }
        if (checkIUpper)
          {
          ptr[realDims[0]-1] = 1;
          }
        }
      ptr+= realDims[0];
      }
    }
}

void vtkSpyPlotReader::SetGlobalLevels(vtkHierarchicalDataSet *hb,
                                       int processNumber,
                                       int numProcessors,
                                       int rightHasBounds,
                                       int leftHasBounds)
{
  int parent = 0;
  int left= vtkCommunicator::GetLeftChildProcessor(processNumber);
  int right=left+1;
  if(processNumber>0) // not root (nothing to do if root)
    {
    parent= vtkCommunicator::GetParentProcessor(processNumber);
    }
  
  unsigned int numberOfLevels=hb->GetNumberOfLevels();
  // If this is an SMR SpyPlot we need to first determine
  // the global number of levels
  
  if (this->IsAMR)
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
    }
  // At this point, the global number of levels is set in each processor.
  // Update each level
  unsigned int level;
  int intMsgValue;
  for (level = 0; level<numberOfLevels; level++)
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
        // we have to start at the end because the
        // original blocks location and final location
        // may overlap.
        int j;
        for (i=numberOfDataSets-1, j=globalIndex+numberOfDataSets-1;
             i>=0; --i, --j)
          {
          hb->SetDataSet(level,j,hb->GetDataSet(level,i));
          }
        // add null pointers at the beginning
        for (i = 0; i<globalIndex; i++)
          {
          hb->SetDataSet(level,i,0);
          }
        }
      // add null pointers at the end
      for (i=globalIndex+numberOfDataSets;
           i<totalNumberOfDataSets; i++)
        {
        hb->SetDataSet(level,i,0);
        }
      }
    } 
}

void createSpyPlotLevelArray(vtkCellData *cd, int size, int level)
{
  vtkDataArray *array=cd->GetArray("levels");
  if(array!=0)
    {
    cd->RemoveArray("levels"); // if this is not the first step,
    // make sure we have a clean array
    }
  
  array = vtkIntArray::New();
  cd->AddArray(array);
  array->Delete();
  
  array->SetName("levels");
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(size);
  int *ptr=static_cast<int *>(array->GetVoidPointer(0));
  int i;
  for (i=0; i < size; i++)
    {
    ptr[i]=level;
    }
}
        
