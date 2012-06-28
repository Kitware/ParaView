/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractHistogram.h"

#include "vtkAttributeDataReductionFilter.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkReductionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkPVConfig.h"

#ifdef PARAVIEW_USE_MPI
#include "vtkMPICommunicator.h"
#endif

#include <string>
#include <vtksys/RegularExpression.hxx>

vtkStandardNewMacro(vtkPExtractHistogram);
vtkCxxSetObjectMacro(vtkPExtractHistogram, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPExtractHistogram::vtkPExtractHistogram()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkPExtractHistogram::~vtkPExtractHistogram()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
bool vtkPExtractHistogram::InitializeBinExtents(
  vtkInformationVector** inputVector,
  vtkDoubleArray* bin_extents,
  double& min, double& max)
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1 ||
    this->UseCustomBinRanges)
    {
    // Nothing extra to do for single process.
    return this->Superclass::InitializeBinExtents(inputVector, 
      bin_extents, min, max);
    }
#ifndef PARAVIEW_USE_MPI
  return this->Superclass::InitializeBinExtents( inputVector, bin_extents, min, max );

#else
  int num_processes = this->Controller->GetNumberOfProcesses();
  vtkMPICommunicator* comm = vtkMPICommunicator::SafeDownCast(
    this->Controller->GetCommunicator());
  if (!comm)
    {
    vtkErrorMacro("vtkMPICommunicator is needed.");
    return false;
    }
  int cc;

  // We need to obtain the data array ranges from all processes.
  double data[3] = {0.0, 0.0, 0.0}; // 0--valid, (1,2) -- range.
  double *gathered_data= new double[3*num_processes];
  std::string array_name = "";

  //get local range and array name. if I don't have that array locally,
  //then my contribution will be marked as invalid and ignored
  bool retVal = false;
  if (this->Superclass::InitializeBinExtents(inputVector, bin_extents,
        min, max))
    {
    data[0] = 1.0;
    data[1] = min;
    data[2] = max;
    array_name = bin_extents->GetName();
    retVal = true;
    }

  // If the requested component is out-of-range for the input, we return an
  // empty dataset
  if (!comm->AllGather(data, gathered_data, 3))
    {
    vtkErrorMacro("Gather failed!");
    delete[] gathered_data;
    return 0;
    }

  // Gather array name (for when some process doesn't have it).
  vtkIdType *arrayname_lengths = new vtkIdType[num_processes];
  vtkIdType my_length = array_name.size() + 1; //gather null terminated strings.

  // Collect name lengths.
  comm->AllGather(&my_length, arrayname_lengths, 1);

  vtkIdType total_size = 0;
  vtkIdType *offsets = new vtkIdType[num_processes];
  for (cc=0; cc < num_processes; cc++)
    {
    offsets[cc] = total_size;
    total_size += arrayname_lengths[cc];
    }

  // Allocate char array to gather array names from all processes.
  char* gathered_array_names = new char[total_size];
  comm->AllGatherV(
    const_cast<char*>(array_name.c_str()), gathered_array_names,
    my_length, arrayname_lengths, offsets);

  // Locate first non-empty array name, that's our arrayname.
  for (cc=0; cc < num_processes; cc++)
    {
    if (arrayname_lengths[cc] > 1)
      {
      array_name = gathered_array_names + offsets[cc];
      break;
      }
    }

  delete[] gathered_array_names;
  delete[] offsets;
  delete[] arrayname_lengths;

  bin_extents->SetName(array_name.c_str());

  // Now compute the total range from all the local ranges.
  double range[2] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  for (cc=0; cc < num_processes; cc++)
    {
    if (gathered_data[3*cc] != 1.0)
      {
      // the process does not have valid data array.
      continue;
      }
    if (range[0] > gathered_data[3*cc+1])
      {
      range[0] = gathered_data[3*cc+1];
      }
    if (range[1] < gathered_data[3*cc+2])
      {
      range[1] = gathered_data[3*cc+2];
      }
    }
  delete[] gathered_data;

  if (range[0] == VTK_DOUBLE_MAX && range[1] == VTK_DOUBLE_MIN)
    {
    // No process reported a valid range:
    range[0] = 0;
    range[1] = 1;
    }

  if (range[0] == range[1])
    {
    // Give it some width.
    range[1] = range[0]+1;
    }

  min = range[0];
  max = range[1];
  this->FillBinExtents(bin_extents, min, max);
  return retVal;
#endif
}

//-----------------------------------------------------------------------------
int vtkPExtractHistogram::RequestData(vtkInformation *request,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // All processes generate the histogram.
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }

  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
    {
    // Nothing to do for single process.
    return 1;
    }

  // Now we need to collect and reduce data from all nodes on the root.
  vtkSmartPointer<vtkReductionFilter> reduceFilter = 
    vtkSmartPointer<vtkReductionFilter>::New();
  reduceFilter->SetController(this->Controller);

  bool isRoot = (this->Controller->GetLocalProcessId() ==0);
  if (isRoot)
    {
    // PostGatherHelper needs to be set only on the root node.
    vtkSmartPointer<vtkAttributeDataReductionFilter> rf = 
      vtkSmartPointer<vtkAttributeDataReductionFilter>::New();
    rf->SetAttributeType(vtkAttributeDataReductionFilter::ROW_DATA);
    rf->SetReductionType(vtkAttributeDataReductionFilter::ADD);
    reduceFilter->SetPostGatherHelper(rf);
    }

  vtkTable* output = vtkTable::GetData(outputVector, 0);
  vtkSmartPointer<vtkTable> copy = vtkSmartPointer<vtkTable>::New();
  copy->ShallowCopy(output);
  reduceFilter->SetInputData(copy);
  reduceFilter->Update();
  if (isRoot)
    {
    // We save the old bin_extents and then revert to be restored later since
    // the reduction reduces the bin_extents as well.
    vtkSmartPointer<vtkDataArray> oldExtents = 
      output->GetRowData()->GetArray((int)0);
    output->ShallowCopy(reduceFilter->GetOutput());
    output->GetRowData()->GetArray((int)0)->DeepCopy(oldExtents);
    if (this->CalculateAverages)
      {
      vtkDataArray* bin_values = 
        output->GetRowData()->GetArray("bin_values");
      vtksys::RegularExpression reg_ex("^(.*)_average$");
      int numArrays = output->GetRowData()->GetNumberOfArrays();
      for (int i=0; i<numArrays; i++)
        {
        vtkDataArray* array = output->GetRowData()->GetArray(i);
        if (array && reg_ex.find(array->GetName()))
          {
          int numComps = array->GetNumberOfComponents();
          std::string name = reg_ex.match(1) + "_total";
          vtkDataArray* tarray = output->GetRowData()->GetArray(name.c_str());
          for (vtkIdType idx=0; idx<this->BinCount; idx++)
            {
            for (int j=0; j<numComps; j++)
              {
              array->SetComponent(idx, j,
                tarray->GetComponent(idx, j)/bin_values->GetTuple1(idx));
              }
            }
          }
        }
      }
    }
  else
    {
    output->Initialize();
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPExtractHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
