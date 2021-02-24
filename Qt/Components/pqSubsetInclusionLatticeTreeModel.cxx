/*=========================================================================

   Program: ParaView
   Module:  pqSubsetInclusionLatticeTreeModel.cxx

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
#include "pqSubsetInclusionLatticeTreeModel.h"

#include "vtkCommand.h"
#include "vtkSmartPointer.h"
#include "vtkSubsetInclusionLattice.h"

#include <cassert>

//-----------------------------------------------------------------------------
class pqSubsetInclusionLatticeTreeModel::pqInternals
{
public:
  vtkSmartPointer<vtkSubsetInclusionLattice> SourceSIL;
  std::vector<unsigned long> SourceSILObserverIds;

  vtkNew<vtkSubsetInclusionLattice> SIL;
  std::vector<unsigned long> SILObserverIds;

  pqInternals(pqSubsetInclusionLatticeTreeModel*) {}

  ~pqInternals() { this->removeObservers(); }

  void removeObservers()
  {
    for (auto ids : this->SILObserverIds)
    {
      this->SIL->RemoveObserver(ids);
    }

    if (this->SourceSIL)
    {
      for (auto ids : this->SourceSILObserverIds)
      {
        this->SourceSIL.GetPointer()->RemoveObserver(ids);
      }
    }
    this->SILObserverIds.clear();
    this->SourceSILObserverIds.clear();
  }

  int silNodeId(const QModelIndex& idx) const
  {
    if (idx.isValid())
    {
      return static_cast<int>(idx.internalId());
    }
    return 0;
  }
};

//-----------------------------------------------------------------------------
pqSubsetInclusionLatticeTreeModel::pqSubsetInclusionLatticeTreeModel(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqSubsetInclusionLatticeTreeModel::pqInternals(this))
{
}

//-----------------------------------------------------------------------------
pqSubsetInclusionLatticeTreeModel::~pqSubsetInclusionLatticeTreeModel() = default;

//-----------------------------------------------------------------------------
void pqSubsetInclusionLatticeTreeModel::setSubsetInclusionLattice(vtkSubsetInclusionLattice* sil)
{
  pqInternals& internals = (*this->Internals);
  if (internals.SourceSIL.GetPointer() != sil)
  {
    this->beginResetModel();
    internals.removeObservers();
    internals.SourceSIL = sil;
    internals.SIL->DeepCopy(internals.SourceSIL);
    if (sil)
    {
      internals.SILObserverIds.push_back(internals.SIL->AddObserver(vtkCommand::StateChangedEvent,
        this, &pqSubsetInclusionLatticeTreeModel::silSelectionModified));
      internals.SourceSILObserverIds.push_back(internals.SourceSIL.GetPointer()->AddObserver(
        vtkCommand::ModifiedEvent, this, &pqSubsetInclusionLatticeTreeModel::silStructureModified));
    }
    this->endResetModel();
  }
}

//-----------------------------------------------------------------------------
vtkSubsetInclusionLattice* pqSubsetInclusionLatticeTreeModel::subsetInclusionLattice() const
{
  return this->Internals->SIL;
}

//-----------------------------------------------------------------------------
void pqSubsetInclusionLatticeTreeModel::silSelectionModified(
  vtkObject*, unsigned long, void* calldata)
{
  const int nodeid = *(reinterpret_cast<const int*>(calldata));
  QModelIndex idx = this->indexForNode(nodeid);
  QVector<int> roles = { Qt::CheckStateRole };
  Q_EMIT this->dataChanged(idx, idx, roles);
  Q_EMIT this->selectionModified();
}

//-----------------------------------------------------------------------------
void pqSubsetInclusionLatticeTreeModel::silStructureModified()
{
  this->beginResetModel();
  pqInternals& internals = (*this->Internals);
  auto selection = internals.SIL->GetSelection();
  internals.SIL->DeepCopy(internals.SourceSIL);
  internals.SIL->SetSelection(selection);
  this->endResetModel();
}

//-----------------------------------------------------------------------------
int pqSubsetInclusionLatticeTreeModel::columnCount(const QModelIndex&) const
{
  return 1;
}

//-----------------------------------------------------------------------------
int pqSubsetInclusionLatticeTreeModel::rowCount(const QModelIndex& parentIdx) const
{
  const pqInternals& internals = (*this->Internals);
  int nodeIdx = internals.silNodeId(parentIdx);
  return static_cast<int>(internals.SIL->GetChildren(nodeIdx).size());
}

//-----------------------------------------------------------------------------
QModelIndex pqSubsetInclusionLatticeTreeModel::index(
  int row, int column, const QModelIndex& parentIdx) const
{
  if (!parentIdx.isValid() && (row < 0 || column < 0))
  {
    return QModelIndex();
  }

  const pqInternals& internals = (*this->Internals);
  const int parentNodeIdx = internals.silNodeId(parentIdx);
  const auto children = internals.SIL->GetChildren(parentNodeIdx);
  if (static_cast<size_t>(row) < children.size())
  {
    return this->createIndex(row, column, static_cast<quintptr>(children[row]));
  }
  return QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex pqSubsetInclusionLatticeTreeModel::parent(const QModelIndex& idx) const
{
  if (!idx.isValid())
  {
    return QModelIndex();
  }

  const pqInternals& internals = (*this->Internals);
  const int nodeIdx = internals.silNodeId(idx);
  if (nodeIdx <= 0)
  {
    return QModelIndex();
  }

  const int parentNodeIdx = internals.SIL->GetParent(nodeIdx);
  assert(parentNodeIdx >= 0);
  if (parentNodeIdx == 0)
  {
    return QModelIndex();
  }

  int childIndex = 0;
  const int parentsParentNodeIdx = internals.SIL->GetParent(parentNodeIdx, &childIndex);
  (void)parentsParentNodeIdx;
  assert(parentsParentNodeIdx != -1);

  return this->createIndex(childIndex, 0, static_cast<quintptr>(parentNodeIdx));
}

//-----------------------------------------------------------------------------
QVariant pqSubsetInclusionLatticeTreeModel::data(const QModelIndex& idx, int role) const
{
  if (!idx.isValid())
  {
    return QVariant();
  }

  const pqInternals& internals = (*this->Internals);
  const int nodeIdx = internals.silNodeId(idx);

  switch (role)
  {
    case Qt::DisplayRole:
      return internals.SIL->GetNodeName(nodeIdx);

    case Qt::CheckStateRole:
      switch (internals.SIL->GetSelectionState(nodeIdx))
      {
        case vtkSubsetInclusionLattice::NotSelected:
          return Qt::Unchecked;
        case vtkSubsetInclusionLattice::Selected:
          return Qt::Checked;
        case vtkSubsetInclusionLattice::PartiallySelected:
          return Qt::PartiallyChecked;
      }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqSubsetInclusionLatticeTreeModel::flags(const QModelIndex& idx) const
{
  return this->Superclass::flags(idx) | Qt::ItemIsUserCheckable | Qt::ItemIsTristate;
}

//-----------------------------------------------------------------------------
bool pqSubsetInclusionLatticeTreeModel::setData(
  const QModelIndex& idx, const QVariant& value, int role)
{
  if (!idx.isValid() || idx.column() != 0 || role != Qt::CheckStateRole)
  {
    return false;
  }

  pqInternals& internals = (*this->Internals);
  const int nodeIdx = internals.silNodeId(idx);
  switch (value.value<Qt::CheckState>())
  {
    case Qt::Checked:
      internals.SIL->Select(nodeIdx);
      return true;

    case Qt::Unchecked:
      internals.SIL->Deselect(nodeIdx);
      return true;

    default:
      break;
  }

  return false;
}

//-----------------------------------------------------------------------------
QModelIndex pqSubsetInclusionLatticeTreeModel::indexForNode(int node) const
{
  if (node <= 0)
  {
    return QModelIndex();
  }

  int index = 0;

  pqInternals& internals = (*this->Internals);
  internals.SIL->GetParent(node, &index);
  return this->createIndex(index, 0, static_cast<quintptr>(node));
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSubsetInclusionLatticeTreeModel::selection() const
{
  QList<QVariant> retval;
  const pqInternals& internals = (*this->Internals);
  const auto vtkSel = internals.SIL->GetSelection();
  for (auto apath : vtkSel)
  {
    retval.push_back(QVariant(apath.first.c_str()));
    retval.push_back(QVariant(apath.second ? 1 : 0));
  }
  return retval;
}

//-----------------------------------------------------------------------------
void pqSubsetInclusionLatticeTreeModel::setSelection(const QList<QVariant>& qtSel)
{
  std::map<std::string, bool> vtkSel;
  for (int cc = 0, max = qtSel.size(); (cc + 1) < max; cc += 2)
  {
    vtkSel[qtSel[cc].toString().toUtf8().constData()] = qtSel[cc + 1].toBool();
  }

  pqInternals& internals = (*this->Internals);
  internals.SIL->SetSelection(vtkSel);
  Q_EMIT this->selectionModified();
}
