/*=========================================================================

   Program: ParaView
   Module:    pqSILModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqSILModel.h"

// Server Manager Includes.
#include "vtkAdjacentVertexIterator.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkInEdgeIterator.h"
#include "vtkOutEdgeIterator.h"
#include "vtkStringArray.h"

// Qt Includes.
#include <QtDebug>

// ParaView Includes.

#include <cassert>

#define PQ_INVALID_INDEX -1947

inline bool INDEX_IS_VALID(const QModelIndex& idx)
{
  return (idx.row() != PQ_INVALID_INDEX && idx.column() != PQ_INVALID_INDEX);
}

//-----------------------------------------------------------------------------
pqSILModel::pqSILModel(QObject* _parent/*=0*/) : Superclass(_parent)
{
  this->SIL = 0;
  this->ModelIndexCache = new QMap<vtkIdType, QModelIndex>();
}

//-----------------------------------------------------------------------------
pqSILModel::~pqSILModel()
{
  delete this->ModelIndexCache;
  this->ModelIndexCache = 0;
}

//-----------------------------------------------------------------------------
void pqSILModel::update(vtkGraph* sil)
{
  bool prev = this->blockSignals(true);
  this->SIL = sil;
  this->ModelIndexCache->clear();

  vtkIdType numVertices = sil->GetNumberOfVertices();
  int cursize = this->CheckStates.size();
  this->CheckStates.resize(numVertices);
  for (int cc=cursize; cc < numVertices; cc++)
    {
    this->CheckStates[cc] = Qt::Checked;
    }

  // Update the list of hierarchies.
  this->Hierarchies.clear();
  this->HierarchyVertexIds.clear();

  vtkStringArray* names = vtkStringArray::SafeDownCast(
    this->SIL->GetVertexData()->GetAbstractArray("Names"));
  vtkAdjacentVertexIterator* iter = vtkAdjacentVertexIterator::New();
  this->SIL->GetAdjacentVertices(0, iter);

  int childNo = 0;
  while (iter->HasNext())
    {
    vtkIdType vertexid = iter->Next();
    QString hierarchyName = QString(names->GetValue(vertexid));
    this->Hierarchies[hierarchyName] = 
      this->createIndex(childNo, 0, static_cast<quint32>(vertexid));
    this->collectLeaves(vertexid, this->HierarchyVertexIds[hierarchyName]);
    childNo++;

    // This updates the checkstates for the tree, starting with the leaf nodes
    // thus ensuring that at the end of the initialization, all check states
    // will be correct.
    foreach (vtkIdType vid, this->HierarchyVertexIds[hierarchyName])
      {
      this->update_check(vid);
      }
    }
  iter->Delete();
  
  this->blockSignals(prev);
  this->reset();
}

