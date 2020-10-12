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
#include "vtkMergeBlocks.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVolumeRepresentationPreprocessor);

//----------------------------------------------------------------------------
vtkVolumeRepresentationPreprocessor::vtkVolumeRepresentationPreprocessor()
  : TetrahedraOnly(0)
  , ExtractedBlockIndex(0)
{
}

//----------------------------------------------------------------------------
vtkVolumeRepresentationPreprocessor::~vtkVolumeRepresentationPreprocessor()
{
}

//----------------------------------------------------------------------------
int vtkVolumeRepresentationPreprocessor::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkSmartPointer<vtkDataObject> input =
    vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (auto cd = vtkCompositeDataSet::SafeDownCast(input))
  {
    // extract a dataset from the multiblock data.
    input = this->ExtractDataSet(cd);

    // check for error
    if (!input)
    {
      vtkErrorMacro("Could not extract a dataset from multiblock input.");
      return 0;
    }
  }
  else
  {
    // try to down cast input DataOject to DataSet
    input = vtkDataSet::SafeDownCast(input);

    // check for error
    if (!input)
    {
      vtkErrorMacro("Could not downcast data object input to dataset.");
      return 0;
    }
  }

  // push dataset through the triangle filter
  input = this->Tetrahedralize(input);

  // copy to output
  output->ShallowCopy(input);
  output->RemoveGhostCells();
  return 1;
}

//----------------------------------------------------------------------------
/// Pushes input dataset through a vtkDataSetTriangleFilter and returns
/// the output.
vtkSmartPointer<vtkUnstructuredGrid> vtkVolumeRepresentationPreprocessor::Tetrahedralize(
  vtkDataObject* input)
{
  vtkNew<vtkDataSetTriangleFilter> tetrahedralizer;
  tetrahedralizer->SetInputData(input);
  tetrahedralizer->Update();
  tetrahedralizer->SetTetrahedraOnly(this->TetrahedraOnly);
  return tetrahedralizer->GetOutput();
}

//----------------------------------------------------------------------------
/// Extracts a single block from a multiblock dataset and attempts to downcast
/// the extracted block to dataset before returning.
vtkSmartPointer<vtkDataSet> vtkVolumeRepresentationPreprocessor::ExtractDataSet(
  vtkCompositeDataSet* input)
{
  vtkNew<vtkMergeBlocks> merger;
  if (this->ExtractedBlockIndex != 0)
  {
    vtkNew<vtkExtractBlock> extractor;
    extractor->AddIndex(this->ExtractedBlockIndex);
    extractor->SetInputData(input);
    merger->SetInputConnection(extractor->GetOutputPort());
  }
  else
  {
    merger->SetInputData(input);
  }

  // Once we fix the volume mapper to support composite datasets
  // better we should remove using vtkMergeBlocks.
  // ref: paraview/paraview#19955.
  merger->MergePointsOff();
  merger->Update();
  return vtkDataSet::SafeDownCast(merger->GetOutputDataObject(0));
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
