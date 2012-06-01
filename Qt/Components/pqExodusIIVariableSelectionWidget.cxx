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
#include <QMap>
#include <QPixmap>
#include <QPointer>
#include <QtDebug>

#include "pqTreeWidgetItemObject.h"
#include "pqSMAdaptor.h"

namespace
{
  // this is simply map to determine the icons to use for the properties on
  // exodus reader.
  class pqPixmapMap : public QMap<QString, QPixmap>
    {
  public:
    pqPixmapMap()
      {
      this->insert("GenerateObjectIdCellArray",
        QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
      this->insert("GenerateGlobalElementIdArray",
        QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
      this->insert("GenerateGlobalNodeIdArray",
        QPixmap(":/pqWidgets/Icons/pqNodalData16.png"));

      this->insert("ElementVariables",
        QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
      this->insert("FaceVariables",
        QPixmap(":/pqWidgets/Icons/pqFaceCenterData16.png"));
      this->insert("EdgeVariables",
        QPixmap(":/pqWidgets/Icons/pqEdgeCenterData16.png"));

      this->insert("SideSetResultArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqSideSetData16.png"));
      this->insert("NodeSetResultArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqNodeSetData16.png"));
      this->insert("FaceSetResultArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqFaceSetData16.png"));
      this->insert("EdgeSetResultArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqEdgeSetData16.png"));
      this->insert("ElementSetResultArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqElemSetData16.png"));

      this->insert("PointVariables",
        QPixmap(":/pqWidgets/Icons/pqNodalData16.png"));
      this->insert("GlobalVariables",
        QPixmap(":/pqWidgets/Icons/pqGlobalData16.png"));

      this->insert("SideSetArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqSideSetData16.png"));
      this->insert("NodeSetArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqNodeSetData16.png"));
      this->insert("FaceSetArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqFaceSetData16.png"));
      this->insert("EdgeSetArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqEdgeSetData16.png"));
      this->insert("ElementSetArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqElemSetData16.png"));

      this->insert("NodeMapArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqNodeMapData16.png"));
      this->insert("EdgeMapArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqEdgeMapData16.png"));
      this->insert("FaceMapArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqFaceMapData16.png"));
      this->insert("ElementMapArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqElemMapData16.png"));

      this->insert("PointArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqNodalData16.png"));
      this->insert("CellArrayStatus",
        QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
      this->insert("SetStatus",
        QPixmap(":/pqWidgets/Icons/pqSideSetData16.png"));
      }
    };
}

class pqExodusIIVariableSelectionWidget::pqInternals
{
public:
  pqPixmapMap Pixmaps;

  QMap<QString, QList<QPointer<pqTreeWidgetItemObject> > > Items;
  pqTreeWidgetItemObject* findItem(
    pqTreeWidget* widget, const QString& key, const QString& text)
    {
    foreach (pqTreeWidgetItemObject* item, this->Items[key])
      {
      if (item && item->text(0) == text)
        {
        return item;
        }
      }
    QStringList argument; argument.push_back(text);
    pqTreeWidgetItemObject* item = new pqTreeWidgetItemObject(widget, argument);
    item->setData(0, Qt::ToolTipRole, text);
    item->setData(0, Qt::UserRole, key);
    if (this->Pixmaps.contains(key))
      {
      item->setData(0, Qt::DecorationRole, this->Pixmaps[key]);
      }

    this->Items[key].push_back(item);
    return item;
    }

  QVariant value(const QString& key)
    {
    if (!this->Items.contains(key) || this->Items[key].size() == 0)
      {
      return QVariant();
      }
    if (key == "GenerateObjectIdCellArray" ||
      key == "GenerateGlobalElementIdArray" ||
      key == "GenerateGlobalNodeIdArray")
      {
      const pqTreeWidgetItemObject* item = this->Items[key][0];
      return item? item->isChecked() : false;
      }
    QList<QList<QVariant > > values;
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
  : Superclass(parentObject),
  Internals(new pqInternals())
{
  this->installEventFilter(this);
}

//-----------------------------------------------------------------------------
pqExodusIIVariableSelectionWidget::~pqExodusIIVariableSelectionWidget()
{
  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
bool pqExodusIIVariableSelectionWidget::eventFilter(
  QObject* object, QEvent* qevent)
{
  if (qevent->type() == QEvent::DynamicPropertyChange)
    {
    QDynamicPropertyChangeEvent* dpevent =
      dynamic_cast<QDynamicPropertyChangeEvent*>(qevent);
    this->propertyChanged(dpevent->propertyName());
    }
  return this->Superclass::eventFilter(object, qevent);
}

//-----------------------------------------------------------------------------
void pqExodusIIVariableSelectionWidget::propertyChanged(const QString& pname)
{
  if (!this->Internals->Pixmaps.contains(pname))
    {
    qDebug() << "Unexpected property: " << pname;
    return;
    }

  QVariant properyValue = this->property(pname.toAscii().data());
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
    QList<QList<QVariant> > status_values = properyValue.value<QList<QList<QVariant> > >();
    QMap<QString, bool> statuses;
    foreach (const QList<QVariant>& tuple, status_values)
      {
      if (tuple.size() == 2)
        {
        this->setStatus(pname, tuple[0].toString(), tuple[1].toBool());
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqExodusIIVariableSelectionWidget::setStatus(
  const QString& key, const QString& text, bool value)
{
  pqTreeWidgetItemObject* item = this->Internals->findItem(this, key, text);
  item->setChecked(value);
  QObject::connect(item, SIGNAL(checkedStateChanged(bool)),
    this, SLOT(updateProperty()), Qt::UniqueConnection);
}

//-----------------------------------------------------------------------------
void pqExodusIIVariableSelectionWidget::updateProperty()
{
  pqTreeWidgetItemObject* item = qobject_cast<pqTreeWidgetItemObject*>(
    this->sender());
  if (item)
    {
    QString key = item->data(0, Qt::UserRole).toString();
    QVariant newValue = this->Internals->value(key);
    if (this->property(key.toAscii().data()) != newValue)
      {
      this->setProperty(key.toAscii().data(), newValue);
      }
    }
  emit this->widgetModified();
}
