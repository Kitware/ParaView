/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLegacyGlyphFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLegacyGlyphFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMaskPoints.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkUniformGrid.h"

#include <algorithm>
#include <stdlib.h>
#include <time.h>

vtkStandardNewMacro(vtkPVLegacyGlyphFilter);

//-----------------------------------------------------------------------------
vtkPVLegacyGlyphFilter::vtkPVLegacyGlyphFilter()
  : RandomPtsInDataset()
{
  this->SetColorModeToColorByScalar();
  this->SetScaleModeToScaleByVector();
  this->MaskPoints = vtkMaskPoints::New();
  this->RandomMode = this->MaskPoints->GetRandomMode();
  this->MaximumNumberOfPoints = 5000;
  this->NumberOfProcesses = vtkMultiProcessController::GetGlobalController()
    ? vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses()
    : 1;
  this->UseMaskPoints = 1;
  this->InputIsUniformGrid = 0;
  this->KeepRandomPoints = 0;
  this->MaximumNumberOfPointsOld = 0;

  this->BlockOnRatio = 0;
  this->BlockMaxNumPts = 0;
  this->BlockPointCounter = 0;
  this->BlockNumGlyphedPts = 0;
  this->BlockGlyphAllPoints = 0;
}

//-----------------------------------------------------------------------------
vtkPVLegacyGlyphFilter::~vtkPVLegacyGlyphFilter()
{
  if (this->MaskPoints)
  {
    this->MaskPoints->Delete();
  }
}

//-----------------------------------------------------------------------------
void vtkPVLegacyGlyphFilter::SetRandomMode(int mode)
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
int vtkPVLegacyGlyphFilter::GetRandomMode()
{
  return this->MaskPoints->GetRandomMode();
}

//-----------------------------------------------------------------------------
void vtkPVLegacyGlyphFilter::SetUseMaskPoints(int useMaskPoints)
{
  if (useMaskPoints == this->UseMaskPoints)
  {
    // no change
    return;
  }
  this->UseMaskPoints = useMaskPoints;
  this->BlockGlyphAllPoints = !this->UseMaskPoints;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVLegacyGlyphFilter::SetKeepRandomPoints(int keepRandomPoints)
{
  if (keepRandomPoints == this->KeepRandomPoints)
  {
    // no change
    return;
  }
  this->KeepRandomPoints = keepRandomPoints;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPVLegacyGlyphFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVLegacyGlyphFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
vtkIdType vtkPVLegacyGlyphFilter::GatherTotalNumberOfPoints(vtkIdType localNumPts)
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
        controller->Receive(&tmp, 1, i, vtkPVLegacyGlyphFilter::GlyphNPointsGather);
        totalNumPts += tmp;
      }
      // Send results back to all processes.
      for (i = 1; i < controller->GetNumberOfProcesses(); ++i)
      {
        controller->Send(&totalNumPts, 1, i, vtkPVLegacyGlyphFilter::GlyphNPointsScatter);
      }
    }
    else
    {
      controller->Send(&localNumPts, 1, 0, vtkPVLegacyGlyphFilter::GlyphNPointsGather);
      controller->Receive(&totalNumPts, 1, 0, vtkPVLegacyGlyphFilter::GlyphNPointsScatter);
    }
  }

  return totalNumPts;
}

//-----------------------------------------------------------------------------
int vtkPVLegacyGlyphFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->BlockOnRatio = 0;

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
  if (hdInput)
  {
    return this->RequestCompositeData(request, inputVector, outputVector);
  }

  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!dsInput)
  {
    if (input)
    {
      vtkErrorMacro("This filter cannot process input of type: " << input->GetClassName());
    }
    return 0;
  }

  // Glyph everything?
  if (!this->UseMaskPoints)
  {
    // yes.
    this->BlockGlyphAllPoints = !this->UseMaskPoints;
    int retVal = this->Superclass::RequestData(request, inputVector, outputVector);
    return retVal;
  }

  // Glyph a subset.
  double maxNumPts = static_cast<double>(this->MaximumNumberOfPoints);
  vtkIdType numPts = dsInput->GetNumberOfPoints();
  vtkIdType totalNumPts = this->GatherTotalNumberOfPoints(numPts);

  // What fraction of the points will this processes get allocated?
  maxNumPts = (double)((double)(maxNumPts) * (double)(numPts) / (double)(totalNumPts));

  maxNumPts = (maxNumPts > numPts) ? numPts : maxNumPts;

  // We will glyph this many points.
  this->BlockMaxNumPts = static_cast<vtkIdType>(maxNumPts + 0.5);
  if (this->BlockMaxNumPts == 0)
  {
    return 1;
  }
  this->CalculatePtsToGlyph(numPts);

  vtkInformationVector* inputVs[2];

  vtkInformationVector* inputV = inputVector[0];
  inputVs[0] = vtkInformationVector::New();
  inputVs[0]->SetNumberOfInformationObjects(1);
  vtkInformation* newInInfo = vtkInformation::New();
  newInInfo->Copy(inputV->GetInformationObject(0));
  inputVs[0]->SetInformationObject(0, newInInfo);
  newInInfo->Delete();
  inputVs[1] = inputVector[1];

  // We have set all ofthe parameters that will be used in
  // our overloaded IsPoitVisible. Now let the glypher take over.
  newInInfo->Set(vtkDataObject::DATA_OBJECT(), dsInput);
  int retVal = this->Superclass::RequestData(request, inputVs, outputVector);

  inputVs[0]->Delete();
  return retVal;
}

