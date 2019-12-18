/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyphFilterLegacy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGlyphFilterLegacy.h"

// VTK includes
#include "vtkAppendPolyData.h"
#include "vtkBoundingBox.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOctreePointLocator.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTuple.h"
#include "vtkUniformGrid.h"

// C/C++ includes
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>

class vtkPVGlyphFilterLegacy::vtkInternals
{
  vtkBoundingBox Bounds;
  double NearestPointRadius;
  std::vector<vtkTuple<double, 3> > Points;
  std::vector<vtkIdType> PointIds;
  size_t NextPointId;

  vtkNew<vtkOctreePointLocator> Locator;

  void SetupLocator(vtkDataSet* ds)
  {
    if (this->Locator->GetDataSet() == ds)
    {
      return;
    }

    this->Locator->Initialize();
    this->Locator->SetDataSet(ds);
    this->Locator->BuildLocator();

    this->PointIds.clear();
    std::set<vtkIdType> pointset;
    for (std::vector<vtkTuple<double, 3> >::iterator iter = this->Points.begin(),
                                                     end = this->Points.end();
         iter != end; ++iter)
    {
      double dist2;
      vtkIdType ptId = this->Locator->FindClosestPointWithinRadius(
        this->NearestPointRadius, iter->GetData(), dist2);
      if (ptId >= 0)
      {
        pointset.insert(ptId);
      }
    }
    this->PointIds.insert(this->PointIds.begin(), pointset.begin(), pointset.end());
    this->NextPointId = 0;
  }

public:
  void Reset()
  {
    this->Bounds.Reset();
    this->Points.clear();
    this->Locator->Initialize();
    this->Locator->SetDataSet(NULL);
  }

  //---------------------------------------------------------------------------
  // Update internal datastructures for the given dataset. This will collect
  // bounds information for all datasets for SPATIALLY_UNIFORM_DISTRIBUTION.
  void UpdateWithDataset(vtkDataSet* ds, vtkPVGlyphFilterLegacy* self)
  {
    assert(ds != NULL && self != NULL);
    if (self->GetGlyphMode() != vtkPVGlyphFilterLegacy::SPATIALLY_UNIFORM_DISTRIBUTION)
    {
      // nothing to do.
      return;
    }

    // collect local bounds information.
    double bds[6];
    ds->GetBounds(bds);
    if (vtkBoundingBox::IsValid(bds))
    {
      this->Bounds.AddBounds(bds);
    }
  }

  //---------------------------------------------------------------------------
  // Again, primarily for SPATIALLY_UNIFORM_DISTRIBUTION. We sync the bounds
  // information among all ranks.
  // Subsequently, we also build the list of random sample points using the
  // synchronized bounds.
  void SynchronizeGlobalInformation(vtkPVGlyphFilterLegacy* self)
  {
    if (self->GetGlyphMode() != vtkPVGlyphFilterLegacy::SPATIALLY_UNIFORM_DISTRIBUTION)
    {
      return; // nothing to do.
    }

    vtkMultiProcessController* controller = self->GetController();
    if (controller && controller->GetNumberOfProcesses() > 1)
    {
      double local_min[3] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
      double local_max[3] = { VTK_DOUBLE_MIN, VTK_DOUBLE_MIN, VTK_DOUBLE_MIN };
      if (this->Bounds.IsValid())
      {
        this->Bounds.GetMinPoint(local_min[0], local_min[1], local_min[2]);
        this->Bounds.GetMaxPoint(local_max[0], local_max[1], local_max[2]);
      }
      double global_min[3], global_max[3];

      controller->AllReduce(local_min, global_min, 3, vtkCommunicator::MIN_OP);
      controller->AllReduce(local_max, global_max, 3, vtkCommunicator::MAX_OP);

      this->Bounds.SetBounds(
        global_min[0], global_max[0], global_min[1], global_max[1], global_min[2], global_max[2]);
    }

    if (!this->Bounds.IsValid())
    {
      return;
    }

    // build up list of points to glyph.
    vtkNew<vtkMinimalStandardRandomSequence> randomGenerator;
    randomGenerator->SetSeed(self->GetSeed());
    this->Points.resize(self->GetMaximumNumberOfSamplePoints());
    for (vtkIdType cc = 0; cc < self->GetMaximumNumberOfSamplePoints(); cc++)
    {
      vtkTuple<double, 3>& tuple = this->Points[cc];
      randomGenerator->Next();
      tuple[0] = randomGenerator->GetRangeValue(
        this->Bounds.GetMinPoint()[0], this->Bounds.GetMaxPoint()[0]);
      randomGenerator->Next();
      tuple[1] = randomGenerator->GetRangeValue(
        this->Bounds.GetMinPoint()[1], this->Bounds.GetMaxPoint()[1]);
      randomGenerator->Next();
      tuple[2] = randomGenerator->GetRangeValue(
        this->Bounds.GetMinPoint()[2], this->Bounds.GetMaxPoint()[2]);
    }

    // we use diagonal, instead of actual length for each side to avoid the
    // issue with one of the lengths being 0.
    double side = std::sqrt(this->Bounds.GetDiagonalLength());
    double volume = side * side * side;
    if (volume > 0.0)
    {
      assert(volume > 0.0);
      assert(self->GetMaximumNumberOfSamplePoints() > 0);
      double volumePerGlyph = volume / self->GetMaximumNumberOfSamplePoints();
      double delta = std::pow(volumePerGlyph, 1.0 / 3.0);
      this->NearestPointRadius = std::pow(2 * delta, 1.0 / 2.0) / 2.0;
    }
    else
    {
      this->NearestPointRadius = 0.0001;
    }
  }

