/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeEditor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAttributeEditor.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkClipVolume.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkIntArray.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkExecutive.h"

#include <math.h>

#include "vtkDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkToolkits.h"
#include "vtkAppendFilter.h"
#include "vtkExtractCells.h"
#include "vtkFieldData.h"
#include "vtkInformationVector.h"
#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#endif

vtkCxxRevisionMacro(vtkAttributeEditor, "1.1");
vtkStandardNewMacro(vtkAttributeEditor);
vtkCxxSetObjectMacro(vtkAttributeEditor,ClipFunction,vtkImplicitFunction);
vtkCxxSetObjectMacro(vtkAttributeEditor,Controller,vtkMultiProcessController);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
vtkAttributeEditor::vtkAttributeEditor(vtkImplicitFunction *cf)
{
  this->ClipFunction = cf;
  this->Locator = NULL;
  this->Value = 0.0;
  this->IsPointPick = 0;

  this->MergeTolerance = 0.01;

  this->AttributeValue = 0.0;
  this->AttributeMode = -1;
  this->EditMode = 0;
  this->UnfilteredDataset = 0;
/*
  this->SetNumberOfOutputPorts(2);
  vtkUnstructuredGrid *output2 = vtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();
*/
  // Point stuff:

  this->SetNumberOfInputPorts(1);
  this->PickCell = 0;
  this->WorldPoint[0] = this->WorldPoint[1] = this->WorldPoint[2] = 0.0;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->PointMap = 0;
  this->RegionPointIds = 0;
  this->BestInputIndex = -1;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
                               vtkDataSetAttributes::SCALARS);

  vtkInformation* info = this->GetInputPortInformation(0);
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(),1);

}

//----------------------------------------------------------------------------
vtkAttributeEditor::~vtkAttributeEditor()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  this->SetClipFunction(NULL);

  this->SetController(0);

}

//----------------------------------------------------------------------------
vtkDataSet *vtkAttributeEditor::GetInput(int idx)
{
  if (idx >= this->GetNumberOfInputConnections(0) || idx < 0)
    {
    return NULL;
    }
  
  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}


//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAttributeEditor::RemoveInput(vtkDataSet *ds)
{
  vtkAlgorithmOutput *algOutput = 0;
  if (ds)
    {
    algOutput = ds->GetProducerPort();
    }

  this->RemoveInputConnection(0, algOutput);
}


