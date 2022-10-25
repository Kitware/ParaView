/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectArraysExtractBlocks.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectArraysExtractBlocks.h"

#include "vtkCompositeDataSet.h"
#include "vtkExtractBlockUsingDataAssembly.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPassSelectedArrays.h"

#include <algorithm>

class vtkSelectArraysExtractBlocks::vtkInternals
{
public:
  vtkNew<vtkPassSelectedArrays> PassSelectedArrays;
  vtkNew<vtkExtractBlockUsingDataAssembly> ExtractBlocks;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSelectArraysExtractBlocks);

//----------------------------------------------------------------------------
vtkSelectArraysExtractBlocks::vtkSelectArraysExtractBlocks()
  : Internals(new vtkSelectArraysExtractBlocks::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSelectArraysExtractBlocks::~vtkSelectArraysExtractBlocks() = default;

//----------------------------------------------------------------------------
void vtkSelectArraysExtractBlocks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkSelectArraysExtractBlocks::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkSelectArraysExtractBlocks::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkCompositeDataSet* input = vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (!input)
  {
    vtkErrorMacro("Unable to retrieve the input !");
    return 0;
  }

  vtkCompositeDataSet* output = vtkCompositeDataSet::GetData(outputVector, 0);
  if (!output)
  {
    vtkErrorMacro("Unable to retrieve the output !");
    return 0;
  }

  // Construct the internal pipeline
  std::vector<vtkAlgorithm*> pipeline;
  if (this->PassArraysEnabled)
  {
    pipeline.push_back(this->Internals->PassSelectedArrays);
  }
  if (this->ExtractBlocksEnabled)
  {
    pipeline.push_back(this->Internals->ExtractBlocks);
  }
  if (pipeline.size() == 2)
  {
    pipeline.back()->SetInputConnection(pipeline.front()->GetOutputPort());
  }

  // Execute pipeline (if any)
  if (!pipeline.empty())
  {
    pipeline.front()->SetInputDataObject(input);
    pipeline.back()->Update();
    output->ShallowCopy(pipeline.back()->GetOutputDataObject(0));
  }
  else
  {
    output->ShallowCopy(input);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectArraysExtractBlocks::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // If extract blocks is disabled, output type is the same as input type.
  if (!this->ExtractBlocksEnabled)
  {
    auto input = vtkCompositeDataSet::GetData(inputVector[0], 0);
    auto output = vtkCompositeDataSet::GetData(outputVector, 0);
    if (output == nullptr || output->GetDataObjectType() != input->GetDataObjectType())
    {
      output = input->NewInstance();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
    return 1;
  }
  // If extract blocks is enabled, output type is defined by ExtractBlocks. We do that to handle
  // vtkOverlappingAMR properly.
  else
  {
    return this->Internals->ExtractBlocks->ProcessRequest(request, inputVector, outputVector);
  }
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkSelectArraysExtractBlocks::GetPointDataArraySelection()
{
  return this->Internals->PassSelectedArrays->GetPointDataArraySelection();
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkSelectArraysExtractBlocks::GetCellDataArraySelection()
{
  return this->Internals->PassSelectedArrays->GetCellDataArraySelection();
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkSelectArraysExtractBlocks::GetFieldDataArraySelection()
{
  return this->Internals->PassSelectedArrays->GetFieldDataArraySelection();
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkSelectArraysExtractBlocks::GetVertexDataArraySelection()
{
  return this->Internals->PassSelectedArrays->GetVertexDataArraySelection();
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkSelectArraysExtractBlocks::GetEdgeDataArraySelection()
{
  return this->Internals->PassSelectedArrays->GetEdgeDataArraySelection();
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkSelectArraysExtractBlocks::GetRowDataArraySelection()
{
  return this->Internals->PassSelectedArrays->GetRowDataArraySelection();
}

//----------------------------------------------------------------------------
bool vtkSelectArraysExtractBlocks::AddSelector(const char* selector)
{
  if (this->Internals->ExtractBlocks->AddSelector(selector))
  {
    this->Modified();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSelectArraysExtractBlocks::ClearSelectors()
{
  this->Internals->ExtractBlocks->ClearSelectors();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectArraysExtractBlocks::SetAssemblyName(const char* assemblyName)
{
  this->Internals->ExtractBlocks->SetAssemblyName(assemblyName);
  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkSelectArraysExtractBlocks::GetAssemblyName() const
{
  return this->Internals->ExtractBlocks->GetAssemblyName();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSelectArraysExtractBlocks::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->PassArraysEnabled)
  {
    mTime = std::max(mTime, this->Internals->PassSelectedArrays->GetMTime());
  }
  if (this->ExtractBlocksEnabled)
  {
    mTime = std::max(mTime, this->Internals->ExtractBlocks->GetMTime());
  }
  return mTime;
}
