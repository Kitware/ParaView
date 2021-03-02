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
public:
  class DataT
  {
    QVariant Value;
    bool ValueDerived = true;

  public:
    const QVariant& value() const { return this->Value; };

    template <typename T>
    T value() const
    {
      return this->Value.value<T>();
    }
    void setValue(const QVariant& var, bool is_derived)
    {
      this->Value = var;
      this->ValueDerived = is_derived || !var.isValid();
    }

    bool isDerived() const { return this->ValueDerived; }
  };

  vtkMTimeType DataAssemblyTimeStamp = 0;
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

  const std::unordered_map<int, DataT>& data(int role) const { return this->Data.at(role); }

  bool updateParentCheckStates(int node);

  void setRoleProperty(int role, pqDataAssemblyTreeModel::RoleProperties property)
  {
    this->RoleProperties[role] = property;
  }

  pqDataAssemblyTreeModel::RoleProperties roleProperty(int role) const
  {
    try
    {
      return this->RoleProperties.at(role);
    }
    catch (std::out_of_range&)
    {
      return pqDataAssemblyTreeModel::Standard;
    }
  }

  QVariant dataFromParent(int node, int role) const
  {
    if (!this->DataAssembly)
    {
      return QVariant();
    }
    int parent = this->DataAssembly->GetParent(node);
    return parent != -1 ? this->data(parent, role) : QVariant();
  }

private:
  std::unordered_map<int, std::unordered_map<int, DataT> > Data;
  std::map<int, pqDataAssemblyTreeModel::RoleProperties> RoleProperties;
};

//-----------------------------------------------------------------------------
QVariant pqDataAssemblyTreeModel::pqInternals::data(int node, int role) const
{
  try
  {
    return role < 0 ? QVariant(this->Data.at(-role).at(node).isDerived())
                    : this->Data.at(role).at(node).value();
  }
  catch (std::out_of_range&)
  {
    return QVariant();
  }
}

//-----------------------------------------------------------------------------
bool pqDataAssemblyTreeModel::pqInternals::setData(int node, int role, const QVariant& value)
{
  if (!this->DataAssembly || node < 0)
  {
    return false;
  }

  auto& role_map = this->Data[role];
  auto& item = role_map[node];
  if (item.value() == value && !item.isDerived())
  {
    return false;
  }

  // is this a request to clear the value, instead of setting it?
  const bool clearValue = (value.isValid() == false);

  if (item.isDerived() && clearValue)
  {
    // we got a request to clear state, but the value is already not explicitly
    // specified, so nothing to do.
    return false;
  }

  auto rproperty = this->roleProperty(role);
  if (clearValue && rproperty == RoleProperties::Inherited)
  {
    // we clear values until overridden
    rproperty = RoleProperties::InheritedUntilOverridden;
  }

  // get a value from parent when clearing a role that's inheritable.
  const QVariant actualValue =
    (clearValue && rproperty != RoleProperties::Standard) ? dataFromParent(node, role) : value;

  if (rproperty == RoleProperties::Inherited)
  {
    // value is inherited down the whole tree.
    vtkNew<CallbackDataVisitor> visitor;
    visitor->VisitCallback = [&](
      int id) { role_map[id].setValue(actualValue, /*isInherited*/ clearValue || id != node); };
    this->DataAssembly->Visit(node, visitor);
  }
  else if (rproperty == RoleProperties::InheritedUntilOverridden)
  {
    // value is inherited till explicitly overridden.
    vtkNew<CallbackDataVisitor> visitor;
    visitor->GetTraverseSubtreeCallback = [&](int id) {
      auto& curItem = role_map[id];
      if (id == node || curItem.isDerived())
      {
        role_map[id].setValue(actualValue, /*isInherited*/ clearValue || id != node);
        return true; // traverse subtree
      }
      return false; // skip subtree.
    };
    this->DataAssembly->Visit(node, visitor);
  }
  else
  {
    item.setValue(actualValue, /*isInherited*/ clearValue);
  }
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

    if (new_state == role_map[node].value())
    {
      break;
    }
    role_map[node].setValue(new_state, true);
  }
  return true;
}

//-----------------------------------------------------------------------------
pqDataAssemblyTreeModel::pqDataAssemblyTreeModel(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqDataAssemblyTreeModel::pqInternals())
  , UserCheckable(false)
{
  // set default to propagage checkstate change to the entire subtree.
  this->setRoleProperty(Qt::CheckStateRole, pqDataAssemblyTreeModel::Inherited);
}

//-----------------------------------------------------------------------------
pqDataAssemblyTreeModel::~pqDataAssemblyTreeModel() = default;

