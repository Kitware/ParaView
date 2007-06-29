/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractHistogram.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIOStream.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkIntArray.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"

vtkStandardNewMacro(vtkExtractHistogram);
vtkCxxRevisionMacro(vtkExtractHistogram, "1.18");
//-----------------------------------------------------------------------------
vtkExtractHistogram::vtkExtractHistogram() :
  Component(0),
  BinCount(10)
{
  this->SetInputArrayToProcess(
    0,
    0,
    0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    vtkDataSetAttributes::SCALARS);
}

//-----------------------------------------------------------------------------
vtkExtractHistogram::~vtkExtractHistogram()
{
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Component: " << this->Component << "\n";
  os << indent << "BinCount: " << this->BinCount << "\n";
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::FillInputPortInformation (int port, 
                                                   vtkInformation *info)
{
  this->Superclass::FillInputPortInformation(port, info);
  
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractHistogram::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Extents are {0, no. of bins, 0, 0, 0, 0};
  int extent[6] = {0,0,0,0,0,0};
  extent[1] = this->BinCount;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  // Setup ExtentTranslator so that all downstream piece requests are
  // converted to whole extent update requests, as need by the histogram filter.
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (strcmp(
      sddp->GetExtentTranslator(outInfo)->GetClassName(), 
      "vtkOnePieceExtentTranslator") != 0)
    {
    vtkExtentTranslator* et = vtkOnePieceExtentTranslator::New();
    sddp->SetExtentTranslator(outInfo, et);
    et->Delete();
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // This filter changes the ExtentTranslator on the output
  // to always update whole extent on this filter, irrespective of 
  // what piece the downstream filter is requesting. Hence, we need to
  // propagate the actual extents upstream. If upstream is structured
  // data we need to use the ExtentTranslator of the input, otherwise
  // we just set the piece information. All this is taken care of
  // by SetUpdateExtent().

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (outInfo->Has(sddp->UPDATE_NUMBER_OF_PIECES()) && 
    outInfo->Has(sddp->UPDATE_PIECE_NUMBER()) &&
    outInfo->Has(sddp->UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    int piece = outInfo->Get(sddp->UPDATE_PIECE_NUMBER());
    int numPieces = outInfo->Get(sddp->UPDATE_NUMBER_OF_PIECES());
    int ghostLevel = outInfo->Get(sddp->UPDATE_NUMBER_OF_GHOST_LEVELS());

    sddp->SetUpdateExtent(inInfo, piece, numPieces, ghostLevel);
    }
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkExtractHistogram::InitializeBinExtents(
  vtkInformationVector** inputVector,
  vtkDoubleArray* bin_extents)
{
  double range[2];
  range[0] = VTK_DOUBLE_MAX;
  range[1] = -VTK_DOUBLE_MAX;

  //obtain a pointer to the name of the vtkDataArray to bin up 
  //and find the range of the data values within it

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet *cdin = vtkCompositeDataSet::SafeDownCast(input);
  if (cdin)
    {
    //for composite datasets, visit each leaf data set and compute the total
    //range
    vtkCompositeDataIterator *cdit = cdin->NewIterator();
    cdit->InitTraversal();
    bool foundone = false;
    while(!cdit->IsDoneWithTraversal())
      {
      vtkDataObject *dObj = cdit->GetCurrentDataObject();      
      vtkDataArray* data_array = this->GetInputArrayToProcess(0, dObj);
      if (
        data_array &&
        !(this->Component < 0 && 
          this->Component >= data_array->GetNumberOfComponents())
        )
        {
        if (!foundone)
          {
          bin_extents->SetName(data_array->GetName());
          foundone = true;
          }
        double tRange[2];
        data_array->GetRange(tRange, this->Component);
        if (tRange[0] < range[0])
          {
          range[0] = tRange[0];
          }
        if (tRange[1] > range[1])
          {
          range[1] = tRange[1];
          }
        }
      cdit->GoToNextItem();
      }
    cdit->Delete();

    if (!foundone)
      {
      vtkErrorMacro("Failed to locate array to process in composite input.");
      return false;
      }
    }
  else
    {
    vtkDataArray* data_array = this->GetInputArrayToProcess(0, inputVector);
    if (!data_array)
      {
      vtkErrorMacro("Failed to locate array to process.");
      return false;
      }
    
    // If the requested component is out-of-range for the input, we return an
    // empty dataset
    if(this->Component < 0 && 
       this->Component >= data_array->GetNumberOfComponents())
      {
      vtkWarningMacro("Requested component " 
                      <<  this->Component << " is not available."); 
      return true;
      }

    data_array->GetRange(range, this->Component);

    bin_extents->SetName(data_array->GetName());
    }
  
  // Calculate the extents of each bin, based on the range of values in the
  // input ...  
  if (range[0] == range[1])
    {
    // Give it some width.
    range[1] = range[0]+1;
    }    

  double bin_delta = (range[1] - range[0]) / this->BinCount;
  bin_extents->SetValue(0, range[0]);
  for(int i = 1; i < this->BinCount; ++i)
    {
    bin_extents->SetValue(i, range[0] + (i * bin_delta));
    }
  bin_extents->SetValue(this->BinCount, range[1]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::BinAnArray(vtkDataArray *data_array, 
                                     vtkIntArray *bin_values,
                                     vtkDoubleArray *bin_extents)
{
  // If the requested component is out-of-range for the input,
  // the bin_values will be 0, so no need to do any actual counting.
  if(data_array == NULL || 
    this->Component < 0 || 
    this->Component >= data_array->GetNumberOfComponents())
    {
    return;
    }

  int num_of_tuples = data_array->GetNumberOfTuples();
  for(int i = 0; i != num_of_tuples; ++i)
    {
    if (i%1000 == 0)
      {
      this->UpdateProgress(0.10 + 0.90*i/num_of_tuples);
      }
    const double value = data_array->GetComponent(i, this->Component);
    for(int j = 0; j != this->BinCount; ++j)
      {
      // if we're at the last bin, and this value didn't go in any other bin,
      // it goes in the last one
      if(j == this->BinCount - 1)
        {
        bin_values->SetValue(j, bin_values->GetValue(j) + 1);
        break;
        }
      // check that the value is less than the right hand value of the bin
      else if(value < bin_extents->GetValue(j+1))
        {
        bin_values->SetValue(j, bin_values->GetValue(j) + 1);
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::RequestData(vtkInformation* /*request*/, 
                                     vtkInformationVector** inputVector, 
                                     vtkInformationVector* outputVector)
{
  // Build an empty output grid in advance, so we can bail-out if we
  // encounter any problems
  vtkInformation* const output_info = outputVector->GetInformationObject(0);
  vtkRectilinearGrid* const output_data = vtkRectilinearGrid::SafeDownCast(
    output_info->Get(vtkDataObject::DATA_OBJECT()));
  output_data->Initialize();
  output_data->SetDimensions(this->BinCount+1, 1, 1);

  vtkDoubleArray* const bin_extents = vtkDoubleArray::New();
  bin_extents->SetNumberOfComponents(1);
  bin_extents->SetNumberOfTuples(this->BinCount + 1);
  bin_extents->SetName("bin_extents");
  bin_extents->FillComponent(0, 0.0);
  output_data->SetXCoordinates(bin_extents);
  output_data->GetPointData()->AddArray(bin_extents);

  // Insert values into bins ...
  vtkIntArray* const bin_values = vtkIntArray::New();
  bin_values->SetNumberOfComponents(1);
  bin_values->SetNumberOfTuples(this->BinCount);
  bin_values->SetName("bin_values");
  bin_values->FillComponent(0, 0.0);
  output_data->GetCellData()->AddArray(bin_values);

  vtkDoubleArray* const otherCoords = vtkDoubleArray::New();
  otherCoords->SetNumberOfComponents(1);
  otherCoords->SetNumberOfTuples(1);
  otherCoords->SetTuple1(0, 0.0);
  output_data->SetYCoordinates(otherCoords);
  output_data->SetZCoordinates(otherCoords);
  otherCoords->Delete();

  // Initializes the bin_extents array.
  if (!this->InitializeBinExtents(inputVector, bin_extents))
    {
    bin_values->Delete();
    bin_extents->Delete();
    return 0;
    }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet *cdin = vtkCompositeDataSet::SafeDownCast(input);
  if (cdin)
    {
    //for composite datasets visit each leaf dataset and add in its counts
    vtkCompositeDataIterator *cdit = cdin->NewIterator();
    cdit->InitTraversal();
    while(!cdit->IsDoneWithTraversal())
      {
      vtkDataObject *dObj = cdit->GetCurrentDataObject();      
      vtkDataArray* data_array = this->GetInputArrayToProcess(0, dObj);
      this->BinAnArray(data_array, bin_values, bin_extents);
      cdit->GoToNextItem();
      }
    cdit->Delete();
    }
  else
    {
    vtkDataArray* data_array = this->GetInputArrayToProcess(0, inputVector);
    this->BinAnArray(data_array, bin_values, bin_extents);
    }
  bin_values->Delete();
  bin_extents->Delete();

  return 1;
}
