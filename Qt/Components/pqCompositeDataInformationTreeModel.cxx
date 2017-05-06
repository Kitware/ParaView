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

#include <QList>
#include <QSet>

#include <cassert>
#include <vector>

namespace
{
inline bool isroot(const QModelIndex& idx)
{
  return (idx.isValid() && idx.internalId() == 0);
}
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
typedef quint32 qindexdatatype;
#else
typedef quintptr qindexdatatype;
#endif

class pqCompositeDataInformationTreeModel::pqInternals
{
public:
  class CNode
  {
    typedef pqCompositeDataInformationTreeModel::pqInternals PType;
    friend PType;

    QString Name;
    unsigned int Index;
    int DataType;
    int NumberOfPieces;
    CNode* Parent;
    std::vector<CNode> Children;
    Qt::CheckState CheckState;

    QModelIndex createIndex(pqCompositeDataInformationTreeModel* dmodel)
    {
      if (this->Parent)
      {
        return dmodel->createIndex(
          this->Parent->childIndex(*this), 0, static_cast<qindexdatatype>(this->flatIndex()));
      }
      else
      {
        return dmodel->createIndex(0, 0, static_cast<qindexdatatype>(0));
      }
    }

    void setChildrenCheckState(Qt::CheckState state, pqCompositeDataInformationTreeModel* dmodel)
    {
      for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter)
      {
        iter->CheckState = state;
        iter->setChildrenCheckState(state, dmodel);
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
        switch (iter->CheckState)
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
      if (this->CheckState != target)
      {
        this->CheckState = target;
        if (this->Parent)
        {
          this->Parent->updateCheckState(dmodel);
        }
        QModelIndex idx = this->createIndex(dmodel);
        dmodel->dataChanged(idx, idx);
      }
    }

    CNode& find(unsigned int findex)
    {
      if (this->Index == findex)
      {
        return (*this);
      }
      for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter)
      {
        CNode& found = iter->find(findex);
        if (found != PType::nullNode())
        {
          return found;
        }
      }
      return PType::nullNode();
    }

  public:
    CNode()
      : Index(VTK_UNSIGNED_INT_MAX)
      , DataType(0)
      , NumberOfPieces(-1)
      , Parent(nullptr)
      , CheckState(Qt::Unchecked)
    {
    }
    ~CNode() {}

    bool operator==(const CNode& other) const
    {
      return this->Name == other.Name && this->Index == other.Index &&
        this->DataType == other.DataType && this->NumberOfPieces == other.NumberOfPieces &&
        this->Parent == other.Parent && this->Children == other.Children;
    }
    bool operator!=(const CNode& other) const { return !(*this == other); }

    void reset() { (*this) = PType::nullNode(); }

    int childrenCount() const
    {
      return this->NumberOfPieces >= 0 ? this->NumberOfPieces
                                       : static_cast<int>(this->Children.size());
    }

    unsigned int flatIndex() const { return this->Index; }
    const QString& name() const { return this->Name; }
    QString dataTypeAsString() const
    {
      return this->DataType == -1 ? "Unknown"
                                  : vtkDataObjectTypes::GetClassNameFromTypeId(this->DataType);
    }

    CNode& child(int idx)
    {
      return idx < static_cast<int>(this->Children.size()) ? this->Children[idx]
                                                           : PType::nullNode();
    }
    const CNode& child(int idx) const
    {
      return idx < static_cast<int>(this->Children.size()) ? this->Children[idx]
                                                           : PType::nullNode();
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
    const CNode& parent() const { return this->Parent ? *this->Parent : PType::nullNode(); }

    Qt::CheckState checkState() const { return this->CheckState; }

    bool setChecked(bool val, pqCompositeDataInformationTreeModel* dmodel)
    {
      if ((val == true && this->CheckState != Qt::Checked) ||
        (val == false && this->CheckState != Qt::Unchecked))
      {
        this->CheckState = val ? Qt::Checked : Qt::Unchecked;
        this->setChildrenCheckState(this->CheckState, dmodel);
        if (this->Parent)
        {
          this->Parent->updateCheckState(dmodel);
        }

        QModelIndex idx = this->createIndex(dmodel);
        dmodel->dataChanged(idx, idx);
        return true;
      }
      return false;
    }

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
  };

  pqInternals() {}

