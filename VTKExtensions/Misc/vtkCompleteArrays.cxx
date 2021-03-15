/*=========================================================================

  Program:   ParaView
  Module:    vtkCompleteArrays.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompleteArrays.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkClientServerStream.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataObjectTypes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkTable.h"

// this doesn't directly use vtkPVDataSetAttributesInformation since
// vtkPVDataSetAttributesInformation sorts arrays. We instead explicitly use
// vtkPVArrayInformation to serialize information about arrays.
namespace
{
vtkAbstractArray* vtkNewArray(vtkPVArrayInformation* aInfo)
{
  if (vtkAbstractArray* array = vtkAbstractArray::CreateArray(aInfo->GetDataType()))
  {
    array->SetNumberOfComponents(aInfo->GetNumberOfComponents());
    array->SetName(aInfo->GetName());
    return array;
  }
  return nullptr;
}

void vtkSerialize(vtkClientServerStream& css, vtkDataSetAttributes* dsa)
{
  int attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
  dsa->GetAttributeIndices(attributeIndices);

  css << vtkClientServerStream::Reply
      << vtkClientServerStream::InsertArray(attributeIndices, vtkDataSetAttributes::NUM_ATTRIBUTES)
      << dsa->GetNumberOfArrays();

  for (int cc = 0, max = dsa->GetNumberOfArrays(); cc < max; ++cc)
  {
    vtkNew<vtkPVArrayInformation> arrayInfo;
    arrayInfo->CopyFromArray(dsa->GetAbstractArray(cc));

    vtkClientServerStream acss;
    arrayInfo->CopyToStream(&acss);

    const unsigned char* data;
    size_t length;
    acss.GetData(&data, &length);
    css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));
  }

  css << vtkClientServerStream::End;
}

bool vtkDeserialize(vtkClientServerStream& css, int msgIdx, vtkDataSetAttributes* dsa)
{
  dsa->Initialize();

  int idx = 0;
  int numArrays;
  int attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];

  if (!css.GetArgument(msgIdx, idx++, attributeIndices, vtkDataSetAttributes::NUM_ATTRIBUTES))
  {
    return false;
  }
  if (!css.GetArgument(msgIdx, idx++, &numArrays))
  {
    return false;
  }
  for (int cc = 0; cc < numArrays; ++cc)
  {
    vtkTypeUInt32 length;
    if (!css.GetArgumentLength(msgIdx, idx, &length))
    {
      return false;
    }
    std::vector<unsigned char> data(length);
    if (!css.GetArgument(msgIdx, idx++, &data[0], length))
    {
      return false;
    }

    vtkClientServerStream acss;
    acss.SetData(&*data.begin(), length);

    vtkNew<vtkPVArrayInformation> ai;
    ai->CopyFromStream(&acss);

    if (vtkAbstractArray* aa = vtkNewArray(ai.Get()))
    {
      dsa->AddArray(aa);
      aa->Delete();
    }
  }
  for (int cc = 0; cc < vtkDataSetAttributes::NUM_ATTRIBUTES; ++cc)
  {
    if (attributeIndices[cc] != -1)
    {
      dsa->SetActiveAttribute(attributeIndices[cc], cc);
    }
  }
  return true;
}
}

vtkStandardNewMacro(vtkCompleteArrays);
vtkCxxSetObjectMacro(vtkCompleteArrays, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkCompleteArrays::vtkCompleteArrays()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  if (this->Controller)
  {
    this->Controller->Register(this);
  }
}

//-----------------------------------------------------------------------------
vtkCompleteArrays::~vtkCompleteArrays()
{
  if (this->Controller)
  {
    this->Controller->UnRegister(this);
    this->Controller = nullptr;
  }
}

//----------------------------------------------------------------------------
int vtkCompleteArrays::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 0)
  {
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCompleteArrays::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCompleteArrays::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->FastDelete();
      }
    }
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkCompleteArrays::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkTable* inputTable = vtkTable::SafeDownCast(input);
  // let vtkTable pass-through this filter
  if (inputTable)
  {
    vtkTable* outputTable = vtkTable::SafeDownCast(output);
    outputTable->ShallowCopy(inputTable);
    return 1;
  }

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::SafeDownCast(input);
  // let vtkHyperTreeGrid pass-through this filter
  if (inputHTG)
  {
    vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(output);
    outputHTG->ShallowCopy(inputHTG);
    return 1;
  }

  vtkDataSet* outputDS = vtkDataSet::SafeDownCast(output);
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);
  if (inputDS && outputDS)
  {
    this->CompleteArraysOnBlock(inputDS, outputDS);
    return 1;
  }

  // Iterate over composite datasets and complete arrays
  vtkDataObjectTree* outputDOT = vtkDataObjectTree::SafeDownCast(output);
  vtkDataObjectTree* inputDOT = vtkDataObjectTree::SafeDownCast(input);
  if (inputDOT && outputDOT)
  {
    outputDOT->CopyStructure(inputDOT);

    using Opts = vtk::DataObjectTreeOptions;
    auto inIter = vtk::Range(inputDOT, Opts::TraverseSubTree);
    auto outIter = vtk::Range(outputDOT, Opts::TraverseSubTree);

    auto inNode = inIter.begin();
    auto outNode = outIter.begin();
    for (; inNode != inIter.end(); ++inNode, ++outNode)
    {
      vtkDataObject* inputDO = *inNode;
      if (inputDO && inputDO->IsA("vtkDataObjectTree"))
      {
        // Skip interior nodes
        continue;
      }
      if (inputDO)
      {
        *outNode = inputDO->NewInstance();
        outNode->ShallowCopy(inputDO);
        outNode->FastDelete();
      }
      vtkDataSet* dataSetIn = vtkDataSet::SafeDownCast(inputDO);
      vtkDataSet* dataSetOut = vtkDataSet::SafeDownCast(*outNode);
      this->CompleteArraysOnBlock(dataSetIn, dataSetOut);

      // Set the block in the output if it was instantiated in the call above.
      if (!dataSetIn && dataSetOut)
      {
        *outNode = dataSetOut;
        outNode->FastDelete();
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkCompleteArrays::CompleteArraysOnBlock(vtkDataSet* inputDS, vtkDataSet*& outputDS)
{
  // Initialize
  //
  vtkDebugMacro(<< "Completing array");

  if (inputDS && outputDS)
  {
    outputDS->CopyStructure(inputDS);
    outputDS->GetFieldData()->PassData(inputDS->GetFieldData());
    outputDS->GetPointData()->PassData(inputDS->GetPointData());
    outputDS->GetCellData()->PassData(inputDS->GetCellData());
  }

  if (this->Controller->GetNumberOfProcesses() <= 1)
  {
    return;
  }
  int myProcId = this->Controller->GetLocalProcessId();

  // array is [num points on proc 0, num cells on proc 0,
  //           num points on this proc, num cells on this proc,
  //           proc id if this process has point,
  //           proc id if this process has cells]

  vtkIdType localInfo[6] = { -1, -1, 0, 0, -1, -1 };
  if (inputDS)
  {
    if (myProcId == 0)
    {
      localInfo[0] = inputDS->GetNumberOfPoints();
      localInfo[1] = inputDS->GetNumberOfCells();
    }
    localInfo[2] = inputDS->GetNumberOfPoints();
    localInfo[3] = inputDS->GetNumberOfCells();
    if (inputDS->GetNumberOfPoints() > 0)
    {
      localInfo[4] = myProcId;
    }
    if (inputDS->GetNumberOfCells() > 0)
    {
      localInfo[5] = myProcId;
    }
  }
  vtkIdType globalInfo[6];
  this->Controller->AllReduce(localInfo, globalInfo, 6, vtkCommunicator::MAX_OP);

  // Idea is that if process 0 doesn't have the proper point data and cell data
  // arrays (if cells exist) then we need to get that information from another proc.
  // We only need to get that information from one process so we look for the highest
  // process id with cells (if they exist) or points (if no cells exist) to
  // provide that information.
  if (globalInfo[1] > 0 || (globalInfo[3] == 0 && globalInfo[0] > 0) || globalInfo[2] == 0)
  {
    // process 0 has all of the needed information already (globalInfo[2] == 0
    // means there is no information at all)
    return;
  }

  int infoProc = static_cast<int>(globalInfo[5]); // a process that has the information proc 0 needs
  if (globalInfo[3] == 0)
  { // there are no cells so we find a process with points
    infoProc = static_cast<int>(globalInfo[4]);
  }

  if (myProcId == 0)
  {
    // Collected information from the remote processes.

    // Process 0 may have a nullptr block - get the data type from the process
    // that has the data information and instantiate a placeholder block.
    int typeAndLength[2];
    this->Controller->Receive(typeAndLength, 2, infoProc, 389002);

    if (!outputDS)
    {
      int dataSetType = typeAndLength[0];
      outputDS = vtkDataSet::SafeDownCast(vtkDataObjectTypes::NewDataObject(dataSetType));
    }

    int length = typeAndLength[1];
    std::vector<unsigned char> data(static_cast<size_t>(length));
    this->Controller->Receive(&data[0], length, infoProc, 389003);

    vtkClientServerStream css;
    css.SetData(&data[0], static_cast<size_t>(length));

    vtkDeserialize(css, 0, outputDS->GetPointData());
    vtkDeserialize(css, 1, outputDS->GetCellData());
    if (vtkPointSet* ps = vtkPointSet::SafeDownCast(outputDS))
    {
      vtkNew<vtkPoints> pts;
      int dataType;
      if (css.GetArgument(2, 0, &dataType))
      {
        pts->SetDataType(dataType);
      }
      ps->SetPoints(pts.Get());
    }
    else if (auto rg = vtkRectilinearGrid::SafeDownCast(outputDS))
    {
      rg->SetXCoordinates(nullptr);
      rg->SetYCoordinates(nullptr);
      rg->SetZCoordinates(nullptr);

      int dataType;
      if (css.GetArgument(2, 0, &dataType) && dataType != VTK_VOID)
      {
        if (auto array = vtkAbstractArray::CreateArray(dataType))
        {
          rg->SetXCoordinates(vtkDataArray::SafeDownCast(array));
          array->FastDelete();
        }
      }

      if (css.GetArgument(2, 1, &dataType) && dataType != VTK_VOID)
      {
        if (auto array = vtkAbstractArray::CreateArray(dataType))
        {
          rg->SetYCoordinates(vtkDataArray::SafeDownCast(array));
          array->FastDelete();
        }
      }
      if (css.GetArgument(2, 2, &dataType) && dataType != VTK_VOID)
      {
        if (auto array = vtkAbstractArray::CreateArray(dataType))
        {
          rg->SetZCoordinates(vtkDataArray::SafeDownCast(array));
          array->FastDelete();
        }
      }
    }
  }
  else if (myProcId == infoProc)
  {
    vtkClientServerStream css;
    vtkSerialize(css, inputDS->GetPointData());
    vtkSerialize(css, inputDS->GetCellData());

    if (vtkPointSet* ps = vtkPointSet::SafeDownCast(inputDS))
    {
      css << vtkClientServerStream::Reply << ps->GetPoints()->GetDataType()
          << vtkClientServerStream::End;
    }
    else if (auto rg = vtkRectilinearGrid::SafeDownCast(inputDS))
    {
      css << vtkClientServerStream::Reply
          << (rg->GetXCoordinates() ? rg->GetXCoordinates()->GetDataType() : VTK_VOID)
          << (rg->GetYCoordinates() ? rg->GetYCoordinates()->GetDataType() : VTK_VOID)
          << (rg->GetZCoordinates() ? rg->GetZCoordinates()->GetDataType() : VTK_VOID)
          << vtkClientServerStream::End;
    }

    int dataSetType = inputDS->GetDataObjectType();
    size_t length;
    const unsigned char* data;
    css.GetData(&data, &length);
    // Bundle data set type and length into a single message for efficiency
    int typeAndLength[2] = { dataSetType, static_cast<int>(length) };
    this->Controller->Send(typeAndLength, 2, 0, 389002);
    this->Controller->Send(const_cast<unsigned char*>(data), typeAndLength[1], 0, 389003);
  }
  return;
}

//-----------------------------------------------------------------------------
void vtkCompleteArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Controller)
  {
    os << indent << "Controller: " << this->Controller << endl;
  }
  else
  {
    os << indent << "Controller: (none)\n";
  }
}
