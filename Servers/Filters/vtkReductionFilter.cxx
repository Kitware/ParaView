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

#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRemoteConnection.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkToolkits.h"

#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#endif

#include <vtkstd/vector>

vtkStandardNewMacro(vtkReductionFilter);
vtkCxxRevisionMacro(vtkReductionFilter, "1.5");
vtkCxxSetObjectMacro(vtkReductionFilter, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkReductionFilter, PreReductionHelper, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkReductionFilter, ReductionHelper, vtkAlgorithm);

//-----------------------------------------------------------------------------
vtkReductionFilter::vtkReductionFilter()
{
  this->Controller= 0;
  this->RawData = 0;
  this->PreReductionHelper = 0;
  this->ReductionHelper = 0;
  this->PassThrough = -1;
}

//-----------------------------------------------------------------------------
vtkReductionFilter::~vtkReductionFilter()
{
  this->SetPreReductionHelper(0);
  this->SetReductionHelper(0);
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
int vtkReductionFilter::RequestData(vtkInformation*,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = 0;
  vtkDataSet* output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
    {
    input = vtkDataSet::SafeDownCast(
      inputVector[0]->GetInformationObject(0)->Get(
        vtkDataObject::DATA_OBJECT()));
    }
  
  this->Reduce(input, output);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::Reduce(vtkDataSet* input, vtkDataSet* output)
{
  //run the PreReduction filter on our input
  //result goes into preOutput
  vtkDataSet *preOutput = NULL;
  if (this->PreReductionHelper == NULL)
    {
    //allow a passthrough
    preOutput = input->NewInstance();
    preOutput->ShallowCopy(input);
    }
  else
    {
    this->PreReductionHelper->RemoveAllInputs();
    this->PreReductionHelper->AddInputConnection(input->GetProducerPort());
    this->PreReductionHelper->Update();
    vtkDataSet* reduced_output = 
      vtkDataSet::SafeDownCast(this->PreReductionHelper->GetOutputDataObject(0));
    if (this->ReductionHelper != NULL)
      {
      vtkInformation* info = this->ReductionHelper->GetInputPortInformation(0);
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
          vtkWarningMacro("Prereduction filter's output type is not same as the reduction filter's input type.");
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

  this->MarshallData(preOutput);
  if (myId == 0)
    {
    int *data_lengths = new int[numProcs];
    int *offsets = new int[numProcs];

    // Collect data lengths from all satellites.
    com->Gather(&this->DataLength, data_lengths, 1, 0);

    // Compute total buffer size, and offsets to use while collecting data.
    int total_size = 0;
    int cc;
    for (cc=0; cc < numProcs; ++cc)
      {
      offsets[cc] = total_size;
      total_size += data_lengths[cc];
      }
    char* gathered_data = new char[total_size];
    com->GatherV(this->RawData, gathered_data, this->DataLength,
      data_lengths, offsets, 0);

    // Form vtkDataSets from collected data.
    // Meanwhile if the user wants to see only one node's data
    // then pass only that through
    vtkstd::vector<vtkSmartPointer<vtkDataSet> > data_sets;
    for (cc=0; cc < numProcs; ++cc)
      {
      if (this->PassThrough<0 || this->PassThrough==cc)
        {        
        vtkDataSet* ds = this->Reconstruct(
          gathered_data + offsets[cc], data_lengths[cc]);
        data_sets.push_back(ds);
        ds->Delete();
        }
      }

    // Now run the ReductionHelper on the collected results from each node
    // result goes into output
    if (!this->ReductionHelper)
      {
      //allow a passthrough
      //in this case just send the data from one node
      output->ShallowCopy(data_sets[0]);
      }
    else
      {
      this->ReductionHelper->RemoveAllInputs();
      //connect all (or just the selected selected) datasets to the reduction
      //algorithm
      if (this->PassThrough == -1)
        {
        for (cc=0; cc<numProcs; ++cc)
          {
          this->ReductionHelper->AddInputConnection(
            data_sets[cc]->GetProducerPort());
          }
        } 
      else
        {
        this->ReductionHelper->AddInputConnection(
          data_sets[0]->GetProducerPort());
        }
       
      this->ReductionHelper->Update();
      vtkDataSet* reduced_output = 
        vtkDataSet::SafeDownCast(
          this->ReductionHelper->GetOutputDataObject(0));

      if (output->IsA(reduced_output->GetClassName()))
        {
        output->ShallowCopy(reduced_output);
        }
      else
        {
        vtkErrorMacro("Reduction output type is not same as the reduction filter's output type.");
        }
      }

    delete[] data_lengths;
    delete[] offsets;
    delete[] gathered_data;
    }
  else
    {
    // Send our data length to the root.
    com->Gather(&this->DataLength, 0, 1, 0);
    // Send the data to be gathered on the root.
    com->GatherV(this->RawData, 0, this->DataLength, 0, 0, 0);
    output->ShallowCopy(preOutput);
    }

  preOutput->Delete();
  delete []this->RawData;
  this->RawData = 0;
  this->DataLength = 0;
#endif
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::MarshallData(vtkDataSet* input)
{
  vtkDataSet* data = input->NewInstance();
  data->ShallowCopy(input);

  vtkDataSetWriter* writer = vtkDataSetWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->SetInput(data);
  writer->Write();

  delete []this->RawData;
  this->DataLength = writer->GetOutputStringLength();
  this->RawData = writer->RegisterAndGetOutputString();
  writer->Delete();
  data->Delete();
}

//-----------------------------------------------------------------------------
vtkDataSet* vtkReductionFilter::Reconstruct(char* raw_data, int data_length)
{
  vtkDataSetReader* reader= vtkDataSetReader::New();
  reader->ReadFromInputStringOn();
  vtkCharArray* string_data = vtkCharArray::New();
  string_data->SetArray(raw_data, data_length, 1);
  reader->SetInputArray(string_data);
  reader->Update();

  vtkDataSet* output = reader->GetOutput()->NewInstance();
  output->ShallowCopy(reader->GetOutput());

  reader->Delete();
  string_data->Delete();
  return output;
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreReductionHelper: " << this->PreReductionHelper << endl;
  os << indent << "ReductionHelper: " << this->ReductionHelper << endl;
  os << indent << "Controller: " << this->Controller << endl;
}
