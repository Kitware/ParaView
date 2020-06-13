/*=========================================================================

   Program: ParaView
   Module:  pqDataAssemblyTreeModel.cxx

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
#include "pqDataAssemblyTreeModel.h"

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <set>
#include <unordered_map>

namespace
{
int getNodeID(const QModelIndex& indx)
{
  if (indx.isValid())
  {
    return static_cast<int>(indx.internalId());
  }
  return -1;
}

class CallbackDataVisitor : public vtkDataAssemblyVisitor
{
public:
  static CallbackDataVisitor* New();
  vtkTypeMacro(CallbackDataVisitor, vtkDataAssemblyVisitor);

  std::function<void(int)> VisitCallback;
  void Visit(int nodeid) override
  {
    if (this->VisitCallback)
    {
      this->VisitCallback(nodeid);
    }
  }

  std::function<bool(int)> GetTraverseSubtreeCallback;
  bool GetTraverseSubtree(int nodeid) override
  {
    if (this->GetTraverseSubtreeCallback)
    {
      return this->GetTraverseSubtreeCallback(nodeid);
    }
    return true;
  }

protected:
  CallbackDataVisitor() = default;
  ~CallbackDataVisitor() override = default;

private:
  CallbackDataVisitor(const CallbackDataVisitor&) = delete;
  void operator=(const CallbackDataVisitor&) = delete;
};
vtkStandardNewMacro(CallbackDataVisitor);

} // end of namespace

class pqDataAssemblyTreeModel::pqInternals
{
  std::unordered_map<int, std::unordered_map<int, QVariant> > Data;

public:
  vtkSmartPointer<vtkDataAssembly> DataAssembly;

  QVariant data(int node, int role) const;
  bool setData(int node, int role, const QVariant& value);
  void clearData() { this->Data.clear(); }
  void clearData(int role)
  {
    auto iter = this->Data.find(role);
    if (iter != this->Data.end())
    {
      iter->second.clear();
    }
  }
  const std::unordered_map<int, QVariant>& data(int role) const { return this->Data.at(role); }

  bool updateParentCheckStates(int node);
};

//-----------------------------------------------------------------------------
QVariant pqDataAssemblyTreeModel::pqInternals::data(int node, int role) const
{
  return this->Data.at(role).at(node);
}

//-----------------------------------------------------------------------------
bool pqDataAssemblyTreeModel::pqInternals::setData(int node, int role, const QVariant& value)
{
  if (!this->DataAssembly || node < 0)
  {
    return false;
  }

  auto& role_map = this->Data[role];
  auto& cur_value = role_map[node];
  if (cur_value == value)
  {
    return false;
  }

  vtkNew<CallbackDataVisitor> vistor;
  vistor->VisitCallback = [&](int id) { role_map[id] = value; };
  this->DataAssembly->Visit(node, vistor);
  return true;
}

//-----------------------------------------------------------------------------
bool pqDataAssemblyTreeModel::pqInternals::updateParentCheckStates(int node)
{
  if (!this->DataAssembly || node <= 0)
  {
    return false;
  }

  auto& role_map = this->Data[Qt::CheckStateRole];
  while ((node = this->DataAssembly->GetParent(node)) != -1)
  {
    auto children = this->DataAssembly->GetChildNodes(node, /*traverse_subtree=*/false);
    int checked_count = 0, unchecked_count = 0, partially_checked_count = 0;
    for (auto& child : children)
    {
      auto iter = role_map.find(child);
      if (iter == role_map.end() || iter->second.value<Qt::CheckState>() == Qt::Unchecked)
      {
        ++unchecked_count;
      }
      else if (iter->second.value<Qt::CheckState>() == Qt::Checked)
      {
        ++checked_count;
      }
      else
      {
        // partially checked
        ++partially_checked_count;
        break;
      }
    }
    QVariant new_state;
    if (partially_checked_count != 0 || (checked_count > 0 && unchecked_count > 0))
    {
      new_state = Qt::PartiallyChecked;
    }
    else if (checked_count == 0)
    {
      new_state = Qt::Unchecked;
    }
    else
    {
      Q_ASSERT(unchecked_count == 0 && partially_checked_count == 0);
      new_state = Qt::Checked;
    }

    if (new_state == role_map[node])
    {
      break;
    }
    role_map[node] = new_state;
  }
  return true;
}

