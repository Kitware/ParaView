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

#include "vtkAMRBox.h"
#include "vtkBoundingBox.h"
#include "vtkByteSwap.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkExtractCTHPart.h" // for the BOUNDS key
#include "vtkFloatArray.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkProcessGroup.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
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

vtkStandardNewMacro(vtkSpyPlotReader);
vtkCxxSetObjectMacro(vtkSpyPlotReader,GlobalController,vtkMultiProcessController);

static void createSpyPlotLevelArray(vtkCellData *cd, int size, int level);

//-----------------------------------------------------------------------------
vtkSpyPlotReader::vtkSpyPlotReader()
{
  this->SetNumberOfInputPorts(0);
  
  this->Map = new vtkSpyPlotReaderMap;
  this->Bounds = new vtkBoundingBox;
  this->BoxSize[0] = -1;
  this->BoxSize[1] = -1;
  this->BoxSize[2] = -1;
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
  this->ComputeDerivedVariables = 1;
  this->DownConvertVolumeFraction = 1;
  this->MergeXYZComponents = 1;

  // this has all of the processes.
  this->GlobalController = 0;
  this->SetGlobalController(vtkMultiProcessController::GetGlobalController());

  this->DistributeFiles=0; // by default, distribute blocks, not files.
  this->GenerateLevelArray=0; // by default, do not generate level array.
  this->GenerateBlockIdArray=0; // by default, do not generate block id array.
  this->GenerateActiveBlockArray = 0; // by default do not generate active array
  this->GenerateTracerArray = 0; // by default do not generate tracer array
  this->IsAMR = 1;
  this->UpdateFileCallCount = 0;

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
  this->SetGlobalController(0);
}


//-----------------------------------------------------------------------------
// Create either vtkHierarchicalBoxDataSet or vtkMultiBlockDataSet based on
// whether the dataset is AMR.
void vtkSpyPlotReader::SetFileName(const char* filename)
{  
  if ( this->FileName == NULL && filename == NULL) { return;}
  if ( this->FileName && filename && (!strcmp(this->FileName,filename))) { return;}
  if (this->FileName) { delete [] this->FileName; }
  if (filename)
    {
    size_t n = strlen(filename) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (filename);
    this->FileName = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
   else
    {
    this->FileName = NULL;
    }
  this->UpdateFileCallCount = 0;
  this->Modified();
}


//-----------------------------------------------------------------------------
// Create either vtkHierarchicalBoxDataSet or vtkMultiBlockDataSet based on
// whether the dataset is AMR.
int vtkSpyPlotReader::RequestDataObject(vtkInformation *req,
  vtkInformationVector **vtkNotUsed(inV),
  vtkInformationVector *outV)
{
  vtkInformation *outInfo = outV->GetInformationObject(0);
  vtkCompositeDataSet* outData = NULL;

  // Call UpdateFile (which sets IsAMR) because RequestInformation isn't called
  // before RequestDataObject
  this->UpdateFile (req, outV);
  
  // We only need IsAMR set from UpdateFile, reset the CurrentFile
  this->SetCurrentFileName (0);

  if (this->IsAMR)
    {
    outData = vtkHierarchicalBoxDataSet::New();
    }
  else
    {
    outData = vtkMultiBlockDataSet::New();
    }

  outData->SetPipelineInformation(outInfo);
  outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), outData->GetExtentType());
  outInfo->Set(vtkDataObject::DATA_OBJECT(), outData);
  outData->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