//----------------------------------------------------------------------------
// We are overloading this so that blanking will be supported
// otehrwise we could use vtkMaskPoints filter.
int vtkPVLegacyGlyphFilter::IsPointVisible(vtkDataSet* ds, vtkIdType ptId)
{
  if (this->BlockGlyphAllPoints == 1)
  {
    return 1;
  }

  // check if point has been blanked. If so skip it and
  // do not count it.
  if (this->InputIsUniformGrid)
  {
    vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(ds);
    if (ug && !ug->IsPointVisible(ptId))
    {
      return 0;
    }
  }

  // Have we glyphed enough points yet? And are we
  // at the next point? If so we'll return 1 indicating
  // that this point should be glyphed and compute the
  // next point.
  int pointIsVisible = 0;
  if ((this->BlockNumGlyphedPts < this->BlockMaxNumPts) &&
    (this->BlockPointCounter == this->BlockNextPoint))
  {
    this->BlockNumGlyphedPts++;
    if (this->RandomMode)
    {
      if (this->RandomPtsInDataset.empty())
      {
        return 0;
      }

      if (this->BlockNumGlyphedPts < this->BlockMaxNumPts)
      {
        this->BlockNextPoint = this->RandomPtsInDataset[this->BlockNumGlyphedPts];
      }
      else
      {
        this->BlockNextPoint = this->BlockMaxNumPts;
      }
    }
    else
    {
      this->BlockNextPoint = this->BlockNumGlyphedPts;
    }
    pointIsVisible = 1;
  }

  // Count all non-blanked points.
  ++this->BlockPointCounter;
  return pointIsVisible;
}

//----------------------------------------------------------------------------
int vtkPVLegacyGlyphFilter::RequestCompositeData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkCompositeDataSet* hdInput =
    vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // output
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    vtkErrorMacro("Expected vtkPolyData in output.");
    return 0;
  }

  // Get number of points we have in this block
  // and the number of points in all blocks.
  vtkIdType numPts = hdInput->GetNumberOfPoints();
  vtkIdType totalNumPts = this->GatherTotalNumberOfPoints(numPts);

  vtkAppendPolyData* append = vtkAppendPolyData::New();
  int numInputs = 0;

  vtkInformationVector* inputVs[2];

  vtkInformationVector* inputV = inputVector[0];
  inputVs[0] = vtkInformationVector::New();
  inputVs[0]->SetNumberOfInformationObjects(1);
  vtkInformation* newInInfo = vtkInformation::New();
  newInInfo->Copy(inputV->GetInformationObject(0));
  inputVs[0]->SetInformationObject(0, newInInfo);
  newInInfo->FastDelete();
  inputVs[1] = inputVector[1];

  int retVal = 1;
  this->InputIsUniformGrid = 0;

  vtkCompositeDataIterator* iter = hdInput->NewIterator();

  while (!iter->IsDoneWithTraversal())
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
    {
      // Uniform grids might be blanked, we make a note if we
      // have a uniform grid to facilitate blanking friendly
      // glyph sampling.
      vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(ds);
      if (ug)
      {
        this->InputIsUniformGrid = 1;
      }
      else
      {
        this->InputIsUniformGrid = 0;
      }

      // Certain AMR data sets provide blanking information
      // We will skip blanked points and glyph what's
      // left if any.
      vtkIdType numBlankedPts = 0;
      vtkInformation* blockInfo = iter->GetCurrentMetaData();
      if (blockInfo)
      {
        if (blockInfo->Has(vtkHierarchicalBoxDataSet::NUMBER_OF_BLANKED_POINTS()))
        {
          numBlankedPts = blockInfo->Get(vtkHierarchicalBoxDataSet::NUMBER_OF_BLANKED_POINTS());
        }
      }
      // To fix Bug-9334, the output should be a new output that does not interfere
      // with the output of this filter, which seems to be the cause of this bug.
      vtkNew<vtkPolyData> tmpOut;
      vtkNew<vtkPolyData> newoutput;
      vtkNew<vtkInformationVector> outputV;
      vtkNew<vtkInformation> newOutInfo;
      newOutInfo->Copy(info);
      newOutInfo->Set(vtkDataObject::DATA_OBJECT(), newoutput.GetPointer());
      outputV->SetInformationObject(0, newOutInfo.GetPointer());

      // Calculate the number of points on this block that need to be glyphed
      double nPtsNotBlanked = static_cast<double>(ds->GetNumberOfPoints() - numBlankedPts);
      double nPtsVisibleOverAll = static_cast<double>(this->MaximumNumberOfPoints);
      double nPtsInDataSet = static_cast<double>(totalNumPts);
      double fractionOfPtsInBlock = nPtsNotBlanked / nPtsInDataSet;
      double nPtsVisibleOverBlock = nPtsVisibleOverAll * fractionOfPtsInBlock;
      nPtsVisibleOverBlock =
        ((nPtsVisibleOverBlock > nPtsNotBlanked) ? nPtsNotBlanked : nPtsVisibleOverBlock);

      // We will glyph this many points.
      this->BlockMaxNumPts = static_cast<vtkIdType>(nPtsVisibleOverBlock + 0.5);
      if (this->BlockMaxNumPts == 0)
      {
        iter->GoToNextItem();
        continue;
      }
      this->CalculatePtsToGlyph(nPtsNotBlanked);

      // We have set all of the parameters that will be used in
      // our overloaded IsPointVisible. Now let vktGlyph3D take over.
      newInInfo->Set(vtkDataObject::DATA_OBJECT(), ds);
      retVal = this->Superclass::RequestData(request, inputVs, outputV.GetPointer());

      // vktGlyph3D failed, so we skip the rest and fail as well.
      if (!retVal)
      {
        vtkErrorMacro("vtkGlyph3D failed.");
        iter->Delete();
        inputVs[0]->Delete();
        append->Delete();
        return 0;
      }

      // Accumulate the results.
      tmpOut->ShallowCopy(newoutput.GetPointer());
      append->AddInputData(tmpOut.GetPointer());

      numInputs++;
    }
    iter->GoToNextItem();
  }

  // copy the accumulated results to the output.
  if (numInputs > 0)
  {
    append->Update();
    output->ShallowCopy(append->GetOutput());
  }

  // clean up
  iter->Delete();
  inputVs[0]->Delete();
  append->Delete();

  return retVal;
}

