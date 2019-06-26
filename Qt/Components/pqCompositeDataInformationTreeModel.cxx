/*=========================================================================

   Program: ParaView
   Module:  pqCompositeDataInformationTreeModel.cxx

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
#include "pqCompositeDataInformationTreeModel.h"

#include "vtkDataObjectTypes.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVLogger.h"

#include <QList>
#include <QSet>
#include <QStringList>
#include <QtDebug>

#include <cassert>
#include <unordered_map>
#include <vector>

namespace pqCompositeDataInformationTreeModelNS
{

inline bool isroot(const QModelIndex& idx)
{
  return (idx.isValid() && idx.internalId() == 0);
}

class CNode
{
  QString Name;
  unsigned int Index;
  unsigned int LeafIndex;
  int DataType;
  int NumberOfPieces;
  CNode* Parent;
  std::vector<CNode> Children;

  std::pair<Qt::CheckState, bool> CheckState; // bool is true if value was explicitly set,
                                              // false, if value is inherited.
  Qt::CheckState
    ForceSetState; // for non-leaf nodes, since check-state changes based on children state,
                   // we may loose the value that was explicitly
                   // set. This helps us keep track of it. This is
                   // only needed to supported `checkStates()` API.
  std::vector<std::pair<QVariant, bool> >
    CustomColumnState; // bool is true if value was explicitly set,
                       // false, if value is inherited.

  void setChildrenCheckState(
    Qt::CheckState state, bool force, pqCompositeDataInformationTreeModel* dmodel)
  {
    for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter)
    {
      if (force == true || iter->CheckState.second == false)
      {
        iter->CheckState.first = state;
        iter->CheckState.second = false; // flag value as inherited.
        iter->setChildrenCheckState(state, force, dmodel);
      }
    }
    if (this->Children.size() > 0)
    {
      dmodel->dataChanged(
        this->Children.front().createIndex(dmodel), this->Children.back().createIndex(dmodel));
    }
  }

  void updateCheckState(pqCompositeDataInformationTreeModel* dmodel)
  {
    int state = 0;
    for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter)
    {
      switch (iter->CheckState.first)
      {
        case Qt::Unchecked:
          state |= 0x01;
          break;

        case Qt::PartiallyChecked:
          state |= 0x02;
          break;

        case Qt::Checked:
          state |= 0x04;
          break;
      }
    }
    Qt::CheckState target;
    switch (state)
    {
      case 0x01:
      case 0:
        target = Qt::Unchecked;
        break;
      case 0x04:
        target = Qt::Checked;
        break;

      case 0x02:
      default:
        target = Qt::PartiallyChecked;
    }
    if (this->CheckState.first != target)
    {
      this->CheckState.first = target;
      if (this->Parent)
      {
        this->Parent->updateCheckState(dmodel);
      }
      QModelIndex idx = this->createIndex(dmodel);
      dmodel->dataChanged(idx, idx);
    }
  }

public:
  CNode()
    : Index(VTK_UNSIGNED_INT_MAX)
    , LeafIndex(VTK_UNSIGNED_INT_MAX)
    , DataType(0)
    , NumberOfPieces(-1)
    , Parent(nullptr)
    , CheckState(Qt::Unchecked, false)
    , ForceSetState(Qt::Unchecked)
    , CustomColumnState()
  {
  }
  ~CNode() {}

  static CNode& nullNode()
  {
    static CNode NullNode;
    return NullNode;
  }

  bool operator==(const CNode& other) const
  {
    return this->Name == other.Name && this->Index == other.Index &&
      this->DataType == other.DataType && this->NumberOfPieces == other.NumberOfPieces &&
      this->Parent == other.Parent && this->Children == other.Children;
  }
  bool operator!=(const CNode& other) const { return !(*this == other); }

  void reset() { (*this) = CNode::nullNode(); }

  int childrenCount() const
  {
    return this->NumberOfPieces >= 0 ? this->NumberOfPieces
                                     : static_cast<int>(this->Children.size());
  }

  QModelIndex createIndex(const pqCompositeDataInformationTreeModel* dmodel, int col = 0) const
  {
    if (this->Parent)
    {
      return dmodel->createIndex(
        this->Parent->childIndex(*this), col, static_cast<quintptr>(this->flatIndex()));
    }
    else
    {
      return dmodel->createIndex(0, col, static_cast<quintptr>(0));
    }
  }

  inline unsigned int flatIndex() const { return this->Index; }
  inline unsigned int leafIndex() const { return this->LeafIndex; }
  inline const QString& name() const { return this->Name; }
  QString dataTypeAsString() const
  {
    return this->DataType == -1 ? "Unknown"
                                : vtkDataObjectTypes::GetClassNameFromTypeId(this->DataType);
  }

  CNode& child(int idx)
  {
    return idx < static_cast<int>(this->Children.size()) ? this->Children[idx] : CNode::nullNode();
  }
  const CNode& child(int idx) const
  {
    return idx < static_cast<int>(this->Children.size()) ? this->Children[idx] : CNode::nullNode();
  }

  int childIndex(const CNode& achild) const
  {
    for (size_t cc = 0, max = this->Children.size(); cc < max; ++cc)
    {
      if (this->Children[cc] == achild)
      {
        return static_cast<int>(cc);
      }
    }
    return 0;
  }
  const CNode& parent() const { return this->Parent ? *this->Parent : CNode::nullNode(); }

  Qt::CheckState checkState() const { return this->CheckState.first; }

  bool setChecked(bool val, bool force, pqCompositeDataInformationTreeModel* dmodel)
  {
    if ((val == true && this->CheckState.first != Qt::Checked) ||
      (val == false && this->CheckState.first != Qt::Unchecked))
    {
      if (force == true || this->CheckState.second == false)
      {
        this->CheckState.first = val ? Qt::Checked : Qt::Unchecked;
        this->ForceSetState = this->CheckState.first;
        this->CheckState.second = true;
        this->setChildrenCheckState(this->CheckState.first, force, dmodel);
        if (this->Parent)
        {
          this->Parent->updateCheckState(dmodel);
        }

        QModelIndex idx = this->createIndex(dmodel);
        dmodel->dataChanged(idx, idx);
        return true;
      }
    }
    return false;
  }

  void markCheckedStateAsInherited() { this->CheckState.second = false; }

  void checkedNodes(QSet<unsigned int>& set, bool leaves_only) const
  {
    if (this->checkState() == Qt::Unchecked)
    {
      // do nothing.
    }
    else if (this->checkState() == Qt::Checked &&
      (leaves_only == false || this->Children.size() == 0))
    {
      set.insert(this->flatIndex());
    }
    else
    {
      // partially checked or (leaves_only==true and non-leaf node).
      for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter)
      {
        iter->checkedNodes(set, leaves_only);
      }
    }
  }

  void checkStates(QList<QPair<unsigned int, bool> >& states) const
  {
    // add any explicitly toggled nodes to the "states".
    if (this->CheckState.second)
    {
      states.push_back(QPair<unsigned int, bool>(this->flatIndex(), this->ForceSetState));
    }
    for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter)
    {
      iter->checkStates(states);
    }
  }

  const QVariant& customColumnState(int col) const
  {
    assert(col >= 0 && col < static_cast<int>(this->CustomColumnState.size()));
    return this->CustomColumnState[col].first;
  }

  bool isCustomColumnStateInherited(int col) const
  {
    if (col >= 0 && col < static_cast<int>(this->CustomColumnState.size()))
    {
      return (this->CustomColumnState[col].second == false);
    }
    return true;
  }

  // Set custom column state for a specific node. If `value` is invalid, then
  // it's treated as unset (or default). Setting a value will propagate the
  // value to all children who haven't explicitly overridden the value
  // themselves. To force set on all children, pass `force` as true. If value it
  // set to `invalid` it acts are clearing the column state.
  void setCustomColumnState(
    int col, const QVariant& value, bool force, pqCompositeDataInformationTreeModel* dmodel)
  {
    assert(col >= 0 && col < static_cast<int>(this->CustomColumnState.size()));
    std::pair<QVariant, bool>& value_pair = this->CustomColumnState[col];
    if (value_pair.first != value)
    {
      value_pair.first = value;
      QModelIndex idx = this->createIndex(dmodel, col + 1);
      dmodel->dataChanged(idx, idx);
    }

    // flag that this value was explicitly set, unless value is invalid -- which
    // acts as value being cleared.
    value_pair.second = value.isValid() ? true : false;

    // if value is invalid i.e. the value is being cleared then we
    // fetch the value from the parent. This makes sense since in such a case,
    // the value would indeed be inherited from the parent.
    if (!value.isValid() && this->Parent != nullptr)
    {
      value_pair.first = this->Parent->CustomColumnState[col].first;
    }

    // now, propagate over all children and pass this value.
    for (auto citer = this->Children.begin(); citer != this->Children.end(); ++citer)
    {
      CNode& child = (*citer);
      if (force == true || child.CustomColumnState[col].second == false)
      {
        child.setCustomColumnState(col, value_pair.first, force, dmodel);
        child.CustomColumnState[col].second = false; // since the value is inherited.
      }
    }
  }

  void customColumnStates(int col, QList<QPair<unsigned int, QVariant> >& values) const
  {
    assert(col >= 0 && col < static_cast<int>(this->CustomColumnState.size()));
    const std::pair<QVariant, bool>& value_pair = this->CustomColumnState[col];

    // add explicitly set values.
    if (value_pair.first.isValid() && value_pair.second)
    {
      values.push_back(QPair<unsigned int, QVariant>(this->flatIndex(), value_pair.first));
    }

    // iterate over children to do the same.
    for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter)
    {
      iter->customColumnStates(col, values);
    }
  }

  bool build(vtkPVDataInformation* info, bool expand_multi_piece, unsigned int& index,
    unsigned int& leaf_index, int custom_column_count,
    std::unordered_map<unsigned int, CNode*>& lookupMap)
  {
    this->reset();
    this->Index = index++;
    lookupMap[this->Index] = this;
    if (info == nullptr || info->GetCompositeDataClassName() == 0)
    {
      this->Name = info != nullptr ? info->GetPrettyDataTypeString() : "(empty)";
      this->DataType = info != nullptr ? info->GetDataSetType() : -1;
      this->CustomColumnState.resize(custom_column_count);
      this->LeafIndex = leaf_index++;
      return false;
    }

    this->Name = info->GetPrettyDataTypeString();
    this->DataType = info->GetCompositeDataSetType();
    this->CustomColumnState.resize(custom_column_count);

    vtkPVCompositeDataInformation* cinfo = info->GetCompositeDataInformation();

    bool is_amr = (this->DataType == VTK_HIERARCHICAL_DATA_SET ||
      this->DataType == VTK_HIERARCHICAL_BOX_DATA_SET || this->DataType == VTK_UNIFORM_GRID_AMR ||
      this->DataType == VTK_NON_OVERLAPPING_AMR || this->DataType == VTK_OVERLAPPING_AMR);
    bool is_multipiece = cinfo->GetDataIsMultiPiece() != 0;
    if (!is_multipiece || expand_multi_piece)
    {
      this->Children.resize(cinfo->GetNumberOfChildren());
      for (unsigned int cc = 0, max = cinfo->GetNumberOfChildren(); cc < max; ++cc)
      {
        CNode& childNode = this->Children[cc];
        childNode.build(cinfo->GetDataInformation(cc), expand_multi_piece, index, leaf_index,
          custom_column_count, lookupMap);
        // note:  build() will reset childNode, so don't set any ivars before calling it.
        childNode.Parent = this;
        // if Name for block was provided, use that instead of the data type.
        const char* name = cinfo->GetName(cc);
        if (name && name[0])
        {
          childNode.Name = name;
        }
        else if (is_multipiece)
        {
          childNode.Name = QString("Dataset %1").arg(cc);
        }
        else if (is_amr)
        {
          childNode.Name = QString("Level %1").arg(cc);
        }
      }
    }
    else if (is_multipiece)
    {
      index += cinfo->GetNumberOfChildren();
      this->LeafIndex = leaf_index;
      // move leaf_index forward by however many pieces this multipiece dataset
      // has.
      leaf_index += cinfo->GetNumberOfChildren();
    }
    return true;
  }
};
}

using namespace pqCompositeDataInformationTreeModelNS;

class pqCompositeDataInformationTreeModel::pqInternals
{
public:
  pqInternals() {}
  ~pqInternals() {}

  static CNode& nullNode() { return CNode::nullNode(); }

  CNode& find(const QModelIndex& idx)
  {
    if (!idx.isValid())
    {
      return nullNode();
    }

    unsigned int findex = static_cast<unsigned int>(idx.internalId());
    return this->find(findex);
  }

  inline CNode& find(unsigned int findex)
  {
    auto iter = this->CNodeMap.find(findex);
    return (iter != this->CNodeMap.end()) ? (*iter->second) : nullNode();
  }

  /**
   * Builds the data-structure using vtkPVDataInformation (may be null).
   * @returns true if the info refers to a composite dataset otherwise returns
   * false.
   */
  bool build(vtkPVDataInformation* info, bool expand_multi_piece)
  {
    unsigned int index = 0;
    unsigned int leaf_index = 0;

    this->CNodeMap.clear();
    bool retVal = this->Root.build(
      info, expand_multi_piece, index, leaf_index, this->CustomColumns.size(), this->CNodeMap);

    return retVal;
  }

  CNode& rootNode() { return this->Root; }

  void clearCheckState(pqCompositeDataInformationTreeModel* dmodel)
  {
    this->Root.setChecked(dmodel->defaultCheckState(), true, dmodel);
    this->Root
      .markCheckedStateAsInherited(); // this avoid us interpreting the value as explicitly set.
  }

  int addColumn(const QString& propertyName)
  {
    this->CustomColumns.push_back(propertyName);
    return this->CustomColumns.size() - 1;
  }
  const QStringList& customColumns() const { return this->CustomColumns; }
  void clearColumns() { this->CustomColumns.clear(); }
  int customColumnIndex(const QString& pname) const { return this->CustomColumns.indexOf(pname); }
