/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRepresentationPreprocessor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVolumeRepresentationPreprocessor.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkExtractBlock.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVolumeRepresentationPreprocessor);

//----------------------------------------------------------------------------
vtkVolumeRepresentationPreprocessor::vtkVolumeRepresentationPreprocessor()
{
  this->DataSetTriangleFilter = vtkDataSetTriangleFilter::New();
  this->ExtractBlockFilter = vtkExtractBlock::New();
  this->ExtractBlockFilter->SetPruneOutput(1);
  this->ExtractedBlockIndex = VTK_UNSIGNED_INT_MAX;
  this->SetExtractedBlockIndex(0);
  this->SetTetrahedraOnly(0);
}

//----------------------------------------------------------------------------
vtkVolumeRepresentationPreprocessor::~vtkVolumeRepresentationPreprocessor()
{
  this->DataSetTriangleFilter->Delete();
  this->ExtractBlockFilter->Delete();
}

//----------------------------------------------------------------------------
int vtkVolumeRepresentationPreprocessor::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject* input = vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // input to the triangle filter is a vtkDataSet
  vtkDataSet* triangleFilterInput = 0;

  if (input->IsA("vtkMultiBlockDataSet"))
  {
    // extract a dataset from the multiblock data.
    triangleFilterInput = this->MultiBlockToDataSet(vtkMultiBlockDataSet::SafeDownCast(input));

    // check for error
    if (!triangleFilterInput)
    {
      vtkErrorMacro("Could not extract a dataset from multiblock input.");
      return 0;
    }
  }
  else
  {
    // try to down cast input DataOject to DataSet
    triangleFilterInput = vtkDataSet::SafeDownCast(input);

    // check for error
    if (!triangleFilterInput)
    {
      vtkErrorMacro("Could not downcast data object input to dataset.");
      return 0;
    }
  }

  // push dataset through the triangle filter
  vtkUnstructuredGrid* triangleFilterOutput = this->TriangulateDataSet(triangleFilterInput);

  // copy to output
  output->ShallowCopy(triangleFilterOutput);
  output->RemoveGhostCells();
  return 1;
}

//----------------------------------------------------------------------------
/// Pushes input dataset through a vtkDataSetTriangleFilter and returns
/// the output.
vtkUnstructuredGrid* vtkVolumeRepresentationPreprocessor::TriangulateDataSet(vtkDataSet* input)
{
  // shallow copy the input and connect to triangle filter
  vtkDataSet* clone = input->NewInstance();
  clone->ShallowCopy(input);
  this->DataSetTriangleFilter->SetInputData(clone);
  clone->Delete();

  // update the triangulate filter
  this->DataSetTriangleFilter->Update();
  this->DataSetTriangleFilter->SetInputData(0);

  // return output of triangle filter
  return this->DataSetTriangleFilter->GetOutput();
}

//----------------------------------------------------------------------------
/// Extracts a single block from a multiblock dataset and attempts to downcast
/// the extracted block to dataset before returning.
vtkDataSet* vtkVolumeRepresentationPreprocessor::MultiBlockToDataSet(vtkMultiBlockDataSet* input)
{
  // shallow copy the input and connect to extract block filter
  vtkMultiBlockDataSet* clone = input->NewInstance();
  clone->ShallowCopy(input);
  this->ExtractBlockFilter->SetInputData(clone);
  clone->Delete();

  // update extract block filter
  this->ExtractBlockFilter->Update();
  this->ExtractBlockFilter->SetInputData(0);

  // output is a vtkMultiBlockDataSet with a single leaf node.
  vtkMultiBlockDataSet* output = this->ExtractBlockFilter->GetOutput();

  // use an iterator to get the dataset at the leaf node.
  vtkDataSet* dataset = 0;
  vtkCompositeDataIterator* iter = output->NewIterator();
  iter->GoToFirstItem();
  dataset = vtkDataSet::SafeDownCast(output->GetDataSet(iter));
  iter->Delete();

  // return the dataset
  return dataset;
}

//----------------------------------------------------------------------------
/// Choose which block to volume render.  Ignored if input is not multiblock.
void vtkVolumeRepresentationPreprocessor::SetExtractedBlockIndex(unsigned int index)
{
  if (this->ExtractedBlockIndex != index)
  {
    this->ExtractedBlockIndex = index;
    this->ExtractBlockFilter->RemoveAllIndices();
    this->ExtractBlockFilter->AddIndex(this->ExtractedBlockIndex);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentationPreprocessor::SetTetrahedraOnly(int value)
{
  this->TetrahedraOnly = value;
  this->DataSetTriangleFilter->SetTetrahedraOnly(this->TetrahedraOnly);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkVolumeRepresentationPreprocessor::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentationPreprocessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ExtractedBlockIndex: " << this->ExtractedBlockIndex << "\n";
  os << indent << "TetrahedraOnly: " << (this->TetrahedraOnly ? "On" : "Off") << "\n";
}

//----------------------------------------------------------------------------
