/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrowGlyphFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkArrowGlyphFilter.h"

#include "vtkArrowSource.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMaskPoints.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkArrowGlyphFilter);
vtkCxxSetObjectMacro(vtkArrowGlyphFilter, ArrowSourceObject, vtkArrowSource);

//----------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkArrowGlyphFilter::vtkArrowGlyphFilter()
{
  this->ScaleByOrientationVectorMagnitude = 1;
  this->OrientationVectorArray = NULL;
  //
  this->ScaleFactor = 1.0;
  this->ScaleArray = NULL;
  //
  this->ShaftRadiusFactor = 1.0;
  this->ShaftRadiusArray = NULL;
  //
  this->TipRadiusFactor = 1.0;
  this->TipRadiusArray = NULL;
  //
  this->MaskPoints = vtkMaskPoints::New();
  this->RandomMode = this->MaskPoints->GetRandomMode();
  this->MaximumNumberOfPoints = 5000;
  //  this->NumberOfProcesses = vtkMultiProcessController::GetGlobalController() ?
  //    vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() : 1;
  this->UseMaskPoints = 1;
  //
  this->SetNumberOfInputPorts(1);
  //
  this->ArrowSourceObject = NULL; // vtkSmartPointer<vtkArrowSource>::New();
}

//----------------------------------------------------------------------------
vtkArrowGlyphFilter::~vtkArrowGlyphFilter()
{
  if (this->OrientationVectorArray)
  {
    delete[] OrientationVectorArray;
  }
  if (this->ScaleArray)
  {
    delete[] ScaleArray;
  }
  if (this->ShaftRadiusArray)
  {
    delete[] ShaftRadiusArray;
  }
  if (this->TipRadiusArray)
  {
    delete[] TipRadiusArray;
  }
  if (this->MaskPoints)
  {
    this->MaskPoints->Delete();
  }
  this->SetArrowSourceObject(NULL);
}

//----------------------------------------------------------------------------
vtkMTimeType vtkArrowGlyphFilter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;
  if (this->ArrowSourceObject != NULL)
  {
    time = this->ArrowSourceObject->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//-----------------------------------------------------------------------------
void vtkArrowGlyphFilter::SetRandomMode(int mode)
{
  if (mode == this->MaskPoints->GetRandomMode())
  {
    // no change
    return;
  }
  // Store random mode to so that we don't have to call
  // MaskPoints->GetRandomMode() in tight loop.
  this->MaskPoints->SetRandomMode(mode);
  this->RandomMode = mode;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkArrowGlyphFilter::GetRandomMode()
{
  return this->MaskPoints->GetRandomMode();
}

//-----------------------------------------------------------------------------
void vtkArrowGlyphFilter::SetUseMaskPoints(int useMaskPoints)
{
  if (useMaskPoints == this->UseMaskPoints)
  {
    return;
  }
  this->UseMaskPoints = useMaskPoints;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkIdType vtkArrowGlyphFilter::GatherTotalNumberOfPoints(vtkIdType localNumPts)
{
  // Although this is not perfectly process invariant, it is better
  // than we had before (divide by number of processes).
  vtkIdType totalNumPts = localNumPts;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller)
  {
    vtkIdType tmp;
    // This could be done much easier with MPI specific calls.
    if (controller->GetLocalProcessId() == 0)
    {
      int i;
      // Sum points on all processes.
      for (i = 1; i < controller->GetNumberOfProcesses(); ++i)
      {
        controller->Receive(&tmp, 1, i, GlyphNPointsGather);
        totalNumPts += tmp;
      }
      // Send results back to all processes.
      for (i = 1; i < controller->GetNumberOfProcesses(); ++i)
      {
        controller->Send(&totalNumPts, 1, i, GlyphNPointsScatter);
      }
    }
    else
    {
      controller->Send(&localNumPts, 1, 0, GlyphNPointsGather);
      controller->Receive(&totalNumPts, 1, 0, GlyphNPointsScatter);
    }
  }

  return totalNumPts;
}

//----------------------------------------------------------------------------
int vtkArrowGlyphFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!dsInput)
  {
    if (input)
    {
      vtkErrorMacro("This filter cannot process input of type: " << input->GetClassName());
    }
    return 0;
  }

  // Glyph a subset.
  vtkIdType maxNumPts = this->MaximumNumberOfPoints;
  vtkIdType numPts = dsInput->GetNumberOfPoints();
  vtkIdType totalNumPts = this->GatherTotalNumberOfPoints(numPts);

  // What fraction of the points will this processes get allocated?
  maxNumPts = (vtkIdType)((double)(maxNumPts) * (double)(numPts) / (double)(totalNumPts));

  maxNumPts = (maxNumPts < 1) ? 1 : maxNumPts;

  vtkInformationVector* inputVs[2];

  vtkInformationVector* inputV = inputVector[0];
  inputVs[0] = vtkInformationVector::New();
  inputVs[0]->SetNumberOfInformationObjects(1);
  vtkInformation* newInInfo = vtkInformation::New();
  newInInfo->Copy(inputV->GetInformationObject(0));
  inputVs[0]->SetInformationObject(0, newInInfo);
  newInInfo->Delete();
  inputVs[1] = inputVector[1];

  int retVal = this->MaskAndExecute(numPts, maxNumPts, dsInput, request, inputVs, outputVector);

  inputVs[0]->Delete();
  return retVal;
}

