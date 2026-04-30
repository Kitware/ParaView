// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEdgesCacheInternal.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkDataSet.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
vtkEdgesCacheInternal::vtkEdgesCacheInternal(
  const std::string& pointFlagArrayName, double pointFlag)
  : PointFlagArray(pointFlagArrayName)
  , InputPointFlag(pointFlag)
{
}

//----------------------------------------------------------------------------
vtkEdgesCacheInternal::vtkEdgesCacheInternal() = default;

//----------------------------------------------------------------------------
vtkEdgesCacheInternal::~vtkEdgesCacheInternal() = default;

//----------------------------------------------------------------------------
void vtkEdgesCacheInternal::InvalidateCache()
{
  this->OriginalEdges.clear();
  this->OriginalPoints.clear();
}

//----------------------------------------------------------------------------
bool vtkEdgesCacheInternal::UpdateAttributes(vtkDataObject* input, vtkDataObject* output)
{
  auto compositeInput = vtkCompositeDataSet::SafeDownCast(input);
  auto compositeOutput = vtkCompositeDataSet::SafeDownCast(output);

  auto datasetInput = vtkDataSet::SafeDownCast(input);
  auto pointSetOutput = vtkPointSet::SafeDownCast(output);

  if (compositeInput && compositeOutput)
  {
    vtkSmartPointer<vtkCompositeDataIterator> inputIter;
    inputIter.TakeReference(compositeInput->NewIterator());
    inputIter->SkipEmptyNodesOn();
    vtkSmartPointer<vtkCompositeDataIterator> outIter;
    outIter.TakeReference(compositeInput->NewIterator());
    outIter->SkipEmptyNodesOn();
    outIter->InitTraversal();

    bool ret = true;
    for (inputIter->InitTraversal(); !inputIter->IsDoneWithTraversal(); inputIter->GoToNextItem())
    {
      auto inputLeaf = vtkDataSet::SafeDownCast(inputIter->GetCurrentDataObject());
      auto outLeaf = vtkPointSet::SafeDownCast(outIter->GetCurrentDataObject());
      ret = ret && this->UpdateLeafAttributes(inputLeaf, outLeaf);
      outIter->GoToNextItem();
    }
    return ret;
  }
  else if (datasetInput && pointSetOutput)
  {
    return this->UpdateLeafAttributes(datasetInput, pointSetOutput);
  }

  vtkLog(ERROR,
    "Unsupported input or output types: " << input << " and " << output
                                          << ". Output will be empty");

  return false;
}

//----------------------------------------------------------------------------
bool vtkEdgesCacheInternal::UpdateLeafAttributes(vtkDataSet* inputDataSet, vtkPointSet* outPointSet)
{
  // cache useful value for future interpolation
  if (this->OriginalEdges.count(inputDataSet) == 0 && this->OriginalPoints.count(inputDataSet) == 0)
  {
    this->CacheLeafEdges(inputDataSet, outPointSet);
  }

  vtkPointData* outPD = outPointSet->GetPointData();
  vtkPointData* inPD = inputDataSet->GetPointData();

  outPD->InterpolateAllocate(inPD, outPointSet->GetNumberOfPoints());
  outPD->SetNumberOfTuples(outPointSet->GetNumberOfPoints());
  if (this->OriginalPoints.count(inputDataSet) != 0)
  {
    outPD->CopyData(inPD, this->OriginalPoints[inputDataSet].Source,
      this->OriginalPoints[inputDataSet].Destination);
  }

  if (this->OriginalEdges.count(inputDataSet) != 0)
  {
    std::vector<vtkEdgeInternal>& edges = this->OriginalEdges[inputDataSet];

    vtkSMPTools::For(0, edges.size(), 1000,
      [&](size_t firstEdge, size_t lastEdge)
      {
        for (auto edgeId = firstEdge; edgeId < lastEdge; edgeId++)
        {
          outPD->InterpolateEdge(inPD, edges[edgeId].OutId, edges[edgeId].Ids[0],
            edges[edgeId].Ids[1], edges[edgeId].Parametric);
        }
      });
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkEdgesCacheInternal::CacheLeafEdges(vtkDataSet* inputDataSet, vtkPointSet* outPointSet)
{
  auto outPoly = vtkPolyData::SafeDownCast(outPointSet);
  if (outPoly)
  {
    outPoly->BuildLinks();
  }
  auto outUG = vtkUnstructuredGrid::SafeDownCast(outPointSet);
  if (outUG)
  {
    outUG->BuildLinks();
  }

  auto originalCellIds =
    outPointSet->GetCellData()->GetArray(vtkDataObjectMeshCache::GetDefaultIdsName().c_str());
  auto originalPointIds =
    outPointSet->GetPointData()->GetArray(vtkDataObjectMeshCache::GetDefaultIdsName().c_str());

  // see vtkTableBasedClipDataSet
  auto clipPointType = this->PointFlagArray.empty()
    ? nullptr
    : outPointSet->GetPointData()->GetArray(this->PointFlagArray.c_str());

  this->OriginalEdges[inputDataSet].reserve(outPointSet->GetNumberOfPoints());

  vtkNew<vtkGenericCell> tmpCell;
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType cutPointId = 0; cutPointId < outPointSet->GetNumberOfPoints(); cutPointId++)
  {
    // first, check if output point is copied from input
    if (clipPointType)
    {
      double pointType = clipPointType->GetTuple1(cutPointId);
      if (pointType == this->InputPointFlag)
      {
        this->OriginalPoints[inputDataSet].Source->InsertNextId(
          originalPointIds->GetTuple1(cutPointId));
        this->OriginalPoints[inputDataSet].Destination->InsertNextId(cutPointId);
        continue;
      }
    }

    // otherwise, we should introspect and find edge
    outPointSet->GetPointCells(cutPointId, cellIds);
    vtkIdType inputCellId = originalCellIds->GetTuple1(cellIds->GetId(0));
    inputDataSet->GetCell(inputCellId, tmpCell);

    double outCoord[3];
    outPointSet->GetPoint(cutPointId, outCoord);
    double minDist2 = VTK_DOUBLE_MAX;
    std::vector<double> weights;
    weights.resize(2);
    std::vector<vtkIdType> ids;
    vtkEdgeInternal originalEdge;
    for (vtkIdType edgeId = 0; edgeId < tmpCell->GetNumberOfEdges(); edgeId++)
    {
      vtkCell* edgeCell = tmpCell->GetEdge(edgeId);
      double dummyClosest[3], dummyPcoords[3];
      double dist2;
      int dummysubid;
      std::array<double, 2> tmpWeights;
      edgeCell->EvaluatePosition(
        outCoord, dummyClosest, dummysubid, dummyPcoords, dist2, tmpWeights.data());
      if (dist2 < minDist2)
      {
        minDist2 = dist2;
        // The cell being a line, the second weight is the parametric value
        originalEdge = vtkEdgeInternal{ cutPointId, tmpWeights[1], edgeCell };
      }
    }
    this->OriginalEdges[inputDataSet].push_back(originalEdge);
  }
}
