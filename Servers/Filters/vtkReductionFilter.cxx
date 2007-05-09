/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReductionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReductionFilter.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkRectilinearGrid.h"
#include "vtkRemoteConnection.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkToolkits.h"

#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#endif

#include <vtkstd/vector>

vtkStandardNewMacro(vtkReductionFilter);
vtkCxxRevisionMacro(vtkReductionFilter, "1.13.2.1");
vtkCxxSetObjectMacro(vtkReductionFilter, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkReductionFilter, PreGatherHelper, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkReductionFilter, PostGatherHelper, vtkAlgorithm);

//-----------------------------------------------------------------------------
vtkReductionFilter::vtkReductionFilter()
{
  this->Controller= 0;
  this->RawData = 0;
  this->PreGatherHelper = 0;
  this->PostGatherHelper = 0;
  this->PassThrough = -1;
  this->GenerateProcessIds = 0;
}

//-----------------------------------------------------------------------------
vtkReductionFilter::~vtkReductionFilter()
{
  this->SetPreGatherHelper(0);
  this->SetPostGatherHelper(0);
  this->SetController(0);
  delete []this->RawData;
}

//-----------------------------------------------------------------------------
int vtkReductionFilter::FillInputPortInformation(int idx, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return this->Superclass::FillInputPortInformation(idx, info);
}

