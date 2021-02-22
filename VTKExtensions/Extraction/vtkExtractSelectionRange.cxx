/*=========================================================================

  Program:   ParaView
  Module:    vtkExtractSelectionRange.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectionRange.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <map>
#include <set>
#include <vector>

vtkStandardNewMacro(vtkExtractSelectionRange);
//----------------------------------------------------------------------------
vtkExtractSelectionRange::vtkExtractSelectionRange()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  this->Component = 0;
  this->ArrayName = nullptr;
  this->FieldType = vtkSelectionNode::CELL;
}

//----------------------------------------------------------------------------
vtkExtractSelectionRange::~vtkExtractSelectionRange()
{
  this->SetArrayName(nullptr);
}

//----------------------------------------------------------------------------
int vtkExtractSelectionRange::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    // Can work with composite datasets.
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  else
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectionRange::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkSelection* selection = vtkSelection::GetData(inputVector[1]);

  this->Range[0] = VTK_DOUBLE_MAX;
  this->Range[1] = VTK_DOUBLE_MIN;

  unsigned int numNodes = selection->GetNumberOfNodes();

  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = selection->GetNode(cc);

    if (!node)
    {
      vtkDebugMacro(<< "Selection node nullptr");
      continue;
    }

    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
    if (!idList)
    {
      vtkDebugMacro(<< "Selection node list nullptr");
      continue;
    }

    int contentType = node->GetContentType();
    if (contentType != vtkSelectionNode::INDICES)
    {
      vtkDebugMacro("Unhandled ContentType: " << contentType);
      continue;
    }

    vtkInformation* selProperties = node->GetProperties();
    unsigned int composite_index = 0;
    if (selProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
    {
      composite_index = (unsigned int)selProperties->Get(vtkSelectionNode::COMPOSITE_INDEX());
    }

    vtkDebugMacro(<< "composite_index " << composite_index);

    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(input);
    if (!dataSet)
    {
      vtkCompositeDataSet* compDataSet = vtkCompositeDataSet::SafeDownCast(input);
      if (compDataSet)
      {
        vtkCompositeDataIterator* compDataIt = compDataSet->NewIterator();
        compDataIt->InitTraversal();
        while (!compDataIt->IsDoneWithTraversal())
        {
          if (composite_index == compDataIt->GetCurrentFlatIndex())
          {
            break;
          }
          compDataIt->GoToNextItem();
        }
        if (compDataIt->IsDoneWithTraversal())
        {
          vtkDebugMacro(<< "dataSet no found");
          continue;
        }

        dataSet = vtkDataSet::SafeDownCast(compDataIt->GetCurrentDataObject());
        compDataIt->Delete();
      }
    }

    if (!dataSet)
    {
      vtkDebugMacro(<< "dataSet no found " << input->GetClassName());
      continue;
    }

    vtkDebugMacro(<< "dataset nb points " << dataSet->GetNumberOfPoints());

    vtkDataArray* dataArray;
    if (this->FieldType == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      dataArray = dataSet->GetPointData()->GetArray(this->ArrayName);
    }
    else if (this->FieldType == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      dataArray = dataSet->GetCellData()->GetArray(this->ArrayName);
    }
    else
    {
      vtkDebugMacro(<< "Field type not supported");
      continue;
    }

    if (!dataArray)
    {
      vtkDebugMacro(<< "No data array found");
      continue;
    }

    if ((this->Component == -1 && dataArray->GetNumberOfComponents() == 1) ||
      (this->Component >= dataArray->GetNumberOfComponents()))
    {
      this->Component = 0;
    }

    vtkIdType idmax = 0;
    for (vtkIdType i = 0; i < idList->GetNumberOfTuples(); i++)
    {
      vtkIdType id = idList->GetTuple1(i);
      if (id > idmax)
      {
        idmax = id;
      }
    }
    if (idmax > dataArray->GetNumberOfTuples())
    {
      vtkDebugMacro(<< "max id > array max id");
      continue;
    }

    double tempRange[] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
    for (vtkIdType i = 0; i < idList->GetNumberOfTuples(); i++)
    {
      vtkIdType id = idList->GetValue(i);
      double* values = dataArray->GetTuple(id);
      double value;
      if (this->Component == -1)
      {
        // todo: support other norms
        value = vtkMath::Norm(values);
      }
      else
      {
        value = values[this->Component];
      }

      if (value < tempRange[0])
      {
        tempRange[0] = value;
      }
      if (value > tempRange[1])
      {
        tempRange[1] = value;
      }
    }

    if (tempRange[0] < this->Range[0])
    {
      this->Range[0] = tempRange[0];
    }
    if (tempRange[1] > this->Range[1])
    {
      this->Range[1] = tempRange[1];
    }

    vtkDebugMacro(<< "Current range " << this->Range[0] << " " << this->Range[1]);
  }

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    double range[2];
    controller->AllReduce(&this->Range[0], &range[0], 1, vtkCommunicator::MIN_OP);
    controller->AllReduce(&this->Range[1], &range[1], 1, vtkCommunicator::MAX_OP);
    this->Range[0] = range[0];
    this->Range[1] = range[1];
  }

  vtkDebugMacro(<< "Final range " << this->Range[0] << " " << this->Range[1]);

  vtkNew<vtkDoubleArray> rangeArray;
  rangeArray->SetName("Range");
  rangeArray->SetNumberOfValues(2);
  rangeArray->SetValue(0, this->Range[0]);
  rangeArray->SetValue(1, this->Range[1]);

  vtkTable* output = vtkTable::GetData(outputVector);
  output->SetNumberOfRows(2);
  output->AddColumn(rangeArray.Get());
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectionRange::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayName: " << (this->ArrayName ? this->ArrayName : "") << endl;
  os << indent << "FieldType: " << this->FieldType << endl;
  os << indent << "Component: " << this->Component << endl;
}
