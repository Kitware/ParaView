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

#include "vtkCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTable.h"
#include "vtkUnsignedIntArray.h"

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
int vtkMarkSelectedRows::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
    }

  return this->Superclass::FillInputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkMarkSelectedRows::RequestData(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkSelection* inputSelection = vtkSelection::GetData(inputVector[1], 0);
  vtkTable* input = vtkTable::GetData(inputVector[0], 0);
  vtkTable* output = vtkTable::GetData(outputVector, 0);
  output->ShallowCopy(input);

  vtkCharArray* selected = vtkCharArray::New();
  selected->SetName("__vtkIsSelected__");
  selected->SetNumberOfTuples(output->GetNumberOfRows());
  selected->FillComponent(0, 0);
  output->AddColumn(selected);
  selected->Delete();

  if (!inputSelection)
    {
    return 1;
    }

  // Locate the selection node that may be applicable to the input.
  // This is determined by using the existence of special array in the input
  // such as vtkOriginalIndices, vtkOriginalProcessIds, vtkCompositeIndexArray
  // etc.
  vtkUnsignedIntArray* compIndexArray = vtkUnsignedIntArray::SafeDownCast(
    input->GetColumnByName("vtkCompositeIndexArray"));
  vtkIdTypeArray* originalIdsArray = vtkIdTypeArray::SafeDownCast(
    input->GetColumnByName("vtkOriginalIndices"));
  for (unsigned int ii=0; ii < inputSelection->GetNumberOfNodes(); ii++)
    {
    vtkSelectionNode* node = inputSelection->GetNode(ii);
    if ((node->GetFieldType() == vtkSelectionNode::POINT &&
        this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS) ||
      (node->GetFieldType() == vtkSelectionNode::CELL &&
       this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS) ||
      (node->GetFieldType() == vtkSelectionNode::ROW &&
       this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_ROWS))
      {
      // field association matches -- keep going.
      }
    else
      {
      continue;
      }

    vtkInformation* properties = node->GetProperties();
    bool has_cid = properties->Has(vtkSelectionNode::COMPOSITE_INDEX()) != 0;
    unsigned int cid = has_cid? properties->Get(vtkSelectionNode::COMPOSITE_INDEX()) : 0;
    bool has_amr = properties->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
      properties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL());
    int hindex = has_amr?
      properties->Get(vtkSelectionNode::HIERARCHICAL_INDEX()) : 0;
    int hlevel = has_amr?
      properties->Get(vtkSelectionNode::HIERARCHICAL_LEVEL()) : 0;
    for (vtkIdType cc=0; cc < output->GetNumberOfRows(); cc++)
      {
      if (compIndexArray)
        {
        if (compIndexArray->GetNumberOfComponents() == 2 && has_amr)
          {
          int level = -1, dataset = -1;
          level = compIndexArray->GetValue(2*cc);
          dataset = compIndexArray->GetValue(2*cc + 1);
          if (level != hlevel || dataset != hindex)
            {
            continue;
            }
          }
        else if (compIndexArray->GetNumberOfComponents() == 1 && has_cid)
          {
          if (compIndexArray->GetValue(cc) != cid)
            {
            continue;
            }
          }
        }

      vtkIdType originalId = originalIdsArray->GetValue(cc);
      if (node->GetSelectionList()->LookupValue(vtkVariant(originalId)) != -1)
        {
        selected->SetValue(cc, 1);
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkMarkSelectedRows::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