// The problems this filter has are numerous. I will stick to fixing the
// specific BUG #14279 in this change. A refactored vtkPVLegacyGlyphFilter is
// imminent. BUG #14279 happens since rand() returns a number in 0 and RAND_MAX
// and RAND_MAX can be less than maxId, which the old implementation missed.
static vtkIdType vtkPVLegacyGlyphFilterRandomId(vtkIdType maxId)
{
  double randomNumber = rand() / static_cast<double>(RAND_MAX);
  // randomNumber is now in range [0.0, 1.0]
  return static_cast<vtkIdType>(randomNumber * maxId);
}

//-----------------------------------------------------------------------------
void vtkPVLegacyGlyphFilter::CalculatePtsToGlyph(double PtsNotBlanked)
{
  // When mask points is checked, we glyph the first N points. When mask points
  // and random mode is checked, we glyph N random points from the total number
  // of points on the block.

  if (this->BlockMaxNumPts > PtsNotBlanked)
  {
    vtkErrorMacro("This filter cannot glyph points more than: " << PtsNotBlanked);
    return;
  }

  this->BlockPointCounter = 0;
  this->BlockNumGlyphedPts = 0;

  if (!this->KeepRandomPoints || !this->RandomPtsInDataset.size() ||
    this->MaximumNumberOfPoints != this->MaximumNumberOfPointsOld)
  {
    // Reset the random points vector
    this->RandomPtsInDataset.clear();

    // Populate Random points in the vector if random mode selected
    if (this->RandomMode)
    {
      srand(time(NULL));
      int r;
      for (int i = 0; i < this->BlockMaxNumPts; i++)
      {
        r = vtkPVLegacyGlyphFilterRandomId(static_cast<vtkIdType>(floor(PtsNotBlanked)));
        while (std::find(this->RandomPtsInDataset.begin(), this->RandomPtsInDataset.end(), r) !=
          this->RandomPtsInDataset.end())
        {
          r = vtkPVLegacyGlyphFilterRandomId(static_cast<vtkIdType>(floor(PtsNotBlanked)));
        }
        this->RandomPtsInDataset.push_back(static_cast<vtkIdType>(r));
      }
      std::sort(this->RandomPtsInDataset.begin(), this->RandomPtsInDataset.end());
    }

    this->MaximumNumberOfPointsOld = this->MaximumNumberOfPoints;
  }

  // Identify the first point to glyph.
  if (this->RandomMode && this->RandomPtsInDataset.size() > 0) // FIXME this was a quick fix to
                                                               // prevent some test failure with mpi
                                                               // pvcrs.FindDataDialog.Flow
  {
    this->BlockNextPoint = this->RandomPtsInDataset[0];
  }
  else
  {
    this->BlockNextPoint = 0;
  }
}

//-----------------------------------------------------------------------------
void vtkPVLegacyGlyphFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->MaskPoints, "MaskPoints");
}

//-----------------------------------------------------------------------------
void vtkPVLegacyGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MaximumNumberOfPoints: " << this->GetMaximumNumberOfPoints() << endl;

  os << indent << "UseMaskPoints: " << (this->UseMaskPoints ? "on" : "off") << endl;

  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}