  //---------------------------------------------------------------------------
  inline bool IsPointVisible(vtkDataSet* ds, vtkIdType ptId, vtkPVGlyphFilterLegacy* self)
  {
    assert(ds != NULL && self != NULL);
    switch (self->GetGlyphMode())
    {
      case vtkPVGlyphFilterLegacy::ALL_POINTS:
        return true;

      case vtkPVGlyphFilterLegacy::EVERY_NTH_POINT:
        return self->GetStride() <= 1 || (ptId % self->GetStride()) == 0;

      case vtkPVGlyphFilterLegacy::SPATIALLY_UNIFORM_DISTRIBUTION:
        // This will initialize the point locator and build the list of PointIds
        // that should be glyphed.
        this->SetupLocator(ds);

        // since PointIds is a sorted list, and we know that IsPointVisible will
        // be called for in monotonically increasing fashion for a specific ds, we
        // use this->NextPointId to simplify the "contains" check.

        while (
          (this->NextPointId < this->PointIds.size()) && (this->PointIds[this->NextPointId] < ptId))
        {
          // this is need since it is possible (due to ghost cells or other
          // masking employed by vtkGlyph3D) that certain ptIds are never tested
          // since they are rejected earlier on.
          this->NextPointId++;
        }

        if (this->NextPointId < this->PointIds.size() && ptId == this->PointIds[this->NextPointId])
        {
          this->NextPointId++;
          return true;
        }
    }
    return false;
  }
};

vtkStandardNewMacro(vtkPVGlyphFilterLegacy);
vtkCxxSetObjectMacro(vtkPVGlyphFilterLegacy, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPVGlyphFilterLegacy::vtkPVGlyphFilterLegacy()
  : GlyphMode(vtkPVGlyphFilterLegacy::ALL_POINTS)
  , MaximumNumberOfSamplePoints(5000)
  , Seed(1)
  , Stride(1)
  , Controller(0)
  , Internals(new vtkPVGlyphFilterLegacy::vtkInternals())
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkPVGlyphFilterLegacy::~vtkPVGlyphFilterLegacy()
{
  this->SetController(NULL);
  delete this->Internals;
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilterLegacy::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }

  if (port == 0)
  {
    // we can handle composite datasets.
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilterLegacy::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilterLegacy::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilterLegacy::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (vtkCompositeDataSet::GetData(inputVector[0], 0))
  {
    vtkMultiBlockDataSet* mds = vtkMultiBlockDataSet::GetData(outputVector, 0);
    if (mds == NULL)
    {
      mds = vtkMultiBlockDataSet::New();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), mds);
      mds->FastDelete();
    }
  }
  else
  {
    vtkPolyData* pd = vtkPolyData::GetData(outputVector, 0);
    if (pd == NULL)
    {
      pd = vtkPolyData::New();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), pd);
      pd->FastDelete();
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilterLegacy::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformationVector* sourceVector = inputVector[1];

  this->Internals->Reset();

  vtkDataSet* ds = vtkDataSet::GetData(inputVector[0], 0);
  vtkCompositeDataSet* cds = vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (ds)
  {
    this->Internals->UpdateWithDataset(ds, this);
    this->Internals->SynchronizeGlobalInformation(this);

    if (!this->IsInputArrayToProcessValid(ds))
    {
      return 1;
    }

    vtkPolyData* outputPD = vtkPolyData::GetData(outputVector);
    assert(outputPD);
    if (this->UseCellCenters(ds))
    {
      return this->ExecuteWithCellCenters(ds, sourceVector, outputPD) ? 1 : 0;
    }
    else
    {
      return this->Execute(ds, sourceVector, outputPD) ? 1 : 0;
    }
  }
  else if (cds)
  {
    vtkMultiBlockDataSet* outputMD = vtkMultiBlockDataSet::GetData(outputVector);
    assert(outputMD);

    outputMD->CopyStructure(cds);

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cds->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* current = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (current)
      {
        this->Internals->UpdateWithDataset(current, this);
      }
    }
    this->Internals->SynchronizeGlobalInformation(this);

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* currentDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (currentDS)
      {
        if (!this->IsInputArrayToProcessValid(currentDS))
        {
          continue;
        }

        bool res;
        vtkNew<vtkPolyData> outputPD;
        if (this->UseCellCenters(currentDS))
        {
          res = this->ExecuteWithCellCenters(currentDS, sourceVector, outputPD.GetPointer());
        }
        else
        {
          res = this->Execute(currentDS, sourceVector, outputPD.GetPointer());
        }
        if (!res)
        {
          vtkErrorMacro("Glyph generation failed for block: " << iter->GetCurrentFlatIndex());
          this->Internals->Reset();
          return 0;
        }
        outputMD->SetDataSet(iter, outputPD.GetPointer());
      }
    }
  }
  this->Internals->Reset();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPVGlyphFilterLegacy::IsPointVisible(vtkDataSet* ds, vtkIdType ptId)
{
  return (this->Superclass::IsPointVisible(ds, ptId) != 0) &&
    (this->Internals->IsPointVisible(ds, ptId, this));
}