private:
  CNode Root;
  QStringList CustomColumns;
  std::unordered_map<unsigned int, CNode*> CNodeMap;
};

//-----------------------------------------------------------------------------
pqCompositeDataInformationTreeModel::pqCompositeDataInformationTreeModel(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqCompositeDataInformationTreeModel::pqInternals())
  , UserCheckable(false)
  , OnlyLeavesAreUserCheckable(false)
  , ExpandMultiPiece(false)
  , Exclusivity(false)
  , DefaultCheckState(false)
{
}

//-----------------------------------------------------------------------------
pqCompositeDataInformationTreeModel::~pqCompositeDataInformationTreeModel()
{
}

//-----------------------------------------------------------------------------
int pqCompositeDataInformationTreeModel::columnCount(const QModelIndex&) const
{
  pqInternals& internals = (*this->Internals);
  return 1 + internals.customColumns().size();
}

//-----------------------------------------------------------------------------
int pqCompositeDataInformationTreeModel::rowCount(const QModelIndex& parentIdx) const
{
  if (!parentIdx.isValid())
  {
    // return number of top-level items.
    return 1;
  }
  pqInternals& internals = (*this->Internals);
  const CNode& node = internals.find(parentIdx);
  assert(node.childrenCount() >= 0);
  return node.childrenCount();
}