//-----------------------------------------------------------------------------
void pqDataAssemblyTreeModel::setDataAssembly(vtkDataAssembly* assembly)
{
  auto& internals = (*this->Internals);
  const auto stamp = assembly ? assembly->GetMTime() : 0;
  if (internals.DataAssemblyTimeStamp != stamp)
  {
    this->beginResetModel();
    internals.clearData();
    if (assembly)
    {
      // we keep a copy here since vtkDataAssembly pass in change outside the
      // model's scope which can cause odd issues with the view. It's better to
      // keep a deep-copy instead.
      internals.DataAssembly.TakeReference(vtkDataAssembly::New());
      internals.DataAssembly->DeepCopy(assembly);
    }
    else
    {
      internals.DataAssembly = nullptr;
    }
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
void pqDataAssemblyTreeModel::setRoleProperty(
  int role, pqDataAssemblyTreeModel::RoleProperties property)
{
  auto& internals = (*this->Internals);
  internals.setRoleProperty(role, property);
}

//-----------------------------------------------------------------------------
pqDataAssemblyTreeModel::RoleProperties pqDataAssemblyTreeModel::roleProperty(int role) const
{
  auto& internals = (*this->Internals);
  return internals.roleProperty(role);
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
  if (!assembly)
  {
    return QVariant();
  }

  const auto node = ::getNodeID(indx);
  switch (role)
  {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
      return assembly->HasAttribute(node, "label")
        ? assembly->GetAttributeOrDefault(node, "label", "")
        : assembly->GetNodeName(node);
  }

  auto value = internals.data(node, role);
  if (value.isValid() == false && role == Qt::CheckStateRole && this->userCheckable())
  {
    return Qt::Unchecked;
  }
  return value;
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

  // checkstate is the only role where we have to travel up the parent chain
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
  QList<QPair<QString, QVariant> > pairs;
  for (const auto& path : paths)
  {
    pairs.push_back(qMakePair(path, QVariant(Qt::Checked)));
  }
  this->setData(pairs, Qt::CheckStateRole);
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

  /*
   * We don't simply call `data(Qt::CheckStateRole)` since want to create a more compact
   * checked nodes list than tracking explicit on/off states.
   */

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

//-----------------------------------------------------------------------------
QList<QPair<QString, QVariant> > pqDataAssemblyTreeModel::data(int role) const
{
  auto& internals = (*this->Internals);
  const auto assembly = internals.DataAssembly.GetPointer();
  if (!assembly)
  {
    return {};
  }

  QList<QPair<QString, QVariant> > values;
  for (const auto& pair : internals.data(role))
  {
    if (!pair.second.isDerived())
    {
      values.push_back(
        qMakePair(QString::fromStdString(assembly->GetNodePath(pair.first)), pair.second.value()));
    }
  }
  return values;
}

//-----------------------------------------------------------------------------
bool pqDataAssemblyTreeModel::setData(const QList<QPair<QString, QVariant> >& values, int role)
{
  auto& internals = (*this->Internals);
  const auto assembly = internals.DataAssembly.GetPointer();
  if (!assembly)
  {
    return false;
  }

  internals.clearData(role);

  std::vector<int> allSelectedNodes;
  for (const auto& pair : values)
  {
    const auto& selector = pair.first;
    const auto& value = pair.second;
    const auto selectedNodes = assembly->SelectNodes({ selector.toStdString() });
    for (const auto& node : selectedNodes)
    {
      internals.setData(node, role, value);
    }

    if (role == Qt::CheckStateRole)
    {
      allSelectedNodes.insert(allSelectedNodes.end(), selectedNodes.begin(), selectedNodes.end());
    }
  }

  // now, update parent check-states if role is Qt::CheckStateRole.
  std::set<int> visited_parents;
  for (const auto& node : allSelectedNodes)
  {
    Q_ASSERT(role == Qt::CheckStateRole);

    // update parent node check states
    const auto prnt = assembly->GetParent(node);
    if (visited_parents.find(prnt) == visited_parents.end())
    {
      internals.updateParentCheckStates(node);
      visited_parents.insert(prnt);
    }
  }

  // fire data changed events
  this->fireDataChanged(this->index(0, 0), { role });
  return true;
}

//-----------------------------------------------------------------------------
int pqDataAssemblyTreeModel::nodeId(const QModelIndex& idx) const
{
  return ::getNodeID(idx);
}

//-----------------------------------------------------------------------------
QList<int> pqDataAssemblyTreeModel::nodeId(const QModelIndexList& idxes) const
{
  QList<int> result;
  for (auto& idx : idxes)
  {
    result.push_back(::getNodeID(idx));
  }
  return result;
}
