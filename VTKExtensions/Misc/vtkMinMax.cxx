// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMinMax.h"

#include "vtkAbstractArray.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

#include "vtkMultiProcessController.h"

vtkStandardNewMacro(vtkMinMax);

struct vtkMinMax::vtkMinMaxWorker
{
  template <class TArrayIn, class TArrayOut>
  void operator()(TArrayIn* inArray, TArrayOut* outArray, vtkMinMax* self)
  {
    auto in = vtk::DataArrayTupleRange(inArray);
    auto out = vtk::DataArrayTupleRange(outArray);
    vtkIdType numTuples = inArray->GetNumberOfTuples();
    int numComp = inArray->GetNumberOfComponents();
    int compIdx = self->ComponentIdx;
    char* firstPasses = self->GetFirstPasses();
    for (vtkIdType idx = 0; idx < numTuples; ++idx)
    {
      if ((self->GhostArray != nullptr) &&
        (self->GhostArray->GetValue(idx) & vtkDataSetAttributes::DUPLICATECELL))
      {
        // skip cell and point attributes that don't belong to me
        continue;
      }
      // go over each component of the tuple
      for (int jdx = 0; jdx < numComp; jdx++)
      {
        auto iTuple = in[idx];
        auto oTuple = out[0];

        if (firstPasses[compIdx + jdx])
        {
          firstPasses[compIdx + jdx] = 0;
          oTuple[jdx] = iTuple[jdx];
          continue;
        }

        switch (self->GetOperation())
        {
          case vtkMinMax::MIN:
          {
            oTuple[jdx] = std::min(oTuple[jdx], iTuple[jdx]);
            break;
          }
          case vtkMinMax::MAX:
          {
            oTuple[jdx] = std::max(oTuple[jdx], iTuple[jdx]);
            break;
          }
          case vtkMinMax::SUM:
          {
            oTuple[jdx] += iTuple[jdx];
            break;
          }
          default:
          {
            oTuple[jdx] = iTuple[jdx];
            break;
          }
        }
      }
    }
  }
};

//-----------------------------------------------------------------------------
vtkMinMax::vtkMinMax()
{
  this->Operation = vtkMinMax::MIN;
  this->CFirstPass = nullptr;
  this->PFirstPass = nullptr;
  this->FirstPasses = nullptr;
  this->MismatchOccurred = 0;
}

//-----------------------------------------------------------------------------
vtkMinMax::~vtkMinMax()
{
  delete[] this->CFirstPass;
  delete[] this->PFirstPass;
}

//-----------------------------------------------------------------------------
void vtkMinMax::SetOperation(const char* str)
{
  if (strncmp(str, "MIN", 3) == 0)
  {
    this->SetOperation(vtkMinMax::MIN);
  }
  else if (strncmp(str, "MAX", 3) == 0)
  {
    this->SetOperation(vtkMinMax::MAX);
  }
  else if (strncmp(str, "SUM", 3) == 0)
  {
    this->SetOperation(vtkMinMax::SUM);
  }
  else
  {
    vtkErrorMacro("Unrecognized operation type defaulting to MIN");
    this->SetOperation(0);
  }
}

