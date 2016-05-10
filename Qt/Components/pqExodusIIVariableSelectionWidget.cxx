/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqExodusIIVariableSelectionWidget.h"

#include <QDynamicPropertyChangeEvent>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QPointer>
#include <QSet>
#include <QtDebug>

#include "pqSMAdaptor.h"
#include "pqTimer.h"
#include "pqTreeWidgetCheckHelper.h"
#include "pqTreeWidgetItemObject.h"

namespace
{
// this is simply map to determine the icons to use for the properties on
// exodus reader.
class pqPixmapMap : public QMap<QString, QPixmap>
{
public:
  pqPixmapMap()
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
    this->insert("SetStatus", QPixmap(":/pqWidgets/Icons/pqSideSetData16.png"));
  }
};
}

class pqExodusIIVariableSelectionWidget::pqInternals
{
public:
  pqPixmapMap Pixmaps;
  pqTimer Timer;
  QSet<QString> KeysToUpdate;
  bool UpdatingSelectionCheckStates;

  pqInternals()
    : UpdatingSelectionCheckStates(false)
  {
  }

  QMap<QString, QList<QPointer<pqTreeWidgetItemObject> > > Items;
  pqTreeWidgetItemObject* findItem(pqTreeWidget* widget, const QString& key, const QString& text)
  {
    foreach (pqTreeWidgetItemObject* item, this->Items[key])
    {
      if (item && item->text(0) == text)
      {
        return item;
      }
    }
    QStringList argument;
    argument.push_back(text);
    pqTreeWidgetItemObject* item = new pqTreeWidgetItemObject(widget, argument);
    item->setData(0, Qt::ToolTipRole, text);
    item->setData(0, Qt::UserRole, key);
    // BUG #0013739. If we leave the item User-Checkable, it interferes with the
    // selection of item and we experience odd behaviors when user clicks on the
    // check-box or clicks on the name of the item. Hence we make the item
    // non-user checkable. However, pqTreeWidgetCheckHelper ends up handling the
    // click as click on the name of the item and hence we still get the item's
    // checkstate being updated when the user clicks on it. At the same time,
    // the item gets selected as well and hence the group-check-state-changing
    // works as well.
    item->setFlags(item->flags() & (~Qt::ItemIsUserCheckable));
    if (this->Pixmaps.contains(key))
    {
      item->setData(0, Qt::DecorationRole, this->Pixmaps[key]);
    }

    this->Items[key].push_back(item);
    return item;
  }

  // returns the strings for the given key.
  QStringList texts(const QString& key)
  {
    QStringList values;
    foreach (pqTreeWidgetItemObject* item, this->Items[key])
    {
      if (item)
      {
        values << item->text(0);
      }
    }
    return values;
  }

  // remove a particular item
  void purge(const QString& key, const QString& text)
  {
    for (int cc = 0; cc < this->Items[key].size(); cc++)
    {
      pqTreeWidgetItemObject* item = this->Items[key][cc];
      if (item && item->text(0) == text)
      {
        delete item;
        this->Items[key].removeAt(cc);
        break;
      }
    }
  }

  QVariant value(const QString& key)
  {
    if (!this->Items.contains(key) || this->Items[key].size() == 0)
    {
      return QVariant();
    }
    if (key == "GenerateObjectIdCellArray" || key == "GenerateGlobalElementIdArray" ||
      key == "GenerateGlobalNodeIdArray")
    {
      const pqTreeWidgetItemObject* item = this->Items[key][0];
      return item ? item->isChecked() : false;
    }
    QList<QList<QVariant> > values;
    foreach (const pqTreeWidgetItemObject* item, this->Items[key])
    {
      QList<QVariant> tuple;
      tuple << item->text(0) << item->isChecked();
      values.push_back(tuple);
    }

    return QVariant::fromValue(values);
  }
};

//-----------------------------------------------------------------------------
pqExodusIIVariableSelectionWidget::pqExodusIIVariableSelectionWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals())
{
  this->Internals->Timer.setSingleShot(true);

  this->installEventFilter(this);
  QObject::connect(&this->Internals->Timer, SIGNAL(timeout()), this, SLOT(updateProperty()));

  // Make a click anywhere on a variable's row change its checked/unchecked state.
  new pqTreeWidgetCheckHelper(this, 0, this);

  this->setSelectionMode(QAbstractItemView::ContiguousSelection);
  this->setSelectionBehavior(QAbstractItemView::SelectRows);
}

