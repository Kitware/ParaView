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
#include "vtkMemberFunctionCommand.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSMSILDomain.h"
#include "vtkSMSILModel.h"
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
pqSILModel::pqSILModel(QObject* _parent /*=0*/)
  : Superclass(_parent)
{
  this->SILDomain = nullptr;
  this->SILDomainObserverId = 0;
  this->SILModel = vtkSMSILModel::New();
  vtkCommand* observer = vtkMakeMemberFunctionCommand(*this, &pqSILModel::checkStateUpdated);
  this->SILModel->AddObserver(vtkCommand::UpdateDataEvent, observer);
  observer->Delete();
  this->ModelIndexCache = new QMap<vtkIdType, QModelIndex>();
}

//-----------------------------------------------------------------------------
pqSILModel::~pqSILModel()
{
  delete this->ModelIndexCache;
  this->ModelIndexCache = 0;
  this->SILModel->Delete();

  this->setSILDomain(nullptr);
}

//-----------------------------------------------------------------------------
void pqSILModel::update()
{
  this->beginResetModel();
  bool prev = this->blockSignals(true);
  auto sil = this->SILDomain->GetSIL();
  this->SILModel->Initialize(sil);
  this->ModelIndexCache->clear();

  // Update the list of hierarchies.
  this->Hierarchies.clear();
  this->HierarchyVertexIds.clear();

  vtkStringArray* names =
    vtkStringArray::SafeDownCast(sil->GetVertexData()->GetAbstractArray("Names"));
  vtkAdjacentVertexIterator* iter = vtkAdjacentVertexIterator::New();
  sil->GetAdjacentVertices(0, iter);
  int childNo = 0;
  while (iter->HasNext())
  {
    vtkIdType vertexid = iter->Next();
    QString hierarchyName = QString(names->GetValue(vertexid));
    this->Hierarchies[hierarchyName] =
      this->createIndex(childNo, 0, static_cast<quint32>(vertexid));
    this->collectLeaves(vertexid, this->HierarchyVertexIds[hierarchyName]);
    childNo++;
  }
  iter->Delete();
  this->blockSignals(prev);
  this->endResetModel();
}

