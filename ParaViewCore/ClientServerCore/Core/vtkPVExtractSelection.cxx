/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractSelection.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
//#include "vtkExecutivePortKey.h"
#include "vtkGraph.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelector.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonSelector.h"
#endif

#include <vector>

class vtkPVExtractSelection::vtkSelectionNodeVector
  : public std::vector<vtkSmartPointer<vtkSelectionNode> >
{
};

vtkStandardNewMacro(vtkPVExtractSelection);

//----------------------------------------------------------------------------
vtkPVExtractSelection::vtkPVExtractSelection()
{
  this->SetNumberOfOutputPorts(3);
}

//----------------------------------------------------------------------------
vtkPVExtractSelection::~vtkPVExtractSelection()
{
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  }
  else // for port 1, 2
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestDataObject(request, inputVector, outputVector))
  {
    return 0;
  }

  // Second and output is selection
  for (int i = 1; i < this->GetNumberOfOutputPorts(); ++i)
  {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkSelection* selOut = vtkSelection::GetData(info);
    if (!selOut || !selOut->IsA("vtkSelection"))
    {
      vtkDataObject* newOutput = vtkSelection::New();
      if (!newOutput)
      {
        vtkErrorMacro("Could not create vtkSelectionOutput");
        return 0;
      }
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      this->GetOutputPortInformation(i)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      newOutput->Delete();
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkSelection* sel = vtkSelection::GetData(inputVector[1], 0);

  vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(inputDO);
  vtkCompositeDataSet* cdOutput = vtkCompositeDataSet::GetData(outputVector, 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  if (!sel)
  {
    return 1;
  }

  if (vtkSelection* output2 = vtkSelection::GetData(outputVector, 2))
  {
    // See vtkPVSingleOutputExtractSelection to know why this check is needed.
    output2->ShallowCopy(sel);
  }

  // Call the superclass's RequestData()
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }

  if (this->GetNumberOfOutputPorts() < 2)
  {
    return 1;
  }

  // make an ids selection for the second output
  // we can do this because all of the extractSelectedX filters produce
  // arrays called "vtkOriginalXIds" that record what input cells produced
  // each output cell, at least as long as PRESERVE_TOPOLOGY is off
  // when we start allowing PreserveTopology, this will have to instead run
  // through the vtkInsidedNess arrays, and for every on entry, record the
  // entries index
  //
  // TODO: The ExtractSelectedGraph filter does not produce the vtkOriginalXIds,
  // so to add support for vtkGraph selection in ParaView the filter will have
  // to be extended. This requires test cases in ParaView to confirm it functions
  // as expected.
  vtkSelection* output = vtkSelection::GetData(outputVector, 1);

  output->Initialize();

  // If input selection content type is vtkSelectionNode::BLOCKS, then we simply
  // need to shallow copy the input as the output.
  if (this->GetContentType(sel) == vtkSelectionNode::BLOCKS)
  {
    output->ShallowCopy(sel);
    return 1;
  }

  vtkSelectionNodeVector oVector;
  if (cdOutput)
  {
    // this is the collection of vtkSelectionNodes that don't have any
    // COMPOSITE_INDEX or HIERARCHICAL_INDEX qualification i.e. they are
    // applicable to all nodes in the composite dataset.
    vtkSelectionNodeVector non_composite_nodes;

    for (unsigned int cc = 0; cc < sel->GetNumberOfNodes(); cc++)
    {
      vtkInformation* properties = sel->GetNode(cc)->GetProperties();
      if (!properties->Has(vtkSelectionNode::COMPOSITE_INDEX()) &&
        !properties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
        !properties->Has(vtkSelectionNode::HIERARCHICAL_INDEX()))
      {
        non_composite_nodes.push_back(sel->GetNode(cc));
      }
    }

    // For composite datasets, the output of this filter is
    // vtkSelectionNode::SELECTIONS instance with vtkSelection instances for some
    // nodes in the composite dataset. COMPOSITE_INDEX() or
    // HIERARCHICAL_LEVEL(), HIERARCHICAL_INDEX() keys are set on each of the
    // vtkSelection instances correctly to help identify the block they came
    // from.
    vtkCompositeDataIterator* iter = cdInput->NewIterator();
    vtkHierarchicalBoxDataIterator* hbIter = vtkHierarchicalBoxDataIterator::SafeDownCast(iter);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkSelectionNode* curSel = this->LocateSelection(iter->GetCurrentFlatIndex(), sel);
      if (!curSel && hbIter)
      {
        curSel = this->LocateSelection(hbIter->GetCurrentLevel(), hbIter->GetCurrentIndex(), sel);
      }

      outputDO = vtkDataObject::SafeDownCast(cdOutput->GetDataSet(iter));

      vtkSelectionNodeVector curOVector;
      if (curSel && outputDO)
      {
        this->RequestDataInternal(curOVector, outputDO, curSel);
      }

      for (vtkSelectionNodeVector::iterator giter = non_composite_nodes.begin();
           giter != non_composite_nodes.end(); ++giter)
      {
        this->RequestDataInternal(curOVector, outputDO, giter->GetPointer());
      }

      for (vtkSelectionNodeVector::iterator viter = curOVector.begin(); viter != curOVector.end();
           ++viter)
      {
        // RequestDataInternal() will not set COMPOSITE_INDEX() for
        // hierarchical datasets.
        viter->GetPointer()->GetProperties()->Set(
          vtkSelectionNode::COMPOSITE_INDEX(), iter->GetCurrentFlatIndex());
        oVector.push_back(viter->GetPointer());
      }
    }
    iter->Delete();
  }
  else if (outputDO) // and not composite dataset.
  {
    unsigned int numNodes = sel->GetNumberOfNodes();
    for (unsigned int i = 0; i < numNodes; i++)
    {
      this->RequestDataInternal(oVector, outputDO, sel->GetNode(i));
    }
  }

  vtkSelectionNodeVector::iterator iter;
  for (iter = oVector.begin(); iter != oVector.end(); ++iter)
  {
    output->AddNode(iter->GetPointer());
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkPVExtractSelection::LocateSelection(
  unsigned int level, unsigned int index, vtkSelection* sel)
{
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node)
    {
      if (node->GetProperties()->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
        node->GetProperties()->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
        static_cast<unsigned int>(
          node->GetProperties()->Get(vtkSelectionNode::HIERARCHICAL_LEVEL())) == level &&
        static_cast<unsigned int>(
          node->GetProperties()->Get(vtkSelectionNode::HIERARCHICAL_INDEX())) == index)
      {
        return node;
      }
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkPVExtractSelection::LocateSelection(
  unsigned int composite_index, vtkSelection* sel)
{
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node)
    {
      if (node->GetProperties()->Has(vtkSelectionNode::COMPOSITE_INDEX()) &&
        node->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX()) ==
          static_cast<int>(composite_index))
      {
        return node;
      }
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSelector> vtkPVExtractSelection::NewSelectionOperator(
  vtkSelectionNode::SelectionContent type)
{
  if (type == vtkSelectionNode::QUERY)
  {
// Return a query operator
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
    return vtkSmartPointer<vtkPythonSelector>::New();
#else
    vtkErrorMacro(<< "vtkSelectionNode::QUERY is supported only when Python is enabled.");
    return nullptr;
#endif
  }
  else
  {
    return this->Superclass::NewSelectionOperator(type);
  }
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::RequestDataInternal(
  vtkSelectionNodeVector& outputs, vtkDataObject* dataObjectOutput, vtkSelectionNode* sel)
{
  // DON'T CLEAR THE outputs.

  vtkDataSet* ds = vtkDataSet::SafeDownCast(dataObjectOutput);
  vtkTable* table = vtkTable::SafeDownCast(dataObjectOutput);
  vtkGraph* graph = vtkGraph::SafeDownCast(dataObjectOutput);

  int ft = vtkSelectionNode::CELL;
  if (sel && sel->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()))
  {
    ft = sel->GetProperties()->Get(vtkSelectionNode::FIELD_TYPE());
  }

  if (ds && ft == vtkSelectionNode::CELL)
  {
    vtkSelectionNode* output = vtkSelectionNode::New();
    output->GetProperties()->Copy(sel->GetProperties(), /*deep=*/1);
    output->SetContentType(vtkSelectionNode::INDICES);
    vtkIdTypeArray* oids =
      vtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetArray("vtkOriginalCellIds"));
    if (oids)
    {
      output->SetSelectionList(oids);
      outputs.push_back(output);
    }
    output->Delete();
  }

  // no else, since original point indices are always passed.
  if (ds && (ft == vtkSelectionNode::CELL || ft == vtkSelectionNode::POINT))
  {
    vtkSelectionNode* output = vtkSelectionNode::New();
    output->GetProperties()->Copy(sel->GetProperties(), /*deep=*/1);
    output->SetFieldType(vtkSelectionNode::POINT);
    output->SetContentType(vtkSelectionNode::INDICES);
    vtkIdTypeArray* oids =
      vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetArray("vtkOriginalPointIds"));
    if (oids)
    {
      output->SetSelectionList(oids);
      outputs.push_back(output);
    }
    output->Delete();
  }

  if (table && ft == vtkSelectionNode::ROW)
  {
    vtkSelectionNode* output = vtkSelectionNode::New();
    output->GetProperties()->Copy(sel->GetProperties(), /*deep=*/1);
    output->SetFieldType(vtkSelectionNode::ROW);
    output->SetContentType(vtkSelectionNode::INDICES);
    vtkIdTypeArray* oids =
      vtkIdTypeArray::SafeDownCast(table->GetRowData()->GetArray("vtkOriginalRowIds"));
    if (oids)
    {
      output->SetSelectionList(oids);
      outputs.push_back(output);
    }
    output->Delete();
  }

  // The ExtractSelectedGraph filter does not produce the vtkOriginal*Ids array,
  // it will need extending to be able to follow the same pattern (with some
  // test cases to verify functionality).
  if (graph && ft == vtkSelectionNode::VERTEX)
  {
  }
  if (graph && ft == vtkSelectionNode::EDGE)
  {
  }
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::GetContentType(vtkSelection* sel)
{
  int ctype = -1;
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    int nodeCType = node->GetContentType();
    if (ctype == -1)
    {
      ctype = nodeCType;
    }
    else if (nodeCType != ctype)
    {
      return 0;
    }
  }
  return ctype;
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