//-----------------------------------------------------------------------------
QModelIndex pqCompositeDataInformationTreeModel::index(
  int row, int column, const QModelIndex& parentIdx) const
{
  if (!parentIdx.isValid() && row == 0)
  {
    return this->createIndex(0, column, static_cast<quintptr>(0));
  }

  if (row < 0 || column < 0 || column >= this->columnCount())
  {
    return QModelIndex();
  }

  pqInternals& internals = (*this->Internals);
  const CNode& node = internals.find(parentIdx);
  const CNode& child = node.child(row);
  return this->createIndex(row, column, static_cast<quintptr>(child.flatIndex()));
}

//-----------------------------------------------------------------------------
QModelIndex pqCompositeDataInformationTreeModel::parent(const QModelIndex& idx) const
{
  if (!idx.isValid() || isroot(idx))
  {
    return QModelIndex();
  }

  pqInternals& internals = (*this->Internals);
  const CNode& node = internals.find(idx);
  const CNode& parentNode = node.parent();
  if (parentNode == CNode::nullNode())
  {
    return QModelIndex();
  }

  if (parentNode == internals.rootNode())
  {
    return this->createIndex(0, 0, static_cast<quintptr>(0));
  }

  const CNode& parentsParentNode = parentNode.parent();
  return this->createIndex(
    parentsParentNode.childIndex(parentNode), 0, static_cast<quintptr>(parentNode.flatIndex()));
}

