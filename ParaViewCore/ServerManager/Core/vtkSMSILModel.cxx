/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSILModel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSILModel.h"

#include "vtkAdjacentVertexIterator.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkInEdgeIterator.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMSILDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"

#include <map>
#include <string>
#include <vector>

class vtkSMSILModel::vtkInternals
{
public:
  typedef std::vector<vtkSMSILModel::CheckState> CheckStatesType;
  CheckStatesType CheckStates;

  typedef std::map<std::string, vtkIdType> VertexNameMapType;
  VertexNameMapType VertexNameMap;

  // Returns the vertex ids for all leaf nodes in the subtree identified by
  // vertexid.
  void CollectLeaves(
    vtkGraph* sil, vtkIdType vertexid, std::set<vtkIdType>& list, bool traverse_cross_edges)
  {
    vtkDataArray* crossEdgesArray =
      vtkDataArray::SafeDownCast(sil->GetEdgeData()->GetAbstractArray("CrossEdges"));

    bool has_child_edge = false;
    vtkOutEdgeIterator* iter = vtkOutEdgeIterator::New();
    sil->GetOutEdges(vertexid, iter);
    while (iter->HasNext())
    {
      vtkOutEdgeType edge = iter->Next();
      if (traverse_cross_edges || crossEdgesArray->GetTuple1(edge.Id) == 0)
      {
        has_child_edge = true;
        this->CollectLeaves(sil, edge.Target, list, traverse_cross_edges);
      }
    }
    iter->Delete();

    if (!has_child_edge)
    {
      list.insert(vertexid);
    }
  }
};

vtkStandardNewMacro(vtkSMSILModel);
//-----------------------------------------------------------------------------
vtkSMSILModel::vtkSMSILModel()
{
  this->SIL = 0;
  this->Property = 0;
  this->Proxy = 0;
  this->PropertyObserver = vtkMakeMemberFunctionCommand(*this, &vtkSMSILModel::OnPropertyModified);
  this->DomainObserver = vtkMakeMemberFunctionCommand(*this, &vtkSMSILModel::OnDomainModified);
  this->Internals = new vtkInternals();
  this->BlockUpdate = false;
}

