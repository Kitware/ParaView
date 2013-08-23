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
#include "vtkPythonExtractSelection.h"

#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCellTypes.h"
#include "vtkCharArray.h"
#include "vtkDataObjectTypes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPythonInterpreter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

#include <assert.h>
#include <map>
#include <vector>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPythonExtractSelection);
//----------------------------------------------------------------------------
vtkPythonExtractSelection::vtkPythonExtractSelection()
{
  this->PreserveTopology = 0;
  this->Inverse = 0;
  this->SetExecuteMethod(vtkPythonExtractSelection::ExecuteScript, this);

  // eventually, once this class starts taking in a real vtkSelection.
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkPythonExtractSelection::~vtkPythonExtractSelection()
{
}

//----------------------------------------------------------------------------
int vtkPythonExtractSelection::FillInputPortInformation(int port, vtkInformation *info)
{
  if(port == 0)
    {
    this->Superclass::FillInputPortInformation(port, info);
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    }
  else if(port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  else
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonExtractSelection::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Output type is same as input
  vtkDataObject *input = vtkDataObject::GetData(inputVector[0], 0);
  if (input)
    {
    const char* outputType = "vtkUnstructuredGrid";
    if (input->IsA("vtkCompositeDataSet"))
      {
      outputType = "vtkMultiBlockDataSet";
      }
    else if (input->IsA("vtkTable"))
      {
      outputType = "vtkTable";
      }

    // for each output
    for (int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(outputType))
        {
        vtkDataObject* newOutput =
          vtkDataObjectTypes::NewDataObject(outputType);
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
        }
      }
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPythonExtractSelection::ExecuteScript(void* arg)
{
  vtkPythonExtractSelection* self =
    static_cast<vtkPythonExtractSelection*>(arg);
  if (self)
    {
    self->Exec();
    }
}

//----------------------------------------------------------------------------
void vtkPythonExtractSelection::Exec()
{
  // ensure Python is initialized.
  vtkPythonInterpreter::Initialize();

  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", this);
  char *aplus = addrofthis;
  if ((addrofthis[0] == '0') &&
      ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
    {
    aplus += 2; //skip over "0x"
    }

  vtksys_ios::ostringstream stream;
  stream << "from paraview import extract_selection as pv_es" << endl
         << "from vtkPVClientServerCoreCorePython import vtkPythonExtractSelection" << endl
         << "me = vtkPythonExtractSelection('" << aplus << " ')" << endl
         << "pv_es.Exec(me, me.GetInputDataObject(0, 0),  me.GetInputDataObject(1, 0), me.GetOutputDataObject(0))" << endl
         << "del me" << endl;

  vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPythonExtractSelection::ExtractElements(
  vtkDataObject* data, vtkSelection *selection, vtkCharArray* mask)
{
  vtkDataSet* ds = vtkDataSet::SafeDownCast(data);
  vtkTable* table = vtkTable::SafeDownCast(data);
  vtkSelectionNode* selNode = selection->GetNode(0);

  // get the inverse flag for the selection
  vtkInformation* selProperties = selNode->GetProperties();
  this->Inverse = selProperties->Get(vtkSelectionNode::INVERSE());

  if (ds)
    {
    switch (selNode->GetFieldType())
      {
    case vtkSelectionNode::POINT:
      return this->ExtractPoints(ds, mask);

    case vtkSelectionNode::CELL:
      return this->ExtractCells(ds, mask);
      }
    }
  else if (table)
    {
    return this->ExtractElements(table, mask);
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkPythonExtractSelection::ExtractPoints(
  vtkDataSet* input, vtkCharArray* mask)
{
  assert(mask && input && mask->GetNumberOfTuples() ==
    input->GetNumberOfPoints());
  vtkIdType numPoints = input->GetNumberOfPoints();

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::New();
  vtkDataSetAttributes* inputPD = input->GetPointData();
  vtkDataSetAttributes* outputPD = output->GetPointData();

  if (this->PreserveTopology)
    {
    // TODO: Determine whether either numpy_to_vtk can return a
    //   vtkSignedCharArray in a backwards-compatible manner (unlikely)
    //   or the vtkPVExtractSelection and vtkExtractSelection filters
    //   can use a vtkCharArray instead of a vtkSignedCharArray (more
    //   likely possible). That would save us a deep copy here.
    output->ShallowCopy(input);
    vtkSignedCharArray* inside = vtkSignedCharArray::New();
    inside->DeepCopy(mask);
    inside->SetName("vtkInsidedness");
    outputPD->AddArray(inside);
    inside->FastDelete();
    }
  else
    {
    vtkPoints* outputPoints = vtkPoints::New();
    outputPoints->Allocate(numPoints);

    output->SetPoints(outputPoints);
    output->Allocate(1);
    outputPoints->FastDelete();

    outputPD->SetCopyGlobalIds(1);
    outputPD->SetCopyPedigreeIds(1);
    outputPD->CopyAllocate(inputPD, numPoints);

    vtkIdTypeArray* originalIds = vtkIdTypeArray::New();
    originalIds->SetName("vtkOriginalPointIds");
    originalIds->Allocate(numPoints);

    const char* pmask = mask->GetPointer(0);

    std::vector<vtkIdType> pointIds;
    for (vtkIdType id = 0; id < numPoints; ++id)
      {
      // extract selected points if inverse flag is false and vice versa
      if (pmask[id] == this->Inverse)
        {
        continue;
        }

      vtkIdType newId = outputPoints->InsertNextPoint(input->GetPoint(id));
      outputPD->CopyData(inputPD, id, newId);
      pointIds.push_back(newId);
      originalIds->InsertValue(newId, id);
      }

    if(!pointIds.empty())
      {
      output->InsertNextCell(VTK_POLY_VERTEX,
        static_cast<vtkIdType>(pointIds.size()), &pointIds[0]);
      }

    outputPD->AddArray(originalIds);
    // unmark global and pedigree ids.
    outputPD->SetActiveAttribute(-1, vtkDataSetAttributes::GLOBALIDS);
    outputPD->SetActiveAttribute(-1, vtkDataSetAttributes::PEDIGREEIDS);

    originalIds->FastDelete();

    output->Squeeze();
    }
  return output;
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkPythonExtractSelection::ExtractCells(
  vtkDataSet* input, vtkCharArray* mask)
{
  assert(mask && input && mask->GetNumberOfTuples() ==
    input->GetNumberOfCells());

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::New();
  vtkCellData* inputCD = input->GetCellData();
  vtkCellData* outputCD = output->GetCellData();

  if (this->PreserveTopology)
    {
    // TODO: Determine whether either numpy_to_vtk can return a
    //   vtkSignedCharArray in a backwards-compatible manner (unlikely)
    //   or the vtkPVExtractSelection and vtkExtractSelection filters
    //   can use a vtkCharArray instead of a vtkSignedCharArray (more
    //   likely possible). That would save us a deep copy here.
    output->ShallowCopy(input);
    vtkSignedCharArray* inside = vtkSignedCharArray::New();
    inside->DeepCopy(mask);
    inside->SetName("vtkInsidedness");
    outputCD->AddArray(inside);
    inside->FastDelete();
    }
  else
    {
    vtkIdType numCells = input->GetNumberOfCells();
    vtkIdType numPoints = input->GetNumberOfPoints();

    vtkPoints* outputPoints = vtkPoints::New();
    outputPoints->Allocate(numPoints);

    output->SetPoints(outputPoints);
    output->Allocate(numCells);
    outputPoints->FastDelete();

    vtkPointData *inputPD = input->GetPointData();
    vtkPointData *outputPD = output->GetPointData();

    outputCD->SetCopyGlobalIds(1);
    outputPD->SetCopyGlobalIds(1);
    outputCD->SetCopyPedigreeIds(1);
    outputPD->SetCopyPedigreeIds(1);
    outputCD->CopyAllocate(inputCD);
    outputPD->CopyAllocate(inputPD);

    vtkIdTypeArray* originalPointIds = vtkIdTypeArray::New();
    originalPointIds->SetName("vtkOriginalPointIds");
    originalPointIds->Allocate(numPoints);

    vtkIdTypeArray* originalCellIds = vtkIdTypeArray::New();
    originalCellIds->SetName("vtkOriginalCellIds");
    originalCellIds->Allocate(numCells);

    std::map<vtkIdType, vtkIdType> outPointIdMap;
    const char* pmask = mask->GetPointer(0);

    for (vtkIdType inCellId = 0; inCellId < numCells; inCellId++)
      {
      if (pmask[inCellId] == this->Inverse)
        {
        continue;
        }

      vtkCell *cell = input->GetCell(inCellId);

      std::vector<vtkIdType> outPointsInCell;

      // insert points from the cell
      for (vtkIdType j = 0; j < cell->GetNumberOfPoints(); j++)
        {
        vtkIdType inPointId = cell->GetPointId(j);

        vtkIdType outPointId = -1;

        std::map<vtkIdType, vtkIdType>::iterator iter =
          outPointIdMap.find(inPointId);
        if (iter == outPointIdMap.end())
          {
          // insert copy of the old point
          outPointId = outputPoints->InsertNextPoint(input->GetPoint(inPointId));

          // copy old point data
          outputPD->CopyData(inputPD, inPointId, outPointId);

          // add point id to the mapping
          outPointIdMap[inPointId] = outPointId;

          // add old point id to original point ids
          originalPointIds->InsertNextValue(inPointId);
          }
        else
          {
          // already added the point, use its new id
          outPointId = iter->second;
          }

        // add point id
        outPointsInCell.push_back(outPointId);
        }

      // add new cell
      vtkIdType outCellId = output->InsertNextCell(cell->GetCellType(),
        static_cast<vtkIdType>(outPointsInCell.size()),
        outPointsInCell.size() > 0? &outPointsInCell[0] : NULL);

      // copy cell data
      outputCD->CopyData(inputCD, inCellId, outCellId);

      originalCellIds->InsertNextValue(inCellId);
      }

    outputPD->AddArray(originalPointIds);
    // unmark global and pedigree ids.
    outputPD->SetActiveAttribute(-1, vtkDataSetAttributes::GLOBALIDS);
    outputPD->SetActiveAttribute(-1, vtkDataSetAttributes::PEDIGREEIDS);
    originalPointIds->FastDelete();

    outputCD->AddArray(originalCellIds);
    // unmark global and pedigree ids.
    outputCD->SetActiveAttribute(-1, vtkDataSetAttributes::GLOBALIDS);
    outputCD->SetActiveAttribute(-1, vtkDataSetAttributes::PEDIGREEIDS);
    originalCellIds->FastDelete();
    output->Squeeze();
    }

  return output;
}

//----------------------------------------------------------------------------
vtkTable* vtkPythonExtractSelection::ExtractElements(
  vtkTable* vtkNotUsed(data), vtkCharArray* vtkNotUsed(mask))
{
  vtkErrorMacro("Not supported yet. Aborting for debugging purposes.");
  abort();
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPythonExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