//-----------------------------------------------------------------------------
QVariant pqCompositeDataInformationTreeModel::data(const QModelIndex& idx, int role) const
{
  if (!idx.isValid())
  {
    return QVariant();
  }

  // short-circuit roles we don't care about.
  switch (role)
  {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case Qt::CheckStateRole:
    case ValueInheritedRole:
    case LeafIndexRole:
      break;

    case CompositeIndexRole:
      return this->compositeIndex(idx);

    default:
      return QVariant();
  }

  pqInternals& internals = (*this->Internals);
  const CNode& node = internals.find(idx);
  if (node == CNode::nullNode())
  {
    return QVariant();
  }

  int col = idx.column();
  if (col == 0)
  {
    switch (role)
    {
      case Qt::DisplayRole:
        return node.name();

      case Qt::ToolTipRole:
        return QString("<b>Name</b>: %1<br/><b>Type</b>: %2")
          .arg(node.name())
          .arg(node.dataTypeAsString());

      case Qt::CheckStateRole:
        return this->UserCheckable ? QVariant(node.checkState()) : QVariant();

      case LeafIndexRole:
        return node.leafIndex() != VTK_UNSIGNED_INT_MAX ? QVariant(node.leafIndex()) : QVariant();
    }
  }
  else
  {
    col--; // since 0 is non-custom column.
    switch (role)
    {
      case Qt::DisplayRole:
        return node.customColumnState(col);

      case ValueInheritedRole:
        return node.isCustomColumnStateInherited(col);

      case LeafIndexRole:
        return node.leafIndex() != VTK_UNSIGNED_INT_MAX ? QVariant(node.leafIndex()) : QVariant();
    }
  }
  return QVariant();
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqCompositeDataInformationTreeModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags pflags = this->Superclass::flags(idx);
  if (this->UserCheckable && idx.column() == 0)
  {
    if (this->OnlyLeavesAreUserCheckable == false || !this->hasChildren(idx))
    {
      pflags |= Qt::ItemIsUserCheckable | Qt::ItemIsTristate;
    }
  }

  // can't use Qt::ItemIsAutoTristate till we drop support for Qt 4 :(.
  return pflags;
}