//----------------------------------------------------------------------------
int vtkArrowGlyphFilter::MaskAndExecute(vtkIdType numPts, vtkIdType maxNumPts, vtkDataSet* input,
  vtkInformation* vtkNotUsed(request), vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)

{
  // ------
  // Use some code from vtkGlyp3D to setup stuff
  // ------

  //
  // shallow copy input so that internal pipeline doesn't trash information
  // pass input into maskfilter and update
  //
  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->MaskPoints->SetInputData(inputCopy);
  inputCopy->Delete();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->MaskPoints->SetMaximumNumberOfPoints(maxNumPts);
  this->MaskPoints->SetOnRatio(numPts / maxNumPts);

  vtkInformation* maskPointsInfo = this->MaskPoints->GetExecutive()->GetOutputInformation(0);
  maskPointsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  maskPointsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  maskPointsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  this->MaskPoints->Update();

  // How many points will we be glyphing (in this process)
  vtkPoints* maskedpoints = this->MaskPoints->GetOutput()->GetPoints();
  vtkIdType numMaskedPoints = maskedpoints->GetNumberOfPoints();

  // ------
  // Now we insert the new code specially for our arrow filter
  // ------

  //
  // get the input and output
  //
  vtkDataSet* minput = this->MaskPoints->GetOutput();
  vtkPointData* inPd = minput->GetPointData();

  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData* outPd = output->GetPointData();

  //
  // If no Arrow Source was supplied, instantiate a default one
  //
  if (!this->ArrowSourceObject)
  {
    vtkSmartPointer<vtkArrowSource> arrow = this->ArrowSourceObject->NewInstance();
    this->SetArrowSourceObject(arrow);
  }
  // Make sure its up-to-date so we get the num points from it correctly
  this->ArrowSourceObject->Update();

  //
  // We won't modify the arrow source provided (that the users sees in the GUI),
  // we'll use a private copy for our work.
  //
  vtkSmartPointer<vtkArrowSource> internalArrow = this->ArrowSourceObject->NewInstance();
  internalArrow->SetTipLength(this->ArrowSourceObject->GetTipLength());
  internalArrow->SetTipRadius(this->ArrowSourceObject->GetTipRadius());
  internalArrow->SetTipResolution(this->ArrowSourceObject->GetTipResolution());
  internalArrow->SetShaftRadius(this->ArrowSourceObject->GetShaftRadius());
  internalArrow->SetShaftResolution(this->ArrowSourceObject->GetShaftResolution());
  internalArrow->SetInvert(this->ArrowSourceObject->GetInvert());
  internalArrow->Update();

  double Ashaftradius = this->ArrowSourceObject->GetShaftRadius();
  double Atipradius = this->ArrowSourceObject->GetTipRadius();
  // Not used: double   Atiplength = this->ArrowSourceObject->GetTipLength();

  // and get useful information from it
  vtkPolyData* arrow = internalArrow->GetOutput();
  vtkPoints* arrowpoints = arrow->GetPoints();
  vtkIdType numArrowPoints = arrowpoints->GetNumberOfPoints();

  //
  // Find the arrays to be used for Scale/ShaftRadius/etc
  // if not present, we will use default values based on particle size
  //
  vtkDataArray* orientdata = this->OrientationVectorArray
    ? minput->GetPointData()->GetArray(this->OrientationVectorArray)
    : NULL;
  vtkDataArray* scaledata =
    this->ScaleArray ? minput->GetPointData()->GetArray(this->ScaleArray) : NULL;
  vtkDataArray* shaftradiusdata =
    this->ShaftRadiusArray ? minput->GetPointData()->GetArray(this->ShaftRadiusArray) : NULL;
  vtkDataArray* tipradiusdata =
    this->TipRadiusArray ? minput->GetPointData()->GetArray(this->TipRadiusArray) : NULL;
  bool orientMagnitude = false;
  bool shaftradiusMagnitude = false;
  bool tipradiusMagnitude = false;
  bool scaleMagnitude = false;
  //
  if (orientdata && this->ScaleByOrientationVectorMagnitude)
  {
    orientMagnitude = true;
  }
  if (shaftradiusdata && shaftradiusdata->GetNumberOfComponents() == 3)
  {
    shaftradiusMagnitude = true;
  }
  if (tipradiusdata && tipradiusdata->GetNumberOfComponents() == 3)
  {
    tipradiusMagnitude = true;
  }
  if (scaledata && scaledata->GetNumberOfComponents() == 3)
  {
    scaleMagnitude = true;
  }

  // we know the output will require NumPoints in Arrow * NumPoints in MaskPoints
  // so we can pre-allocate the output space.
  vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
  newPoints->Allocate(numArrowPoints * numMaskedPoints);
  outPd->CopyAllocate(inPd, numArrowPoints * numMaskedPoints);

  // Setting up for calls to PolyData::InsertNextCell()
  output->Allocate(numArrowPoints * arrow->GetNumberOfCells() * 3, 5000);

  // setup a transform that we can use to move the arrows around
  vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();

  // track pt, cell increments for copying old point data into new geometry
  vtkIdType ptIncr = 0;
  vtkSmartPointer<vtkIdList> pts = vtkSmartPointer<vtkIdList>::New();
  pts->Allocate(VTK_CELL_SIZE);

  //
  // Loop over all our points and do the actual glyphing
  //
  for (vtkIdType i = 0; i < numMaskedPoints; i++)
  {

    // The variables we use to control each individual glyph
    double sradius = 1.0;
    double tradius = 1.0;
    double scale = 1.0;
    double vMag = 0.0;
    double* orientvector = NULL;

    // update progress bar
    if (!(i % 10000))
    {
      this->UpdateProgress(static_cast<double>(i) / numArrowPoints);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    /* // @TODO fix parallel ghost cell skipping of points

        // Check ghost points.
        // If we are processing a piece, we do not want to duplicate
        // glyphs on the borders.  The correct check here is:
        // ghostLevel > 0.  I am leaving this over glyphing here because
        // it make a nice example (sphereGhost.tcl) to show the
        // point ghost levels with the glyph filter.  I am not certain
        // of the usefulness of point ghost levels over 1, but I will have
        // to think about it.
        if (inGhostLevels && inGhostLevels[inPtId] > requestedGhostLevel) {
          continue;
        }

        if (!this->IsPointVisible(input, inPtId)) {
          continue;
          }
    */

    // Get Input point
    double* x = maskedpoints->GetPoint(i);

    // translate to Input point
    trans->Identity();
    trans->Translate(x[0], x[1], x[2]);

    if (orientdata)
    {
      orientvector = orientdata->GetTuple3(i);
      vMag = vtkMath::Norm(orientvector);
    }
    if (tipradiusdata)
    {
      if (!tipradiusMagnitude)
        tradius = tipradiusdata->GetTuple1(i);
      else
        tradius = vtkMath::Norm(tipradiusdata->GetTuple3(i));
    }
    if (shaftradiusdata)
    {
      if (!shaftradiusMagnitude)
        sradius = shaftradiusdata->GetTuple1(i);
      else
        sradius = vtkMath::Norm(shaftradiusdata->GetTuple3(i));
    }
    if (scaledata)
    {
      if (!scaleMagnitude)
        scale = scaledata->GetTuple1(i);
      else
        scale = vtkMath::Norm(scaledata->GetTuple3(i));
    }

    double vNew[3];
    if (vMag > 0.0)
    {
      // if there is no y or z component
      if (orientvector[1] == 0.0 && orientvector[2] == 0.0)
      {
        if (orientvector[0] < 0)
        { // just flip x if we need to
          trans->RotateWXYZ(180.0, 0, 1, 0);
        }
      }
      else
      {
        vNew[0] = (orientvector[0] + vMag) / 2.0;
        vNew[1] = orientvector[1] / 2.0;
        vNew[2] = orientvector[2] / 2.0;
        trans->RotateWXYZ(180.0, vNew[0], vNew[1], vNew[2]);
      }
    }

    // Overall glyph scaling is combined from ...
    if (orientMagnitude)
    {
      scale = this->ScaleFactor * vMag * scale;
    }
    else
    {
      scale = this->ScaleFactor * scale;
    }
    trans->Scale(scale, scale, scale);
    //
    internalArrow->SetShaftRadius(Ashaftradius * sradius * this->ShaftRadiusFactor);
    internalArrow->SetTipRadius(Atipradius * tradius * this->TipRadiusFactor);
    internalArrow->Update();

    // pointers may have changed, so refresh them here before copying
    arrow = internalArrow->GetOutput();
    arrowpoints = arrow->GetPoints();
    numArrowPoints = arrowpoints->GetNumberOfPoints();

    // transform the arrow point to correct glyph position/orientation
    trans->TransformPoints(arrowpoints, newPoints);

    // for each arrow point, copy original input point
    for (int a = 0; a < numArrowPoints; a++)
    {
      outPd->CopyData(inPd, i, ptIncr + a);
    }

    // Copy all topology (transformation independent)
    int numArrowCells = arrow->GetNumberOfCells();
    for (vtkIdType cellId = 0; cellId < numArrowCells; cellId++)
    {
      vtkCell* cell = arrow->GetCell(cellId);
      vtkIdList* cellPts = cell->GetPointIds();
      pts->Reset();
      int npts = cellPts->GetNumberOfIds();
      for (int p = 0; p < npts; p++)
      {
        pts->InsertId(p, cellPts->GetId(p) + ptIncr);
      }
      output->InsertNextCell(cell->GetCellType(), pts);
    }

    ptIncr += numArrowPoints;
  }
  output->SetPoints(newPoints);
  return 1;
}

int vtkArrowGlyphFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkArrowGlyphFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
  }
  return 0;
}