//----------------------------------------------------------------------------
int vtkMinMax::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkMinMax::RequestData(vtkInformation* vtkNotUsed(reqInfo), vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  int numInputs;
  int idx, numArrays;
  vtkCompositeDataSet* cdobj = nullptr;

  // get hold of input, output
  vtkPolyData* output = vtkPolyData::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* input0 = vtkDataSet::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

  // if input0 is a composite dataset, use first leaf node to
  // setup output attribute arrays
  if (!input0)
  {
    cdobj = vtkCompositeDataSet::SafeDownCast(
      inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
    if (cdobj)
    {
      vtkCompositeDataIterator* cdit = cdobj->NewIterator();
      input0 = vtkDataSet::SafeDownCast(cdit->GetCurrentDataObject());
      cdit->Delete();
    }
  }
  if (!input0)
  {
    vtkErrorMacro("Can't find a dataset to get attribute shape from.");
    return 0;
  }

  // make output arrays of same type and width as input, but make them just one
  // element long
  vtkFieldData* icd = input0->GetCellData();
  vtkFieldData* ocd = output->GetCellData();
  ocd->CopyStructure(icd);
  numArrays = icd->GetNumberOfArrays();
  for (idx = 0; idx < numArrays; idx++)
  {
    ocd->GetArray(idx)->SetNumberOfTuples(1);
  }
  vtkFieldData* ipd = input0->GetPointData();
  vtkFieldData* opd = output->GetPointData();
  opd->CopyStructure(ipd);
  numArrays = ipd->GetNumberOfArrays();
  for (idx = 0; idx < numArrays; idx++)
  {
    opd->GetArray(idx)->SetNumberOfTuples(1);
  }

  // initialize first pass flags for the cell data
  int numComp;
  numComp = ocd->GetNumberOfComponents();
  delete[] this->CFirstPass;
  this->CFirstPass = new char[numComp];
  for (idx = 0; idx < numComp; idx++)
  {
    this->CFirstPass[idx] = 1;
  }
  numComp = ipd->GetNumberOfComponents();
  delete[] this->PFirstPass;
  this->PFirstPass = new char[numComp];
  for (idx = 0; idx < numComp; idx++)
  {
    this->PFirstPass[idx] = 1;
  }

  // make output 1 point and cell in the output as placeholders for the results
  vtkPoints* points = vtkPoints::New();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  output->SetPoints(points);
  points->Delete();

  vtkCellArray* cells = vtkCellArray::New();
  vtkIdType ptId = 0;
  cells->InsertNextCell(1, &ptId);
  output->SetVerts(cells);
  cells->Delete();

  // keep a flag in case someone cares about data not lining up exactly
  this->MismatchOccurred = 0;

  // go through each input and perform the operation on all of its data
  // we accumulate the results into the output arrays, so there is no need for
  // a second pass over the per input results
  numInputs = this->GetNumberOfInputConnections(0);
  vtkInformation* inInfo;
  vtkDataSet* inputN;

  for (idx = 0; idx < numInputs; ++idx)
  {
    inInfo = inputVector[0]->GetInformationObject(idx);
    if (!cdobj)
    {
      inputN = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

      // set first pass flags to point to cell data
      this->ComponentIdx = 0;
      this->FlagsForCells();
      // operate on the cell data
      this->OperateOnField(inputN->GetCellData(), ocd);

      // ditto for point data
      this->ComponentIdx = 0;
      this->FlagsForPoints();
      this->OperateOnField(inputN->GetPointData(), opd);
    }
    else
    {
      // with composite data, we need an "innest" loop to go over sub datasets
      cdobj = vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
      vtkCompositeDataIterator* cdit = cdobj->NewIterator();
      while (!cdit->IsDoneWithTraversal())
      {
        inputN = vtkDataSet::SafeDownCast(cdit->GetCurrentDataObject());

        this->ComponentIdx = 0;
        this->FlagsForCells();
        this->OperateOnField(inputN->GetCellData(), ocd);

        this->ComponentIdx = 0;
        this->FlagsForPoints();
        this->OperateOnField(inputN->GetPointData(), opd);

        cdit->GoToNextItem();
      }
      cdit->Delete();
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkMinMax::FlagsForPoints()
{
  this->FirstPasses = this->PFirstPass;
}

//-----------------------------------------------------------------------------
void vtkMinMax::FlagsForCells()
{
  this->FirstPasses = this->CFirstPass;
}

//-----------------------------------------------------------------------------
void vtkMinMax::OperateOnField(vtkFieldData* ifd, vtkFieldData* ofd)
{
  assert((int)vtkDataSetAttributes::DUPLICATECELL == (int)vtkDataSetAttributes::DUPLICATEPOINT);
  this->GhostArray =
    vtkUnsignedCharArray::SafeDownCast(ifd->GetArray(vtkDataSetAttributes::GhostArrayName()));

  int numArrays = ofd->GetNumberOfArrays();
  for (int idx = 0; idx < numArrays; idx++)
  {
    vtkAbstractArray* ia = ifd->GetArray(idx);
    vtkAbstractArray* oa = ofd->GetArray(idx);

    // type check
    // oa will not be nullptr since we are iterating over ifd
    // input and output numtuples don't need to match (out will always be 1)
    if (ia == nullptr || ia->GetDataType() != oa->GetDataType() ||
      ia->GetNumberOfComponents() != oa->GetNumberOfComponents() ||
      (strcmp(ia->GetName(), oa->GetName()) != 0))
    {
      // a mismatch between arrays, ignore this input array
      this->MismatchOccurred = 1;
      // if mismatches make and entire field in the output invalid
      // the firstpasses bitsets will show if a entire output value is invalid
    }
    else
    {
      // operate on all of the elements of this array
      this->OperateOnArray(ia, oa);
    }

    // update first pass flag index to move to the next array
    this->ComponentIdx += oa->GetNumberOfComponents();
  }
}

//-----------------------------------------------------------------------------
void vtkMinMax::OperateOnArray(vtkAbstractArray* ia, vtkAbstractArray* oa)
{
  auto inDa = vtkDataArray::SafeDownCast(ia);
  auto outDa = vtkDataArray::SafeDownCast(oa);
  if (!inDa || !outDa)
  {
    vtkErrorMacro(<< "Can't operate on non-data arrays");
    this->MismatchOccurred = 1;
    return;
  }
  vtkMinMaxWorker worker;
  if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(inDa, outDa, worker, this))
  {
    worker(inDa, outDa, this);
  }
}

//-----------------------------------------------------------------------------
void vtkMinMax::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Operation: " << this->Operation << endl;
  os << indent << "FirstPasses: " << (this->FirstPasses ? this->FirstPasses : "None") << endl;
  os << indent << "MismatchOccurred: " << this->MismatchOccurred << endl;
}
