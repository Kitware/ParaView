/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkSelectedRows.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"

#include <assert.h>

vtkStandardNewMacro(vtkMarkSelectedRows);
//----------------------------------------------------------------------------
vtkMarkSelectedRows::vtkMarkSelectedRows()
{
  this->SetNumberOfInputPorts(2);
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
}

//----------------------------------------------------------------------------
vtkMarkSelectedRows::~vtkMarkSelectedRows()
{
}

//----------------------------------------------------------------------------
int vtkMarkSelectedRows::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* newOutput = nullptr;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (auto inputCD = vtkDataObject::GetData(inInfo))
  {
    auto output = vtkDataObjectTree::GetData(outInfo);
    if (output != nullptr && output->GetDataObjectType() == inputCD->GetDataObjectType())
    {
      return 1;
    }
    newOutput = inputCD->NewInstance();
  }
  else
  {
    if (vtkTable::GetData(outInfo))
    {
      return 1;
    }
    newOutput = vtkTable::New();
  }
  if (newOutput)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    newOutput->Delete();
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkMarkSelectedRows::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkMarkSelectedRows::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* extractedDO = vtkDataObject::GetData(inputVector[1], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  if (extractedDO == NULL)
  {
    // We don't have any information about what items were selected, we cannot
    // annotate the input dataset any further. Just return the input as is.
    outputDO->ShallowCopy(inputDO);
    return 1;
  }

  if (!inputDO->IsA(extractedDO->GetClassName()))
  {
    vtkWarningMacro("Input data types mismatch.");
    outputDO->ShallowCopy(inputDO);
    return 1;
  }

  vtkTable* inputTable = vtkTable::SafeDownCast(inputDO);
  vtkTable* extractedTable = vtkTable::SafeDownCast(extractedDO);
  vtkTable* outputTable = vtkTable::SafeDownCast(outputDO);
  if (inputTable)
  {
    assert(extractedTable != NULL && outputTable != NULL);
    return this->RequestDataInternal(inputTable, extractedTable, outputTable);
  }

  vtkDataObjectTree* inputCD = vtkDataObjectTree::SafeDownCast(inputDO);
  vtkDataObjectTree* extractedCD = vtkDataObjectTree::SafeDownCast(extractedDO);
  vtkDataObjectTree* outputCD = vtkDataObjectTree::SafeDownCast(outputDO);
  if (inputCD)
  {
    assert(extractedCD != NULL && outputCD != NULL);

    // it's possible that extractedCD is an empty multiblock indicating no
    // selected data was extracted from the input. Determine that.
    bool nothing_extracted =
      extractedCD == NULL || extractedCD->GetNumberOfElements(vtkDataObject::ROW) == 0;

    outputCD->CopyStructure(inputCD);
    vtkCompositeDataIterator* iter = inputCD->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkTable* curInput = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
      if (curInput)
      {
        vtkTable* curOutput = vtkTable::New();
        outputCD->SetDataSet(iter, curOutput);
        curOutput->FastDelete();
        vtkTable* curExtractedTable =
          nothing_extracted ? NULL : vtkTable::SafeDownCast(extractedCD->GetDataSet(iter));

        this->RequestDataInternal(curInput, curExtractedTable, curOutput);
      }
    }
    iter->Delete();
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkMarkSelectedRows::RequestDataInternal(
  vtkTable* input, vtkTable* extractedInput, vtkTable* output)
{
  output->ShallowCopy(input);

  vtkCharArray* selected = vtkCharArray::New();
  selected->SetName("__vtkIsSelected__");
  selected->SetNumberOfTuples(output->GetNumberOfRows());
  selected->FillComponent(0, 0);
  output->AddColumn(selected);
  selected->Delete();

  if (!extractedInput)
  {
    return 1;
  }

  vtkIdTypeArray* selectedIdsArray = 0;

  switch (this->FieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      selectedIdsArray =
        vtkIdTypeArray::SafeDownCast(extractedInput->GetColumnByName("vtkOriginalPointIds"));
      break;

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      selectedIdsArray =
        vtkIdTypeArray::SafeDownCast(extractedInput->GetColumnByName("vtkOriginalCellIds"));
      break;

    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      selectedIdsArray =
        vtkIdTypeArray::SafeDownCast(extractedInput->GetColumnByName("vtkOriginalRowIds"));
      break;

    default:
      break;
  }

  if (!selectedIdsArray)
  {
    return 1;
  }

  // Locate the selection node that may be applicable to the input.
  vtkIdTypeArray* originalIdsArray =
    vtkIdTypeArray::SafeDownCast(input->GetColumnByName("vtkOriginalIndices"));

  for (vtkIdType i = 0; i < output->GetNumberOfRows(); i++)
  {
    vtkIdType originalId = originalIdsArray ? originalIdsArray->GetValue(i) : i;
    if (selectedIdsArray->LookupValue(originalId) != -1)
    {
      selected->SetValue(i, 1);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkMarkSelectedRows::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