//-----------------------------------------------------------------------------
bool pqCompositeDataInformationTreeModel::setData(
  const QModelIndex& idx, const QVariant& value, int role)
{
  if (!idx.isValid())
  {
    return false;
  }

  if ((idx.column() == 0 && role != Qt::CheckStateRole) ||
    (idx.column() > 0 && role != Qt::DisplayRole))
  {
    return false;
  }

  pqInternals& internals = (*this->Internals);
  CNode& node = internals.find(idx);
  if (node == CNode::nullNode())
  {
    return false;
  }

  if (idx.column() == 0 && role == Qt::CheckStateRole)
  {
    Qt::CheckState checkState = value.value<Qt::CheckState>();
    if (checkState == Qt::Checked && this->exclusivity())
    {
      internals.clearCheckState(this);
    }
    node.setChecked(checkState == Qt::Checked, true, this);
    return true;
  }
  else if (idx.column() > 0 && role == Qt::DisplayRole)
  {
    int col = idx.column() - 1;
    node.setCustomColumnState(col, value, /*force=*/false, this);
  }
  return false;
}

//-----------------------------------------------------------------------------
QVariant pqCompositeDataInformationTreeModel::headerData(
  int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && section == 0 && role == Qt::DisplayRole)
  {
    return this->HeaderLabel;
  }
  return this->Superclass::headerData(section, orientation, role);
}