  ~pqInternals() {}

  static CNode& nullNode() { return NullNode; }

  CNode& find(const QModelIndex& idx)
  {
    if (!idx.isValid())
    {
      return nullNode();
    }

    vtkIdType findex = static_cast<unsigned int>(idx.internalId());
    return this->find(findex);
  }

  CNode& find(unsigned int findex)
  {
    if (findex == VTK_UNSIGNED_INT_MAX)
    {
      return nullNode();
    }

    if (findex == 0)
    {
      return this->Root;
    }

    return this->Root.find(findex);
  }

  /**
   * Builds the data-structure using vtkPVDataInformation (may be null).
   * @returns true if the info refers to a composite dataset otherwise returns
   * false.
   */
  bool build(vtkPVDataInformation* info, bool expand_multi_piece = false)
  {
    unsigned int index = 0;
    return this->build(info, expand_multi_piece, this->Root, index);
  }

  CNode& rootNode() { return this->Root; }

private:
  bool build(vtkPVDataInformation* info, bool expand_multi_piece, CNode& node, unsigned int& index)
  {
    node.reset();
    node.Index = index++;
    if (info == nullptr || info->GetCompositeDataClassName() == 0)
    {
      node.Name = info != nullptr ? info->GetPrettyDataTypeString() : "";
      node.DataType = info != nullptr ? info->GetDataSetType() : -1;
      return false;
    }

    node.Name = info->GetPrettyDataTypeString();
    node.DataType = info->GetCompositeDataSetType();

    vtkPVCompositeDataInformation* cinfo = info->GetCompositeDataInformation();

    bool is_amr = (node.DataType == VTK_HIERARCHICAL_DATA_SET ||
      node.DataType == VTK_HIERARCHICAL_BOX_DATA_SET || node.DataType == VTK_UNIFORM_GRID_AMR ||
      node.DataType == VTK_NON_OVERLAPPING_AMR || node.DataType == VTK_OVERLAPPING_AMR);
    bool is_multipiece = cinfo->GetDataIsMultiPiece() != 0;
    if (!is_multipiece || expand_multi_piece)
    {
      node.Children.resize(cinfo->GetNumberOfChildren());
      for (unsigned int cc = 0, max = cinfo->GetNumberOfChildren(); cc < max; ++cc)
      {
        CNode& childNode = node.Children[cc];
        this->build(cinfo->GetDataInformation(cc), expand_multi_piece, childNode, index);
        // note:  build() will reset childNode, so don't set any ivars before calling it.
        childNode.Parent = &node;
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
    }
    return true;
  }

private:
  CNode Root;
  static CNode NullNode;
};

pqCompositeDataInformationTreeModel::pqInternals::CNode
  pqCompositeDataInformationTreeModel::pqInternals::NullNode;

//-----------------------------------------------------------------------------
pqCompositeDataInformationTreeModel::pqCompositeDataInformationTreeModel(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqCompositeDataInformationTreeModel::pqInternals())
  , UserCheckable(false)
  , ExpandMultiPiece(false)
  , Exclusivity(false)
{
}

//-----------------------------------------------------------------------------
pqCompositeDataInformationTreeModel::~pqCompositeDataInformationTreeModel()
{
}

//-----------------------------------------------------------------------------
int pqCompositeDataInformationTreeModel::columnCount(const QModelIndex&) const
{
  return 1;
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
  const pqInternals::CNode& node = internals.find(parentIdx);
  assert(node.childrenCount() >= 0);
  return node.childrenCount();
}

//-----------------------------------------------------------------------------
QModelIndex pqCompositeDataInformationTreeModel::index(
  int row, int column, const QModelIndex& parentIdx) const
{
  if (!parentIdx.isValid() && row == 0 && column == 0)
  {
    return this->createIndex(0, 0, static_cast<qindexdatatype>(0));
  }

  if (row < 0 || column < 0 || column >= this->columnCount())
  {
    return QModelIndex();
  }

  pqInternals& internals = (*this->Internals);
  const pqInternals::CNode& node = internals.find(parentIdx);
  const pqInternals::CNode& child = node.child(row);
  return this->createIndex(row, column, static_cast<qindexdatatype>(child.flatIndex()));
}

//-----------------------------------------------------------------------------
QModelIndex pqCompositeDataInformationTreeModel::parent(const QModelIndex& idx) const
{
  if (!idx.isValid() || isroot(idx))
  {
    return QModelIndex();
  }

  pqInternals& internals = (*this->Internals);
  const pqInternals::CNode& node = internals.find(idx);
  const pqInternals::CNode& parentNode = node.parent();
  if (parentNode == internals.nullNode())
  {
    return QModelIndex();
  }

  if (parentNode == internals.rootNode())
  {
    return this->createIndex(0, 0, static_cast<qindexdatatype>(0));
  }

  const pqInternals::CNode& parentsParentNode = parentNode.parent();
  return this->createIndex(parentsParentNode.childIndex(parentNode), idx.column(),
    static_cast<qindexdatatype>(parentNode.flatIndex()));
}

//-----------------------------------------------------------------------------
QVariant pqCompositeDataInformationTreeModel::data(const QModelIndex& idx, int role) const
{
  if (!idx.isValid())
  {
    return QVariant();
  }

  pqInternals& internals = (*this->Internals);
  const pqInternals::CNode& node = internals.find(idx);
  if (node == internals.nullNode())
  {
    return QVariant();
  }

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
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqCompositeDataInformationTreeModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags pflags = this->Superclass::flags(idx);
  if (this->UserCheckable)
  {
    pflags |= Qt::ItemIsUserCheckable | Qt::ItemIsTristate;
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

  if (role != Qt::CheckStateRole)
  {
    return false;
  }

  pqInternals& internals = (*this->Internals);
  pqInternals::CNode& node = internals.find(idx);
  if (node == internals.nullNode())
  {
    return false;
  }

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  int checkState = value.value<int>();
#else
  Qt::CheckState checkState = value.value<Qt::CheckState>();
#endif
  if (checkState == Qt::Checked && this->exclusivity())
  {
    internals.rootNode().setChecked(false, this);
  }
  node.setChecked(checkState == Qt::Checked, this);
  return true;
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
  pqInternals& internals = (*this->Internals);

  this->beginResetModel();
  bool retVal = internals.build(info, this->ExpandMultiPiece);
  this->endResetModel();
  return retVal;
}

//-----------------------------------------------------------------------------
void pqCompositeDataInformationTreeModel::setChecked(const QList<unsigned int>& indices)
{
  pqInternals& internals = (*this->Internals);
  pqInternals::CNode& root = internals.rootNode();
  root.setChecked(false, this);

  foreach (unsigned int findex, indices)
  {
    pqInternals::CNode& node = internals.find(findex);
    node.setChecked(true, this);
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
QList<unsigned int> pqCompositeDataInformationTreeModel::checkedLevels() const
{
  QList<unsigned int> indices;
  pqInternals& internals = (*this->Internals);
  pqInternals::CNode& root = internals.rootNode();
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
  pqInternals::CNode& root = internals.rootNode();
  root.setChecked(false, this);

  unsigned int childrenCount = static_cast<unsigned int>(root.childrenCount());
  foreach (unsigned int idx, indices)
  {
    if (idx < childrenCount)
    {
      root.child(idx).setChecked(true, this);
    }
  }
}

//-----------------------------------------------------------------------------
QList<QPair<unsigned int, unsigned int> >
pqCompositeDataInformationTreeModel::checkedLevelDatasets() const
{
  QList<QPair<unsigned int, unsigned int> > indices;
  pqInternals& internals = (*this->Internals);
  pqInternals::CNode& root = internals.rootNode();
  for (int level = 0; level < root.childrenCount(); ++level)
  {
    pqInternals::CNode& levelNode = root.child(level);
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
  pqInternals::CNode& root = internals.rootNode();
  root.setChecked(false, this);

  unsigned int numLevels = static_cast<unsigned int>(root.childrenCount());

  typedef QPair<unsigned int, unsigned int> IdxPair;
  foreach (const IdxPair& idx, indices)
  {
    if (idx.first < numLevels)
    {
      pqInternals::CNode& level = root.child(idx.first);
      if (idx.second < static_cast<unsigned int>(level.childrenCount()))
      {
        level.child(idx.second).setChecked(true, this);
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
const QModelIndex pqCompositeDataInformationTreeModel::rootIndex() const
{
  return this->createIndex(0, 0, static_cast<qindexdatatype>(0));
}