//----------------------------------------------------------------------------
//
//
int vtkAttributeEditor::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{

  vtkDataSet *readerInput;
  vtkDataSet *filterInput;
  vtkUnstructuredGrid *readerOutput;
  vtkUnstructuredGrid *filterOutput;

  // get the info objects
  vtkInformation *filterInputInfo = inputVector[0]->GetInformationObject(0);
  // get the input
  filterInput = vtkDataSet::SafeDownCast(
    filterInputInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (filterInput == NULL)
    {
    return 1;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  filterOutput = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // for each point or cell in the clipped output we want to find its counterpart in the real output, and set the saliency field value according to AttributeValue (hardcode for now)
  // can you access a point's field data directly?

  vtkPointData *inPD=filterInput->GetPointData();
  vtkCellData *inCD=filterInput->GetCellData();
  vtkFieldData *inFD=filterInput->GetFieldData();

  // filter must have been initialized for the second input to be valid
  vtkInformation *readerInputInfo = inputVector[0]->GetInformationObject(1);
  if(!readerInputInfo)
    {
    // either the two inputs are the same reader, or second input is invalid
    readerInput = filterInput;
    readerOutput = filterOutput;
    }
  else
    {
    readerInput = vtkDataSet::SafeDownCast(
      readerInputInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (readerInputInfo == NULL)
      {
      return 1;
      }
//    readerOutput = vtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(1));

    readerOutput = vtkUnstructuredGrid::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    }

  vtkPointData *myPD = vtkPointData::New();
  myPD->ShallowCopy(readerInput->GetPointData());
  vtkCellData *myCD = vtkCellData::New();
  myCD->ShallowCopy(readerInput->GetCellData());
//  vtkFloatArray *filterArray = vtkFloatArray::SafeDownCast(this->GetInputArrayToProcess(0,inputVector));
//  vtkFloatArray *readerArray = vtkFloatArray::SafeDownCast(this->GetInputArrayToProcess(0,1,inputVector));

/*
  vtkInformation *info = this->GetInputArrayInformation(0);
//  vtkFloatArray *farray = vtkFloatArray::SafeDownCast(inPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
//  vtkFloatArray *rarray = vtkFloatArray::SafeDownCast(myPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
  vtkDataArray *farray = inPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME()));
  vtkDataArray *rarray = myPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME()));

  int type = info->Get(vtkDataObject::FIELD_ARRAY_TYPE());
  switch( type )
   {
    vtkTemplateMacro10(vtkContourGridExecute, this, input, output, inScalars,
                       (VTK_TT *)(scalarArrayPtr), numContours, values,
                       computeScalars, useScalarTree, 
                       scalarTree);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 1;
    }
*/


  // when edit mode is off, either this is the first execution of filter, the source view has been modified, or source view is on
  if(this->EditMode==0)
    {
    if(this->UnfilteredDataset)
      {
      readerOutput->CopyStructure(readerInput);
      readerOutput->GetPointData()->PassData( myPD );
      readerOutput->GetCellData()->PassData ( myCD );
      readerOutput->GetFieldData()->PassData ( readerInput->GetFieldData() );
      }
    else
      {
      filterOutput->CopyStructure(filterInput);
      filterOutput->GetPointData()->PassData( inPD );
      filterOutput->GetCellData()->PassData ( inCD );
      filterOutput->GetFieldData()->PassData ( inFD );
      }

    myPD->Delete();
    myCD->Delete();

    return 1;
    }

  this->EditMode = 0;

  if(this->IsPointPick)
    {
    this->BestInputIndex = -1;

    if (this->PickCell)
      {
      ////////////
      // PICK CELL
      ////////////

      this->CellExecute(readerInput,filterInput);
      this->DeletePointMap();
      }
    else
      {
      ////////////
      // PICK NODE
      ////////////

      this->PointExecute(readerInput,filterInput);
      this->DeletePointMap();
      }
    }
  else
    {
    ////////////
    // BOX PICK
    ////////////

    this->BestInputIndex = 0;

    //this->BoxExecute(readerInput,filterInput);

    vtkDataSet *finput = filterInput;
    vtkDataSet *rinput = readerInput;
    vtkPoints *newPoints;
    double s;
    vtkIdType i;
    double *coords;
    vtkIdType numPts = finput->GetNumberOfPoints();
    vtkIdType numCells = finput->GetNumberOfCells();
    //vtkPointData *inPD=finput->GetPointData();
    //vtkCellData *inCD=finput->GetCellData();
    //vtkFieldData *inFD=finput->GetFieldData();
    //vtkPointData *myPD = vtkPointData::New();
    //myPD->ShallowCopy(rinput->GetPointData());
    //vtkCellData *myCD = vtkCellData::New();
    //myCD->ShallowCopy(rinput->GetCellData());

    vtkInformation *info = this->GetInputArrayInformation(0);

    vtkDebugMacro(<< "Clipping dataset");
    
    // Initialize self; create output objects
    //
    if ( numPts < 1 )
      {
      //vtkErrorMacro(<<"No data to clip");
      return 0;
      }

    if ( !this->ClipFunction )
      {
      vtkErrorMacro(<<"No clip function defined");
      return 0;
      }

    newPoints = vtkPoints::New();
    newPoints->Allocate(numPts,numPts/2);
    
    // locator used to merge potentially duplicate points
    if ( this->Locator == NULL )
      {
      this->CreateDefaultLocator();
      }
    this->Locator->InitPointInsertion (newPoints, finput->GetBounds());

    if ( this->ClipFunction )
      {
      vtkIdType readerId;
      if(info->Get(vtkDataObject::FIELD_ASSOCIATION()) == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        {
        vtkFloatArray *farray = vtkFloatArray::SafeDownCast(inPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
        vtkFloatArray *rarray = vtkFloatArray::SafeDownCast(myPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));

        for ( i=0; i < numPts; i++ )
          {
          coords = finput->GetPoint(i);
          s = this->ClipFunction->FunctionValue(coords);
          readerId = rinput->FindPoint(coords);
          if(this->Value > s )
            {
            farray->SetValue(i,this->AttributeValue);
            rarray->SetValue(readerId,this->AttributeValue);
            }
          }
        }
      else if(info->Get(vtkDataObject::FIELD_ASSOCIATION()) == vtkDataObject::FIELD_ASSOCIATION_CELLS)
        {
        vtkFloatArray *farray = vtkFloatArray::SafeDownCast(inCD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
        vtkFloatArray *rarray = vtkFloatArray::SafeDownCast(myCD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
        vtkCell *cell;
        int subId = 0;
        double pcoords[3];
        double *w = new double[finput->GetMaxCellSize()];

        for ( i=0; i < numCells; i++ )
          {
          cell = finput->GetCell(i);
          coords = finput->GetPoint(cell->GetPointId(0));
          s = this->ClipFunction->FunctionValue(coords);
          readerId = rinput->FindCell(coords,NULL,-1,0.0,subId,pcoords,w);
          if(this->Value > s )
            {
            farray->SetValue(i,this->AttributeValue);
            rarray->SetValue(readerId,this->AttributeValue);
            }
          }
        }
      }

    newPoints->Delete();
    this->Locator->Initialize();//release any extra memory
  // output->Squeeze();

    }

  if(this->UnfilteredDataset)
    {
    readerOutput->CopyStructure(readerInput);
    readerOutput->GetPointData()->PassData( myPD );
    readerOutput->GetCellData()->PassData ( myCD );
    readerOutput->GetFieldData()->PassData ( readerInput->GetFieldData() );
    }
  else
    {
    filterOutput->CopyStructure(filterInput);
    filterOutput->GetPointData()->PassData( inPD );
    filterOutput->GetCellData()->PassData ( inCD );
    filterOutput->GetFieldData()->PassData ( inFD );
    }

  myPD->Delete();
  myCD->Delete();

  return 1;

}

void vtkAttributeEditor::BoxExecute(vtkDataSet *rinput,vtkDataSet *finput)
{
  vtkPoints *newPoints;
  double s;
  vtkIdType i;
  double *coords;
  vtkIdType numPts = finput->GetNumberOfPoints();
  vtkIdType numCells = finput->GetNumberOfCells();
  vtkPointData *inPD=finput->GetPointData();
  vtkCellData *inCD=finput->GetCellData();
  vtkPointData *myPD = vtkPointData::New();
  myPD->ShallowCopy(rinput->GetPointData());
  vtkCellData *myCD = vtkCellData::New();
  myCD->ShallowCopy(rinput->GetCellData());

  vtkInformation *info = this->GetInputArrayInformation(0);

  vtkDebugMacro(<< "Clipping dataset");
  
  // Initialize self; create output objects
  //
  if ( numPts < 1 )
    {
    //vtkErrorMacro(<<"No data to clip");
    return;
    }

  if ( !this->ClipFunction )
    {
    vtkErrorMacro(<<"No clip function defined");
    return;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts,numPts/2);
  
  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPoints, finput->GetBounds());

  if ( this->ClipFunction )
    {
    vtkIdType readerId;
    if(info->Get(vtkDataObject::FIELD_ASSOCIATION()) == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      vtkFloatArray *farray = vtkFloatArray::SafeDownCast(inPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
      vtkFloatArray *rarray = vtkFloatArray::SafeDownCast(myPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));

      for ( i=0; i < numPts; i++ )
        {
        coords = finput->GetPoint(i);
        s = this->ClipFunction->FunctionValue(coords);
        readerId = rinput->FindPoint(coords);
        if(this->Value > s )
          {
          farray->SetValue(i,this->AttributeValue);
          rarray->SetValue(readerId,this->AttributeValue);
          }
        }
      }
    else if(info->Get(vtkDataObject::FIELD_ASSOCIATION()) == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      {
      vtkFloatArray *farray = vtkFloatArray::SafeDownCast(inCD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
      vtkFloatArray *rarray = vtkFloatArray::SafeDownCast(myCD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
      vtkCell *cell;
      int subId = 0;
      double pcoords[3];
      double *w = new double[finput->GetMaxCellSize()];

      for ( i=0; i < numCells; i++ )
        {
        cell = finput->GetCell(i);
        coords = finput->GetPoint(cell->GetPointId(0));
        s = this->ClipFunction->FunctionValue(coords);
        readerId = rinput->FindCell(coords,NULL,-1,0.0,subId,pcoords,w);
        if(this->Value > s )
          {
          farray->SetValue(i,this->AttributeValue);
          rarray->SetValue(readerId,this->AttributeValue);
          }
        }
      }
    }

  newPoints->Delete();
  this->Locator->Initialize();//release any extra memory
 // output->Squeeze();

}


//-----------------------------------------------------------------------------
void vtkAttributeEditor::PointExecute(vtkDataSet *rinput,vtkDataSet *finput)
{
  double pt[3];
  double distance2;
  double bestPt[3];
  double bestDistance2;
  vtkIdType bestId = 0;
  double tmp;
  int numInputs = this->GetExecutive()->GetNumberOfInputPorts();
  int inputIdx;
  vtkIdType numPts = 0, ptId;
  double *coords;

// for each point or cell in the clipped output we want to find its counterpart in the real output, and set the saliency field value according to AttributeValue (hardcode for now)
// can you access a point's field data directly?

  numPts = finput->GetNumberOfPoints();
  vtkPointData *inPD=finput->GetPointData();
  vtkPointData *myPD = vtkPointData::New();
  myPD->ShallowCopy(rinput->GetPointData());
  vtkCellData *myCD = vtkCellData::New();
  myCD->ShallowCopy(rinput->GetCellData());

  vtkInformation *info = this->GetInputArrayInformation(0);
  vtkFloatArray *farray = vtkFloatArray::SafeDownCast(inPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));
  vtkFloatArray *rarray = vtkFloatArray::SafeDownCast(myPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));

  // Find the nearest point in the input.
  bestDistance2 = VTK_LARGE_FLOAT;
  this->BestInputIndex = -1;

  for (inputIdx = 0; inputIdx < numInputs; ++inputIdx)
    {
    finput = (vtkDataSet *)this->GetInput(inputIdx);
    numPts = finput->GetNumberOfPoints();
    for (ptId = 0; ptId < numPts; ++ptId)
      {
      finput->GetPoint(ptId, pt);
      tmp = pt[0]-this->WorldPoint[0];
      distance2 = tmp*tmp;
      tmp = pt[1]-this->WorldPoint[1];
      distance2 += tmp*tmp;
      tmp = pt[2]-this->WorldPoint[2];
      distance2 += tmp*tmp;
      if (distance2 < bestDistance2)
        {
        bestId = ptId;
        this->BestInputIndex = inputIdx;
        bestDistance2 = distance2;
        bestPt[0] = pt[0];
        bestPt[1] = pt[1];
        bestPt[2] = pt[2];
        }
      }
    }
/*
  // Keep only the best seed among the processes.
  vtkIdList* regionCellIds = vtkIdList::New();
  if ( ! this->CompareProcesses(bestDistance2) && numPts > 0)
    {
    // Only one point in map.
    this->InitializePointMap(
          this->GetInput(this->BestInputIndex)->GetNumberOfPoints());
    this->InsertIdInPointMap(bestId);
    }
*/
  
  coords = finput->GetPoint(bestId);
  vtkIdType readerId;
  readerId = rinput->FindPoint(coords);
  farray->SetValue(bestId,this->AttributeValue);
  rarray->SetValue(readerId,this->AttributeValue);

  myPD->Delete();
  myCD->Delete();
//  myFD->Delete();
//  this->CreateOutput(regionCellIds);
//  regionCellIds->Delete();
}

//-----------------------------------------------------------------------------
void vtkAttributeEditor::CellExecute(vtkDataSet *rinput,vtkDataSet *finput)
{
  // Loop over all of the cells.
  int numInputs, inputIdx;
  vtkIdType cellId;
  vtkCell* cell;
  int inside;
  double closestPoint[3];
  int  subId;
  double pcoords[3];
  double dist2;
  double bestDist2 = VTK_LARGE_FLOAT;
  double* weights;
  vtkIdType numCells;
  vtkIdType bestId = -1;

  vtkPointData *inPD=finput->GetPointData();
  vtkPointData *myPD = vtkPointData::New();
  myPD->ShallowCopy(rinput->GetPointData());
  vtkCellData *myCD = vtkCellData::New();
  myCD->ShallowCopy(rinput->GetCellData());

  vtkInformation *info = this->GetInputArrayInformation(0);
  vtkFloatArray *farray = vtkFloatArray::SafeDownCast(inPD->GetScalars(info->Get(vtkDataObject::FIELD_NAME())));

  numInputs = this->GetExecutive()->GetNumberOfInputPorts();

  for (inputIdx = 0; inputIdx < numInputs; ++inputIdx)
    {
    finput = (vtkDataSet *)this->GetInput(0);
    weights = new double[finput->GetMaxCellSize()];
    numCells = finput->GetNumberOfCells();
    for (cellId=0; cellId < numCells; cellId++)
      {
      cell = finput->GetCell(cellId);
      inside = cell->EvaluatePosition(this->WorldPoint, closestPoint, 
                                      subId, pcoords, dist2, weights);
      // Inside does not work the way I thought for 2D cells.
      //if (inside)
      //  {
      //  dist2= 0.0;
      //  }
      if (inside != -1 && dist2 < bestDist2)
        {
        bestId = cellId;
        bestDist2 = dist2;
        this->BestInputIndex = inputIdx;
        }
      }
    delete [] weights;
    weights = NULL;
    }

  // Keep only the best seed cell among the processes.
  vtkIdList* regionCellIds = vtkIdList::New();
  if ( ! this->CompareProcesses(bestDist2) && bestId >= 0)
    {
    finput = (vtkDataSet *)this->GetInput(this->BestInputIndex);
    this->InitializePointMap(finput->GetNumberOfPoints());
    regionCellIds->InsertNextId(bestId);
    // Insert the cell points.
    vtkIdList* cellPtIds = vtkIdList::New();
    finput->GetCellPoints(bestId, cellPtIds);
    vtkIdType i;
    for (i = 0; i < cellPtIds->GetNumberOfIds(); ++i)
      {
      this->InsertIdInPointMap(cellPtIds->GetId(i));
      }
    cellPtIds->Delete();
    }

  farray->SetValue(bestId,this->AttributeValue);
/*
  output->CopyStructure(input);
  outPD->PassData( myPD );
  output->GetCellData()->PassData ( myCD );
*/

//  this->CreateOutput(regionCellIds);
  regionCellIds->Delete();
  myPD->Delete();
  myCD->Delete();

}

//-----------------------------------------------------------------------------
vtkIdType vtkAttributeEditor::InsertIdInPointMap(vtkIdType inId)
{
  vtkIdType outId;
  outId = this->PointMap->GetId(inId);
  if (outId >= 0)
    {
    return outId;
    }
  outId = this->RegionPointIds->GetNumberOfIds();
  this->PointMap->SetId(inId, outId);
  this->RegionPointIds->InsertNextId(inId);
  return outId;
}

//-----------------------------------------------------------------------------
void vtkAttributeEditor::InitializePointMap(vtkIdType numberOfInputPoints)
{
  if (this->PointMap)
    {
    this->DeletePointMap();
    }
  this->PointMap = vtkIdList::New();
  this->PointMap->Allocate(numberOfInputPoints);
  this->RegionPointIds = vtkIdList::New();

  vtkIdType i;
  for (i = 0; i < numberOfInputPoints; ++i)
    {
    this->PointMap->InsertId(i, -1);
    }
}

//-----------------------------------------------------------------------------
void vtkAttributeEditor::DeletePointMap()
{
  if (this->PointMap)
    {
    this->PointMap->Delete();
    this->PointMap = NULL;
    }
  if (this->RegionPointIds)
    {
    this->RegionPointIds->Delete();
    this->RegionPointIds = NULL;
    }
}

//-----------------------------------------------------------------------------
// I made this general so we could grow the region from the seed.
void vtkAttributeEditor::CreateOutput(vtkIdList* regionCellIds)
{
  if (this->BestInputIndex < 0 || this->RegionPointIds == 0)
    {
    return;
    }
  vtkDataSet* input = (vtkDataSet *)this->GetInput(this->BestInputIndex);
  vtkUnstructuredGrid* output = this->GetOutput();
  double pt[3];
  // Preserve the original Ids.
  // Us int here because mapper has a problem with vtkIdTypeArray.
  vtkIntArray* cellIds = vtkIntArray::New();
  vtkIntArray* ptIds = vtkIntArray::New();

  // First copy the points.
  vtkPoints* newPoints = vtkPoints::New();
  vtkIdType numPts, outId, inId;
  numPts = this->RegionPointIds->GetNumberOfIds();
  newPoints->Allocate(numPts);
  output->GetPointData()->CopyAllocate(input->GetPointData(), numPts);
  ptIds->Allocate(numPts);
  for (outId = 0; outId < numPts; ++outId)
    {
    inId = this->RegionPointIds->GetId(outId);
    ptIds->InsertNextValue((int)inId);
    input->GetPoint(inId, pt);
    newPoints->InsertNextPoint(pt[0], pt[1], pt[2]);
    output->GetPointData()->CopyData(input->GetPointData(), inId, outId);
    }
  output->SetPoints(newPoints);
  newPoints->Delete();
  newPoints = NULL;

  // Now copy the cells.
  vtkIdList* inCellPtIds = vtkIdList::New();
  vtkIdList* outCellPtIds = vtkIdList::New();    
  vtkIdType numCells = regionCellIds->GetNumberOfIds();
  output->Allocate(numCells);
  cellIds->Allocate(numCells);
  output->GetCellData()->CopyAllocate(input->GetCellData(), numCells);
  vtkIdType num, i;
  for (outId = 0; outId < numCells; ++outId)
    {
    inId = regionCellIds->GetId(outId);
    cellIds->InsertNextValue((int)(inId));
    input->GetCellPoints(inId, inCellPtIds);
    // Translate the cell to output point ids.
    num = inCellPtIds->GetNumberOfIds();
    outCellPtIds->Initialize();
    outCellPtIds->Allocate(num);
    for (i = 0; i < num; ++i)
      {
      outCellPtIds->InsertId(i, this->PointMap->GetId(inCellPtIds->GetId(i)));
      }
    output->InsertNextCell(input->GetCellType(inId), outCellPtIds);
    output->GetCellData()->CopyData(input->GetCellData(), inId, outId);
    }

  inCellPtIds->Delete();
  outCellPtIds->Delete();

  cellIds->SetName("Id");
  output->GetCellData()->AddArray(cellIds);
  cellIds->Delete();
  cellIds = NULL;
  ptIds->SetName("Id");
  output->GetPointData()->AddArray(ptIds);
  ptIds->Delete();
  ptIds = NULL;
  
  // Add an array that shows which part this point comes from.
  if (this->GetExecutive()->GetNumberOfInputPorts() > 1)
    {
    if (this->PickCell)
      {
      vtkIntArray* partArray = vtkIntArray::New();
      // There should only be one cell, but ...
      vtkIdType id;
      num = output->GetNumberOfCells();
      partArray->SetNumberOfTuples(num);
      for (id = 0; id < num; ++id)
        {
        partArray->SetComponent(id, 0, this->BestInputIndex);
        }
      partArray->SetName("PartIndex");
      this->GetOutput()->GetCellData()->AddArray(partArray);
      partArray->Delete();
      partArray = 0;
      }
    else
      {
      vtkIntArray* partArray = vtkIntArray::New();
      // There should only be one cell, but ...
      vtkIdType id;
      num = output->GetNumberOfPoints();
      partArray->SetNumberOfTuples(num);
      for (id = 0; id < num; ++id)
        {
        partArray->SetComponent(id, 0, this->BestInputIndex);
        }
      partArray->SetName("PartIndex");
      this->GetOutput()->GetPointData()->AddArray(partArray);
      partArray->Delete();
      partArray = 0;
      }
    }
}


//-----------------------------------------------------------------------------
int vtkAttributeEditor::ListContainsId(vtkIdList* ids, vtkIdType id)
{
  vtkIdType i, num;

  // Although this test causes a n^2 cost, the regions will be small.
  // The alternative is to have a table based on the input cell id.
  // Since inputs can be very large, the memory cost would be high.
  num = ids->GetNumberOfIds();
  for (i = 0; i < num; ++i)
    {
    if (id == ids->GetId(i))
      {
      return 1;
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
vtkIdType vtkAttributeEditor::FindPointId(double pt[3], vtkDataSet* input)
{
  double bounds[6];
  double pt2[3];
  double tol;
  double xMin, xMax, yMin, yMax, zMin, zMax;
  //int fixme;  // make a fast version for image and rectilinear grid.
  vtkIdType i, num;

  input->GetBounds(bounds);
  tol = (bounds[5]-bounds[4])+(bounds[3]-bounds[2])+(bounds[1]-bounds[0]);
  tol *= 0.0000001;
  xMin = pt[0]-tol;
  xMax = pt[0]+tol;
  yMin = pt[1]-tol;
  yMax = pt[1]+tol;
  zMin = pt[2]-tol;
  zMax = pt[2]+tol;
  num = input->GetNumberOfPoints();
  for (i = 0; i < num; ++i)
    {
    input->GetPoint(i, pt2);
    if (pt2[0] > xMin && pt2[0] < xMax && 
        pt2[1] > yMin && pt2[1] < yMax && 
        pt2[2] > zMin && pt2[2] < zMax) 
      {
      return i;
      }
    }
  return -1;
}


//----------------------------------------------------------------------------
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
unsigned long vtkAttributeEditor::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if ( this->ClipFunction != NULL )
    {
    time = this->ClipFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}


void vtkAttributeEditor::SetPickFunction(vtkObject *func)
{
  vtkImplicitFunction *imp;

  if( (imp = vtkImplicitFunction::SafeDownCast(func)) )
    {
    this->IsPointPick = 0;
    this->SetClipFunction(imp);
    }
  else
    {
    this->IsPointPick = 1;
    }
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkAttributeEditor::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator)
    {
    return;
    }
  
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }

  if ( locator )
    {
    locator->Register(this);
    }

  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAttributeEditor::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkAttributeEditor::ClipVolume(vtkDataSet *input, vtkUnstructuredGrid *output)
{
  vtkClipVolume *clipVolume = vtkClipVolume::New();
  
  // We cannot set the input directly.  This messes up the partitioning.
  // output->UpdateNumberOfPieces gets set to 1.
  vtkImageData* tmp = vtkImageData::New();
  tmp->ShallowCopy(vtkImageData::SafeDownCast(input));
  
  clipVolume->SetInput(tmp);
  clipVolume->SetValue(this->Value);
//  clipVolume->SetInsideOut(this->InsideOut);
  clipVolume->SetClipFunction(this->ClipFunction);
  clipVolume->SetMergeTolerance(this->MergeTolerance);
  clipVolume->SetDebug(this->Debug);
  clipVolume->Update();
  vtkUnstructuredGrid *clipOutput = clipVolume->GetOutput();

  output->CopyStructure(clipOutput);
  output->GetPointData()->ShallowCopy(clipOutput->GetPointData());
  output->GetCellData()->ShallowCopy(clipOutput->GetCellData());
  clipVolume->Delete();
  tmp->Delete();
}

// Return the method for manipulating scalar data as a string.
const char *vtkAttributeEditor::GetAttributeModeAsString(void)
{
  if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT )
    {
    return "Default";
    }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
    {
    return "UsePointData";
    }
  else 
    {
    return "UseCellData";
    }
}


//-----------------------------------------------------------------------------
int vtkAttributeEditor::CompareProcesses(double bestDist2)
{
  if (this->Controller == NULL)
    {
    return 0;
    }

  double dist2;
  int bestProc = 0;
  // Every process send their best distance to process 0.
  int myId = this->Controller->GetLocalProcessId();
  if (myId == 0)
    {
    int numProcs = this->Controller->GetNumberOfProcesses();
    int idx;
    for (idx = 1; idx < numProcs; ++idx)
      {
      this->Controller->Receive(&dist2, 1 ,idx, 234099);
      if (dist2 < bestDist2)
        {
        bestDist2 = dist2;
        bestProc = idx;
        }
      }
    // Send the result back to all the processes.
    for (idx = 1; idx < numProcs; ++idx)
      {
      this->Controller->Send(&bestProc, 1, idx, 234100);
      }
    }
  else
    { // Other processes.
    this->Controller->Send(&bestDist2, 1, 0, 234099);
    this->Controller->Receive(&bestProc, 1, 0, 234100);
    }
  if (myId != bestProc)
    { // Return without creating an output.
    return 1;
    }

  return 0;
}


//----------------------------------------------------------------------------
int vtkAttributeEditor::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}


//----------------------------------------------------------------------------
void vtkAttributeEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  if ( this->ClipFunction )
    {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
    }
  else
    {
    os << indent << "Clip Function: (none)\n";
    }
  os << indent << "Value: " << this->Value << "\n";
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
  os << indent << "WorldPoint: " 
     << this->WorldPoint[0] << ", " << this->WorldPoint[1] << ", " 
     << this->WorldPoint[2] << endl;
  
  os << indent << "Pick: "
     << (this->PickCell ? "Cell" : "Point")
     << endl;

  os << indent << "SetUnfilteredDataset" << this->GetUnfilteredDataset() << endl;
  os << indent << "SetValue" << this->GetValue() << endl;
  os << indent << "SetAttributeMode" << this->GetAttributeMode() << endl;
  os << indent << "SetAttributeValue" << this->GetAttributeValue() << endl;
  os << indent << "SetEditMode" << this->GetEditMode() << endl;


}