//-----------------------------------------------------------------------------
pqDataAssemblyTreeModel::pqDataAssemblyTreeModel(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqDataAssemblyTreeModel::pqInternals())
  , UserCheckable(false)
{
}

//-----------------------------------------------------------------------------
pqDataAssemblyTreeModel::~pqDataAssemblyTreeModel()
{
}

//-----------------------------------------------------------------------------
void pqDataAssemblyTreeModel::setDataAssembly(vtkDataAssembly* assembly)
{
  if (this->Internals->DataAssembly != assembly)
  {
    this->beginResetModel();
    auto& internals = (*this->Internals);
    internals.clearData();
    internals.DataAssembly = assembly;
    this->endResetModel();
  }
}
//-----------------------------------------------------------------------------
vtkDataAssembly* pqDataAssemblyTreeModel::dataAssembly() const
{
  return this->Internals->DataAssembly;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyTreeModel::setUserCheckable(bool value)
{
  if (this->UserCheckable != value)
  {
    this->beginResetModel();
    this->UserCheckable = value;
    this->endResetModel();
  }
}

//-----------------------------------------------------------------------------
int pqDataAssemblyTreeModel::columnCount(const QModelIndex&) const
{
  return 1;
}

//-----------------------------------------------------------------------------
int pqDataAssemblyTreeModel::rowCount(const QModelIndex& prnt) const
{
  const auto assembly = this->Internals->DataAssembly.GetPointer();
  if (!assembly)
  {
    return 0;
  }

  if (prnt.isValid())
  {
    const auto nodeId = ::getNodeID(prnt);
    return assembly ? assembly->GetNumberOfChildren(nodeId) : 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
QModelIndex pqDataAssemblyTreeModel::index(int row, int column, const QModelIndex& prnt) const
{
  const auto assembly = this->Internals->DataAssembly.GetPointer();
  if (!assembly)
  {
    return QModelIndex();
  }

  if (!prnt.isValid())
  {
    return this->createIndex(row, column, static_cast<quintptr>(0));
  }

  const auto nodeId = ::getNodeID(prnt);
  return assembly
    ? this->createIndex(row, column, static_cast<quintptr>(assembly->GetChild(nodeId, row)))
    : QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex pqDataAssemblyTreeModel::parent(const QModelIndex& indx) const
{
  if (!indx.isValid())
  {
    return QModelIndex();
  }
  const auto nodeId = ::getNodeID(indx);
  if (nodeId <= 0)
  {
    return QModelIndex();
  }

  const auto assembly = this->Internals->DataAssembly.GetPointer();
  const auto pNodeId = assembly->GetParent(nodeId);
  if (pNodeId == 0)
  {
    return this->createIndex(0, 0, static_cast<quintptr>(0));
  }

  const auto ppNodeId = assembly->GetParent(pNodeId);
  assert(ppNodeId != -1);

  return this->createIndex(
    assembly->GetChildIndex(ppNodeId, pNodeId), indx.column(), static_cast<quintptr>(pNodeId));
}

//-----------------------------------------------------------------------------
QVariant pqDataAssemblyTreeModel::data(const QModelIndex& indx, int role) const
{
  auto& internals = (*this->Internals);
  const auto assembly = internals.DataAssembly.GetPointer();
  const auto node = ::getNodeID(indx);
  switch (role)
  {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
      return assembly->GetNodeName(node);
      break;
  }

  try
  {
    return internals.data(node, role);
  }
  catch (const std::out_of_range&)
  {
    if (this->UserCheckable && role == Qt::CheckStateRole)
    {
      return Qt::Unchecked;
    }
  }

  return QVariant();
}
//-----------------------------------------------------------------------------
bool pqDataAssemblyTreeModel::setData(const QModelIndex& indx, const QVariant& value, int role)
{
  auto& internals = (*this->Internals);
  const auto assembly = internals.DataAssembly.GetPointer();
  const auto node = ::getNodeID(indx);
  if (node == -1 || assembly == nullptr)
  {
    return false;
  }

  if (!internals.setData(node, role, value))
  {
    return false;
  }

  this->fireDataChanged(indx, { role });

  // checkstate is the only role where we have to travel put the parent chain
  // and update the checkstate.
  if (role == Qt::CheckStateRole && internals.updateParentCheckStates(node))
  {
    auto prnt = this->parent(indx);
    while (prnt.isValid())
    {
      Q_EMIT this->dataChanged(prnt, prnt, { role });
      prnt = this->parent(prnt);
    }
  }

  Q_EMIT this->modelDataChanged(role);
  return true;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqDataAssemblyTreeModel::flags(const QModelIndex& indx) const
{
  auto f = this->Superclass::flags(indx);
  if (this->userCheckable())
  {
    f |= Qt::ItemIsUserCheckable;
  }
  return f;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyTreeModel::fireDataChanged(const QModelIndex& indx, const QVector<int>& roles)
{
  // fire data modified events.
  Q_EMIT this->dataChanged(indx, indx, roles);

  std::function<void(const QModelIndex&)> fire_data_changed_recursively;
  fire_data_changed_recursively = [&](const QModelIndex& prnt) {
    const int num_rows = this->rowCount(prnt);
    if (num_rows > 0)
    {
      Q_EMIT this->dataChanged(
        this->index(0, indx.column(), prnt), this->index(num_rows - 1, indx.column(), prnt), roles);
      for (int cc = 0; cc < num_rows; ++cc)
      {
        fire_data_changed_recursively(this->index(cc, indx.column(), prnt));
      }
    }
  };
  fire_data_changed_recursively(indx);
}

//-----------------------------------------------------------------------------
void pqDataAssemblyTreeModel::setCheckedNodes(const QStringList& paths)
{
  auto& internals = (*this->Internals);
  if (const auto assembly = internals.DataAssembly.GetPointer())
  {
    std::vector<std::string> path_queries;
    std::transform(paths.begin(), paths.end(), std::back_inserter(path_queries),
      [](const QString& qstring) { return qstring.toLocal8Bit().data(); });

    internals.clearData(Qt::CheckStateRole);
    const QVariant val = Qt::Checked;
    const auto selectedNodes = assembly->SelectNodes(path_queries);
    for (const auto& node : selectedNodes)
    {
      internals.setData(node, Qt::CheckStateRole, val);
    }

    // now, update parent check-states.
    std::set<int> visited_parents;
    for (const auto& node : selectedNodes)
    {
      // update parent node check states
      const auto prnt = assembly->GetParent(node);
      if (visited_parents.find(prnt) == visited_parents.end())
      {
        internals.updateParentCheckStates(node);
        visited_parents.insert(prnt);
      }
    }
  }

  // fire data changed events
  this->fireDataChanged(this->index(0, 0), { Qt::CheckStateRole });
}

//-----------------------------------------------------------------------------
QStringList pqDataAssemblyTreeModel::checkedNodes() const
{
  auto& internals = (*this->Internals);
  const auto assembly = internals.DataAssembly.GetPointer();
  if (!assembly)
  {
    return QStringList();
  }

  const auto node_states = internals.data(Qt::CheckStateRole);
  QStringList paths;

  vtkNew<CallbackDataVisitor> visitor;
  visitor->GetTraverseSubtreeCallback = [&](int id) {
    auto iter = node_states.find(id);
    const Qt::CheckState state =
      iter == node_states.end() ? Qt::Unchecked : iter->second.value<Qt::CheckState>();
    if (state == Qt::Unchecked)
    {
      // this subtree is all unchecked, skip it.
      return false;
    }
    if (state == Qt::Checked)
    {
      // this is a selected node, add it to the paths.
      paths.push_back(QString::fromStdString(assembly->GetNodePath(id)));
      // no need to traverse the substree, however.
      return false;
    }
    else
    {
      // partially checked means, we need to go deeper.
      return true;
    }
  };

  assembly->Visit(visitor, vtkDataAssembly::TraversalOrder::BreadthFirst);
  return paths;
}