//-----------------------------------------------------------------------------
void pqSILModel::collectLeaves(vtkIdType vertexid, std::set<vtkIdType>& id_set)
{
  this->SILModel->GetLeaves(id_set, vertexid, /*traverse_cross_edges=*/false);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSILModel::status(const QString& hierarchyName) const
{
  QList<QVariant> values;
  if (!this->HierarchyVertexIds.contains(hierarchyName))
  {
    return values;
  }

  const std::set<vtkIdType>& vertexIds = this->HierarchyVertexIds[hierarchyName];
  foreach (vtkIdType vertex, vertexIds)
  {
    bool checked = (this->SILModel->GetCheckStatus(vertex) == vtkSMSILModel::CHECKED);
    values.push_back(QString(this->SILModel->GetName(vertex)));
    values.push_back(checked ? 1 : 0);
  }
  return values;
}

//-----------------------------------------------------------------------------
void pqSILModel::setStatus(const QString& hierarchyName, const QList<QVariant>& values)
{
  if (!this->HierarchyVertexIds.contains(hierarchyName))
  {
    return;
  }

  QMap<QString, bool> check_status;
  for (int cc = 0; (cc + 1) < values.size(); cc += 2)
  {
    QString name = values[cc].toString();
    bool checked = values[cc + 1].toBool();
    check_status[name] = checked;
  }

  const std::set<vtkIdType>& vertexIds = this->HierarchyVertexIds[hierarchyName];
  foreach (vtkIdType vertex, vertexIds)
  {
    QString name = QString(this->SILModel->GetName(vertex));
    if (!check_status.contains(name) || check_status[name] == true)
    {
      this->SILModel->SetCheckState(vertex, vtkSMSILModel::CHECKED);
    }
    else
    {
      this->SILModel->SetCheckState(vertex, vtkSMSILModel::UNCHECKED);
    }
  }
  emit this->checkStatusChanged();
}

//-----------------------------------------------------------------------------
void pqSILModel::setSILDomain(vtkSMSILDomain* domain)
{
  if (this->SILDomain == domain)
  {
    // Nothing to do
    return;
  }

  if (this->SILDomain && this->SILDomainObserverId != 0)
  {
    this->SILDomain->RemoveObserver(this->SILDomainObserverId);
    this->SILDomainObserverId = 0;
  }

  this->SILDomain = domain;
  if (this->SILDomain)
  {
    this->SILDomainObserverId = this->SILDomain->AddObserver(
      vtkCommand::DomainModifiedEvent, this, &pqSILModel::domainModified);
  }
}

//-----------------------------------------------------------------------------
void pqSILModel::domainModified()
{
  if (this->SILDomain)
  {
    this->update();
  }
}

//-----------------------------------------------------------------------------
QModelIndex pqSILModel::hierarchyIndex(const QString& hierarchyName) const
{
  if (this->Hierarchies.contains(hierarchyName))
  {
    return this->Hierarchies[hierarchyName];
  }

  // Return a dummy index referring to an empty tree.
  return this->createIndex(PQ_INVALID_INDEX, PQ_INVALID_INDEX);
}

//-----------------------------------------------------------------------------
QModelIndex pqSILModel::index(
  int row, int column, const QModelIndex& parentIndex /*=QModelIndex()*/) const
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
  if (!this->SILModel)
  {
    return QModelIndex();
  }

  auto sil = this->SILModel->GetSIL();
  if (sil && !this->isLeaf(vertexId))
  {
    if (row < sil->GetOutDegree(vertexId))
    {
      vtkOutEdgeType edge = sil->GetOutEdge(vertexId, row);
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
bool pqSILModel::hasChildren(const QModelIndex& parentIndex /*=QModelIndex()*/) const
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
int pqSILModel::rowCount(const QModelIndex& parentIndex /*=QModelIndex()*/) const
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
  return this->SILModel->GetNumberOfChildren(vertexId);
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

  return this->SILModel->GetParentVertex(vertexId);
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
vtkIdType pqSILModel::findVertex(const char* name) const
{
  if (!name)
  {
    return -1;
  }

  return this->SILModel->FindVertex(name);
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
  auto sil = this->SILModel->GetSIL();
  sil->GetOutEdges(parentId, iter);
  vtkDataArray* crossEdgesArray =
    vtkDataArray::SafeDownCast(sil->GetEdgeData()->GetAbstractArray("CrossEdges"));
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
QVariant pqSILModel::data(const QModelIndex& idx, int role /*=Qt::DisplayRole*/) const
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
      return QVariant(this->SILModel->GetName(vertexId));
    }
    break;

    case Qt::CheckStateRole:
      return QVariant(this->SILModel->GetCheckStatus(vertexId));
      break;
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
bool pqSILModel::setData(const QModelIndex& idx, const QVariant& value, int role)
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
    this->SILModel->SetCheckState(
      vertexId, checked ? vtkSMSILModel::CHECKED : vtkSMSILModel::UNCHECKED);
    emit this->checkStatusChanged();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqSILModel::flags(const QModelIndex& idx) const
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

  Qt::ItemFlags item_flags = (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);

  if (this->isLeaf(vertexId) == false)
  {
    item_flags |= Qt::ItemIsTristate;
  }

  return item_flags;
}

//-----------------------------------------------------------------------------
void pqSILModel::checkStateUpdated(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventid), void* calldata)
{
  vtkIdType vertexId = *reinterpret_cast<vtkIdType*>(calldata);
  QModelIndex idx = this->makeIndex(vertexId);
  emit this->dataChanged(idx, idx, QVector<int>{ Qt::CheckStateRole });
}
