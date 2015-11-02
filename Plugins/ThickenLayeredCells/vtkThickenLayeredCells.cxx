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

#include <cassert>
#include <set>
#include <map>
#include <algorithm>

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
    vtkEdgeInfo() : Sum(0), Count(0), Layer(-1)
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
    bool operator()(const vtkEdgeInfo& a, const vtkEdgeInfo& b) const
      {
      return b.Layer < a.Layer;
      }
    };

  typedef std::map<vtkEdge, vtkEdgeInfo> vtkEdgeMap;

  class vtkNodeInfo
    {
    vtkVector2i Layers;
  public:
    vtkNodeInfo() : Layers(-1, -1) {}
    void AddLayer(int layer)
      {
      if (this->Layers[0] == -1)
        {
        this->Layers[0] = layer;
        }
      else if (this->Layers[1] == -1)
        {
        int layer0 = this->Layers[0];
        this->Layers[0] = std::min(layer0, layer);
        this->Layers[1] = std::max(layer0, layer);
        }
      else
        {
        vtkGenericWarningMacro("An egde is shared between more than 2 layers!!!! Ignoring.");
        }
      }
    inline bool OffsetForLayer(int layer, int fixed_layer) const
      {
      assert(layer != -1);
      if (this->Layers[1] == layer)
        {
        return true;
        }
      if (this->Layers[1] == -1 && this->Layers[0] == layer)
        {
        return layer != fixed_layer;
        }
      return false;
      }
    };

  class vtkNodeMap : public std::map<vtkIdType, vtkNodeInfo>
    {
  public:
    void AddEdge(const vtkEdge& edge, int layer)
      {
      (*this)[edge.first].AddLayer(layer);
      (*this)[edge.second].AddLayer(layer);
      }
    };

  int vtkDisplacePoints(vtkEdgeMap& edges, vtkNodeMap& nodes,
    vtkPoints* ipoints, vtkPoints* opoints, int outermost_layer)
  {
  // sort edges by layer, since we displace points in that order.
  // We use descending order since layer 1 is the inner most layer and layer 6  is the outermost
  // and we want to keep layer 6 invariant.
  std::multiset<vtkEdgeInfo, vtkEdgeInfoDescending> sorted_edges;
  for (vtkEdgeMap::const_iterator miter = edges.begin(); miter != edges.end(); ++miter)
    {
    sorted_edges.insert(miter->second);
    }
  for (std::multiset<vtkEdgeInfo, vtkEdgeInfoDescending>::const_iterator seiter = sorted_edges.begin();
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

    int offset_count = 0;
    if (nodes[seiter->Edge.first].OffsetForLayer(seiter->Layer, outermost_layer))
      {
      opoints->SetPoint(seiter->Edge.first, (pt2 - vec * avgThickness).GetData());
      offset_count++;
      }
    else
      {
      opoints->SetPoint(seiter->Edge.first, pt1.GetData());
      }
    if (nodes[seiter->Edge.second].OffsetForLayer(seiter->Layer, outermost_layer))
      {
      opoints->SetPoint(seiter->Edge.second, (pt1 + vec * avgThickness).GetData());
      offset_count++;
      }
    else
      {
      opoints->SetPoint(seiter->Edge.second, pt2.GetData());
      }
    assert(offset_count == 1);
    }
  return 1;
  }
};

vtkStandardNewMacro(vtkThickenLayeredCells);
//----------------------------------------------------------------------------
vtkThickenLayeredCells::vtkThickenLayeredCells()
{
}

//----------------------------------------------------------------------------
vtkThickenLayeredCells::~vtkThickenLayeredCells()
{
}

//----------------------------------------------------------------------------
int vtkThickenLayeredCells::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkUnstructuredGrid* input = vtkUnstructuredGrid::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector);
  output->ShallowCopy(input);

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

  double layerRange[2];
  layer->GetRange(layerRange, 0);

  // Compute average thickness for all the edges.
  vtkEdgeMap edges;
  vtkNodeMap nodes;

  bool warned_once = false;

  // Iterate over cells to locate edges and but those edges in the `edges` map. The edges
  // keep track of the two nodes that form the edge as well as which layer the edge belongs to.
  // At the same time, we build a nodes datastructure which tells us the information about the
  // at-most-two layers that the node is shared with.
  vtkCellIterator *iter = input->NewCellIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
    if (iter->GetCellType() != VTK_WEDGE)
      {
      // We only handle wedges.
      if (!warned_once)
        {
        warned_once = true;
        vtkWarningMacro("This filter currently only supports Wedges. Ignoring all other cell types.");
        }
      continue;
      }
    input->GetCellPoints(iter->GetCellId(), ptIds.GetPointer());
    assert(ptIds->GetNumberOfIds() == 6);
    for (int cc=0; cc < 3; cc++)
      {
      vtkIdType p1 = std::min(ptIds->GetId(cc), ptIds->GetId(cc+3));
      vtkIdType p2 = std::max(ptIds->GetId(cc), ptIds->GetId(cc+3));

      vtkEdgeInfo& info = edges[vtkEdge(p1, p2)];
      info.Edge.first = p1;
      info.Edge.second = p2;
      if (info.Count == 0)
        {
        info.Layer = layer->GetComponent(iter->GetCellId(), 0);
        nodes.AddEdge(info.Edge, info.Layer);
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
  return vtkDisplacePoints(edges, nodes, ipoints, opoints.GetPointer(), layerRange[1]);
}

//----------------------------------------------------------------------------
void vtkThickenLayeredCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