//-----------------------------------------------------------------------------
void pqSILModel::collectLeaves(vtkIdType vertexid, QList<vtkIdType>& list)
{
  vtkDataArray* crossEdgesArray = vtkDataArray::SafeDownCast(
    this->SIL->GetEdgeData()->GetAbstractArray("CrossEdges"));

  bool has_child_edge = false;
  vtkOutEdgeIterator* iter = vtkOutEdgeIterator::New();
  this->SIL->GetOutEdges(vertexid, iter);
  while (iter->HasNext())
    {
    vtkOutEdgeType edge = iter->Next();
    if (crossEdgesArray->GetTuple1(edge.Id) == 0)
      {
      has_child_edge = true;
      this->collectLeaves(edge.Target, list);
      }
    }
  iter->Delete();

  if (!has_child_edge)
    {
    list.push_back(vertexid);
    }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSILModel::status(const QString& hierarchyName) const
{
  QList<QVariant> values;
  if (!this->HierarchyVertexIds.contains(hierarchyName))
    {
    return values;
    }

  const QList<vtkIdType> &vertexIds = this->HierarchyVertexIds[hierarchyName];
  vtkStringArray* names = vtkStringArray::SafeDownCast(
    this->SIL->GetVertexData()->GetAbstractArray("Names"));

  foreach (vtkIdType vertex, vertexIds)
    {
    bool checked = (this->CheckStates[vertex] == Qt::Checked);
    values.push_back(QString(names->GetValue(vertex)));
    values.push_back(checked? 1 : 0);
    }
  return values;
}

//-----------------------------------------------------------------------------
void pqSILModel::setStatus(const QString& hierarchyName,
  const QList<QVariant>& values)
{
  if (!this->HierarchyVertexIds.contains(hierarchyName))
    {
    return;
    }
  
  QMap<QString, bool> check_status;
  for (int cc=0; (cc+1) < values.size(); cc+=2)
    {
    QString name = values[cc].toString();
    bool checked = values[cc+1].toBool();
    check_status[name] = checked;
    }

  const QList<vtkIdType> &vertexIds = this->HierarchyVertexIds[hierarchyName];
  vtkStringArray* names = vtkStringArray::SafeDownCast(
    this->SIL->GetVertexData()->GetAbstractArray("Names"));
  foreach (vtkIdType vertex, vertexIds)
    {
    QString name = QString(names->GetValue(vertex));
    if (!check_status.contains(name) || check_status[name] == true)
      {
      this->check(vertex, true);
      }
    else
      {
      this->check(vertex, false);
      }
    }
  emit this->checkStatusChanged();
}

//-----------------------------------------------------------------------------
QModelIndex pqSILModel::hierarchyIndex(const QString& hierarchyName) const
{
  if (this->Hierarchies.contains(hierarchyName))
    {
    return this->Hierarchies[hierarchyName];
    }

  // Return a dummy index referring to an empty tree.
  return this->createIndex(PQ_INVALID_INDEX, PQ_INVALID_INDEX, static_cast<quint32>(0));
}

//-----------------------------------------------------------------------------
QModelIndex pqSILModel::index(int row, int column,
  const QModelIndex &parentIndex/*=QModelIndex()*/) const
{
  if (row < 0 || column < 0 || column >= this->columnCount())
    {
    return QModelIndex();
    }

  vtkIdType vertexId = 0; // the root for the graph.
  if (parentIndex.isValid())
    {
    vertexId = static_cast<vtkIdType>(parentIndex.internalId());
    }
    
  // Ensure that the vertexId refers to a non-leaf node.
  if (this->SIL && !this->isLeaf(vertexId))
    {
    if (row < this->SIL->GetOutDegree(vertexId))
      {
      vtkOutEdgeType edge = this->SIL->GetOutEdge(vertexId, row);
      return this->createIndex(row, column, static_cast<quint32>(edge.Target));
      }
    }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
int pqSILModel::columnCount(const QModelIndex& vtkNotUsed(parent)) const
{
  return 1;
}

//-----------------------------------------------------------------------------
bool pqSILModel::hasChildren(const QModelIndex &parentIndex/*=QModelIndex()*/) const
{
  if (!INDEX_IS_VALID(parentIndex))
    {
    return false;
    }

  vtkIdType vertexId = 0; // the root for the graph.
  if (parentIndex.isValid())
    {
    vertexId = static_cast<vtkIdType>(parentIndex.internalId());
    }



  return !this->isLeaf(vertexId);
}

//-----------------------------------------------------------------------------
int pqSILModel::rowCount(const QModelIndex &parentIndex/*=QModelIndex()*/) const
{
  if (!INDEX_IS_VALID(parentIndex))
    {
    return 0;
    }

  vtkIdType vertexId = 0; // the root for the graph.
  if (parentIndex.isValid())
    {
    vertexId = static_cast<vtkIdType>(parentIndex.internalId());
    }

  return this->childrenCount(vertexId);
}

//-----------------------------------------------------------------------------
int pqSILModel::childrenCount(vtkIdType vertexId) const
{
  // count children edges (skipping cross edges).
  
  int count = 0;
  vtkOutEdgeIterator* iter = vtkOutEdgeIterator::New();
  this->SIL->GetOutEdges(vertexId, iter);
  vtkDataArray* crossEdgesArray = vtkDataArray::SafeDownCast(
    this->SIL->GetEdgeData()->GetAbstractArray("CrossEdges"));
  while (iter->HasNext())
    {
    vtkOutEdgeType edge = iter->Next();
    if (crossEdgesArray->GetTuple1(edge.Id) == 0)
      {
      count++; 
      }
    }
  iter->Delete();
  return count;
}

//-----------------------------------------------------------------------------
bool pqSILModel::isLeaf(vtkIdType vertexId) const
{
  return (this->childrenCount(vertexId) == 0);
}

//-----------------------------------------------------------------------------
vtkIdType pqSILModel::parent(vtkIdType vertexId) const
{
  if (vertexId == 0)
    {
    qCritical() << "Root has no parent.";
    return 0;
    }

  vtkInEdgeIterator* iter = vtkInEdgeIterator::New();
  this->SIL->GetInEdges(vertexId, iter);
  vtkDataArray* crossEdgesArray = vtkDataArray::SafeDownCast(
    this->SIL->GetEdgeData()->GetAbstractArray("CrossEdges"));
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
  qCritical() << vertexId << " has no parent!";
  return 0;
}

//-----------------------------------------------------------------------------
QModelIndex pqSILModel::parent(const QModelIndex& idx) const
{
  if (!INDEX_IS_VALID(idx))
    {
    return QModelIndex();
    }

  if (idx.isValid())
    {
    vtkIdType vertexId = static_cast<vtkIdType>(idx.internalId());
    vtkIdType parentId = this->parent(vertexId);
    return this->makeIndex(parentId);
    }
  
  return QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex pqSILModel::makeIndex(vtkIdType vertexid) const
{
  if (vertexid == 0)
    {
    return QModelIndex();
    }

  // Use cache if possible.
  if (this->ModelIndexCache->contains(vertexid))
    {
    return (*this->ModelIndexCache)[vertexid];
    }
  
  vtkIdType parentId = this->parent(vertexid);

  int count = 0;
  vtkSmartPointer<vtkOutEdgeIterator> iter = vtkSmartPointer<vtkOutEdgeIterator>::New();
  this->SIL->GetOutEdges(parentId, iter);
  vtkDataArray* crossEdgesArray = vtkDataArray::SafeDownCast(
    this->SIL->GetEdgeData()->GetAbstractArray("CrossEdges"));
  while (iter->HasNext())
    {
    vtkOutEdgeType edge = iter->Next();
    if (crossEdgesArray->GetTuple1(edge.Id) == 0)
      {
      if (edge.Target == vertexid)
        {
        QModelIndex idx = this->createIndex(count, 0, static_cast<quint32>(vertexid));
        // save it in cache to avoid computation in future.
        (*this->ModelIndexCache)[vertexid] = idx;
        return idx;
        }
      count++; 
      }
    }

  qCritical() << "Couldn't make index for vertex: " << vertexid;
  return QModelIndex();
}

//-----------------------------------------------------------------------------
QVariant pqSILModel::data(const QModelIndex &idx,
  int role/*=Qt::DisplayRole*/) const
{
  if (!INDEX_IS_VALID(idx))
    {
    return QVariant();
    }

  vtkIdType vertexId = 0;
  if (idx.isValid())
    {
    vertexId = static_cast<vtkIdType>(idx.internalId());
    }

  switch (role)
    {
  case Qt::DisplayRole:
  case Qt::ToolTipRole:
      {
      vtkStringArray* names = vtkStringArray::SafeDownCast(
        this->SIL->GetVertexData()->GetAbstractArray("Names"));
      return QVariant(names->GetValue(vertexId));
      }
    break;

  case Qt::CheckStateRole:
    return QVariant(this->CheckStates[vertexId]);
    break;
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
bool pqSILModel::setData(const QModelIndex &idx, const QVariant& value, 
  int role)
{
  if (!INDEX_IS_VALID(idx))
    {
    return false;
    }

  vtkIdType vertexId = 0;
  if (idx.isValid())
    {
    vertexId = static_cast<vtkIdType>(idx.internalId());
    }

  // Only check state can be changed for an item.
  if (role == Qt::CheckStateRole)
    {
    bool checked = (value.toInt() == Qt::Checked);
    this->check(vertexId, checked, -1);
    emit this->checkStatusChanged();
    //emit this->dataChanged(idx, idx);
    return true;
    }

  return false;
}


//-----------------------------------------------------------------------------
void pqSILModel::check(vtkIdType vertexid, bool checked, 
  vtkIdType inedgeid/*=-1*/)
{
  Qt::CheckState newState = checked? Qt::Checked : Qt::Unchecked;
  if (this->CheckStates[vertexid] == newState)
    {
    // nothing to change.
    return;
    }

  this->CheckStates[vertexid] = newState;

  // * For each out-edge, update check.
  vtkOutEdgeIterator* outEdgeIter = vtkOutEdgeIterator::New();
  this->SIL->GetOutEdges(vertexid, outEdgeIter);
  while (outEdgeIter->HasNext())
    {
    vtkOutEdgeType edge = outEdgeIter->Next();
    this->check(edge.Target, checked, edge.Id);
    }
  outEdgeIter->Delete();

  //emit this->dataChanged(this->createIndex(0, 0, static_cast<quint32>(firstChild)),
  //  this->createIndex(outDegree-1, 0, static_cast<quint32>(lastChild)));

  // * For each in-edge (except inedgeid), update the check state.
  vtkInEdgeIterator* inEdgeIter = vtkInEdgeIterator::New();
  this->SIL->GetInEdges(vertexid, inEdgeIter);
  while (inEdgeIter->HasNext())
    {
    vtkInEdgeType edge = inEdgeIter->Next();
    if (edge.Id != inedgeid)
      {
      this->update_check(edge.Source);
      }
    }
  inEdgeIter->Delete();

  QModelIndex idx = this->makeIndex(vertexid);
  emit this->dataChanged(idx, idx);
}

//-----------------------------------------------------------------------------
void pqSILModel::update_check(vtkIdType vertexid)
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
    Qt::CheckState childCheckState = this->CheckStates[childVertex];
    switch (childCheckState)
      {
    case Qt::PartiallyChecked:
      partial_child = true;
      break;

    case Qt::Checked:
      checked_children_count++;
      break;

    default:
      break;
      }
    }
  aiter->Delete();

  Qt::CheckState newState;
  if (partial_child)
    {
    newState = Qt::PartiallyChecked;
    }
  else if (children_count == checked_children_count)
    {
    newState = Qt::Checked;
    }
  else if (checked_children_count == 0)
    {
    newState = Qt::Unchecked;
    }
  else
    {
    newState = Qt::PartiallyChecked;
    }

  if (newState != this->CheckStates[vertexid])
    {
    this->CheckStates[vertexid] = newState;
    // Ask all the inedges to update checks.

    vtkInEdgeIterator* inEdgeIter = vtkInEdgeIterator::New();
    this->SIL->GetInEdges(vertexid, inEdgeIter);
    while (inEdgeIter->HasNext())
      {
      this->update_check(inEdgeIter->Next().Source);
      }
    inEdgeIter->Delete();

    QModelIndex idx = this->makeIndex(vertexid);
    emit this->dataChanged(idx, idx);
    }
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqSILModel::flags(const QModelIndex &idx) const
{
  if (!INDEX_IS_VALID(idx))
    {
    return 0;
    }

  vtkIdType vertexId = 0;
  if (idx.isValid())
    {
    vertexId = static_cast<vtkIdType>(idx.internalId());
    }

  Qt::ItemFlags item_flags = 
    (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);

  if (this->isLeaf(vertexId))
    {
    item_flags |= Qt::ItemIsTristate;
    }

  return item_flags;
}