//-----------------------------------------------------------------------------
int vtkReductionFilter::RequestDataObject(
  vtkInformation* reqInfo,
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  if (this->PostGatherHelper != NULL)
    {
    vtkInformation* helpersInfo = 
      this->PostGatherHelper->GetOutputPortInformation(0);

    const char *hOT = helpersInfo->Get(vtkDataObject::DATA_TYPE_NAME());
    const char *helpersOutType = hOT;
    if ((!strcmp(hOT, "vtkDataSet") || !strcmp(hOT, "vtkDataObject")))
      {
      // Output type must be same as input.
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
      vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
      helpersOutType = input? input->GetClassName() : "vtkUnstructuredGrid";
      }
    
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject *output = reqInfo->Get(vtkDataObject::DATA_OBJECT());
      
    if (!output || !output->IsA(helpersOutType)) 
      {
      vtkObject* anObj = vtkDataObjectTypes::NewDataObject(helpersOutType);
      if (!anObj || !anObj->IsA(helpersOutType))
        {
        vtkErrorMacro("Could not create chosen output data type.");
        return 0;
        }
      vtkDataObject* newOutput = vtkDataObject::SafeDownCast(anObj);
      newOutput->SetPipelineInformation(info);
      newOutput->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      }
    return 1;
    }
  else
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  
    if (input)
      {
      // for each output
      for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
        {
        vtkInformation* info = outputVector->GetInformationObject(i);
        vtkDataObject *output =  info->Get(vtkDataObject::DATA_OBJECT());
    
        if (!output || !output->IsA(input->GetClassName())) 
          {
          vtkDataObject* newOutput = input->NewInstance();
          newOutput->SetPipelineInformation(info);
          newOutput->Delete();
          this->GetOutputPortInformation(0)->Set(
            vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
          }
        }
      return 1;
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkReductionFilter::RequestData(vtkInformation*,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* input = 0;
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
    {
    input = inputVector[0]->GetInformationObject(0)->Get(
        vtkDataObject::DATA_OBJECT());
    }
  
  this->Reduce(input, output);

  output->GetInformation()->Set(vtkDataObject::DATA_PIECE_NUMBER(), 
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 
    outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::Reduce(vtkDataObject* input, vtkDataObject* output)
{
  //run the PreReduction filter on our input
  //result goes into preOutput
  vtkDataObject *preOutput = NULL;
  if (this->PreGatherHelper == NULL)
    {
    //allow a passthrough
    preOutput = input->NewInstance();
    preOutput->ShallowCopy(input);
    }
  else
    {        
    //don't just use the input directly, in that case the pipeline info gets
    //messed up and PreGatherHelper won't have piece or time info.
    this->PreGatherHelper->RemoveAllInputs();
    vtkDataObject *incopy = input->NewInstance();
    incopy->ShallowCopy(input);
    this->PreGatherHelper->AddInputConnection(0, incopy->GetProducerPort());
    this->PreGatherHelper->Update();
    vtkDataObject* reduced_output = 
      this->PreGatherHelper->GetOutputDataObject(0);
    incopy->Delete();

    if (this->PostGatherHelper != NULL)
      {
      vtkInformation* info = this->PostGatherHelper->GetInputPortInformation(0);
      if (info) 
        {
        const char* expectedType =
          info->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
        if (reduced_output->IsA(expectedType))
          {
          preOutput = reduced_output->NewInstance();
          preOutput->ShallowCopy(reduced_output);
          }
        else 
          {
          vtkWarningMacro("PreGatherHelper's output type is not compatible with the PostGatherHelper's input type.");
          preOutput = input->NewInstance();
          preOutput->ShallowCopy(input);
          }
        }
      }
    else
      {
      preOutput = reduced_output->NewInstance();
      preOutput->ShallowCopy(reduced_output);
      }
    }

  vtkMultiProcessController* controller = this->Controller;
  if (!controller || controller->GetNumberOfProcesses() <= 1)
    {
    output->ShallowCopy(preOutput);
    preOutput->Delete();
    return;
    }

  vtkDataSet* dsPreOutput = vtkDataSet::SafeDownCast(preOutput);
  if (this->GenerateProcessIds && dsPreOutput)
    {
    // Note that preOutput is never the input directly (it is shallow copied at
    // the least, hence we can add arrays to it.
    vtkIdTypeArray* originalProcessIds = vtkIdTypeArray::New();
    originalProcessIds->SetNumberOfComponents(1);
    originalProcessIds->SetName("vtkOriginalProcessIds");
    originalProcessIds->SetNumberOfTuples(dsPreOutput->GetNumberOfPoints());
    originalProcessIds->FillComponent(0, controller->GetLocalProcessId());
    dsPreOutput->GetPointData()->AddArray(originalProcessIds);
    originalProcessIds->Delete();

    originalProcessIds = vtkIdTypeArray::New();
    originalProcessIds->SetNumberOfComponents(1);
    originalProcessIds->SetName("vtkOriginalProcessIds");
    originalProcessIds->SetNumberOfTuples(dsPreOutput->GetNumberOfCells());
    originalProcessIds->FillComponent(0, controller->GetLocalProcessId());
    dsPreOutput->GetCellData()->AddArray(originalProcessIds);
    originalProcessIds->Delete();
    }


#ifdef VTK_USE_MPI
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
    controller->GetCommunicator());
  if (!com)
    {
    vtkErrorMacro("vtkMPICommunicator needed to perform reduction.");
    return;
    }
  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  if (this->PassThrough > numProcs)
    {
    this->PassThrough = -1;
    }

  this->MarshallData(preOutput);
  if (myId == 0)
    {
    int *data_lengths = new int[numProcs];
    int *offsets = new int[numProcs];
    int *extents = new int[numProcs*6];
    int *extent_lengths = new int[numProcs];
    int *extent_offsets = new int[numProcs];

    // Collect data lengths from all satellites.
    com->Gather(&this->DataLength, data_lengths, 1, 0);

    // Compute total buffer size, and offsets to use while collecting data.
    int total_size = 0;
    int cc;
    for (cc=0; cc < numProcs; ++cc)
      {
      offsets[cc] = total_size;
      total_size += data_lengths[cc];

      extent_offsets[cc] = cc*6;
      extent_lengths[cc] = 6;
      }
    char* gathered_data = new char[total_size];
    com->GatherV(this->RawData, gathered_data, this->DataLength,
      data_lengths, offsets, 0);
    com->GatherV(this->Extent, extents, 6, 
      extent_lengths, extent_offsets, 0);


    // Form vtkDataObjects from collected data.
    // Meanwhile if the user wants to see only one node's data
    // then pass only that through
    vtkstd::vector<vtkSmartPointer<vtkDataObject> > data_sets;
    for (cc=0; cc < numProcs; ++cc)
      {
      if (this->PassThrough<0 || this->PassThrough==cc)
        {        
        vtkDataObject* ds = vtkDataObject::SafeDownCast(this->Reconstruct(
          gathered_data + offsets[cc], data_lengths[cc], &extents[cc*6]));
        data_sets.push_back(ds);
        ds->Delete();
        }
      }

    // Now run the PostGatherHelper on the collected results from each node
    // result goes into output
    if (!this->PostGatherHelper)
      {
      //allow a passthrough
      //in this case just send the data from one node
      output->ShallowCopy(data_sets[0]);
      }
    else
      {
      this->PostGatherHelper->RemoveAllInputs();
      //connect all (or just the selected selected) datasets to the reduction
      //algorithm
      if (this->PassThrough == -1)
        {
        for (cc=0; cc<numProcs; ++cc)
          {
          this->PostGatherHelper->AddInputConnection(
            data_sets[cc]->GetProducerPort());
          }
        } 
      else
        {
        this->PostGatherHelper->AddInputConnection(
          data_sets[0]->GetProducerPort());
        }
       
      this->PostGatherHelper->Update();
      vtkDataObject* reduced_output = 
        this->PostGatherHelper->GetOutputDataObject(0);

      if (output->IsA(reduced_output->GetClassName()))
        {
        output->ShallowCopy(reduced_output);
        }
      else
        {
        cerr << "POST OUT = " << reduced_output->GetClassName() << endl;
        cerr << "REDX OUT = " << output->GetClassName() << endl;
        vtkErrorMacro("PostGatherHelper's output type is not same as the ReductionFilters's output type.");
        }
      }

    delete[] data_lengths;
    delete[] offsets;
    delete[] gathered_data;
    delete[] extents;
    delete[] extent_offsets;
    delete[] extent_lengths;
    }
  else
    {
    // Send our data length to the root.
    com->Gather(&this->DataLength, 0, 1, 0);
    // Send the data to be gathered on the root.
    com->GatherV(this->RawData, 0, this->DataLength, 0, 0, 0);
    // Send the extents.
    com->GatherV(this->Extent, 0, 6, 0, 0, 0);
    output->ShallowCopy(preOutput);
    }

  preOutput->Delete();
  delete []this->RawData;
  this->RawData = 0;
  this->DataLength = 0;
#endif
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::MarshallData(vtkDataObject* input)
{
  vtkDataObject* data = input->NewInstance();
  data->ShallowCopy(input);

  vtkGenericDataObjectWriter* writer = vtkGenericDataObjectWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->SetInput(data);
  writer->Write();

  delete []this->RawData;
  this->DataLength = writer->GetOutputStringLength();
  this->RawData = writer->RegisterAndGetOutputString();
  this->Extent[0] = this->Extent[1] = this->Extent[2] = 
    this->Extent[3] = this->Extent[4] = this->Extent[5] = 0;
  if (data->GetExtentType() == VTK_3D_EXTENT)
    {
    vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(data);
    vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(data);
    vtkImageData* id = vtkImageData::SafeDownCast(data);
    if (rg)
      {
      rg->GetExtent(this->Extent);
      }
    else if (sg)
      {
      sg->GetExtent(this->Extent);
      }
    else if (id)
      {
      id->GetExtent(this->Extent);
      }
    }
  writer->Delete();
  data->Delete();
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkReductionFilter::Reconstruct(char* raw_data, int data_length,
  int *extent)
{
  vtkGenericDataObjectReader* reader= vtkGenericDataObjectReader::New();
  reader->ReadFromInputStringOn();
  vtkCharArray* string_data = vtkCharArray::New();
  string_data->SetArray(raw_data, data_length, 1);
  reader->SetInputArray(string_data);
  reader->Update();

  vtkDataObject* output = reader->GetOutput()->NewInstance();
  output->ShallowCopy(reader->GetOutput());

  // Set the extents if the dataobject supports it.
  if (output->GetExtentType() == VTK_3D_EXTENT)
    {
    vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(output);
    vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(output);
    vtkImageData* id = vtkImageData::SafeDownCast(output);
    if (rg)
      {
      rg->SetExtent(extent);
      }
    else if (sg)
      {
      sg->SetExtent(extent);
      }
    else if (id)
      {
      id->SetExtent(extent);
      }
    }

  reader->Delete();
  string_data->Delete();
  return output;
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreGatherHelper: " << this->PreGatherHelper << endl;
  os << indent << "PostGatherHelper: " << this->PostGatherHelper << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "PassThrough: " << this->PassThrough << endl;
  os << indent << "GenerateProcessIds: " << this->GenerateProcessIds << endl;
}