//-----------------------------------------------------------------------------
bool pqCompositeDataInformationTreeModel::setHeaderData(
  int section, Qt::Orientation orientation, const QVariant& value, int role)
{
  if (orientation == Qt::Horizontal && section == 0 && role == Qt::DisplayRole)
  {
    this->HeaderLabel = value.toString();
    emit this->headerDataChanged(orientation, section, section);
    return true;
  }
  return this->Superclass::setHeaderData(section, orientation, value, role);
}

//-----------------------------------------------------------------------------
bool pqCompositeDataInformationTreeModel::reset(vtkPVDataInformation* info)
{
  vtkVLogScopeFunction(PARAVIEW_LOG_APPLICATION_VERBOSITY());

  pqInternals& internals = (*this->Internals);

  this->beginResetModel();
  bool retVal = internals.build(info, this->ExpandMultiPiece);
  internals.clearCheckState(this);
  this->endResetModel();
  return retVal;
}

//-----------------------------------------------------------------------------
void pqCompositeDataInformationTreeModel::setChecked(const QList<unsigned int>& indices)
{
  pqInternals& internals = (*this->Internals);
  internals.clearCheckState(this);

  foreach (unsigned int findex, indices)
  {
    CNode& node = internals.find(findex);
    node.setChecked(true, true, this);
  }
}

//-----------------------------------------------------------------------------
QList<unsigned int> pqCompositeDataInformationTreeModel::checkedNodes() const
{
  QSet<unsigned int> indices;
  pqInternals& internals = (*this->Internals);
  internals.rootNode().checkedNodes(indices, false);
  return indices.toList();
}

//-----------------------------------------------------------------------------
QList<unsigned int> pqCompositeDataInformationTreeModel::checkedLeaves() const
{
  QSet<unsigned int> indices;
  pqInternals& internals = (*this->Internals);
  internals.rootNode().checkedNodes(indices, true);
  return indices.toList();
}

//-----------------------------------------------------------------------------
QList<QPair<unsigned int, bool> > pqCompositeDataInformationTreeModel::checkStates() const
{
  QList<QPair<unsigned int, bool> > states;
  pqInternals& internals = (*this->Internals);
  internals.rootNode().checkStates(states);
  return states;
}

//-----------------------------------------------------------------------------
void pqCompositeDataInformationTreeModel::setCheckStates(
  const QList<QPair<unsigned int, bool> >& states)
{
  pqInternals& internals = (*this->Internals);
  internals.clearCheckState(this);

  for (auto iter = states.begin(); iter != states.end(); ++iter)
  {
    CNode& node = internals.find(iter->first);
    if (node != CNode::nullNode())
    {
      node.setChecked(iter->second, /*force=*/false, this);
    }
  }
}

//-----------------------------------------------------------------------------
QList<unsigned int> pqCompositeDataInformationTreeModel::checkedLevels() const
{
  QList<unsigned int> indices;
  pqInternals& internals = (*this->Internals);
  CNode& root = internals.rootNode();
  for (int cc = 0; cc < root.childrenCount(); cc++)
  {
    if (root.child(cc).checkState() == Qt::Checked)
    {
      indices.push_back(static_cast<unsigned int>(cc));
    }
  }
  return indices;
}

//-----------------------------------------------------------------------------
void pqCompositeDataInformationTreeModel::setCheckedLevels(const QList<unsigned int>& indices)
{
  pqInternals& internals = (*this->Internals);
  internals.clearCheckState(this);

  CNode& root = internals.rootNode();
  unsigned int childrenCount = static_cast<unsigned int>(root.childrenCount());
  foreach (unsigned int idx, indices)
  {
    if (idx < childrenCount)
    {
      root.child(idx).setChecked(true, true, this);
    }
  }
}

//-----------------------------------------------------------------------------
QList<QPair<unsigned int, unsigned int> >
pqCompositeDataInformationTreeModel::checkedLevelDatasets() const
{
  QList<QPair<unsigned int, unsigned int> > indices;
  pqInternals& internals = (*this->Internals);
  CNode& root = internals.rootNode();
  for (int level = 0; level < root.childrenCount(); ++level)
  {
    CNode& levelNode = root.child(level);
    for (int ds = 0; ds < levelNode.childrenCount(); ++ds)
    {
      if (levelNode.child(ds).checkState() == Qt::Checked)
      {
        indices.push_back(QPair<unsigned int, unsigned int>(level, ds));
      }
    }
  }
  return indices;
}

