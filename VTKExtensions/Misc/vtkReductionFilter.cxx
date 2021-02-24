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
#include "vtkClientServerStreamInstantiator.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkToolkits.h"
#include "vtkTrivialProducer.h"

#include <sstream>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkReductionFilter);
vtkCxxSetObjectMacro(vtkReductionFilter, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkReductionFilter, PreGatherHelper, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkReductionFilter, PostGatherHelper, vtkAlgorithm);

//-----------------------------------------------------------------------------
vtkReductionFilter::vtkReductionFilter()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->PreGatherHelper = nullptr;
  this->PostGatherHelper = nullptr;
  this->PassThrough = -1;
  this->GenerateProcessIds = 0;
  this->ReductionMode = vtkReductionFilter::REDUCE_ALL_TO_ONE;
  this->ReductionProcessId = 0;
}

//-----------------------------------------------------------------------------
vtkReductionFilter::~vtkReductionFilter()
{
  this->SetPreGatherHelper(nullptr);
  this->SetPostGatherHelper(nullptr);
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
int vtkReductionFilter::FillInputPortInformation(int idx, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return this->Superclass::FillInputPortInformation(idx, info);
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::SetPreGatherHelperName(const char* name)
{
  vtkSmartPointer<vtkObjectBase> foo;
  foo.TakeReference(vtkClientServerStreamInstantiator::CreateInstance(name));
  this->SetPreGatherHelper(vtkAlgorithm::SafeDownCast(foo));
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::SetPostGatherHelperName(const char* name)
{
  vtkSmartPointer<vtkObjectBase> foo;
  foo.TakeReference(vtkClientServerStreamInstantiator::CreateInstance(name));
  this->SetPostGatherHelper(vtkAlgorithm::SafeDownCast(foo));
}

//-----------------------------------------------------------------------------
int vtkReductionFilter::RequestDataObject(
  vtkInformation* reqInfo, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->PostGatherHelper != nullptr)
  {
    vtkInformation* helpersInfo = this->PostGatherHelper->GetOutputPortInformation(0);

    const char* hOT = helpersInfo->Get(vtkDataObject::DATA_TYPE_NAME());
    const char* helpersOutType = hOT;
    if ((!strcmp(hOT, "vtkPointSet") || !strcmp(hOT, "vtkDataSet") ||
          !strcmp(hOT, "vtkDataObject")))
    {
      // Output type must be same as input.
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
      vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
      helpersOutType = input ? input->GetClassName() : "vtkUnstructuredGrid";
    }

    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject* output = reqInfo->Get(vtkDataObject::DATA_OBJECT());

    if (!output || !output->IsA(helpersOutType))
    {
      vtkObject* anObj = vtkDataObjectTypes::NewDataObject(helpersOutType);
      if (!anObj || !anObj->IsA(helpersOutType))
      {
        vtkErrorMacro("Could not create chosen output data type.");
        return 0;
      }
      vtkDataObject* newOutput = vtkDataObject::SafeDownCast(anObj);
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    }
    return 1;
  }
  else
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

    if (input)
    {
      // for each output
      for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
      {
        vtkInformation* info = outputVector->GetInformationObject(i);
        vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

        if (!output || !output->IsA(input->GetClassName()))
        {
          vtkDataObject* newOutput = input->NewInstance();
          info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
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
int vtkReductionFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* input = nullptr;
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    input = inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  }

  this->Reduce(input, output);

  output->GetInformation()->Set(vtkDataObject::DATA_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkReductionFilter::PreProcess(vtkDataObject* input)
{
  if (!input)
  {
    return nullptr;
  }

  vtkSmartPointer<vtkDataObject> result;
  if (this->PreGatherHelper == nullptr)
  {
    // allow a passthrough
    result = input;
  }
  else
  {
    // don't just use the input directly, in that case the pipeline info gets
    // messed up and PreGatherHelper won't have piece or time info.
    this->PreGatherHelper->RemoveAllInputs();
    vtkDataObject* incopy = input->NewInstance();
    incopy->ShallowCopy(input);
    vtkNew<vtkTrivialProducer> incopyProducer;
    incopyProducer->SetOutput(incopy);
    this->PreGatherHelper->AddInputConnection(0, incopyProducer->GetOutputPort());
    this->PreGatherHelper->Update();
    result = this->PreGatherHelper->GetOutputDataObject(0);
    incopy->Delete();

    // If a PostGatherHelper is present, we need to ensure that the result produced
    // by this pre-processing stage is acceptable to the PostGatherHelper.
    if (this->PostGatherHelper != nullptr)
    {
      vtkInformation* info = this->PostGatherHelper->GetInputPortInformation(0);
      if (info)
      {
        const char* expectedType = info->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
        if (!result->IsA(expectedType))
        {
          vtkWarningMacro("PreGatherHelper's output type is not compatible with "
                          "the PostGatherHelper's input type.");
          result = input;
        }
      }
    }
  }

  vtkDataObject* clone = result->NewInstance();
  clone->ShallowCopy(result);
  return clone;
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::PostProcess(
  vtkDataObject* output, vtkSmartPointer<vtkDataObject> inputs[], unsigned int num_inputs)
{
  assert(num_inputs > 0);
  if (!this->PostGatherHelper)
  {
    // allow a passthrough
    // in this case just send the data from one node
    output->ShallowCopy(inputs[0]);
  }
  else
  {
    this->PostGatherHelper->RemoveAllInputs();
    // connect all (or just the selected) datasets to the reduction
    // algorithm
    for (unsigned int cc = 0; cc < num_inputs; ++cc)
    {
      this->PostGatherHelper->AddInputDataObject(inputs[cc]);
    }
    this->PostGatherHelper->Update();
    this->PostGatherHelper->RemoveAllInputs();

    vtkDataObject* reduced_output = this->PostGatherHelper->GetOutputDataObject(0);

    if (output->IsA(reduced_output->GetClassName()))
    {
      output->ShallowCopy(reduced_output);
    }
    else
    {
      vtkErrorMacro("POST OUT = " << reduced_output->GetClassName() << "\n"
                                  << "REDX OUT = " << output->GetClassName() << "\n"
                                  << "PostGatherHelper's output type is not same as the "
                                     "ReductionFilters's output type.");
    }
  }
}

//-----------------------------------------------------------------------------
void vtkReductionFilter::Reduce(vtkDataObject* input, vtkDataObject* output)
{
  // run the PreReduction filter on our input
  // result goes into preOutput
  vtkSmartPointer<vtkDataObject> preOutput;
  preOutput.TakeReference(this->PreProcess(input));

  vtkMultiProcessController* controller = this->Controller;
  if (!controller || controller->GetNumberOfProcesses() <= 1)
  {
    if (preOutput)
    {
      vtkSmartPointer<vtkDataObject> inputs[1] = { preOutput };
      this->PostProcess(output, inputs, 1);
    }
    return;
  }

  vtkDataSet* dsPreOutput = vtkDataSet::SafeDownCast(preOutput);
  if (this->GenerateProcessIds && dsPreOutput)
  {
    // Note that preOutput is never the input directly (it is shallow copied at
    // the least, hence we can add arrays to it.
    vtkIdTypeArray* originalProcessIds = nullptr;
    if (dsPreOutput->GetNumberOfPoints() > 0)
    {
      originalProcessIds = vtkIdTypeArray::New();
      originalProcessIds->SetNumberOfComponents(1);
      originalProcessIds->SetName("vtkOriginalProcessIds");
      originalProcessIds->SetNumberOfTuples(dsPreOutput->GetNumberOfPoints());
      originalProcessIds->FillComponent(0, controller->GetLocalProcessId());
      dsPreOutput->GetPointData()->AddArray(originalProcessIds);
      originalProcessIds->Delete();
    }

    if (dsPreOutput->GetNumberOfCells() > 0)
    {
      originalProcessIds = vtkIdTypeArray::New();
      originalProcessIds->SetNumberOfComponents(1);
      originalProcessIds->SetName("vtkOriginalProcessIds");
      originalProcessIds->SetNumberOfTuples(dsPreOutput->GetNumberOfCells());
      originalProcessIds->FillComponent(0, controller->GetLocalProcessId());
      dsPreOutput->GetCellData()->AddArray(originalProcessIds);
      originalProcessIds->Delete();
    }
  }

  vtkTable* tablePreOutput = vtkTable::SafeDownCast(preOutput);
  if (this->GenerateProcessIds && tablePreOutput)
  {
    // Note that preOutput is never the input directly (it is shallow copied at
    // the least, hence we can add arrays to it.
    if (tablePreOutput->GetNumberOfRows() > 0 &&
      !tablePreOutput->GetColumnByName("vtkOriginalProcessIds"))
    {
      vtkIdTypeArray* originalProcessIds = vtkIdTypeArray::New();
      originalProcessIds->SetNumberOfComponents(1);
      originalProcessIds->SetName("vtkOriginalProcessIds");
      originalProcessIds->SetNumberOfTuples(tablePreOutput->GetNumberOfRows());
      originalProcessIds->FillComponent(0, controller->GetLocalProcessId());
      tablePreOutput->AddColumn(originalProcessIds);
      originalProcessIds->Delete();
    }
  }

  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  if (this->PassThrough > numProcs)
  {
    this->PassThrough = -1;
  }

  if (this->ReductionMode != vtkReductionFilter::REDUCE_ALL_TO_ALL)
  {
    if (this->ReductionProcessId < 0)
    {
      vtkWarningMacro("Cannot reduce to process:"
        << this->ReductionProcessId
        << " it is not a valid process id. Please use a value between 0 and " << numProcs - 1
        << " .Reducing to process 0 instead");
      this->ReductionProcessId = 0;
    }
    if (this->ReductionProcessId > numProcs - 1)
    {
      vtkWarningMacro("Cannot reduce to process:" << this->ReductionProcessId
                                                  << " as there is only " << numProcs
                                                  << " processes. Reducing to process 0 instead");
      this->ReductionProcessId = 0;
    }
  }

  std::vector<vtkSmartPointer<vtkDataObject> > data_sets;
  std::vector<vtkSmartPointer<vtkDataObject> > receiveData(numProcs);

  if (vtkSelection* sel = vtkSelection::SafeDownCast(preOutput))
  {
    if (this->ReductionMode == vtkReductionFilter::REDUCE_ALL_TO_ALL)
    {
      for (int i = 0; i < controller->GetNumberOfProcesses(); i++)
      {
        this->GatherSelection(sel, receiveData, i);
      }
    }
    else
    {
      this->GatherSelection(sel, receiveData, this->ReductionProcessId);
    }
  }
  else
  {
    if (this->ReductionMode == vtkReductionFilter::REDUCE_ALL_TO_ALL)
    {
      for (int i = 0; i < controller->GetNumberOfProcesses(); i++)
      {
        controller->Gather(preOutput, receiveData, i);
      }
    }
    else
    {
      controller->Gather(preOutput, receiveData, this->ReductionProcessId);
    }
  }

  // the reduction from all ranks may return nullptr datasets on certain ranks.
  // So, we need to handle receiveData having NULLs.
  assert(static_cast<int>(receiveData.size()) == numProcs);
  if (myId == this->ReductionProcessId ||
    this->ReductionMode == vtkReductionFilter::REDUCE_ALL_TO_ALL)
  {
    if (this->PassThrough >= 0 && receiveData[this->PassThrough] != nullptr)
    {
      data_sets.push_back(receiveData[this->PassThrough]);
    }
    else
    {
      for (int i = 0; i < controller->GetNumberOfProcesses(); ++i)
      {
        if (receiveData[i] != nullptr)
        {
          data_sets.push_back(receiveData[i]);
        }
      }
    }
  }
  else if (myId != this->ReductionProcessId && preOutput &&
    this->ReductionMode == vtkReductionFilter::REDUCE_ALL_TO_ONE)
  {
    data_sets.push_back(preOutput);
  }

  // Now run the PostGatherHelper.
  // If myId==this->ReductionProcessId or this->ReductionMode ==
  // vtkReductionFilter::REDUCE_ALL_TO_ALL
  // data_sets has datasets collected from all satellites otherwise
  // it contains the current process's result.
  if (data_sets.size() > 0)
  {
    this->PostProcess(output, &data_sets[0], static_cast<unsigned int>(data_sets.size()));
  }
}
//----------------------------------------------------------------------------
int vtkReductionFilter::GatherSelection(vtkSelection* sendData,
  std::vector<vtkSmartPointer<vtkDataObject> >& receiveData, int destProcessId)
{
  std::ostringstream sendBufferOstr;
  if (sendData)
  {
    vtkSelectionSerializer::PrintXML(sendBufferOstr, vtkIndent(), 1, sendData);
  }

  std::string sendBufferStr = sendBufferOstr.str();
  vtkNew<vtkCharArray> sendBuffer;
  vtkNew<vtkCharArray> recvBuffer;
  sendBuffer->SetArray(const_cast<char*>(sendBufferStr.c_str()), sendBufferStr.size(), 1);

  vtkNew<vtkIdTypeArray> recvLengths;
  vtkNew<vtkIdTypeArray> offsets;
  if (this->Controller->GatherV(sendBuffer.GetPointer(), recvBuffer.GetPointer(),
        recvLengths.GetPointer(), offsets.GetPointer(), destProcessId))
  {
    if (this->Controller->GetLocalProcessId() == destProcessId)
    {
      for (int i = 0; i < this->Controller->GetNumberOfProcesses(); ++i)
      {
        // offsets has NumberOfProcesses+1 elements
        unsigned int length = (offsets->GetValue(i + 1) - offsets->GetValue(i));
        if (length != 0)
        {
          vtkNew<vtkSelection> sel;
          vtkSelectionSerializer::Parse(
            recvBuffer->GetPointer(offsets->GetValue(i)), length, sel.Get());
          receiveData[i] = sel.Get();
        }
      }
    }
    return 1;
  }

  return 0;
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
