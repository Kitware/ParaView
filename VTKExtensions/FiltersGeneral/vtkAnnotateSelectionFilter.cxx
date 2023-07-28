// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnnotateSelectionFilter.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStringArray.h"

vtkStandardNewMacro(vtkAnnotateSelectionFilter);

//----------------------------------------------------------------------------
vtkAnnotateSelectionFilter::vtkAnnotateSelectionFilter()
{
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
void vtkAnnotateSelectionFilter::ResetLabels()
{
  this->Labels.resize(0);
}

//------------------------------------------------------------------------------
void vtkAnnotateSelectionFilter::SetLabels(int index, const char* label)
{
  if (index < 0)
  {
    vtkErrorMacro(<< "Bad index: " << index);
    return;
  }

  if ((static_cast<size_t>(index) >= this->Labels.size()))
  {
    this->Labels.resize(index + 1);
  }

  this->Labels[index] = label;
}

//----------------------------------------------------------------------------
int vtkAnnotateSelectionFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAnnotateSelectionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSelection* selection = vtkSelection::GetData(inputVector[1], 0);
  if (!selection)
  {
    vtkWarningMacro(
      << "No input selection specified, select a selection or click on 'copy active selection'.");
    return 1;
  }

  int fieldType = selection->GetNode(0)->GetFieldType();
  if (fieldType != vtkSelectionNode::SelectionField::POINT &&
    fieldType != vtkSelectionNode::SelectionField::CELL)
  {
    std::string fieldTypeName = selection->GetNode(0)->GetFieldTypeAsString(fieldType);
    vtkWarningMacro(<< "Unsupported type of selection : " << fieldTypeName
                    << ". Only POINT or CELL selection are supported.");
    return 0;
  }

  auto input = vtkDataSet::GetData(inputVector[0], 0);

  const vtkIdType numberOfElements = fieldType == vtkSelectionNode::SelectionField::CELL
    ? input->GetNumberOfCells()
    : input->GetNumberOfPoints();

  const std::size_t numberOfLabel = this->Labels.size();
  if (numberOfLabel != selection->GetNumberOfNodes())
  {
    vtkWarningMacro(<< "The number of selection is different from the number of label.");
    return 1;
  }

  vtkNew<vtkStringArray> labelArray;
  labelArray->SetName("Annotate Selection");
  labelArray->SetNumberOfComponents(1);
  labelArray->SetNumberOfTuples(numberOfElements);
  for (unsigned int i = 0; i < numberOfElements; i++)
  {
    labelArray->SetValue(i, this->DefaultLabel);
  }

  for (unsigned int i = 0; i < selection->GetNumberOfNodes(); i++)
  {
    vtkSelectionNode* node = selection->GetNode(i);
    auto* selData = node->GetSelectionData();
    // id array in the selection data is always the first one
    auto* idArray = vtkIdTypeArray::SafeDownCast(selData->GetArray(0));
    if (!idArray)
    {
      continue;
    }

    auto range = vtk::DataArrayValueRange(idArray);
    for (auto value : range)
    {
      std::string label = this->Labels[i];
      if (label.empty())
      {
        label = this->DefaultLabel;
      }
      labelArray->SetValue(value, label);
    }
  }

  auto output = vtkDataSet::GetData(outputVector, 0);
  output->ShallowCopy(input);
  if (fieldType == vtkSelectionNode::SelectionField::CELL)
  {
    output->GetCellData()->AddArray(labelArray);
  }
  else if (fieldType == vtkSelectionNode::SelectionField::POINT)
  {
    output->GetPointData()->AddArray(labelArray);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnnotateSelectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
