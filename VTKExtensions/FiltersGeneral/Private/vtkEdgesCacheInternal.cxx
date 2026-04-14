// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEdgesCacheInternal.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

//----------------------------------------------------------------------------
void vtkEdgesCacheInternal::InvalidateCache()
{
  this->OriginalEdges.clear();
}

//----------------------------------------------------------------------------
bool vtkEdgesCacheInternal::UpdateAttributes(vtkDataObject* input, vtkDataObject* output)
{
  auto compositeInput = vtkCompositeDataSet::SafeDownCast(input);
  auto compositeOutput = vtkCompositeDataSet::SafeDownCast(output);

  auto datasetInput = vtkDataSet::SafeDownCast(input);
  auto polyOutput = vtkPolyData::SafeDownCast(output);

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
      auto outLeaf = vtkPolyData::SafeDownCast(outIter->GetCurrentDataObject());
      ret = ret && this->UpdateLeafAttributes(inputLeaf, outLeaf);
      outIter->GoToNextItem();
    }
    return ret;
  }
  else if (datasetInput && polyOutput)
  {
    return this->UpdateLeafAttributes(datasetInput, polyOutput);
  }

  vtkLog(ERROR,
    "Unsupported input or output types: " << input << " and " << output
                                          << ". Output will be empty");

  return false;
}

//----------------------------------------------------------------------------
bool vtkEdgesCacheInternal::UpdateLeafAttributes(vtkDataSet* inputDataSet, vtkPolyData* outPolyData)
{
  // cache useful value for future interpolation
  if (this->OriginalEdges.count(inputDataSet) == 0)
  {
    this->CacheLeafEdges(inputDataSet, outPolyData);
  }

  vtkPointData* outPD = outPolyData->GetPointData();
  vtkPointData* inPD = inputDataSet->GetPointData();

  outPD->InterpolateAllocate(inPD, outPolyData->GetNumberOfPoints());

  std::vector<vtkEdgeInternal>& edges = this->OriginalEdges[inputDataSet];

  for (vtkIdType cutPointId = 0; cutPointId < outPolyData->GetNumberOfPoints(); cutPointId++)
  {
    outPD->InterpolateEdge(inPD, cutPointId, edges[cutPointId].Ids[0], edges[cutPointId].Ids[1],
      edges[cutPointId].Weight);
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkEdgesCacheInternal::CacheLeafEdges(vtkDataSet* inputDataSet, vtkPolyData* outPolyData)
{
  // initialize another kind of cache: interpolation weights.
  outPolyData->BuildLinks();
  auto originalCellIds =
    outPolyData->GetCellData()->GetArray(vtkDataObjectMeshCache::GetTemporaryIdsName().c_str());
  vtkNew<vtkGenericCell> tmpCell;

  this->OriginalEdges[inputDataSet].reserve(outPolyData->GetNumberOfPoints());
  for (vtkIdType cutPointId = 0; cutPointId < outPolyData->GetNumberOfPoints(); cutPointId++)
  {
    vtkIdType nCell;
    vtkIdType* cellIds;
    outPolyData->GetPointCells(cutPointId, nCell, cellIds);
    vtkIdType inputCellId = originalCellIds->GetTuple1(cellIds[0]);
    inputDataSet->GetCell(inputCellId, tmpCell);

    double coord[3];
    outPolyData->GetPoint(cutPointId, coord);
    double minDist2 = VTK_DOUBLE_MAX;
    std::vector<double> weights;
    weights.resize(2);
    std::vector<vtkIdType> ids;
    vtkEdgeInternal originalEdge;
    for (vtkIdType edgeId = 0; edgeId < tmpCell->GetNumberOfEdges(); edgeId++)
    {
      vtkCell* edgeCell = tmpCell->GetEdge(edgeId);
      double closest[3], dummy_pcoords[3];
      double dist2;
      int dummysubid;
      std::array<double, 2> tmpWeights;
      edgeCell->EvaluatePosition(
        coord, closest, dummy_subid, dummy_pcoords, dist2, tmp_weights.data());
      if (dist2 < minDist2)
      {
        minDist2 = dist2;
        originalEdge = vtkEdgeInternal{ tmp_weights[0], edgeCell };
      }
    }
    this->OriginalEdges[inputDataSet].push_back(originalEdge);
  }
}