//-----------------------------------------------------------------------------
vtkSMSILModel::~vtkSMSILModel()
{
  this->Initialize(0);
  this->Initialize(0, 0);

  vtkMemberFunctionCommand<vtkSMSILModel>::SafeDownCast(this->PropertyObserver)->Reset();
  this->PropertyObserver->Delete();
  this->PropertyObserver = 0;

  vtkMemberFunctionCommand<vtkSMSILModel>::SafeDownCast(this->DomainObserver)->Reset();
  this->DomainObserver->Delete();
  this->DomainObserver = 0;
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::SetSIL(vtkGraph* sil)
{
  vtkSetObjectBodyMacro(SIL, vtkGraph, sil);
  if (!this->SIL)
  {
    return;
  }

  vtkIdType numVertices = sil->GetNumberOfVertices();
  int cursize = static_cast<int>(this->Internals->CheckStates.size());
  this->Internals->CheckStates.resize(numVertices);
  for (int cc = cursize; cc < numVertices; cc++)
  {
    this->Internals->CheckStates[cc] = vtkSMSILModel::UNCHECKED;
  }

  // Update the name map.
  vtkStringArray* names =
    vtkStringArray::SafeDownCast(this->SIL->GetVertexData()->GetAbstractArray("Names"));
  this->Internals->VertexNameMap.clear();
  for (vtkIdType kk = 0; kk < numVertices; kk++)
  {
    this->Internals->VertexNameMap[names->GetValue(kk)] = kk;
  }

  if (numVertices > 0)
  {
    this->UpdateCheck(0);
  }
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::Initialize(vtkGraph* sil)
{
  // unset the proxy and property, if any.
  this->Initialize(0, 0);
  this->SetSIL(sil);
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::Initialize(vtkSMProxy* proxy, vtkSMStringVectorProperty* svp)
{
  if (this->Property == svp && this->Proxy == proxy)
  {
    return;
  }
  if (this->Property)
  {
    this->Property->RemoveObserver(this->PropertyObserver);
    auto domain = this->Property->FindDomain<vtkSMSILDomain>();
    if (domain)
    {
      domain->RemoveObserver(this->DomainObserver);
    }
  }
  vtkSetObjectBodyMacro(Proxy, vtkSMProxy, proxy);
  vtkSetObjectBodyMacro(Property, vtkSMStringVectorProperty, svp);
  if (this->Property && this->Proxy)
  {
    // unset the SIL if any.
    this->Property->AddObserver(vtkCommand::ModifiedEvent, this->PropertyObserver);

    auto domain = this->Property->FindDomain<vtkSMSILDomain>();
    if (domain)
    {
      domain->AddObserver(vtkCommand::DomainModifiedEvent, this->DomainObserver);
    }

    this->OnDomainModified();
    this->OnPropertyModified();
  }
}

//-----------------------------------------------------------------------------
vtkIdType vtkSMSILModel::GetChildVertex(vtkIdType parentid, int row)
{
  vtkIdType vertexId = 0; // the root for the graph.

  // This assumes that all out-going edges from a node are of the same type i.e.
  // they are either child edges or cross edges, and not a mix of the two.
  if (row >= 0 && row < this->GetNumberOfChildren(parentid) &&
    row < this->SIL->GetOutDegree(parentid))
  {
    vtkOutEdgeType edge = this->SIL->GetOutEdge(parentid, row);
    vertexId = edge.Target;
  }
  return vertexId;
}

//-----------------------------------------------------------------------------
int vtkSMSILModel::GetNumberOfChildren(vtkIdType vertexId)
{
  // count children edges (skipping cross edges).

  int count = 0;
  if (this->SIL)
  {
    vtkOutEdgeIterator* iter = vtkOutEdgeIterator::New();
    this->SIL->GetOutEdges(vertexId, iter);
    vtkDataArray* crossEdgesArray =
      vtkDataArray::SafeDownCast(this->SIL->GetEdgeData()->GetAbstractArray("CrossEdges"));
    while (iter->HasNext())
    {
      vtkOutEdgeType edge = iter->Next();
      if (crossEdgesArray->GetTuple1(edge.Id) == 0)
      {
        count++;
      }
    }
    iter->Delete();
  }
  return count;
}

//-----------------------------------------------------------------------------
vtkIdType vtkSMSILModel::GetParentVertex(vtkIdType vertexId)
{
  if (vertexId == 0)
  {
    vtkErrorMacro("Root has no parent.");
    return 0;
  }

  vtkInEdgeIterator* iter = vtkInEdgeIterator::New();
  this->SIL->GetInEdges(vertexId, iter);
  vtkDataArray* crossEdgesArray =
    vtkDataArray::SafeDownCast(this->SIL->GetEdgeData()->GetAbstractArray("CrossEdges"));
  while (iter->HasNext())
  {
    vtkInEdgeType edge = iter->Next();
    if (crossEdgesArray->GetTuple1(edge.Id) == 0)
    {
      iter->Delete();
      return edge.Source;
    }
  }
  iter->Delete();
  vtkErrorMacro(<< vertexId << " has no parent! It's possible that the SIL was "
                               "built incorrectly.");
  return 0;
}

//-----------------------------------------------------------------------------
const char* vtkSMSILModel::GetName(vtkIdType vertex)
{
  vtkStringArray* names =
    vtkStringArray::SafeDownCast(this->SIL->GetVertexData()->GetAbstractArray("Names"));
  if (vertex >= 0 && vertex < names->GetNumberOfTuples())
  {
    return names->GetValue(vertex).c_str();
  }

  vtkErrorMacro("Invalid index: " << vertex);
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSMSILModel::GetCheckStatus(vtkIdType vertex)
{
  if (vertex >= 0 && vertex < static_cast<vtkIdType>(this->Internals->CheckStates.size()))
  {
    return this->Internals->CheckStates[vertex];
  }

  return UNCHECKED;
}

//-----------------------------------------------------------------------------
bool vtkSMSILModel::SetCheckState(vtkIdType vertexId, int status)
{
  if (vertexId >= 0 && vertexId < static_cast<vtkIdType>(this->Internals->CheckStates.size()))
  {
    bool checked = (status == vtkSMSILModel::CHECKED);
    this->Check(vertexId, checked, -1);
    this->UpdateProperty();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::Check(vtkIdType vertexid, bool checked, vtkIdType inedgeid /*=-1*/)
{
  vtkSMSILModel::CheckState newState = checked ? vtkSMSILModel::CHECKED : vtkSMSILModel::UNCHECKED;
  if (this->Internals->CheckStates[vertexid] == newState)
  {
    // nothing to change.
    return;
  }

  this->Internals->CheckStates[vertexid] = newState;

  // * For each out-edge, update check.
  vtkOutEdgeIterator* outEdgeIter = vtkOutEdgeIterator::New();
  this->SIL->GetOutEdges(vertexid, outEdgeIter);
  while (outEdgeIter->HasNext())
  {
    vtkOutEdgeType edge = outEdgeIter->Next();
    this->Check(edge.Target, checked, edge.Id);
  }
  outEdgeIter->Delete();

  // * For each in-edge (except inedgeid), update the check state.
  vtkInEdgeIterator* inEdgeIter = vtkInEdgeIterator::New();
  this->SIL->GetInEdges(vertexid, inEdgeIter);
  while (inEdgeIter->HasNext())
  {
    vtkInEdgeType edge = inEdgeIter->Next();
    if (edge.Id != inedgeid)
    {
      this->UpdateCheck(edge.Source);
    }
  }
  inEdgeIter->Delete();

  this->InvokeEvent(vtkCommand::UpdateDataEvent, &vertexid);
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::UpdateCheck(vtkIdType vertexid)
{
  int children_count = 0;
  int checked_children_count = 0;
  bool partial_child = false;

  // Look at the immediate children of vertexid and decide the check state for
  // vertexid.
  vtkAdjacentVertexIterator* aiter = vtkAdjacentVertexIterator::New();
  this->SIL->GetAdjacentVertices(vertexid, aiter);
  while (aiter->HasNext() && partial_child == false)
  {
    children_count++;
    vtkIdType childVertex = aiter->Next();
    vtkSMSILModel::CheckState childCheckState = this->Internals->CheckStates[childVertex];
    switch (childCheckState)
    {
      case vtkSMSILModel::PARTIAL:
        partial_child = true;
        break;

      case vtkSMSILModel::CHECKED:
        checked_children_count++;
        break;

      default:
        break;
    }
  }
  aiter->Delete();

  vtkSMSILModel::CheckState newState;
  if (partial_child)
  {
    newState = vtkSMSILModel::PARTIAL;
  }
  else if (children_count == checked_children_count)
  {
    newState = vtkSMSILModel::CHECKED;
  }
  else if (checked_children_count == 0)
  {
    newState = vtkSMSILModel::UNCHECKED;
  }
  else
  {
    newState = vtkSMSILModel::PARTIAL;
  }

  if (newState != this->Internals->CheckStates[vertexid])
  {
    this->Internals->CheckStates[vertexid] = newState;
    // Ask all the inedges to update checks.

    vtkInEdgeIterator* inEdgeIter = vtkInEdgeIterator::New();
    this->SIL->GetInEdges(vertexid, inEdgeIter);
    while (inEdgeIter->HasNext())
    {
      this->UpdateCheck(inEdgeIter->Next().Source);
    }
    inEdgeIter->Delete();

    this->InvokeEvent(vtkCommand::UpdateDataEvent, &vertexid);
  }
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::OnPropertyModified()
{
  this->UpdateStateFromProperty(this->Property);
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::OnDomainModified()
{
  auto domain = this->Property->FindDomain<vtkSMSILDomain>();
  this->SetSIL(domain->GetSIL());
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::UpdateProperty()
{
  if (this->Proxy && this->Property)
  {
    this->UpdatePropertyValue(this->Property);
    this->Proxy->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::UpdatePropertyValue(vtkSMStringVectorProperty* svp)
{
  if (!svp)
  {
    return;
  }

  if (this->BlockUpdate)
  {
    return;
  }

  this->BlockUpdate = true;

  // TODO: eventually, we may add support to specify the subtree using the
  // SILDomain. For now, I am just going to get the "true leaves".

  std::set<vtkIdType> leaf_ids;
  this->Internals->CollectLeaves(this->SIL, 0, leaf_ids, /*traverse_cross_edges=*/true);

  const char** values = new const char*[leaf_ids.size() * 2 + 1];
  const char* const check_states[] = { "0", "1", "2" };
  int cc = 0;
  // Now get the check states (and names) for all these leaf_ids.
  std::set<vtkIdType>::iterator iter;
  for (iter = leaf_ids.begin(); iter != leaf_ids.end(); ++iter, ++cc)
  {
    values[2 * cc] = this->GetName(*iter);
    values[2 * cc + 1] = check_states[this->GetCheckStatus(*iter)];
  }
  svp->SetElements(values, static_cast<unsigned int>(leaf_ids.size()) * 2);

  delete[] values;
  this->BlockUpdate = false;
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::UpdateStateFromProperty(vtkSMStringVectorProperty* svp)
{
  if (this->BlockUpdate || !svp)
  {
    return;
  }

  this->BlockUpdate = true;
  this->UncheckAll();

  for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfElements(); cc += 2)
  {
    const char* vertexname = svp->GetElement(cc);
    int check_state = atoi(svp->GetElement(cc + 1));
    vtkIdType vertexid = this->FindVertex(vertexname);
    if (vertexid == -1)
    {
      continue;
    }

    switch (check_state)
    {
      case CHECKED:
        this->SetCheckState(vertexid, CHECKED);
        break;

      case UNCHECKED:
        this->SetCheckState(vertexid, UNCHECKED);
        break;
      default:
        break;
    }
  }
  this->BlockUpdate = false;
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::CheckAll()
{
  this->SetCheckState(static_cast<vtkIdType>(0), vtkSMSILModel::CHECKED);
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::UncheckAll()
{
  this->SetCheckState(static_cast<vtkIdType>(0), vtkSMSILModel::UNCHECKED);
}

//-----------------------------------------------------------------------------
vtkIdType vtkSMSILModel::FindVertex(const char* name)
{
  vtkInternals::VertexNameMapType::iterator iter = this->Internals->VertexNameMap.find(name);
  if (iter != this->Internals->VertexNameMap.end())
  {
    return iter->second;
  }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::GetLeaves(
  std::set<vtkIdType>& leaves, vtkIdType root, bool traverse_cross_edges)
{
  this->Internals->CollectLeaves(this->SIL, root, leaves, traverse_cross_edges);
}

//-----------------------------------------------------------------------------
void vtkSMSILModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SIL: " << this->SIL << endl;
}