//-----------------------------------------------------------------------------
bool vtkPVGlyphFilterLegacy::IsInputArrayToProcessValid(vtkDataSet* input)
{
  // confirm that both scalars and vectors are being used by the Glyph filter,
  // otherwise, the mismatch in array association for the two is not an issue.
  if (!this->NeedsScalars() || !this->NeedsVectors())
  {
    return true;
  }

  vtkDataArray* inSScalars = this->GetInputArrayToProcess(0, input);
  vtkDataArray* inVectors = this->GetInputArrayToProcess(1, input);
  int inSScalarsAssociation = this->GetInputArrayAssociation(0, input);
  int inVectorsAssociation = this->GetInputArrayAssociation(1, input);
  if (inSScalarsAssociation != inVectorsAssociation && inSScalars && inVectors)
  {
    std::string inSScalarsType =
      (inSScalarsAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS ? "point" : "cell");
    std::string inVectorsType =
      (inVectorsAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS ? "point" : "cell");
    vtkWarningMacro(<< "Mismatched attributes:\n"
                    << inSScalars->GetName() << " is a " << inSScalarsType << " attribute whereas "
                    << inVectors->GetName() << " is a " << inVectorsType << " attribute.");
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPVGlyphFilterLegacy::NeedsScalars()
{
  if (this->ScaleMode == VTK_SCALE_BY_SCALAR)
  {
    return true;
  }
  if (this->ColorMode == VTK_COLOR_BY_SCALAR)
  {
    return true;
  }
  if (this->IndexMode == VTK_INDEXING_BY_SCALAR)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkPVGlyphFilterLegacy::NeedsVectors()
{
  if (this->ScaleMode == VTK_SCALE_BY_VECTOR || this->ScaleMode == VTK_SCALE_BY_VECTORCOMPONENTS)
  {
    return true;
  }
  if (this->ColorMode == VTK_COLOR_BY_VECTOR)
  {
    return true;
  }
  if (this->VectorMode == VTK_USE_VECTOR)
  {
    return true;
  }
  if (this->IndexMode == VTK_INDEXING_BY_VECTOR)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkPVGlyphFilterLegacy::UseCellCenters(vtkDataSet* input)
{
  int inSScalarsAssociation = this->GetInputArrayAssociation(0, input);
  int inVectorsAssociation = this->GetInputArrayAssociation(1, input);
  return inSScalarsAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS ||
    inVectorsAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS;
}

//-----------------------------------------------------------------------------
bool vtkPVGlyphFilterLegacy::ExecuteWithCellCenters(
  vtkDataSet* input, vtkInformationVector* sourceVector, vtkPolyData* output)
{
  vtkNew<vtkCellCenters> cellCenters;
  cellCenters->SetInputData(input);
  cellCenters->Update();
  input = cellCenters->GetOutput();
  vtkDataArray* inSScalars = input->GetPointData()->GetArray(
    this->GetInputArrayInformation(0)->Get(vtkDataObject::FIELD_NAME()));
  vtkDataArray* inVectors = input->GetPointData()->GetArray(
    this->GetInputArrayInformation(1)->Get(vtkDataObject::FIELD_NAME()));
  return this->Execute(input, sourceVector, output, inSScalars, inVectors);
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilterLegacy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GlyphMode: ";
  switch (this->GlyphMode)
  {
    case ALL_POINTS:
      os << "ALL_POINTS" << endl;
      break;

    case EVERY_NTH_POINT:
      os << "EVERY_NTH_POINT" << endl;
      break;

    case SPATIALLY_UNIFORM_DISTRIBUTION:
      os << "SPATIALLY_UNIFORM_DISTRIBUTION" << endl;
      break;

    default:
      os << "(invalid:" << this->GlyphMode << ")" << endl;
  }
  os << indent << "MaximumNumberOfSamplePoints: " << this->MaximumNumberOfSamplePoints << endl;
  os << indent << "Seed: " << this->Seed << endl;
  os << indent << "Stride: " << this->Stride << endl;
  os << indent << "Controller: " << this->Controller << endl;
}
