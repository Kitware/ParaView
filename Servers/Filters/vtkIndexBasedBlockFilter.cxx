/*=========================================================================

  Program:   ParaView
  Module:    vtkIndexBasedBlockFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIndexBasedBlockFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkUnsignedIntArray.h"

#define VTK_INDEXBASEDBLOCKFILTER_MAX(x, y) (x>y? x : y)

vtkStandardNewMacro(vtkIndexBasedBlockFilter);
vtkCxxRevisionMacro(vtkIndexBasedBlockFilter, "1.25");
vtkCxxSetObjectMacro(vtkIndexBasedBlockFilter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkIndexBasedBlockFilter::vtkIndexBasedBlockFilter()
{
  this->Block = 0;
  this->BlockSize = 1024;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->StartIndex= -1;
  this->EndIndex= -1;
  this->FieldType = POINT;
  this->ProcessID = 0;

  this->CompositeDataSetIndex = 0;
  this->Temporary = vtkMultiPieceDataSet::New();

  this->PointCoordinatesArray = 0;
  this->StructuredCoordinatesArray = 0;
  this->OriginalIndicesArray = 0;
  this->CompositeIndexArray = 0;

  this->GenerateOriginalIds = 1;

  this->CurrentCIndex = 0;
  this->CurrentHIndex = 0;
  this->CurrentHLevel = 0;
}

//----------------------------------------------------------------------------
vtkIndexBasedBlockFilter::~vtkIndexBasedBlockFilter()
{
  this->SetController(0);
  this->Temporary->Delete();
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // we even handle composite datasets.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkIndexBasedBlockFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
vtkMultiPieceDataSet* vtkIndexBasedBlockFilter::GetPieceToProcess(vtkDataObject* dObj)
{
  this->CurrentCIndex = 0;
  this->CurrentHIndex = 0;
  this->CurrentHLevel = 0;

  this->Temporary->SetNumberOfPieces(0);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dObj);
  if (cd)
    {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    iter->VisitOnlyLeavesOff();
    iter->SkipEmptyNodesOff();

    vtkHierarchicalBoxDataIterator* hbIter = 
      vtkHierarchicalBoxDataIterator::SafeDownCast(iter);

    for (iter->InitTraversal(); 
      (!iter->IsDoneWithTraversal() && iter->GetCurrentFlatIndex() 
       <= this->CompositeDataSetIndex); 
      iter->GoToNextItem())
      {
      if (iter->GetCurrentFlatIndex() == this->CompositeDataSetIndex)
        {
        vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::SafeDownCast(
          iter->GetCurrentDataObject());
        if (mp && this->FieldType == FIELD)
          {
          return 0;
          }

        if (!mp)
          {
          this->CurrentCIndex = iter->GetCurrentFlatIndex();
          if (hbIter)
            {
            this->CurrentHLevel = hbIter->GetCurrentLevel();
            this->CurrentHIndex = hbIter->GetCurrentIndex();
            }
          this->Temporary->SetNumberOfPieces(1);
          this->Temporary->SetPiece(0, 
            vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()));
          return this->Temporary;
          }

        // Note Current*Index is the index of the first piece in the
        // vtkMultiPieceDataSet that is returned.
        this->CurrentCIndex = iter->GetCurrentFlatIndex() + 1;
        if (hbIter)
          {
          this->CurrentHLevel = hbIter->GetCurrentLevel();
          this->CurrentHIndex = 0;
          }
        return mp;
        }
      }

    return 0;
    }

  this->Temporary->SetNumberOfPieces(1);
  this->Temporary->SetPiece(0, vtkDataSet::SafeDownCast(dObj));
  return this->Temporary;
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataObject* actualInput = vtkDataObject::GetData(inputVector[0], 0);

  // If input is composite dataset then this method will return the dataset in
  // that composite dataset to process.
  vtkMultiPieceDataSet* input = this->GetPieceToProcess(actualInput);

  // Do communication and decide which processes pass what data through.
  if (!this->DetermineBlockIndices(input, this->StartIndex, this->EndIndex))
    {
    return 0;
    }

  if (!input)
    {
    return 1;
    }

  if (this->StartIndex < 0 || this->EndIndex < 0 || this->EndIndex < this->StartIndex)
    {
    // Nothing to do, the output must be empty since this process does not have
    // the requested block of data.
    return 1;
    }

  if (actualInput->IsA("vtkHierarchicalBoxDataSet"))
    {
    this->CompositeIndexArray = vtkUnsignedIntArray::New();
    this->CompositeIndexArray->SetName("vtkCompositeIndexArray");
    this->CompositeIndexArray->SetNumberOfComponents(2);
    }
  else if (actualInput->IsA("vtkCompositeDataSet"))
    {
    this->CompositeIndexArray = vtkUnsignedIntArray::New();
    this->CompositeIndexArray->SetName("vtkCompositeIndexArray");
    this->CompositeIndexArray->SetNumberOfComponents(1);
    }

  // cout << "Block Indices: " << this->StartIndex << ", " << this->EndIndex << endl;
 
  vtkTable* output = vtkTable::GetData(outputVector, 0);
  output->SetRowData(0);

  vtkIdType pieceNumber = 0;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());
  iter->SkipEmptyNodesOff();
  vtkIdType pieceOffset = 0;
  for (iter->InitTraversal(); (!iter->IsDoneWithTraversal() && pieceOffset <= this->EndIndex);
    iter->GoToNextItem(), pieceNumber++)
    {
    vtkDataSet* piece = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!piece)
      {
      continue;
      }

    if (this->FieldType == FIELD)
      {
      this->PassFieldDataBlock(output,
        this->StartIndex, this->EndIndex,
        piece);
      break;
      }
    else
      {
      // Now determine if piece start/end indices.
      this->PassBlock(pieceNumber, output, pieceOffset, piece);
      }
    }

  vtkSmartPointer<vtkDataSetAttributes> fieldData = output->GetRowData();

  if (fieldData.GetPointer() == 0)
    {
    fieldData = vtkSmartPointer<vtkDataSetAttributes>::New();
    }

  if (this->PointCoordinatesArray)
    {
    fieldData->AddArray(this->PointCoordinatesArray);
    this->PointCoordinatesArray->Delete();
    this->PointCoordinatesArray = 0;
    }

  if (this->StructuredCoordinatesArray)
    {
    fieldData->AddArray(this->StructuredCoordinatesArray);
    this->StructuredCoordinatesArray->Delete();
    this->StructuredCoordinatesArray = 0;
    }

  if (this->OriginalIndicesArray)
    {
    // We add OriginalIndicesArray only if the output doesn't already have some
    // other arrays which typically get added by the vtkPVExtractSelection
    // filter.
    fieldData->AddArray(this->OriginalIndicesArray);
    this->OriginalIndicesArray->Delete();
    this->OriginalIndicesArray = 0;
    }

  if (this->CompositeIndexArray)
    {
    fieldData->AddArray(this->CompositeIndexArray);
    this->CompositeIndexArray->Delete();
    this->CompositeIndexArray = 0;
    }

  // Essential to set the field data at the end so that the number of rows count
  // in the vtkTable output is set up correctly. This seems like a bug in
  // vtkTable.
  output->SetRowData(fieldData);
  return 1;
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockFilter::PassFieldDataBlock(vtkTable* output, 
  vtkIdType startIndex, vtkIdType endIndex, vtkDataSet* input)
{
  vtkFieldData* inFD = input->GetFieldData();
  vtkDataSetAttributes* outFD = vtkDataSetAttributes::New();
  outFD->CopyStructure(inFD);
  output->SetRowData(outFD);
  outFD->Delete();

  for (vtkIdType inIndex=startIndex; inIndex <= endIndex; ++inIndex)
    {
    // The arrays can be different sizes so handle each one
    // separately:
    for (vtkIdType i=0; i < inFD->GetNumberOfArrays(); i++)
      {
      vtkAbstractArray* inArray = inFD->GetArray(i);
      vtkAbstractArray* outArray = outFD->GetArray(i);
      if (inIndex >= inArray->GetNumberOfTuples())
        {
        continue;
        }
      // This will grow the array as well.
      outArray->InsertNextTuple(inIndex, inArray);
      }
    }
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockFilter::PassBlock(
  vtkIdType pieceNumber,
  vtkTable* output, 
  vtkIdType &pieceOffset, vtkDataSet* input)
{
  vtkFieldData* inFD = 0;
  vtkIdType numFieldTuples = 0;
  switch (this->FieldType)
    {
  case FIELD:
    // This case should not call this method at all.
    return;
    
  case CELL:
    inFD = input->GetCellData();
    numFieldTuples = inFD->GetNumberOfTuples();
    break;

  case POINT:
  default:
    inFD = input->GetPointData();
    numFieldTuples = input->GetNumberOfPoints();
    }

  // this condition is unnecessary, since the iteration over the pieces breaks
  // before this condition is reached.
  if (pieceOffset > this->EndIndex)
    {
    pieceOffset += numFieldTuples;
    return;
    }

  // this->StartIndex and this->EndIndex are global indicies computed by
  // treating all pieces as appended together. Now we need to compute the
  // offsets for the current piece.
  vtkIdType startIndex = (pieceOffset>this->StartIndex)? 0 : 
    (this->StartIndex-pieceOffset);

  vtkIdType myGlobalEndIndex = pieceOffset+numFieldTuples -1;
  vtkIdType endIndex = (myGlobalEndIndex < this->EndIndex)?
    (numFieldTuples-1) : (this->EndIndex-pieceOffset);

  // cout << pieceNumber << "(" << numFieldTuples << ")"
  //  << ": " << "(" << startIndex << ", " << endIndex << ")" << "+" << pieceOffset << endl;

  if (startIndex >= numFieldTuples)
    {
    // nothing to do there, the offset is beyond what this input possesses.
    pieceOffset += numFieldTuples;
    return;
    }

  vtkDataSetAttributes* outFD = output->GetRowData();
  if (!outFD)
    {
    // initialize structure if this is the first piece.
    outFD = vtkDataSetAttributes::New();
    output->SetRowData(outFD);
    outFD->Delete();
    outFD->CopyStructure(inFD);
    outFD->Allocate(this->EndIndex-this->StartIndex+1);
    }

  vtkPointSet* psInput = vtkPointSet::SafeDownCast(input);
  vtkRectilinearGrid* rgInput = vtkRectilinearGrid::SafeDownCast(input);
  vtkImageData* idInput = vtkImageData::SafeDownCast(input);
  vtkStructuredGrid* sgInput = vtkStructuredGrid::SafeDownCast(input);
  const int* dimensions = 0;
  if (rgInput)
    {
    dimensions = rgInput->GetDimensions();
    }
  else if (idInput)
    {
    dimensions = idInput->GetDimensions();
    }
  else if (sgInput)
    {
    dimensions = sgInput->GetDimensions();
    }

  int cellDims[3];
  if (this->FieldType == CELL && dimensions)
    {
    cellDims[0] = VTK_INDEXBASEDBLOCKFILTER_MAX(1, (dimensions[0] -1));
    cellDims[1] = VTK_INDEXBASEDBLOCKFILTER_MAX(1, (dimensions[1] -1));
    cellDims[2] = VTK_INDEXBASEDBLOCKFILTER_MAX(1, (dimensions[2] -1));
    dimensions = cellDims;
    }

  if (psInput && !this->PointCoordinatesArray &&
    this->FieldType == POINT)
    {
    this->PointCoordinatesArray = vtkDoubleArray::New();
    this->PointCoordinatesArray->SetName("Point Coordinates");
    this->PointCoordinatesArray->SetNumberOfComponents(3);
    this->PointCoordinatesArray->Allocate(this->EndIndex-this->StartIndex+1);
    }

  if (dimensions && !this->StructuredCoordinatesArray)
    {
    // Compute i,j,k from point id.
    this->StructuredCoordinatesArray = vtkIdTypeArray::New();
    this->StructuredCoordinatesArray->SetName("Structured Coordinates");
    this->StructuredCoordinatesArray->SetNumberOfComponents(3);
    this->StructuredCoordinatesArray->Allocate(this->EndIndex-this->StartIndex+1);
    }

  if (!this->OriginalIndicesArray && this->GenerateOriginalIds)
    {
    this->OriginalIndicesArray = vtkIdTypeArray::New();
    this->OriginalIndicesArray->SetName("vtkOriginalIndices");
    this->OriginalIndicesArray->SetNumberOfComponents(1);
    this->OriginalIndicesArray->Allocate(this->EndIndex-this->StartIndex+1);
    }

  if (this->CompositeIndexArray)
    {
    if (this->CompositeIndexArray->GetNumberOfComponents() == 2)
      {
      unsigned int *ptr = this->CompositeIndexArray->WritePointer(
        this->CompositeIndexArray->GetNumberOfTuples()*2,
        (endIndex-startIndex+1)*2);

      for (vtkIdType cc=startIndex; cc <= endIndex; cc++)
        {
        *ptr = this->CurrentHLevel; 
        ptr++;
        *ptr = (this->CurrentHIndex+pieceNumber); 
        ptr++;
        }
      }
    else
      {
      unsigned int *ptr = this->CompositeIndexArray->WritePointer(
        this->CompositeIndexArray->GetNumberOfTuples(),
        endIndex-startIndex+1);
      for (vtkIdType cc=startIndex; cc <= endIndex; cc++)
        {
        *ptr = (this->CurrentCIndex + pieceNumber);
        ptr++;
        }
      }
    }

  // cout << "PassThrough: " << startIndex << " --> " << endIndex << endl;
  for (vtkIdType inIndex = startIndex; inIndex <= endIndex; ++inIndex)
    {
    if (this->OriginalIndicesArray)
      {
      this->OriginalIndicesArray->InsertNextValue(inIndex);
      }
    outFD->InsertNextTuple(inIndex, inFD);

    if (this->FieldType == POINT && psInput)
      {
      this->PointCoordinatesArray->InsertNextTuple(psInput->GetPoint(inIndex));
      }

    if (dimensions)
      {
      // Compute i,j,k from point id.
      vtkIdType tuple[3];
      tuple[0] = (inIndex % dimensions[0]);
      tuple[1] = (inIndex/dimensions[0]) % dimensions[1];
      tuple[2] = (inIndex/(dimensions[0]*dimensions[1]));
      this->StructuredCoordinatesArray->InsertNextTupleValue(tuple);
      }
    }

  pieceOffset += numFieldTuples;
}

//----------------------------------------------------------------------------
bool vtkIndexBasedBlockFilter::DetermineBlockIndices(vtkMultiPieceDataSet* input,
  vtkIdType& startIndex, vtkIdType& endIndex)
{
  // NOTE: input may be NULL.
  startIndex = -1;
  endIndex = -1;

  // These are absolute indices. If all the data was on a single vtkDataSet
  // instance then the data in the index range [blockStartIndex, blockEndIndex] 
  // is what would have been passed through.
  vtkIdType blockStartIndex = this->Block*this->BlockSize;
  vtkIdType blockEndIndex = blockStartIndex + this->BlockSize - 1;
  // cout << "Whole Range: " << blockStartIndex << "--> " << blockEndIndex << endl;

  // Now all the data is not in single vtkDataSet instance and hence the bulk of
  // this method.

  // vtkMultiPieceDataSet is to be treated as a whole dataset append together
  // (with duplicate points and cells). 

  vtkIdType numFields = 0;
  if (input)
    {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(input->NewIterator());

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataSet* piece = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!piece)
        {
        continue;
        }
      switch (this->FieldType)
        {
      case CELL:
        numFields += piece->GetCellData()->GetNumberOfTuples();
        break;

      case FIELD:
          {
          vtkIdType pieceFields = 0;  
          vtkIdType tempNumFields = 0;
          for(vtkIdType i=0; i<input->GetFieldData()->GetNumberOfArrays(); i++)
            { 
            tempNumFields = input->GetFieldData()->GetArray(i)->GetNumberOfTuples();
            pieceFields = tempNumFields > pieceFields? tempNumFields : pieceFields;
            }
          numFields += pieceFields;
          }
        break;

      case POINT:
      default:
        // we use number-of-points and not number-of-tuples in point data, since
        // even if no point data is available, we are passing the point
        // coordinates over.
        numFields += piece->GetNumberOfPoints();
        }
      }
    }


  int numProcs = this->Controller? this->Controller->GetNumberOfProcesses():1;
  if (numProcs<=1)
    {
    startIndex = blockStartIndex;
    endIndex = (blockEndIndex < numFields)? blockEndIndex : (numFields-1);
    // cout  << "Delivering : " << startIndex << " --> " << endIndex << endl;
    return true;
    }

  int myId = this->Controller->GetLocalProcessId();
  vtkCommunicator* comm = this->Controller->GetCommunicator();
  vtkIdType mydataStartIndex=0;

  if (this->FieldType == FIELD)
    {
    // When working with field data, only use the data from one process,
    // hence no communication with other processes is necessary.
    if(myId != this->ProcessID)
      {
      return true;
      }
    }
  else
    {
    vtkIdType* gathered_data = new vtkIdType[numProcs];

    // cout << myId<< ": numFields: " << numFields<<endl;
    if (!comm->AllGather(&numFields, gathered_data, 1))
      {
      vtkErrorMacro("Failed to gather data from all processes.");
      return false;
      }

    for (int cc=0; cc < myId; cc++)
      {
      mydataStartIndex += gathered_data[cc];
      }
    }

  vtkIdType mydataEndIndex = mydataStartIndex + numFields - 1;

  if ((mydataStartIndex < blockStartIndex && mydataEndIndex < blockStartIndex) || 
    (mydataStartIndex > blockEndIndex))
    {
    // Block doesn't overlap the data we have at all.
    // startIndex = -1;
    // endIndex = -1;
    }
  else
    {
    vtkIdType sIndex = (mydataStartIndex < blockStartIndex)?
      blockStartIndex : mydataStartIndex;
    vtkIdType eIndex = (blockEndIndex < mydataEndIndex)?
      blockEndIndex : mydataEndIndex;

    startIndex = sIndex - mydataStartIndex;
    endIndex = eIndex - mydataStartIndex;
    }

  // startIndex and endIndex are the indicies as if all the
  // pieces in the vtkMultiPieceDataSet were appended together (with duplicate
  // cells and points).

  // cout << myId << ": Num OF Tuples: " << numFields << endl;
  // cout << myId << ": Delivering : " << startIndex << " --> " 
  //   << endIndex << endl;
  return true;
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GenerateOriginalIds: " << this->GenerateOriginalIds << endl;
  os << indent << "Block: " << this->Block << endl;
  os << indent << "BlockSize: " << this->BlockSize << endl;
  os << indent << "FieldType: " << this->FieldType << endl;
  os << indent << "ProcessID: " << this->ProcessID << endl;
  os << indent << "CompositeDataSetIndex: " 
    << this->CompositeDataSetIndex << endl;
}