// Read the case file and the first binary file do get meta
// informations (number of files, number of fields, number of timestep).
int vtkSpyPlotReader::RequestInformation(vtkInformation *request,
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  if(this->GlobalController==0)
    {
    vtkErrorMacro("Controller not specified. This reader requires controller to be set.");
    //return 0;
    }
  if(!this->Superclass::RequestInformation(request,inputVector,outputVector))
    {
    return 0;
    }

  vtkInformation *info=outputVector->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  struct stat fs;
  if(stat(this->FileName,&fs)!=0)
    {
    vtkErrorMacro("Cannot find file " << this->FileName);
    return 0;
    }

  return this->UpdateFile (request, outputVector);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateFile (vtkInformation* request,
                                  vtkInformationVector* outputVector)
{
  if (this->UpdateFileCallCount >= 2)
    {
    //We need to call UpdateFile twice once in RequestDataObject to set isAmr
    //and a second time in RequestInformation so that the time information is properly set
    //because of this design issue, these method should be refactored. But currently
    //I don't know enough about spy plot to cleanly see a way to do this.

    //so whenever a file name is set we will reset UpdateFileCallCount to zero.
    //than the calls to updateFile in request data object and request information will
    //increment the counter and make sure the method doesn't execute more than twice

    //When the file count is greater than two the only thing we have to do is handle
    //changes to the temporal step, so we always call UpdateMetaData
    
    this->UpdateMetaData(request,outputVector);
    return 1;
    }
  this->UpdateFileCallCount++;

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
  // cerr << "spydatafile\n";
  // See if this is part of a series
  vtkstd::string extension = 
    vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  int currentNum=0, isASeries=0;
  size_t esize;
  esize = extension.size();
  if (esize > 0 )
    {
    // See if this is a pure numerical extension - hence it's part 
    // of a series - if the first char is a . we need to exclude it
    // from the check
    const char *a = extension.c_str();
    char *ep;
    size_t b = 0;
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
    //cerr << "Not setting much else\n";
    return 1;
    }
  // cerr << "setting Current file name " << this->FileName << endl;
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
    // cerr << "buffer1 == " << buffer << endl;
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
    // cerr << "buffer2 == " << buffer << endl;
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
    // cerr << "buffer3 == " << buffer << endl;
    this->Map->Files[buffer]=0;
    vtkDebugMacro( << __LINE__ << " Create new uni reader: " 
                   << this->Map->Files[buffer] );
    }
  // Okay now open just the first file to get meta data
  vtkDebugMacro("Reading Meta Data in UpdateCaseFile(ExecuteInformation) from file: " << this->Map->Files.begin()->first.c_str());
  // cerr << "updating meta... " << endl;
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
int vtkSpyPlotRemoveBadGhostCells(
        DataType* dataType,
        vtkDataArray* dataArray,
        int realExtents[6],
        int realDims[3],
        int ptDims[3],
        int realPtDims[3])
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

  //Performance analysis has shown that using ComputeCellId to be a bottleneck of the reader
  //so we are going to replace the method with an incremental algorithm that will reduce the total 
  //number of multiplications.
  vtkIdType kOffset[2]={0,0},jOffset[2]={0,0};
  vtkIdType realCellId=0,oldCellId=0;

  int xyz[3];
  int destXyz[3];
  DataType* dataPtr = static_cast<DataType*>(dataArray->GetVoidPointer(0));
  for (xyz[2] = realExtents[4], destXyz[2] = 0; 
       xyz[2] < realExtents[5]; ++xyz[2], ++destXyz[2])
    {
    kOffset[0] = destXyz[2] * (realPtDims[1]-1);
    kOffset[1] = xyz[2] * (ptDims[1]-1);

    for (xyz[1] = realExtents[2], destXyz[1] = 0;
         xyz[1] < realExtents[3]; ++xyz[1], ++destXyz[1])
      {
      jOffset[0] = (kOffset[0] + destXyz[1]) * (realPtDims[0]-1);
      jOffset[1] = (kOffset[1] + xyz[1]) * (ptDims[0]-1);

      for (xyz[0] = realExtents[0], destXyz[0] = 0;
           xyz[0] < realExtents[1]; ++xyz[0], ++destXyz[0])
        {
        realCellId = jOffset[0] + destXyz[0];
        oldCellId = jOffset[1] + xyz[0];        
        dataPtr[realCellId] = dataPtr[oldCellId];
        //old slow way of calculating cell id
        //dataPtr[vtkStructuredData::ComputeCellId(realPtDims,destXyz)] =
        //  dataPtr[vtkStructuredData::ComputeCellId(ptDims,xyz)];
        }
      }
    }
  dataArray->SetNumberOfTuples(realDims[0]*realDims[1]*realDims[2]);
  return 1;
}
//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateTimeStep(vtkInformation *requestInfo,
                                     vtkInformationVector *outputInfoVec,
                                     vtkCompositeDataSet *outputData)
{
  vtkInformation *outputInfo=outputInfoVec->GetInformationObject(0);

  // Update the timestep.
  int tsLength =
    outputInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps =
    outputInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  //
  if(outputInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    // Get the requested time step. We only supprt requests of a single time
    // step in this reader right now
    double *requestedTimeSteps = 
      outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    //double timeValue = requestedTimeSteps[0];

    // find the first time value larger than requested time value
    // this logic could be improved
    //int cnt = 0;
    //while (cnt < tsLength-1 && steps[cnt] < timeValue)
    //  {
    //  cnt++;
    //  }
    //this->CurrentTimeStep = cnt;

    int cnt=0;
    int closestStep=0;
    double minDist=-1;
    for (cnt=0;cnt<tsLength;cnt++)
      {
      double tdist=(steps[cnt]-requestedTimeSteps[0]>requestedTimeSteps[0]-steps[cnt])?
        steps[cnt]-requestedTimeSteps[0]:
        requestedTimeSteps[0]-steps[cnt];
      if (minDist<0 || tdist<minDist)
        {
        minDist=tdist;
        closestStep=cnt;
        }
      }
    this->CurrentTimeStep=closestStep;

    this->TimeRequestedFromPipeline = true;
    this->UpdateMetaData(requestInfo, outputInfoVec);
    this->TimeRequestedFromPipeline = false;
    }
  else
    {
    this->UpdateMetaData(requestInfo, outputInfoVec);
    }

  outputData->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(),
                            steps+this->CurrentTimeStep, 1);

  return 1;
}
//-----------------------------------------------------------------------------
int vtkSpyPlotReader::AddBlockIdArray(vtkCompositeDataSet *cds)
{
  int blockId=0;
  vtkSmartPointer<vtkCompositeDataIterator> cdIter;
  cdIter.TakeReference(cds->NewIterator());
  cdIter->VisitOnlyLeavesOn();
  cdIter->TraverseSubTreeOn();
  for (cdIter->InitTraversal(); !cdIter->IsDoneWithTraversal();
    cdIter->GoToNextItem(), blockId++)
    {
    vtkDataObject *dataObject = cdIter->GetCurrentDataObject();
    if (dataObject!=0)
      {
      vtkDataSet *ds = vtkDataSet::SafeDownCast(dataObject);
      assert("check: ds_exists" && ds!=0);

      // Add the block id cell data array
      vtkCellData *cd=ds->GetCellData();
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
      vtkIdType numCells = ds->GetNumberOfCells();
      array->SetNumberOfTuples(numCells);
      array->FillComponent(0,blockId);
      }
    }

  return 1;
}
//-----------------------------------------------------------------------------
int vtkSpyPlotReader::AddActiveBlockArray(
      vtkCellData *cd,
      vtkIdType nCells,
      unsigned char status)
{
  vtkUnsignedCharArray* activeArray = vtkUnsignedCharArray::New();
  activeArray->SetName("ActiveBlock");
  //vtkIdType numCells=realDims[0]*realDims[1]*realDims[2];
  activeArray->SetNumberOfTuples(nCells);
  activeArray->FillComponent(0,status);
//   for (vtkIdType myIdx=0; myIdx<numCells; myIdx++)
//     {
//     activeArray->SetValue(myIdx, block->IsActive());
//     }
  cd->AddArray(activeArray);
  activeArray->Delete();

  return 1;
}
//-----------------------------------------------------------------------------
int vtkSpyPlotReader::AddAttributes(vtkHierarchicalBoxDataSet *hbds)
{
  // global bounds
  double b[6];
  this->Bounds->GetBounds(b);
  vtkDoubleArray *da = vtkDoubleArray::New();
  da->SetNumberOfComponents(1);
  da->SetNumberOfTuples(6);
  da->SetName("GlobalBounds");
  for (int q=0; q<6; ++q)
    {
    da->SetValue(q, b[q]);
    }
  hbds->GetFieldData()->AddArray( da );
  da->Delete();
  // global box size
  vtkIntArray *ia = vtkIntArray::New();
  ia->SetNumberOfComponents(1);
  ia->SetNumberOfTuples(3);
  ia->SetName("GlobalBoxSize");
  for (int q=0; q<3; ++q)
    {
    ia->SetValue(q, this->BoxSize[q]);
    }
  hbds->GetFieldData()->AddArray( ia );
  ia->Delete();
  // minimum level in use
  ia = vtkIntArray::New();
  ia->SetNumberOfComponents(1);
  ia->SetNumberOfTuples(1);
  ia->SetName("MinLevel");
  ia->SetValue(0, this->MinLevel);
  hbds->GetFieldData()->AddArray( ia );
  ia->Delete();
  // grid spacing on the min level
  da = vtkDoubleArray::New();
  da->SetNumberOfComponents(1);
  da->SetNumberOfTuples(3);
  da->SetName("MinLevelSpacing");
  for (int q=0; q<3; ++q)
    {
    da->SetValue(q, this->MinLevelSpacing[q]);
    }
  hbds->GetFieldData()->AddArray( da );
  da->Delete();

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
  vtkCompositeDataSet *cds=vtkCompositeDataSet::SafeDownCast(doOutput);
  if(!cds)
    {
    vtkErrorMacro("The output is not a CompositeDataSet");
    return 0;
    }
  if (!info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) ||
      !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    vtkErrorMacro("Expected information not found. "
                  "Cannot provide update extent.");
    return 0;
    }

  cds->Initialize(); // remove all previous blocks
  //int numFiles = this->Map->Files.size();

  const int myGlobalProcId
    = this->GlobalController->GetLocalProcessId();
  const int nProcsAll
    = this->GlobalController->GetNumberOfProcesses();

  // Set to the requested timestep
  this->UpdateTimeStep(request,outputVector,cds);

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

  // Block iterator could use either the global proc id and number of
  // processes from the global control or those from the sub controller
  blockIterator->Init(
                nProcsAll,
                myGlobalProcId,
                this,
                this->Map,
                this->CurrentTimeStep);

  int nBlocks = blockIterator->GetNumberOfBlocksToProcess();
  int progressInterval = nBlocks / 10 + 1;
  int rightHasBounds = 0;
  int leftHasBounds = 0;

  vtkHierarchicalBoxDataSet *hbds 
        = vtkHierarchicalBoxDataSet::SafeDownCast(cds);

  // TODO The following three calls compute meta data
  // this could be done in a single pass, and the meta data
  // should be vtkInformationKeys defined in vtkHierarchicalBoxDataSet
  // rather than placed in the field data as it is here.

  // Note that in the process of getting the bounds 
  // all of the readers will get updated appropriately
  this->SetGlobalBounds(blockIterator, nBlocks,
                        progressInterval, &rightHasBounds,
                        &leftHasBounds);
  // Determine if the box size is constant
  this->SetGlobalBoxSize( blockIterator );
  // Determine the minimum level in use
  // and its grid spacing
  this->SetGlobalMinLevelAndSpacing( blockIterator );
  // export global bounds, minimum level, spacing, and box size
  // in field data arrays for use by downstream filters
  if ( hbds )
    {
    this->AddAttributes(hbds);
    }

  int needTracers = 1;

  // read in the data
  if (nBlocks!=0)
    {
    // TODO This seems wrong, shouldn't the bounds key be defined in
    // the hierarchical dataset?
    double b[6];
    this->Bounds->GetBounds(b);
    info->Set(vtkExtractCTHPart::BOUNDS(), b, 6);

    // Read the blocks/files that are assigned to this process
    int current_block_number;
    for(blockIterator->Start(), current_block_number=1;
        blockIterator->IsActive();
        blockIterator->Next(), current_block_number++)
      {
      if ( !(current_block_number % progressInterval) )
        {
        this->UpdateProgress(
          0.6 + 
          0.4 * static_cast<double>(current_block_number)/nBlocks);
        }
      block=blockIterator->GetBlock();
      int numFields=blockIterator->GetNumberOfFields();
      uniReader=blockIterator->GetUniReader();

      if (this->GenerateTracerArray == 1 && needTracers)
        {
        vtkFieldData *fd = cds->GetFieldData ();
        vtkDataArray *array= fd->GetArray("Tracer Coordinates");
        if (array != 0)
          {
          fd->RemoveArray ("Tracer Coordinates");
          }
        vtkFloatArray *tracers = uniReader->GetTracers ();
        if (tracers != 0) 
          {
          tracers->SetName ("Tracer Coordinates");
          fd->AddArray (tracers);
          }
        needTracers = 0;
        }

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

      // read block
      if (this->IsAMR)
        {
        hasBadGhostCells
          = this->PrepareAMRData(hbds, block, &level,
                                 extents, realExtents,
                                 realDims, &cd);
        }
      else
        {
        vtkRectilinearGrid *rg;
        vtkDebugMacro( "Preparing Block: " << blockID << " " 
                      << uniReader->GetFileName() );
        hasBadGhostCells = this->PrepareData(
          vtkMultiBlockDataSet::SafeDownCast(cds), block, &rg, extents, realExtents,
          realDims, &cd);
        grids.push_back(rg);
        }

      vtkDebugMacro("Executing block: " << blockID);

      // Deal with the field data and map out where the true 
      // ghost cells are
      if(!hasBadGhostCells)
        {
        this->UpdateFieldData(numFields, dims, level, blockID,
                              uniReader, cd);
        this->ComputeDerivedVars(cd, block, uniReader, blockID,dims);
        }
      else // we have some bad ghost cells
        {
        this->UpdateBadGhostFieldData(numFields, dims, realDims,
                                      realExtents, level, blockID,
                                      uniReader, cd);
        this->ComputeDerivedVars(cd, block, uniReader, blockID,realDims);
        }

      // Add active block array, for debugging
      if (this->GenerateActiveBlockArray)
        {
        this->AddActiveBlockArray(cd,
                                  realDims[0]*realDims[1]*realDims[2],
                                  block->IsActive());
        }
      // vectorize
      if (this->MergeXYZComponents)
        {
        this->MergeVectors(cd);
        }           
      }
    delete blockIterator;
    }

    // At this point, each processor has its own blocks
    // They have to exchange the blocks they have get a unique id for
    // each block over the all dataset.

    // Update the number of levels.
    if(this->GlobalController)
      {
      this->SetGlobalLevels(cds);
      }
    // Set the unique block id cell data
    if(this->GenerateBlockIdArray)
      {
      this->AddBlockIdArray(cds);
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
  size_t e1, e2, e3;
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
  size_t e1, e2;
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
    vtkDebugMacro( "Cannot read magic" );
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
void vtkSpyPlotReader::SetMergeXYZComponents(int merge)
{
  if ( merge == this->MergeXYZComponents )
    {
    return;
    }
  this->MergeXYZComponents = merge;
  this->Modified();
}
//-----------------------------------------------------------------------------
void vtkSpyPlotReader::PrintBlockList(vtkHierarchicalBoxDataSet *hbds, int
  vtkNotUsed(myProcId))
{
  unsigned int numberOfLevels=hbds->GetNumberOfLevels();
  unsigned int level;
  //  Display the block list for each level
  numberOfLevels=hbds->GetNumberOfLevels();
  for (level = 0; level<numberOfLevels; level++)
    {
   // cout<<myProcId<<" level="<<level<<"/"<<numberOfLevels<<endl;
    int totalNumberOfDataSets=hbds->GetNumberOfDataSets(level);
    int i;
    for (i = 0; i<totalNumberOfDataSets; i++)
      {
     // cout<<myProcId<<" dataset="<<i<<"/"<<totalNumberOfDataSets;
      vtkAMRBox box;
       if(hbds->GetDataSet(level,i,box)==0)
        {
       // cout<<" Void"<<endl;
        }
      else
        {
       // cout<<" Exists"<<endl;
        }
      }
    }
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

  os << "MergeXYZComponents: ";
  if(this->MergeXYZComponents)
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
  if ( this->GlobalController)
    {
    os << "GlobalController:" << endl;
    this->GlobalController->PrintSelf(os, indent.GetNextIndent());
    }
}
// Get the minimum level that actuall has data and 
// its grid spacing. If this process has no blocks
// then we return spacing and min level close to infinity.
void vtkSpyPlotReader::GetLocalMinLevelAndSpacing(
                              vtkSpyPlotBlockIterator *biter,
                              int *localMinLevel,
                              double spacing[3]) const
{
  biter->Start();
  // If there are no blocks locally we set
  // the spacing and min level close to infinity
  // and return
  if (!biter->IsActive())
    {
    localMinLevel[0]=VTK_INT_MAX;
    spacing[0]=spacing[1]=spacing[2]=VTK_DOUBLE_MAX;
    return;
    }
  // get first block's level
  biter->GetUniReader()->MakeCurrent();
  vtkSpyPlotBlock *thisBlock= biter->GetBlock();
  *localMinLevel = thisBlock->GetLevel();

  vtkSpyPlotBlock *minLevelBlock=thisBlock;

  // compare to all others
  for ( biter->Next(); biter->IsActive(); biter->Next())
    {
    thisBlock = biter->GetBlock();
    int thisMinLevel=thisBlock->GetLevel();
    if (thisMinLevel < *localMinLevel)
      {
      minLevelBlock=thisBlock;
      *localMinLevel=thisMinLevel;
      }
    }
  // now that we have the block with minimum 
  // level, we can get the spacing from him.
  minLevelBlock->GetSpacing(spacing);

  return;
}
//
void vtkSpyPlotReader::SetGlobalMinLevelAndSpacing(
                            vtkSpyPlotBlockIterator *biter)
{
  // get the local min level and its spacing
  // if there are no blocks locally these will be
  // close to infinity.
  int localMinLevel;
  double localMinLevelSpacing[3];
  this->GetLocalMinLevelAndSpacing(biter,&localMinLevel,localMinLevelSpacing);

  // If we are not running in parallel then the local
  // min level and spacing is the global min level and spacing
  if (!this->GlobalController)
    {
    this->MinLevel=localMinLevel;
    for (int q=0; q<3; ++q)
      {
      this->MinLevelSpacing[q]=localMinLevelSpacing[q];
      }
    return;
    }

  // package the level and its spacing because
  // we want to reduce min level but also need
  // its associated spacing to go along for the ride.
  double sendBuf[4]={(double)localMinLevel,
                     localMinLevelSpacing[0],
                     localMinLevelSpacing[1],
                     localMinLevelSpacing[2]};
  // collect the information on proc 0
  int numProcs=this->GlobalController->GetNumberOfProcesses();
  int myRank=this->GlobalController->GetLocalProcessId();

  // proc 0 gets a bigger buffer to collect all the data
  double *recvBuf = myRank==0 ? new double[4*numProcs] : 0;
  this->GlobalController->Gather( sendBuf, recvBuf, 4, 0);

  // reduce min level while preserving its
  // associated spacing
  if (myRank==0)
    {
    // sendBuf will hold the result of the reduction
    // it is already initialized with min level and
    // spacing from proc 0, we start looking in data
    // gathered from proc 1...n
    for (int i=0,j=4; i<numProcs-1; ++i, j+=4)
      {
      if (sendBuf[0]>recvBuf[j])
        {
        // copy
        for (int q=0; q<4; ++q)
          {
          sendBuf[q]=recvBuf[j+q];
          }
        }
      }
    delete [] recvBuf;
    }
  // proc 0 knows the min level and spacing 
  // share this with everyone else
  this->GlobalController->Broadcast(sendBuf,4,0);

  // now everyone has the min level and associated grid spacing
  this->MinLevel=(int)sendBuf[0];
  for (int q=0; q<3; ++q)
    {
    this->MinLevelSpacing[q]=sendBuf[1+q];
    }
}


// Determine the local box size if the box size is constant.
// If the box size varies over the local tree then size will be 
// -1,-1,-1.
// The case where there are no blocks is treated as if the block size
// is constant and the size will be
// inf, inf, inf
//
// returns true if the box size is constant and false if the box size varies 
bool vtkSpyPlotReader::GetLocalBoxSize(vtkSpyPlotBlockIterator *biter,
                                       int localBoxSize[3]) const
{
  vtkSpyPlotBlock *block;
  biter->Start();
  // see if there are any blocks on this process
  // if not then, we'll call block size constant
  // and set the size close to infinite
  if (!biter->IsActive())
    {
    localBoxSize[0]=localBoxSize[1]=localBoxSize[2]=VTK_INT_MAX;
    return true;
    }
  // get first box's size 
  biter->GetUniReader()->MakeCurrent();
  block = biter->GetBlock();
  int thisBoxSize[3];
  block->GetDimensions(localBoxSize);
  // compare to all others
  for ( biter->Next(); biter->IsActive(); biter->Next())
    {
    block = biter->GetBlock();
    biter->GetUniReader()->MakeCurrent();
    block->GetDimensions(thisBoxSize);
    for (int q=0;q<3;++q)
      {
      // if the size changes stop
      if (thisBoxSize[q]!=localBoxSize[q])
        {
        localBoxSize[0] = -1;
        localBoxSize[1] = -1;
        localBoxSize[2] = -1;
        return false;
        }
      }
    }
  // box size is a constant locally
  return true;
}

// Determines if the box size is a constant over the entire data set
// if so sets this->GlobalBoxSize to that size, otehrwise sets 
// it to -1,-1,-1
void vtkSpyPlotReader::SetGlobalBoxSize(vtkSpyPlotBlockIterator *biter)
{
  // Get the local box size
  int localBoxSize[3] = {0,0,0};
  bool isConstantLocally
      = this->GetLocalBoxSize(biter, localBoxSize);

  // If we are not running in parallel then the local
  // size is the global size
  if (!this->GlobalController)
    {
    if (isConstantLocally)
      {
      for (int q=0; q<3; ++q)
        {
        this->BoxSize[q]=localBoxSize[q];
        }
      }
    else
      {
      for (int q=0; q<3; ++q)
        {
        this->BoxSize[q]=-1;
        }
      }
    return;
    }

  // To decide weather or not box size is constant...
  // 1) get the smallest box size across procs
  int globalBoxSize[3] = {-1,-1,-1};
  this->GlobalController->AllReduce(localBoxSize, globalBoxSize, 3, vtkCommunicator::MIN_OP);

  // 2) check if box size we have is the same as the 
  // box size that the other procs have.
  bool isConstantGlobally=true;
  for (int q=0; q<3; ++q)
    {
    // if we have no blocks then we have to 
    // get the true size from other procs, and pretend 
    // that size is constant locally
    if (localBoxSize[q]==VTK_INT_MAX )
      {
      localBoxSize[q]=globalBoxSize[q];
      }
    // if we have blocks the n we just check to see if
    // what we have is the same as what the others
    // have.
    else if (localBoxSize[q]!=globalBoxSize[q])
      {
      isConstantGlobally=false;
      }
    }
  // 3) send a flag indicating change/no change occured
  int lFlag=!isConstantLocally||!isConstantGlobally ? -1 : 1;
  int gFlag=0;
  this->GlobalController->AllReduce(&lFlag,&gFlag,1, vtkCommunicator::MIN_OP);

  // set the global box size acordingly
  switch( gFlag )
    {
    // box size varies
    case -1:
      for (int q=0; q<3; ++q)
        {
        this->BoxSize[q]=-1;
        }
      break;
    // box size is constant
    case 1:
      for (int q=0; q<3; ++q)
        {
        this->BoxSize[q]=localBoxSize[q];
        }
        break;
    default:
      vtkErrorMacro("Invalid flag value verifying that box size is constant.");
    }
 return;
}

// This functions return 1 if the bounds have been set
void vtkSpyPlotReader::GetLocalBounds(vtkSpyPlotBlockIterator *biter,
                                      int nBlocks, int progressInterval)
{
  int i;
  double bounds[6];
  double progressFactor = 0.4 / static_cast<double>(nBlocks);
  vtkSpyPlotBlock *block;

  // every one has bounds, the procs with no blocks
  // have canonical empty bounds.
//   double *pt;
//   pt=const_cast<double *>(this->Bounds->GetMinPoint());
//   pt[0]=pt[1]=pt[2]= VTK_DOUBLE_MAX;
//   pt=const_cast<double *>(this->Bounds->GetMaxPoint());
//   pt[0]=pt[1]=pt[2]=-VTK_DOUBLE_MAX;

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
  if (!this->GlobalController)
    {
    return;
    }
  vtkCommunicator *comm = this->GlobalController->GetCommunicator();
  if (!comm)
    {
    return;
    }

  int processNumber=this->GlobalController->GetLocalProcessId();
  int numProcessors=this->GlobalController->GetNumberOfProcesses();

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

int vtkSpyPlotReader::PrepareAMRData(vtkHierarchicalBoxDataSet *hb,
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

//   double bds[6];
//   this->Bounds->GetBounds(bds);
//   cerr << "{\n";
//   cerr << "level:       [" << *level << "]\n";
//   cerr << "Origin:      [" << origin[0] << "," << origin[1] << "," << origin[2] << "]\n";
//   cerr << "Spacing:     [" << spacing[0] << "," << spacing[1] << "," << spacing[2] << "]\n";
//   cerr << "extents:     [" << extents[0] << "," << extents[1] << "," << extents[2] << "|"
//                            << extents[3] << "," << extents[4] << "," << extents[5] << "]\n";
//   cerr << "realExtents: [" << realExtents[0] << "," << realExtents[1] << "," << realExtents[2] << ","
//                            << realExtents[3] << "," << realExtents[4] << "," << realExtents[5] << "]\n";
//   cerr << "realDims:    [" << realDims[0] << "," << realDims[1] << "," << realDims[2] << "]\n";
//   cerr << "bounds:      [" << bds[0] << "," << bds[1] << "," << bds[2] << ","
//                            << bds[3] << "," << bds[4] << "," << bds[5] << "]\n";
//   cerr << "}\n";

  vtkAMRBox box(realExtents);
  vtkUniformGrid* ug = vtkUniformGrid::New();
  hb->SetDataSet(*level, hb->GetNumberOfDataSets(*level), box, ug);
  ug->SetSpacing(spacing);
  ug->SetExtent(extents);
  ug->SetOrigin(origin);
  *cd = ug->GetCellData();
  ug->Delete();

  return needsFixing;
}

int vtkSpyPlotReader::PrepareData(vtkMultiBlockDataSet *hb,
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
  hb->SetBlock(hb->GetNumberOfBlocks(), *rg);
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
  ghostArray->SetName("vtkGhostLevels");
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
/*
int vtkSpyPlotReader::DistributeGlobalStructure(vtkCompositeDataSet *cds)
{
  // two cases hierarchical box data and multi block of rectilinear 
  // grids

  vtkMultiBlockDataSet* mbDS =
    vtkMultiBlockDataSet::SafeDownCast(composite);

  assert("check: ds must be hierarchical or multiblock" && (hbDS || mbDS));

  if (this->AMR)
    {
    vtkHierarchicalBoxDataSet* hbdS 
    = dynamic_cast<vtkHierarchicalBoxDataSet *>(cds);

    unsigned int nLevelsLocal=hbds->GetNumberOfLevels();
    unsigned int nLevelsGlobal=0;
    this->GlobalController->AllReduce(
                &nLevelsLocal,
                &nLevelsGlobal,
                1,vtkCommunicator::MAX_OP);
    
    }

  return 1;
}*/

//-----------------------------------------------------------------------------
// synch data set structure
void vtkSpyPlotReader::SetGlobalLevels(vtkCompositeDataSet *composite)
{
  // Force the inclusion of empty datatsets.
  bool rightHasBounds=true; //TODO 
  bool leftHasBounds=true;
  int processNumber=this->GlobalController->GetLocalProcessId();
  int numProcessors=this->GlobalController->GetNumberOfProcesses();

  int parent = 0;
  int left= vtkCommunicator::GetLeftChildProcessor(processNumber);
  int right=left+1;
  if(processNumber>0) // not root (nothing to do if root)
    {
    parent= vtkCommunicator::GetParentProcessor(processNumber);
    }

  vtkHierarchicalBoxDataSet* hbDS = 
    vtkHierarchicalBoxDataSet::SafeDownCast(composite);
  vtkMultiBlockDataSet* mbDS =
    vtkMultiBlockDataSet::SafeDownCast(composite);

  assert("check: ds must be hierarchical or multiblock" && (hbDS || mbDS));
  unsigned int numberOfLevels=1;

  // If this is an AMR SpyPlot we need to first determine
  // the global number of levels. Otherwise, the number of levels is 1.
  if (this->IsAMR)
    {
    // hbDS is non-null.
    assert("check: ds is vtkHierarchicalBoxDataSet" && hbDS !=0);
    numberOfLevels = hbDS->GetNumberOfLevels();
    unsigned long ulintMsgValue;
    // Update it from the children
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        // Grab the number of levels from left child
        this->GlobalController->Receive(&ulintMsgValue, 1, left,
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
          this->GlobalController->Receive(&ulintMsgValue, 1, right,
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
      this->GlobalController->Send(&ulintMsgValue, 1, parent,
                             VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS);
      this->GlobalController->Receive(&ulintMsgValue, 1, parent,
                                VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
      numberOfLevels=ulintMsgValue;
      }

    // Send it to children.
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        this->GlobalController->Send(&ulintMsgValue, 1, left,
                               VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          this->GlobalController->Send(&ulintMsgValue, 1, right,
                                 VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
          }
        }
      }

    if(numberOfLevels>hbDS->GetNumberOfLevels())
      {
      hbDS->SetNumberOfLevels(numberOfLevels);
      }
    }
  // At this point, the global number of levels is set in each processor.
  // Update each level
  // i.e. for each level synchronize the number of datasets (or pieces).
  unsigned int level;
  for (level = 0; level<numberOfLevels; level++)
    {
    int intMsgValue;
    int numberOfDataSets= hbDS? hbDS->GetNumberOfDataSets(level):
      mbDS->GetNumberOfBlocks();
    int totalNumberOfDataSets=numberOfDataSets;
    int leftNumberOfDataSets=0;
    int rightNumberOfDataSets=0;
    // Get number of dataset of each child
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        // Grab info the number of datasets from left child
        this->GlobalController->Receive(&intMsgValue, 1, left,
                                  VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS);
        leftNumberOfDataSets=intMsgValue;
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          // Grab info the number of datasets from right child
          this->GlobalController->Receive(&intMsgValue, 1, right,
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
      this->GlobalController->Send(&intMsgValue, 1, parent,
                             VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS);
      this->GlobalController->Receive(&intMsgValue, 1, parent,
                                VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
      totalNumberOfDataSets=intMsgValue;
      this->GlobalController->Receive(&intMsgValue, 1, parent,
                                VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
      globalIndex=intMsgValue;
      }

    // Send it to children.
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        intMsgValue=totalNumberOfDataSets;
        this->GlobalController->Send(&intMsgValue, 1, left,
                               VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
        intMsgValue=globalIndex+numberOfDataSets;
        this->GlobalController->Send(&intMsgValue, 1, left,
                               VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          intMsgValue=totalNumberOfDataSets;
          this->GlobalController->Send(&intMsgValue, 1, right,
                                 VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
          intMsgValue=globalIndex+numberOfDataSets+leftNumberOfDataSets;
          this->GlobalController->Send(&intMsgValue, 1, right,
                                 VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
          }
        }
      }

    // Update the level.
    if(totalNumberOfDataSets>numberOfDataSets)
      {
      // save the current datasets
      if (hbDS)
        {
        if (globalIndex==0)
          {
          hbDS->SetNumberOfDataSets(level, totalNumberOfDataSets);
          }
        else
          {
          vtkstd::vector<vtkSmartPointer<vtkUniformGrid> > datasets;
          vtkstd::vector<vtkAMRBox> boxes;
          int kk;
          for (kk=0; kk < numberOfDataSets; kk++)
            {
            vtkAMRBox box;
            vtkUniformGrid* ug = hbDS->GetDataSet(level, kk, box);
            datasets.push_back(ug);
            boxes.push_back(box);
            }
          hbDS->SetNumberOfDataSets(level, 0); // removes all current datasets.
          hbDS->SetNumberOfDataSets(level, totalNumberOfDataSets);

          // put the datasets back starting at globalIndex.
          // All other indices are in their initialized state.
          for (kk=0; kk < numberOfDataSets; kk++)
            {
            hbDS->SetDataSet(level, kk+globalIndex, boxes[kk], datasets[kk]);
            }
          }
        }
      else // if (mbDS)
        {
        if (globalIndex == 0)
          {
          mbDS->SetNumberOfBlocks(totalNumberOfDataSets);
          }
        else
          {
          int kk;
          vtkstd::vector<vtkSmartPointer<vtkDataSet> > datasets;
          for (kk=0; kk < numberOfDataSets; kk++)
            {
            datasets.push_back(vtkDataSet::SafeDownCast(mbDS->GetBlock(kk)));
            }
          mbDS->SetNumberOfBlocks(0); // removes all current blocks.
          mbDS->SetNumberOfBlocks(totalNumberOfDataSets);
          // put the datasets back starting at globalIndex.
          // All other indices are in their initialized state.
          for (kk=0; kk < numberOfDataSets; kk++)
            {
            mbDS->SetBlock(kk+globalIndex, datasets[kk]);
            }
          }
        }
      }
    } 
}

//-----------------------------------------------------------------------------
// synch data set structure
int vtkSpyPlotReader::ComputeDerivedVars(vtkCellData* data,
  vtkSpyPlotBlock *block, vtkSpyPlotUniReader *reader, const int& blockID,
  int dims[3])
{
  if ( this->ComputeDerivedVariables != 1 )
    {
    return 0;
    }

  int numberOfMaterials = reader->GetNumberOfMaterials();
  
  //get the mass and material volume array for each material
  vtkDataArray** materialMasses = new vtkDataArray*[numberOfMaterials];
  vtkDataArray** materialVolumeFractions = new vtkDataArray*[numberOfMaterials];
  
  //bit mask of which materials we have all the information for
  for ( int i=0; i < numberOfMaterials; i++)    
    {
    materialMasses[i] = reader->GetMaterialMassField(blockID, i);
    materialVolumeFractions[i] = reader->GetMaterialVolumeFractionField(blockID,i);    
    }

  block->SetCoordinateSystem(reader->GetCoordinateSystem());
  block->ComputeDerivedVariables(data, numberOfMaterials,
    materialMasses, materialVolumeFractions, dims, this->DownConvertVolumeFraction);
    
  //cleanup memory and leave  
  delete[] materialMasses;
  delete[] materialVolumeFractions;

  return 1;
}

static void createSpyPlotLevelArray(vtkCellData *cd, int size, int level)
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
        