//-----------------------------------------------------------------------------
pqExodusIIVariableSelectionWidget::~pqExodusIIVariableSelectionWidget()
{
  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
bool pqExodusIIVariableSelectionWidget::eventFilter(QObject* object, QEvent* qevent)
{
  if (qevent->type() == QEvent::DynamicPropertyChange)
  {
    QDynamicPropertyChangeEvent* dpevent = dynamic_cast<QDynamicPropertyChangeEvent*>(qevent);
    this->propertyChanged(dpevent->propertyName());
  }
  return this->Superclass::eventFilter(object, qevent);
}

//-----------------------------------------------------------------------------
void pqExodusIIVariableSelectionWidget::propertyChanged(const QString& pname)
{
  QVariant properyValue = this->property(pname.toLocal8Bit().data());
  if (pname == "GenerateObjectIdCellArray")
  {
    this->setStatus(pname, "Object Ids", properyValue.toBool());
  }
  else if (pname == "GenerateGlobalElementIdArray")
  {
    this->setStatus(pname, "Global Element Ids", properyValue.toBool());
  }
  else if (pname == "GenerateGlobalNodeIdArray")
  {
    this->setStatus(pname, "Global Node Ids", properyValue.toBool());
  }
  else
  {
    QStringList currentStatiiKeys = this->Internals->texts(pname);

    QList<QList<QVariant> > status_values = properyValue.value<QList<QList<QVariant> > >();
    foreach (const QList<QVariant>& tuple, status_values)
    {
      if (tuple.size() == 2)
      {
        this->setStatus(pname, tuple[0].toString(), tuple[1].toBool());
        currentStatiiKeys.removeAll(tuple[0].toString());
      }
    }

    // remove any keys that are no longer present.
    foreach (const QString& key, currentStatiiKeys)
    {
      this->Internals->purge(pname, key);
    }
  }
}

//-----------------------------------------------------------------------------
void pqExodusIIVariableSelectionWidget::setStatus(
  const QString& key, const QString& text, bool value)
{
  pqTreeWidgetItemObject* item = this->Internals->findItem(this, key, text);
  item->setChecked(value);

  // BUG #13726. To avoid dramatic performance degradation when dealing with
  // large list of variables, we use a timer to collapse all update requests.
  QObject::connect(item, SIGNAL(checkedStateChanged(bool)), this, SLOT(delayedUpdateProperty(bool)),
    Qt::UniqueConnection);
}

//-----------------------------------------------------------------------------
void pqExodusIIVariableSelectionWidget::delayedUpdateProperty(bool check_state)
{
  // BUG: 13739. To make toggling of check-states for a bunch of items easier,
  // we follow the style adopted by GMail: we change the state of all selected
  // items to match the current items state.
  if (!this->Internals->UpdatingSelectionCheckStates)
  {
    this->Internals->UpdatingSelectionCheckStates = true;
    foreach (QTreeWidgetItem* selItem, this->selectedItems())
    {
      bool item_state = selItem->checkState(0) == Qt::Unchecked ? false : true;
      if (item_state != check_state)
      {
        selItem->setCheckState(0, check_state ? Qt::Checked : Qt::Unchecked);
      }
    }
    this->Internals->UpdatingSelectionCheckStates = false;
  }

  this->Internals->Timer.start(0);
  // save the pqTreeWidgetItemObject that fired this signal so we can update
  // property using its state later.
  pqTreeWidgetItemObject* item = qobject_cast<pqTreeWidgetItemObject*>(this->sender());
  if (item)
  {
    QString key = item->data(0, Qt::UserRole).toString();
    this->Internals->KeysToUpdate.insert(key);
  }
}

//-----------------------------------------------------------------------------
void pqExodusIIVariableSelectionWidget::updateProperty()
{
  foreach (const QString& key, this->Internals->KeysToUpdate)
  {
    QVariant newValue = this->Internals->value(key);
    if (this->property(key.toLocal8Bit().data()) != newValue)
    {
      this->setProperty(key.toLocal8Bit().data(), newValue);
    }
  }
  this->Internals->KeysToUpdate.clear();
  emit this->widgetModified();
}
