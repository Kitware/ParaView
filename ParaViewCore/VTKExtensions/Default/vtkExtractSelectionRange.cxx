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
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <vector>
#include <set>
#include <map>

vtkStandardNewMacro(vtkExtractSelectionRange);
//----------------------------------------------------------------------------
vtkExtractSelectionRange::vtkExtractSelectionRange()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  this->Component = 0;
  this->ArrayName = NULL;
  this->FieldType = vtkSelectionNode::CELL;
}

//----------------------------------------------------------------------------
vtkExtractSelectionRange::~vtkExtractSelectionRange()
{
  this->SetArrayName(0);
}

//----------------------------------------------------------------------------
void vtkExtractSelectionRange::SetArrayName(const char* arrayName)
{
  if (this->ArrayName == NULL && arrayName == NULL)
    {
    return;
    }

  if (this->ArrayName && arrayName && strcmp(this->ArrayName, arrayName) == 0)
    {
    return;
    }

  delete [] this->ArrayName;
  this->ArrayName = 0;
  if (arrayName)
    {
    size_t n = strlen(arrayName) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (arrayName);
    this->ArrayName = cp1;
    do
      {
      *cp1++ = *cp2++;
      }
    while (--n);
    }

  this->Modified();
}

//----------------------------------------------------------------------------
int vtkExtractSelectionRange::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
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
vtkTable* vtkExtractSelectionRange::GetOutput()
{
  return vtkTable::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkExtractSelectionRange::RequestInformation(
  vtkInformation* info,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectionRange::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkSelection* selection = vtkSelection::GetData(inputVector[1]);

  this->Range[0] = VTK_DOUBLE_MAX;
  this->Range[1] = VTK_DOUBLE_MIN;

  unsigned int numNodes = selection->GetNumberOfNodes();

  for (unsigned int cc=0; cc < numNodes; cc++)
    {
    vtkSelectionNode* node = selection->GetNode(cc);

    if (!node)
      {
      vtkGenericWarningMacro( << "Selection node null" );
      continue;
      }

    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
        node->GetSelectionList());
    if (!idList)
      {
      vtkGenericWarningMacro( << "Selection node list null" );
      continue;
      }

    int contentType = node->GetContentType();
    if (contentType != vtkSelectionNode::INDICES)
      {
      vtkGenericWarningMacro("Unhandled ContentType: " << contentType);
      continue;
      }

    vtkInformation* selProperties = node->GetProperties();
    vtkIdType composite_index = 0;
    if (selProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
      {
      composite_index = selProperties->Get(vtkSelectionNode::COMPOSITE_INDEX());
      }

    vtkGenericWarningMacro( << "composite_index " << composite_index );

    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(input);
    if (!dataSet)
      {
      vtkCompositeDataSet *compDataSet =
        vtkCompositeDataSet::SafeDownCast(input);
      if (compDataSet)
        {
        vtkCompositeDataIterator *compDataIt = compDataSet->NewIterator();
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
          vtkGenericWarningMacro( << "dataSet no found" );
          continue;
          }

        dataSet = vtkDataSet::SafeDownCast(compDataIt->GetCurrentDataObject());
        compDataIt->Delete();
        }
      }

    if (!dataSet)
      {
      vtkGenericWarningMacro( << "dataSet no found " << input->GetClassName() );
      continue;
      }

    vtkGenericWarningMacro( << "dataset nb points " << dataSet->GetNumberOfPoints());

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
      vtkGenericWarningMacro( << "Field type not supported" );
      continue;
      }

    if (!dataArray)
      {
      vtkGenericWarningMacro( << "No data array found" );
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
      vtkGenericWarningMacro( << "max id > array max id" );
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

    vtkGenericWarningMacro( << "Current range " << this->Range[0] << " " << this->Range[1] );

    }

  vtkGenericWarningMacro( << "Final range " << this->Range[0] << " " << this->Range[1] );

  vtkNew<vtkDoubleArray> rangeArray;
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
}
