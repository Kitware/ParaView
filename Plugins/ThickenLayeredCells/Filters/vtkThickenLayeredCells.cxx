/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThickenLayeredCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThickenLayeredCells.h"

#include "vtkCellIterator.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include <algorithm>
#include <cassert>
#include <map>
#include <set>

namespace
{
typedef std::pair<vtkIdType, vtkIdType> vtkEdge;
class vtkEdgeInfo
{
public:
  vtkEdge Edge;
  double Sum;
  int Count;
  int Layer;
  vtkEdgeInfo()
    : Sum(0)
    , Count(0)
    , Layer(-1)
  {
  }
  double GetAverageValue() const
  {
    assert(this->Count > 0);
    return this->Sum / this->Count;
  }
};

class vtkEdgeInfoDescending
{
public:
  bool operator()(const vtkEdgeInfo& a, const vtkEdgeInfo& b) const { return b.Layer < a.Layer; }
};

typedef std::map<vtkEdge, vtkEdgeInfo> vtkEdgeMap;

int vtkDisplacePoints(vtkEdgeMap& edges, vtkPoints* ipoints, vtkPoints* opoints)
{
  // sort edges by layer, since we displace points in that order.
  // We use descending order since layer 1 is the inner most layer and layer 6  is the outermost
  // and we want to keep layer 6 invariant.
  std::multiset<vtkEdgeInfo, vtkEdgeInfoDescending> sorted_edges;
  for (vtkEdgeMap::const_iterator miter = edges.begin(); miter != edges.end(); ++miter)
  {
    sorted_edges.insert(miter->second);
  }
  for (std::multiset<vtkEdgeInfo, vtkEdgeInfoDescending>::const_iterator seiter =
         sorted_edges.begin();
       seiter != sorted_edges.end(); ++seiter)
  {
    const double avgThickness = seiter->GetAverageValue();
    vtkVector3d pt1, pt2;
    ipoints->GetPoint(seiter->Edge.first, pt1.GetData());
    ipoints->GetPoint(seiter->Edge.second, pt2.GetData());

    vtkVector3d vec = pt2 - pt1;
    vec.Normalize();

    opoints->GetPoint(seiter->Edge.first, pt1.GetData());
    opoints->GetPoint(seiter->Edge.second, pt2.GetData());

    // offset the second point.
    opoints->SetPoint(seiter->Edge.second, (pt1 + vec * avgThickness).GetData());
  }
  return 1;
}
};

vtkStandardNewMacro(vtkThickenLayeredCells);
//----------------------------------------------------------------------------
vtkThickenLayeredCells::vtkThickenLayeredCells()
  : EnableThickening(true)
{
}

//----------------------------------------------------------------------------
vtkThickenLayeredCells::~vtkThickenLayeredCells() = default;

//----------------------------------------------------------------------------
int vtkThickenLayeredCells::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkUnstructuredGrid* input = vtkUnstructuredGrid::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector);
  output->ShallowCopy(input);
  if (!this->EnableThickening)
  {
    return 1;
  }

  vtkPoints* ipoints = input->GetPoints();

  vtkNew<vtkPoints> opoints;
  opoints->DeepCopy(ipoints);

  output->SetPoints(opoints.GetPointer());

  vtkNew<vtkIdList> ptIds;

  int association = 0;
  vtkDataArray* thickness = this->GetInputArrayToProcess(0, input, association);
  if (association != vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    vtkErrorMacro("We only support cell data for thickness");
    return 0;
  }

  vtkDataArray* layer = this->GetInputArrayToProcess(1, input, association);
  if (association != vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    vtkErrorMacro("We only support cell data for 'layer'");
    return 0;
  }

  // Compute average thickness for all the edges.
  vtkEdgeMap edges;

  bool warned_once = false;

  // Iterate over cells to locate edges and put those edges in the `edges` map. The edges
  // keep track of the two nodes that form the edge as well as which layer the edge belongs to.
  // The 2 points in an edge in the edgemap are ordered so that the 1st point "outer" point
  // and the 2nd one is inner point.
  vtkCellIterator* iter = input->NewCellIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
  {
    if (iter->GetCellType() != VTK_WEDGE)
    {
      // We only handle wedges.
      if (!warned_once)
      {
        warned_once = true;
        vtkWarningMacro(
          "This filter currently only supports Wedges. Ignoring all other cell types.");
      }
      continue;
    }
    input->GetCellPoints(iter->GetCellId(), ptIds.GetPointer());
    assert(ptIds->GetNumberOfIds() == 6);

    // We make an assumption that the first 3 points in the cell are the lower
    // points of the wedge while the last 3 are the upper points. "lower"
    // meaning closer to the outer shell.
    for (int cc = 0; cc < 3; cc++)
    {
      vtkIdType p1 = ptIds->GetId(cc);
      vtkIdType p2 = ptIds->GetId(cc + 3);

      vtkEdgeInfo& info = edges[vtkEdge(p1, p2)];
      vtkVector3d pt1, pt2;
      ipoints->GetPoint(p1, pt1.GetData());
      ipoints->GetPoint(p2, pt2.GetData());

      if (info.Count == 0)
      {
        info.Edge.first = p1;
        info.Edge.second = p2;
        info.Layer = layer->GetComponent(iter->GetCellId(), 0);
      }
      else
      {
        assert(info.Layer == layer->GetComponent(iter->GetCellId(), 0));
      }
      info.Sum += thickness->GetComponent(iter->GetCellId(), 0);
      info.Count++;
    }
  }
  iter->Delete();
  return vtkDisplacePoints(edges, ipoints, opoints.GetPointer());
}

//----------------------------------------------------------------------------
void vtkThickenLayeredCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
