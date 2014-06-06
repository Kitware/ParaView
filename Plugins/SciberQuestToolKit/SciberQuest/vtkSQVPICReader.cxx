/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQVPICReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQVPICReader.h"

#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTableExtentTranslator.h"
#include "vtkToolkits.h"

#include "vtkMultiProcessController.h"

#include "VPICDataSet.h"
#include "GridExchange.h"
#include "VPICView.h"

#include "SQMacros.h"
#include "XMLUtils.h"
#include "vtkSQLog.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSQVPICReader);

//----------------------------------------------------------------------------
// Constructor for VPIC Reader
//----------------------------------------------------------------------------
vtkSQVPICReader::vtkSQVPICReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->FileName = NULL;
  this->NumberOfNodes = 0;
  this->NumberOfVariables = 0;
  this->CurrentTimeStep = -1;
  this->PointDataArraySelection = vtkDataArraySelection::New();

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkSQVPICReader::SelectionCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);
  // External VPICDataSet for actually reading files
  this->vpicData = 0;
  this->exchanger = 0;
  this->VariableName = 0;
  this->VariableStruct = 0;
  this->TimeSteps = 0;
  this->dataLoaded = 0;
  this->data = 0;

  // One overlap cell on first plane and one extra on last plane
  this->ghostLevel0 = 1;
  this->ghostLevel1 = 2;

  this->Stride[0] = 1;
  this->Stride[1] = 1;
  this->Stride[2] = 1;

  this->XLayout[0] = 1;
  this->YLayout[0] = 1;
  this->ZLayout[0] = 1;
  this->XLayout[1] = -1;
  this->YLayout[1] = -1;
  this->ZLayout[1] = -1;

  this->MPIController = vtkMultiProcessController::GetGlobalController();

  if(this->MPIController)
    {
    this->Rank = this->MPIController->GetLocalProcessId();
    this->TotalRank = this->MPIController->GetNumberOfProcesses();
    }
  else
    {
    this->Rank = 0;
    this->TotalRank = 1;
    }
}

//----------------------------------------------------------------------------
// Destructor for VPIC Reader
//----------------------------------------------------------------------------
vtkSQVPICReader::~vtkSQVPICReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  this->PointDataArraySelection->Delete();

  if (this->vpicData)
    delete this->vpicData;
  if (this->VariableName)
    delete [] this->VariableName;
  if (this->VariableStruct)
    delete [] this->VariableStruct;
  if (this->TimeSteps)
    delete [] this->TimeSteps;
  if (this->dataLoaded)
    delete [] this->dataLoaded;

  if (this->exchanger)
    delete this->exchanger;

  if (this->data)
    {
    for (int var = 0; var < this->NumberOfVariables; var++)
      {
      if (this->data[var])
        {
        this->data[var]->Delete();
        }
      }
    delete [] this->data;
    }

  this->SelectionObserver->Delete();

  // Do not delete the MPIController it is Singleton like and will
  // cleanup itself;
  this->MPIController = NULL;
}


