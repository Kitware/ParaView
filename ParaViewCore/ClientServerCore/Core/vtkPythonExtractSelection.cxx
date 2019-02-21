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
#include "vtkPython.h" // has to be first!

#include "vtkPythonExtractSelection.h"

#include "vtkAbstractArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTypes.h"
#include "vtkExtractSelectedIds.h"
#include "vtkExtractSelectedRows.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSmartPyObject.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>
#include <sstream>

namespace
{
bool CheckAndFlushPythonErrors()
{
  if (PyErr_Occurred())
  {
    PyErr_Print();
    PyErr_Clear();
    return true;
  }
  return false;
}
}

vtkStandardNewMacro(vtkPythonExtractSelection);
//----------------------------------------------------------------------------
vtkPythonExtractSelection::vtkPythonExtractSelection()
{
}

//----------------------------------------------------------------------------
vtkPythonExtractSelection::~vtkPythonExtractSelection()
{
}

//----------------------------------------------------------------------------
int vtkPythonExtractSelection::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    // This filter handles composite datasets, datasets and table. Not graphs and others.
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  }
  else
  {
    assert(port == 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonExtractSelection::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Output type is same as input
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  if (input)
  {
    const char* outputType = NULL;
    if (this->PreserveTopology)
    {
      outputType = input->GetClassName();
    }
    else
    {
      outputType = "vtkUnstructuredGrid";
      if (vtkCompositeDataSet::SafeDownCast(input))
      {
        outputType = "vtkMultiBlockDataSet";
      }
      else if (vtkTable::SafeDownCast(input))
      {
        outputType = "vtkTable";
      }
    }
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
    if (!output || !output->IsA(outputType))
    {
      vtkDataObject* newOutput = vtkDataObjectTypes::NewDataObject(outputType);
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPythonExtractSelection::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // if not selection is specified, return.
  if (inputVector[1]->GetNumberOfInformationObjects() == 0)
  {
    return 1;
  }

  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkSelection* selection = vtkSelection::GetData(inputVector[1], 0);
  if (selection == NULL || selection->GetNumberOfNodes() == 0)
  {
    // empty selection.
    return 1;
  }

  if (selection->GetNumberOfNodes() > 1)
  {
    vtkWarningMacro("vtkPythonExtractSelection currently only supports a selection "
                    "with a single vtkSelectionNode instance. All other instances will be ignored, "
                    "except the first one.");
  }

  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  this->InitializeOutput(output, input);

  // ensure Python is initialized (safe to call many times)
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject module(PyImport_ImportModule("paraview.detail.extract_selection"));
  CheckAndFlushPythonErrors();
  if (!module)
  {
    vtkErrorMacro("Failed to import `paraview.detail.extract_selection` module.");
    return 0;
  }

  vtkSmartPyObject self(vtkPythonUtil::GetObjectFromPointer(this));
  vtkSmartPyObject fname(PyString_FromString("execute"));

  // call `paraview.detail.annotation.execute_on_attribute_data(self)`
  vtkSmartPyObject retVal(
    PyObject_CallMethodObjArgs(module, fname.GetPointer(), self.GetPointer(), nullptr));

  if (CheckAndFlushPythonErrors())
  {
    // some Python errors reported.
    return 0;
  }

  // we don't check the return val since if the call fails on one rank, we may
  // end up with deadlocks so we do the reduction no matter what.
  (void)retVal;

  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonExtractSelection::InitializeOutput(vtkDataObject* output, vtkDataObject* input)
{
  if (this->PreserveTopology)
  {
    // When preserving topology, we need to shallow copy input to output.
    output->ShallowCopy(input);

    vtkCompositeDataSet* outputCD = vtkCompositeDataSet::SafeDownCast(output);
    if (!outputCD)
    {
      return;
    }

    // For composite datasets, the ShallowCopy simply shares the leaf datasets.
    // We need to create new instances for those.
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(outputCD->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataObject* ds = iter->GetCurrentDataObject();
      assert(ds != NULL);

      vtkDataObject* clone = ds->NewInstance();
      clone->ShallowCopy(ds);
      outputCD->SetDataSet(iter, clone);
      clone->FastDelete();
    }
  }
  else
  {
    // not preserving topology. In that case, we just ensure that the output
    // composite dataset has same structure as the input.
    if (vtkCompositeDataSet* outputCD = vtkCompositeDataSet::SafeDownCast(output))
    {
      vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(input);
      assert(inputCD != NULL);
      outputCD->CopyStructure(inputCD);

      // To make it easier for the Python code to pass the "original ids" array back,
      // we initialize the non-null leaf nodes in this composite dataset with empty datasets.
      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(inputCD->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkDataObject* ds = iter->GetCurrentDataObject();
        assert(ds != NULL);

        if (vtkTable::SafeDownCast(ds))
        {
          vtkTable* table = vtkTable::New();
          outputCD->SetDataSet(iter, table);
          table->FastDelete();
        }
        else if (vtkDataSet::SafeDownCast(ds))
        {
          vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
          outputCD->SetDataSet(iter, ug);
          ug->FastDelete();
        }
        else
        {
          vtkWarningMacro("Composite data has unsupported type: " << ds->GetClassName());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
bool vtkPythonExtractSelection::ExtractElements(
  int attributeType, vtkDataObject* input, vtkDataObject* output)
{
  assert(this->PreserveTopology == 0);
  if (vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(input))
  {
    return this->ExtractElements(attributeType, inputCD, vtkCompositeDataSet::SafeDownCast(output));
  }

  int fieldType;
  switch (attributeType)
  {
    case vtkDataObject::CELL:
      fieldType = vtkSelectionNode::CELL;
      break;

    case vtkDataObject::POINT:
      fieldType = vtkSelectionNode::POINT;
      break;

    case vtkDataObject::ROW:
      fieldType = vtkSelectionNode::ROW;
      break;

    default:
      vtkWarningMacro("Unsupported attributeType: " << attributeType);
      return false;
  }

  // sanity check: ensure that the attribute type specified is valid for the type of
  // input dataset.
  if (input->GetAttributes(attributeType) == NULL)
  {
    vtkWarningMacro("Incorrect attributeType '" << attributeType << "' "
                                                                    "for input data type '"
                                                << input->GetClassName() << "'");
    return false;
  }

  // extract_selection.execute() puts the selected ids array in field data of the output dataset.
  // This is done so to keep the code clean for the case with composite datasets.
  vtkAbstractArray* idsToExtact = output->GetFieldData()->GetAbstractArray("vtkSelectedIds");
  if (idsToExtact && idsToExtact->GetNumberOfTuples() > 0)
  {
    vtkNew<vtkSelection> selection;
    vtkNew<vtkSelectionNode> node;
    selection->AddNode(node.GetPointer());
    node->SetContentType(vtkSelectionNode::INDICES);
    node->SetFieldType(fieldType);
    node->SetSelectionList(idsToExtact);

    vtkSmartPointer<vtkAlgorithm> extractor;
    if (vtkTable::SafeDownCast(input))
    {
      vtkNew<vtkExtractSelectedRows> filter;
      filter->SetAddOriginalRowIdsArray(true);
      extractor = filter.GetPointer();
    }
    else
    {
      vtkNew<vtkExtractSelectedIds> filter;
      filter->PreserveTopologyOff();
      extractor = filter.GetPointer();
    }
    extractor->SetInputDataObject(0, input);
    extractor->SetInputDataObject(1, selection.GetPointer());
    extractor->Update();

    idsToExtact = NULL;
    // note: the ShallowCopy will overwrite output->FieldData, hence idsToExtact will be
    // dangling.
    output->ShallowCopy(extractor->GetOutputDataObject(0));
    return true;
  }

  output->Initialize();
  return false;
}

//----------------------------------------------------------------------------
bool vtkPythonExtractSelection::ExtractElements(
  int attributeType, vtkCompositeDataSet* input, vtkCompositeDataSet* output)
{
  assert(this->PreserveTopology == 0);

  // this method simply iterates over all the leaf nodes in the dataset and calls
  // ExtractElements.
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (!this->ExtractElements(
          attributeType, iter->GetCurrentDataObject(), output->GetDataSet(iter)))
    {
      output->SetDataSet(iter, NULL);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPythonExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