//-----------------------------------------------------------------------------
void pqCompositeDataInformationTreeModel::setCheckedLevelDatasets(
  const QList<QPair<unsigned int, unsigned int> >& indices)
{
  pqInternals& internals = (*this->Internals);
  internals.clearCheckState(this);

  CNode& root = internals.rootNode();
  unsigned int numLevels = static_cast<unsigned int>(root.childrenCount());

  typedef QPair<unsigned int, unsigned int> IdxPair;
  foreach (const IdxPair& idx, indices)
  {
    if (idx.first < numLevels)
    {
      CNode& level = root.child(idx.first);
      if (idx.second < static_cast<unsigned int>(level.childrenCount()))
      {
        level.child(idx.second).setChecked(true, true, this);
      }
    }
  }
}

//-----------------------------------------------------------------------------
unsigned int pqCompositeDataInformationTreeModel::compositeIndex(const QModelIndex& idx) const
{
  if (idx.isValid() && idx.model() == this)
  {
    return static_cast<unsigned int>(idx.internalId());
  }
  return 0;
}

//-----------------------------------------------------------------------------
QModelIndex pqCompositeDataInformationTreeModel::find(unsigned int idx) const
{
  pqInternals& internals = (*this->Internals);
  CNode& node = internals.find(idx);

  if (node != CNode::nullNode())
  {
    return node.createIndex(this);
  }
  return QModelIndex();
}

//-----------------------------------------------------------------------------
const QModelIndex pqCompositeDataInformationTreeModel::rootIndex() const
{
  return this->createIndex(0, 0, static_cast<quintptr>(0));
}

//-----------------------------------------------------------------------------
int pqCompositeDataInformationTreeModel::addColumn(const QString& propertyName)
{
  pqInternals& internals = (*this->Internals);
  // since 1st column on the model is non-custom, we offset by 1.
  return internals.addColumn(propertyName) + 1;
}

//-----------------------------------------------------------------------------
int pqCompositeDataInformationTreeModel::columnIndex(const QString& propertyName)
{
  pqInternals& internals = (*this->Internals);
  int idx = internals.customColumnIndex(propertyName);
  return idx >= 0 ? (idx + 1) : -1;
}

//-----------------------------------------------------------------------------
void pqCompositeDataInformationTreeModel::clearColumns()
{
  pqInternals& internals = (*this->Internals);
  internals.clearColumns();
}

//-----------------------------------------------------------------------------
void pqCompositeDataInformationTreeModel::setColumnStates(
  const QString& pname, const QList<QPair<unsigned int, QVariant> >& values)
{
  pqInternals& internals = (*this->Internals);
  int col = internals.customColumnIndex(pname);
  if (col < 0)
  {
    qCritical() << "Unknown property: " << pname;
    return;
  }

  typedef QPair<unsigned int, QVariant> PairT;

  CNode& root = internals.rootNode();

  // clear all values.
  root.setCustomColumnState(col, QVariant(), /*force=*/true, this);
  foreach (const PairT& pair, values)
  {
    CNode& node = internals.find(pair.first);
    if (node != CNode::nullNode())
    {
      if (pair.second.isValid()) // invalid value is treated as cleared.
      {
        node.setCustomColumnState(col, pair.second, /*force=*/false, this);
      }
    }
  }
}

//-----------------------------------------------------------------------------
QList<QPair<unsigned int, QVariant> > pqCompositeDataInformationTreeModel::columnStates(
  const QString& pname) const
{
  QList<QPair<unsigned int, QVariant> > value;

  pqInternals& internals = (*this->Internals);
  int col = internals.customColumnIndex(pname);
  if (col < 0)
  {
    qCritical() << "Unknown property: " << pname;
    return value;
  }
  internals.rootNode().customColumnStates(col, value);
  return value;
}