//----------------------------------------------------------------------------
// Initialize from a xml document.
//----------------------------------------------------------------------------
int vtkSQVPICReader::Initialize(
      vtkPVXMLElement *root,
      const char *fileName,
      std::vector<std::string> &arrays)
{
  vtkPVXMLElement *elem=GetOptionalElement(root,"vtkSQVPICReader");
  if (elem==0)
    {
    return -1;
    }

  // force first pipeline pass
  this->SetFileName(fileName);
  this->UpdateInformation();

  int stride[3]={-1,-1,-1};
  GetOptionalAttribute<int,3>(elem,"stride",stride);
  if (stride[0]>0)
    {
    this->SetStride(stride);
    }

  // subset the data
  // when the user passes -1, we'll use the whole extent
  int wholeExtent[6];
  this->GetXLayout(wholeExtent);
  this->GetYLayout(wholeExtent+2);
  this->GetZLayout(wholeExtent+4);
  int subset[6]={-1,-1,-1,-1,-1,-1};
  GetOptionalAttribute<int,2>(elem,"ISubset",subset);
  GetOptionalAttribute<int,2>(elem,"JSubset",subset+2);
  GetOptionalAttribute<int,2>(elem,"KSubset",subset+4);
  for (int i=0; i<6; ++i)
    {
    if (subset[i]<0) subset[i]=wholeExtent[i];
    }
  this->SetXExtent(subset);
  this->SetYExtent(subset+2);
  this->SetZExtent(subset+4);

  // select arrays to process
  // when none are provided we process all available
  this->DisableAllPointArrays();
  int nArrays=0;
  elem=GetOptionalElement(elem,"arrays");
  if (elem)
    {
    ExtractValues(elem->GetCharacterData(),arrays);
    nArrays=arrays.size();
    if (nArrays<1)
      {
      sqErrorMacro(pCerr(),"Error: parsing <arrays>.");
      return -1;
      }
    // vpic array names contain spaces, for our convinience these
    // will be replaced with - in the xml config.
    for (int i=0; i<nArrays; ++i)
      {
      std::string &arrayName=arrays[i];
      int arrayNameLen=arrayName.size();
      for (int j=0; j<arrayNameLen; ++j)
        {
        if (arrayName[j]=='-') arrayName[j]=' ';
        }
      }
    }
  else
    {
    nArrays=this->GetNumberOfPointArrays();
    for (int i=0; i<nArrays; ++i)
      {
      const char * arrayName=this->GetPointArrayName(i);
      arrays.push_back(arrayName);
      }
    }

  #if defined vtkSQVPICReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  *log
    << "# ::vtkSQVPICReader" << "\n"
    << "#   stride=" << stride << "\n"
    << "#   wholeExtent=" << Tuple<int>(wholeExtent,6) << "\n"
    << "#   subsetExtent=" << Tuple<int>(subset,6) << "\n";
  for (int i=0; i<nArrays; ++i)
    {
    *log << "#   arrayName_" << i << "=" << arrays[i] << "\n";
    }
  *log << "\n";
  #endif

  return 0;
}

