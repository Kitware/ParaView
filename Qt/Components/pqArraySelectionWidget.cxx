/*=========================================================================

   Program: ParaView
   Module:  pqArraySelectionWidget.cxx

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
#include "pqArraySelectionWidget.h"

#include <QEvent>
#include <QHeaderView>
#include <QMap>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QScopedValueRollback>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyle>
#include <QStyleOptionButton>

#include <map>

namespace
{
// this is simply map to determine the icons to use for the properties on
// exodus reader.
class PixmapMap : public QMap<QString, QPixmap>
{
public:
  PixmapMap()
  {
    this->insert("GenerateObjectIdCellArray", QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
    this->insert(
      "GenerateGlobalElementIdArray", QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
    this->insert("GenerateGlobalNodeIdArray", QPixmap(":/pqWidgets/Icons/pqNodalData16.png"));

    this->insert("ElementVariables", QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
    this->insert("FaceVariables", QPixmap(":/pqWidgets/Icons/pqFaceCenterData16.png"));
    this->insert("EdgeVariables", QPixmap(":/pqWidgets/Icons/pqEdgeCenterData16.png"));

    this->insert("SideSetResultArrayStatus", QPixmap(":/pqWidgets/Icons/pqSideSetData16.png"));
    this->insert("NodeSetResultArrayStatus", QPixmap(":/pqWidgets/Icons/pqNodeSetData16.png"));
    this->insert("FaceSetResultArrayStatus", QPixmap(":/pqWidgets/Icons/pqFaceSetData16.png"));
    this->insert("EdgeSetResultArrayStatus", QPixmap(":/pqWidgets/Icons/pqEdgeSetData16.png"));
    this->insert("ElementSetResultArrayStatus", QPixmap(":/pqWidgets/Icons/pqElemSetData16.png"));

    this->insert("PointVariables", QPixmap(":/pqWidgets/Icons/pqNodalData16.png"));
    this->insert("GlobalVariables", QPixmap(":/pqWidgets/Icons/pqGlobalData16.png"));

    this->insert("SideSetArrayStatus", QPixmap(":/pqWidgets/Icons/pqSideSetData16.png"));
    this->insert("NodeSetArrayStatus", QPixmap(":/pqWidgets/Icons/pqNodeSetData16.png"));
    this->insert("FaceSetArrayStatus", QPixmap(":/pqWidgets/Icons/pqFaceSetData16.png"));
    this->insert("EdgeSetArrayStatus", QPixmap(":/pqWidgets/Icons/pqEdgeSetData16.png"));
    this->insert("ElementSetArrayStatus", QPixmap(":/pqWidgets/Icons/pqElemSetData16.png"));

    this->insert("NodeMapArrayStatus", QPixmap(":/pqWidgets/Icons/pqNodeMapData16.png"));
    this->insert("EdgeMapArrayStatus", QPixmap(":/pqWidgets/Icons/pqEdgeMapData16.png"));
    this->insert("FaceMapArrayStatus", QPixmap(":/pqWidgets/Icons/pqFaceMapData16.png"));
    this->insert("ElementMapArrayStatus", QPixmap(":/pqWidgets/Icons/pqElemMapData16.png"));

    this->insert("PointArrayStatus", QPixmap(":/pqWidgets/Icons/pqNodalData16.png"));
    this->insert("CellArrayStatus", QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
    this->insert("ColumnArrayStatus", QPixmap(":/pqWidgets/Icons/pqSpreadsheet16.png"));
    this->insert("SetStatus", QPixmap(":/pqWidgets/Icons/pqSideSetData16.png"));
  }
};

// this keeps tracks for pixmaps to use to render checkboxes
// in the header.
class CheckboxPixMaps
{
  QVector<QPixmap> Pixmaps;

public:
  CheckboxPixMaps(QWidget* parent)
  {
    QStyle::State styleOptions[3] = { QStyle::State_Off, QStyle::State_NoChange, QStyle::State_On };

    QStyleOptionButton option;
    QRect rect = parent->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option);
    option.rect = QRect(QPoint(0, 0), rect.size());
    for (int i = 0; i < 3; i++)
    {
      QPixmap apixmap(rect.size());
      apixmap.fill(QColor(0, 0, 0, 0));
      QPainter painter(&apixmap);
      option.state = styleOptions[i] | QStyle::State_Enabled;
      parent->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter);
      this->Pixmaps.push_back(apixmap);
    }
  }

  QPixmap pixmap(Qt::CheckState state) const
  {
    int istate = static_cast<int>(state);
    return (istate >= 0 && istate < 3) ? this->Pixmaps[istate] : QPixmap();
  }
};
}

class pqArraySelectionWidget::Model : public QStandardItemModel
{
  using Superclass = QStandardItemModel;
  QPointer<pqArraySelectionWidget> Widget;
  PixmapMap Pixmaps;
  CheckboxPixMaps Checkboxes;

public:
  Model(int rs, int cs, pqArraySelectionWidget* parentObject)
    : Superclass(rs, cs, parentObject)
    , Widget(parentObject)
    , Checkboxes(parentObject)
  {
  }

  ~Model() override {}

  void setStatus(const QString& key, const QString& label, bool value)
  {
    using map_type = std::map<QString, bool>;
    this->setStatus(key, map_type{ { label, value } });
  }

  void setStatus(const QString& key, const std::map<QString, bool>& value_map)
  {
    auto& items_map = this->GroupedItemsMap[key];
    // any items not in value_map should be removed.
    for (auto iter = items_map.begin(); iter != items_map.end();)
    {
      if (value_map.find(iter->first) == value_map.end())
      {
        this->removeRow(iter->second);
        iter = items_map.erase(iter);
      }
      else
      {
        ++iter;
      }
    }

    const QVariant pixmap = this->Pixmaps.contains(key) ? QVariant(this->Pixmaps[key]) : QVariant();

    for (const auto& pair : value_map)
    {
      auto iter = items_map.find(pair.first);
      if (iter == items_map.end())
      {
        auto item = new QStandardItem(pair.first);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
          Qt::ItemNeverHasChildren);
        item->setData(pixmap, Qt::DecorationRole);

        // fixme: icon
        this->appendRow(item);

        // add to map.
        iter = items_map.insert(std::pair<QString, QStandardItem*>(pair.first, item)).first;
      }
      Q_ASSERT(iter != items_map.end());
      Q_ASSERT(iter->second != nullptr);
      iter->second->setCheckState(pair.second ? Qt::Checked : Qt::Unchecked);
    }
    // potentially changed, so just indicate that.
    emit this->headerDataChanged(Qt::Horizontal, 0, 0);
  }

  void remove(const QString& key)
  {
    auto iter = this->GroupedItemsMap.find(key);
    if (iter != this->GroupedItemsMap.end())
    {
      for (const auto& itemspair : iter->second)
      {
        this->removeRow(itemspair.second);
      }
      this->GroupedItemsMap.erase(iter);
    }
  }

  bool setData(const QModelIndex& idx, const QVariant& value, int role) override
  {
    const bool retval = this->Superclass::setData(idx, value, role);
    if (role == Qt::CheckStateRole)
    {
      const QString key = this->keyFromIndex(idx);
      if (!key.isEmpty())
      {
        this->Widget->updateProperty(key, this->status(key));
        emit this->headerDataChanged(Qt::Horizontal, 0, 0);
      }
    }
    return retval;
  }

private:
  Q_DISABLE_COPY(Model);

  void removeRow(QStandardItem* item)
  {
    auto idx = this->indexFromItem(item);
    if (idx.isValid())
    {
      this->removeRows(idx.row(), 1);
    }
  }

  QString keyFromIndex(const QModelIndex& idx) const
  {
    return this->keyFromItem(this->itemFromIndex(idx));
  }

  QString keyFromItem(QStandardItem* item) const
  {
    for (const auto& pair1 : this->GroupedItemsMap)
    {
      for (const auto& pair2 : pair1.second)
      {
        if (pair2.second == item)
        {
          return pair1.first;
        }
      }
    }
    return QString();
  }

  QVariant status(const QString& key)
  {
    auto iter = this->GroupedItemsMap.find(key);
    if (iter == this->GroupedItemsMap.end() || iter->second.size() == 0)
    {
      return QVariant();
    }

    if ((key == "GenerateObjectIdCellArray" || key == "GenerateGlobalElementIdArray" ||
          key == "GenerateGlobalNodeIdArray") &&
      iter->second.size() == 1)
    {
      return QVariant(iter->second.begin()->second->checkState() == Qt::Checked ? 1 : 0);
    }

    QList<QList<QVariant> > values;
    for (const auto& pair : iter->second)
    {
      values.push_back(QList<QVariant>{
        QVariant(pair.first), QVariant(pair.second->checkState() == Qt::Checked ? 1 : 0) });
    }
    return QVariant::fromValue(values);
  }

  std::map<QString, std::map<QString, QStandardItem*> > GroupedItemsMap;
};

//-----------------------------------------------------------------------------
pqArraySelectionWidget::pqArraySelectionWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , UpdatingProperty(false)
{
  this->setObjectName("ArraySelectionWidget");
  this->setRootIsDecorated(false);
  this->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->setSelectionMode(QAbstractItemView::ExtendedSelection);

  auto mymodel = new pqArraySelectionWidget::Model(0, 1, this);
  this->setModel(mymodel);
}

//-----------------------------------------------------------------------------
pqArraySelectionWidget::~pqArraySelectionWidget()
{
}

//-----------------------------------------------------------------------------
bool pqArraySelectionWidget::event(QEvent* evt)
{
  if (evt->type() == QEvent::DynamicPropertyChange && !this->UpdatingProperty)
  {
    auto devt = dynamic_cast<QDynamicPropertyChangeEvent*>(evt);
    this->propertyChanged(devt->propertyName());
    return true;
  }

  return this->Superclass::event(evt);
}

//-----------------------------------------------------------------------------
void pqArraySelectionWidget::propertyChanged(const QString& pname)
{
  Q_ASSERT(this->UpdatingProperty == false);

  auto amodel = dynamic_cast<pqArraySelectionWidget::Model*>(this->model());
  Q_ASSERT(amodel);

  QVariant value = this->property(pname.toLocal8Bit().data());
  if (!value.isValid())
  {
    // dynamic property is being removed, clear it from the model.
    amodel->remove(pname);
    return;
  }

  if (pname == "GenerateObjectIdCellArray")
  {
    amodel->setStatus(pname, "Object Ids", value.toBool());
  }
  else if (pname == "GenerateGlobalElementIdArray")
  {
    amodel->setStatus(pname, "Global Element Ids", value.toBool());
  }
  else if (pname == "GenerateGlobalNodeIdArray")
  {
    amodel->setStatus(pname, "Global Node Ids", value.toBool());
  }
  else
  {
    const QList<QList<QVariant> > status_values = value.value<QList<QList<QVariant> > >();
    std::map<QString, bool> statuses;
    for (const QList<QVariant>& tuple : status_values)
    {
      if (tuple.size() == 2)
      {
        statuses[tuple[0].toString()] = tuple[1].toBool();
      }
    }
    amodel->setStatus(pname, statuses);
  }
}

//-----------------------------------------------------------------------------
void pqArraySelectionWidget::updateProperty(const QString& pname, const QVariant& value)
{
  // for scope
  {
    QScopedValueRollback<bool> rollback(this->UpdatingProperty, true);
    this->setProperty(pname.toLocal8Bit().data(), value);
  }
  emit this->widgetModified();
}

//-----------------------------------------------------------------------------
void pqArraySelectionWidget::setHeaderLabel(const QString& label)
{
  auto amodel = dynamic_cast<pqArraySelectionWidget::Model*>(this->model());
  Q_ASSERT(amodel);
  amodel->setHeaderData(0, Qt::Horizontal, label, Qt::DisplayRole);
}

//-----------------------------------------------------------------------------
QString pqArraySelectionWidget::headerLabel() const
{
  auto amodel = dynamic_cast<const pqArraySelectionWidget::Model*>(this->model());
  Q_ASSERT(amodel);
  return amodel->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString();
}