//----------------------------------------------------------------------------
// Verify that the file exists
//----------------------------------------------------------------------------
int vtkSQVPICReader::RequestInformation(
  vtkInformation *vtkNotUsed(reqInfo),
  vtkInformationVector **vtkNotUsed(inVector),
  vtkInformationVector *outVector)
{
  // Verify that file exists
  if ( !this->FileName ) {
    vtkErrorMacro("No filename specified");
    return 0;
  }

  // Get ParaView information and output pointers
  vtkInformation* outInfo = outVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // RequestInformation() is called for every Modified() event which means
  // when more variable data is selected, time step is changed or stride
  // is changed it will be called again
  // Only want to create the VPICDataSet one time

  if (this->vpicData == 0) {

    // Create the general VPICDataSet structure first time method is called
    // At this point we only know the file name driving the data set but
    // no variables or strides have been selected

    // Object which will know all of structure and processor part of the data
    this->vpicData = new VPICDataSet();
    this->vpicData->setRank(this->Rank);
    this->vpicData->setTotalRank(this->TotalRank);

    // Set the variable names and types
    // Build the partition table which shows the relation of each file
    // within the entire problem set, but does not partition between processors
    this->vpicData->initialize(this->FileName);

    // Copy in variable names to be offered
    this->NumberOfVariables = this->vpicData->getNumberOfVariables();
    this->VariableName = new vtkStdString[this->NumberOfVariables];

    // Data is SCALAR, VECTOR or TENSOR
    this->VariableStruct = new int[this->NumberOfVariables];

    for (int var = 0; var < this->NumberOfVariables; var++) {
      this->VariableName[var] = this->vpicData->getVariableName(var);
      this->VariableStruct[var] = this->vpicData->getVariableStruct(var);
      this->PointDataArraySelection->AddArray(this->VariableName[var].c_str());
    }

    // Allocate the ParaView data arrays which will hold the variable data
    this->data = new vtkFloatArray*[this->NumberOfVariables];
    this->dataLoaded = new int[this->NumberOfVariables];
    for (int var = 0; var < this->NumberOfVariables; var++) {
      this->data[var] = vtkFloatArray::New();
      this->data[var]->SetName(VariableName[var].c_str());
      this->dataLoaded[var] = 0;
    }

    // Set the overall problem file decomposition for the GUI extent range
    int layoutSize[DIMENSION];
    this->vpicData->getLayoutSize(layoutSize);
    this->XLayout[0] = 0;       this->XLayout[1] = layoutSize[0] - 1;
    this->YLayout[0] = 0;       this->YLayout[1] = layoutSize[1] - 1;
    this->ZLayout[0] = 0;       this->ZLayout[1] = layoutSize[2] - 1;

    // Maximum number of pieces (processors) is number of files
    this->NumberOfPieces = this->vpicData->getNumberOfParts();
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

    // Collect temporal information
    this->NumberOfTimeSteps = this->vpicData->getNumberOfTimeSteps();
    this->TimeSteps = NULL;

    if (this->NumberOfTimeSteps > 0) {
      this->TimeSteps = new double[this->NumberOfTimeSteps];

      for (int step = 0; step < this->NumberOfTimeSteps; step++)
         this->TimeSteps[step] = (double) this->vpicData->getTimeStep(step);

      // Tell the pipeline what steps are available
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                   this->TimeSteps, this->NumberOfTimeSteps);

      // Range is required to get GUI to show things
      double tRange[2];
      tRange[0] = this->TimeSteps[0];
      tRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                   tRange, 2);
    } else {
      outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                   this->TimeSteps, this->NumberOfTimeSteps);
    }
  }

  // Set the current stride within the dataset
  // If it is a new stride the dataset will indicate that a new partition
  // must be done so that new grid subextents are set on each processor
  this->vpicData->setView(this->XExtent, this->YExtent, this->ZExtent);
  this->vpicData->setStride(this->Stride);

  // Repartition only has to be done when the stride changes
  // To handle the loading for the very first time, vpicData stride is set
  // to 0 so that by setting to the default of 1, the partition has be to done
  if (this->vpicData->needsGridCalculation() == true) {

    // If grid is recalculated all data must be realoaded
    for (int var = 0; var < this->NumberOfVariables; var++)
      this->dataLoaded[var] = 0;

    // Partitions the data between processors and sets grid extents
    this->vpicData->calculateGridExtents();

    this->NumberOfCells = this->vpicData->getNumberOfCells();
    this->NumberOfNodes = this->vpicData->getNumberOfNodes();

    // Set the whole extent
    this->vpicData->getGridSize(this->Dimension);
    this->vpicData->getWholeExtent(this->WholeExtent);
    output->SetDimensions(this->Dimension);
    output->SetWholeExtent(this->WholeExtent);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 this->WholeExtent, 6);

    // Let the pipeline know how we want the data to be broken up
    // Some processors might not get a piece of data to render
    vtkTableExtentTranslator *extentTable = vtkTableExtentTranslator::New();
    int processorUsed = this->vpicData->getProcessorUsed();

    if(this->MPIController)
      {
      this->MPIController->AllReduce(&processorUsed, &this->UsedRank,
                                     1, vtkCommunicator::SUM_OP);
      }

    extentTable->SetNumberOfPieces(this->UsedRank);

    for (int piece = 0; piece < this->UsedRank; piece++) {
      int subextent[6];
      this->vpicData->getSubExtent(piece, subextent);
      extentTable->SetExtentForPiece(piece, subextent);
    }
    this->vpicData->getSubExtent(this->Rank, this->SubExtent);
    extentTable->SetPiece(this->Rank);
    extentTable->SetWholeExtent(this->WholeExtent);
    extentTable->SetExtent(this->SubExtent);

    vtkStreamingDemandDrivenPipeline* pipeline =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
    pipeline->SetExtentTranslator(outInfo, extentTable);
    extentTable->Delete();

    // Reset the SubExtent on this processor to include ghost cells
    // Leave the subextents in the extent table as the size without ghosts
    for (int dim = 0; dim < DIMENSION; dim++) {
      if (this->SubExtent[dim*2] != 0)
        this->SubExtent[dim*2] -= 1;
      if (this->SubExtent[dim*2+1] != this->Dimension[dim] - 1)
        this->SubExtent[dim*2+1] += 1;
    }

    // Set the subextent dimension size
    if (processorUsed == 1) {
      this->SubDimension[0] = this->SubExtent[1] - this->SubExtent[0] + 1;
      this->SubDimension[1] = this->SubExtent[3] - this->SubExtent[2] + 1;
      this->SubDimension[2] = this->SubExtent[5] - this->SubExtent[4] + 1;
    } else {
      this->SubDimension[0] = 0;
      this->SubDimension[1] = 0;
      this->SubDimension[2] = 0;
    }

    // Total size of the subextent
    this->NumberOfTuples = 1;
    for (int dim = 0; dim < DIMENSION; dim++)
      this->NumberOfTuples *= this->SubDimension[dim];

    // Set ghost cell edges
    this->NumberOfGhostTuples = 1;
    for (int dim = 0; dim < DIMENSION; dim++) {

      // Local block dimensions for loading a component of data
      // Different number of ghost cells are added depending on where the
      // processor is in the problem grid
      this->GhostDimension[dim] = this->SubDimension[dim];

      // If processor is on an edge don't write a ghost cell (offset the start)
      this->Start[dim] = 0;
      if (SubExtent[dim*2] == 0) {
        this->Start[dim] = this->ghostLevel0;
        this->GhostDimension[dim] += this->ghostLevel0;
      }

      // Processors not on last plane already have one overlap cell
      if (SubExtent[dim*2 + 1] == (this->Dimension[dim] - 1)) {
        this->GhostDimension[dim] += this->ghostLevel1;
      }

      // Size of the local block for loading a component of data with ghosts
      this->NumberOfGhostTuples *= this->GhostDimension[dim];
    }

#ifdef VTK_USE_MPI
    if (this->TotalRank>1)
      {
      // Set up the GridExchange for sharing ghost cells on this view
      int decomposition[DIMENSION];
      this->vpicData->getDecomposition(decomposition);

      if (this->exchanger)
        delete this->exchanger;

      this->exchanger = new GridExchange
        (this->Rank, this->TotalRank, decomposition,
         this->GhostDimension, this->ghostLevel0, this->ghostLevel1);
      }
#endif
  }
  return 1;
}

//----------------------------------------------------------------------------
// Data is read into a vtkImageData
// BLOCK structured means data is organized by variable and then by cell
//----------------------------------------------------------------------------
int vtkSQVPICReader::RequestData(
  vtkInformation *vtkNotUsed(reqInfo),
  vtkInformationVector **vtkNotUsed(inVector),
  vtkInformationVector *outVector)
{
  vtkInformation *outInfo = outVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Even if the pipeline asks for a smaller subextent, give it the
  // full subextent with ghosts
  vtkStreamingDemandDrivenPipeline* pipeline =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  pipeline->SetUpdateExtent(outInfo, this->SubExtent);

  // Set the subextent for this processor
  output->SetExtent(this->SubExtent);

  // Ask VPICDataSet to check for additional time steps
  // If found VPICDataSet will update its structure
  this->vpicData->addNewTimeSteps();
  int numberOfTimeSteps = this->vpicData->getNumberOfTimeSteps();

  // If more time steps ParaView must update information
  if (numberOfTimeSteps > this->NumberOfTimeSteps) {

    this->NumberOfTimeSteps = numberOfTimeSteps;
    delete [] this->TimeSteps;
    this->TimeSteps = new double[this->NumberOfTimeSteps];

    for (int step = 0; step < this->NumberOfTimeSteps; step++)
      this->TimeSteps[step] = (double) this->vpicData->getTimeStep(step);

    // Tell the pipeline what steps are available
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 this->TimeSteps, this->NumberOfTimeSteps);

    // Range is required to get GUI to show things
    double tRange[2];
    tRange[0] = this->TimeSteps[0];
    tRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
  }

  // Collect the time step requested
  double* requestedTimeSteps = NULL;
  vtkInformationDoubleVectorKey* timeKey =
    static_cast<vtkInformationDoubleVectorKey*>
      (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

  // Actual time for the time step
  double dTime = this->TimeSteps[0];
  if (outInfo->Has(timeKey)) {
    requestedTimeSteps = outInfo->Get(timeKey);
    dTime = requestedTimeSteps[0];
  }

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), &dTime, 1);

  // Index of the time step to request
  int timeStep = 0;
  while (timeStep < this->NumberOfTimeSteps &&
         this->TimeSteps[timeStep] < dTime)
    timeStep++;

  // If this is a new time step read all the data from files
  int timeChanged = 0;
  if (this->CurrentTimeStep != timeStep) {
    timeChanged = 1;
    this->CurrentTimeStep = timeStep;
  }

  // Get size information from the VPICDataSet to set ImageData
  double origin[DIMENSION], step[DIMENSION];
  this->vpicData->getOrigin(origin);
  this->vpicData->getStep(step);
  output->SetSpacing(step);
  output->SetOrigin(origin);

  // Examine each variable to see if it is selected
  for (int var = 0; var < this->NumberOfVariables; var++) {

    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var)) {
      if (this->dataLoaded[var] == 0 || timeChanged) {
        LoadVariableData(var, timeStep);
        this->dataLoaded[var] = 1;
      }
      output->GetPointData()->AddArray(this->data[var]);

    } else {
      this->dataLoaded[var] = 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Load one variable data array of BLOCK structure into ParaView
//----------------------------------------------------------------------------
void vtkSQVPICReader::LoadVariableData(int var, int timeStep)
{
  this->data[var]->Delete();
  this->data[var] = vtkFloatArray::New();
  this->data[var]->SetName(VariableName[var].c_str());

  /*
  if (this->Rank == 0)
    cout << "LoadVariableData " << this->VariableName[var]
         << " time " << timeStep << std::endl;
  */

  // First set the number of components for this variable
  int numberOfComponents = 0;
  if (this->VariableStruct[var] == SCALAR) {
    numberOfComponents = 1;
    this->data[var]->SetNumberOfComponents(numberOfComponents);
  }
  else if (this->VariableStruct[var] == VECTOR) {
    numberOfComponents = DIMENSION;
    this->data[var]->SetNumberOfComponents(numberOfComponents);
  }
  else if (this->VariableStruct[var] == TENSOR) {
    numberOfComponents = TENSOR_DIMENSION;
    this->data[var]->SetNumberOfComponents(TENSOR9_DIMENSION);
  }

  // Second set the number of tuples which will allocate all tuples
  this->data[var]->SetNumberOfTuples(this->NumberOfTuples);

  // For each component of the requested variable load data
  float* block = new float[this->NumberOfGhostTuples];
  float* varData = this->data[var]->GetPointer(0);

  for (int comp = 0; comp < numberOfComponents; comp++) {

    // Fetch the data for a single component into temporary storage
    this->vpicData->loadVariableData(block, this->ghostLevel0,
                                     this->GhostDimension, timeStep, var, comp);

    // Exchange the single component block retrieved from files to get ghosts
#ifdef VTK_USE_MPI
    if (this->TotalRank>1)
      {
      this->exchanger->exchangeGrid(block);
      }
#endif

    // Load the ghost component block into ParaView array
    if (this->VariableStruct[var] != TENSOR) {
      LoadComponent(varData, block, comp, numberOfComponents);
    }

    else {
      // Tensors are 6 point and must be written as 9 point
      // (0->0) (1->4) (2->8) (3->5,7) (4->2,6) (5->1,3)
      switch (comp) {
      case 0:
        LoadComponent(varData, block, 0, TENSOR9_DIMENSION);
        break;
      case 1:
        LoadComponent(varData, block, 4, TENSOR9_DIMENSION);
        break;
      case 2:
        LoadComponent(varData, block, 8, TENSOR9_DIMENSION);
        break;
      case 3:
        LoadComponent(varData, block, 5, TENSOR9_DIMENSION);
        LoadComponent(varData, block, 7, TENSOR9_DIMENSION);
        break;
      case 4:
        LoadComponent(varData, block, 2, TENSOR9_DIMENSION);
        LoadComponent(varData, block, 6, TENSOR9_DIMENSION);
        break;
      case 5:
        LoadComponent(varData, block, 1, TENSOR9_DIMENSION);
        LoadComponent(varData, block, 3, TENSOR9_DIMENSION);
        break;
      }
    }
  }
  delete [] block;
}

//----------------------------------------------------------------------------
// Load one component from the local VPIC ghost enhanced block into the
// ParaView vtkFloatArray taking into account whether the processor is
// on the front plane, the back plane or in the middle which affects
// the ghost cells which can be loaded.  ParaView array is contiguous
// memory so start at the right location and offset by number of components
//----------------------------------------------------------------------------
void vtkSQVPICReader::LoadComponent(float* varData, float* block,
                                  int comp, int numberOfComponents)
{

  // Load into the data array by tuple so place data every comp'th spot
  int pos = comp;
  for (int k = 0; k < this->SubDimension[2]; k++) {
    int kk = k + this->Start[2];
    for (int j = 0; j < this->SubDimension[1]; j++) {
      int jj = j + this->Start[1];
      for (int i = 0; i < this->SubDimension[0]; i++) {
        int ii = i + this->Start[0];

        int index = (kk * this->GhostDimension[0] * this->GhostDimension[1]) +
                    (jj * this->GhostDimension[0]) + ii;

        varData[pos] = block[index];
        pos += numberOfComponents;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSQVPICReader::SelectionCallback(vtkObject*, unsigned long vtkNotUsed(eventid),
                                      void* clientdata, void* vtkNotUsed(calldata))
{
  static_cast<vtkSQVPICReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
vtkImageData* vtkSQVPICReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkImageData* vtkSQVPICReader::GetOutput(int idx)
{
  if (idx)
    {
    return NULL;
    }
  else
    {
    return vtkImageData::SafeDownCast( this->GetOutputDataObject(idx) );
    }
}

//----------------------------------------------------------------------------
int vtkSQVPICReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkSQVPICReader::EnableAllPointArrays()
{
    this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkSQVPICReader::DisableAllPointArrays()
{
    this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
const char* vtkSQVPICReader::GetPointArrayName(int index)
{
  return this->VariableName[index].c_str();
}

//----------------------------------------------------------------------------
int vtkSQVPICReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkSQVPICReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
    this->PointDataArraySelection->EnableArray(name);
  else
    this->PointDataArraySelection->DisableArray(name);
}

void vtkSQVPICReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName != NULL ? this->FileName : "") << std::endl;
  os << indent << "Stride: {" << this->Stride[0] << ", " << this->Stride[1]
     << ", " << this->Stride[2] << "}" << std::endl;
  os << indent << "XLayout: {" << this->XLayout[0] << ", " << this->XLayout[1] << "}" << std::endl;
  os << indent << "YLayout: {" << this->YLayout[0] << ", " << this->YLayout[1] << "}" << std::endl;
  os << indent << "ZLayout: {" << this->ZLayout[0] << ", " << this->ZLayout[1] << "}" << std::endl;
  os << indent << "XExtent: {" << this->XExtent[0] << ", " << this->XExtent[1] << "}" << std::endl;
  os << indent << "YExtent: {" << this->YExtent[0] << ", " << this->YExtent[1] << "}" << std::endl;
  os << indent << "ZExtent: {" << this->ZExtent[0] << ", " << this->ZExtent[1] << "}" << std::endl;

  this->Superclass::PrintSelf(os, indent);
}
