/*=========================================================================

   Program: ParaView
   Module:  pqYoungsMaterialPropertyWidget.cxx

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
#include "pqYoungsMaterialPropertyWidget.h"
#include "ui_pqYoungsMaterialPropertyWidget.h"

#include "pqArraySelectionWidget.h"
#include "pqArraySelectorPropertyWidget.h"
#include "pqPropertyLinksConnection.h"
#include "pqSMAdaptor.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QMap>
#include <QSortFilterProxyModel>
#include <QStandardItem>

#include <cassert>

namespace
{
class pqYoungsMaterialPropertyLinksConnection : public pqPropertyLinksConnection
{
  typedef pqPropertyLinksConnection Superclass;

public:
  pqYoungsMaterialPropertyLinksConnection(QObject* qobject, const char* qproperty,
    const char* qsignal, vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex,
    bool use_unchecked_modified_event, QObject* parentObject = 0)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }

  ~pqYoungsMaterialPropertyLinksConnection() override = default;

protected:
  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    return pqSMAdaptor::getMultipleElementProperty(
      this->propertySM(), (use_unchecked ? pqSMAdaptor::UNCHECKED : pqSMAdaptor::CHECKED));
  }
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    pqSMAdaptor::setMultipleElementProperty(this->propertySM(), value.value<QList<QVariant> >(),
      (use_unchecked ? pqSMAdaptor::UNCHECKED : pqSMAdaptor::CHECKED));
  }
  Q_DISABLE_COPY(pqYoungsMaterialPropertyLinksConnection)
};
}

class pqYoungsMaterialPropertyWidget::pqInternals
{
public:
  Ui::YoungsMaterialPropertyWidget Ui;
  QPointer<pqArraySelectionWidget> VolumeFractionArrays;
  pqArraySelectorPropertyWidget* OrderingArrays;
  pqArraySelectorPropertyWidget* NormalArrays;
  QMap<QString, QString> NormalArraysMap;
  QMap<QString, QString> OrderingArraysMap;
};

//-----------------------------------------------------------------------------
pqYoungsMaterialPropertyWidget::pqYoungsMaterialPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smgroup->GetProperty("VolumeFractionArrays"), smproxy, parentObject)
  , Internals(new pqYoungsMaterialPropertyWidget::pqInternals())
{
  pqInternals& internals = (*this->Internals);

  QWidget* frame = new QWidget(this);
  Ui::YoungsMaterialPropertyWidget& ui = internals.Ui;
  ui.setupUi(frame);
  this->layout()->addWidget(frame);

  assert(smproxy != nullptr);
  assert(smgroup->GetProperty("VolumeFractionArrays"));
  assert(smgroup->GetProperty("OrderingArrays"));
  assert(smgroup->GetProperty("NormalArrays"));

  auto volumeFractionArraysWidget =
    this->findChild<pqArraySelectionWidget*>("ArraySelectionWidget");
  assert(volumeFractionArraysWidget);
  internals.VolumeFractionArrays = volumeFractionArraysWidget;

  QObject::connect(volumeFractionArraysWidget->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(updateComboBoxes()));

  internals.OrderingArrays =
    new pqArraySelectorPropertyWidget(smgroup->GetProperty("OrderingArrays"), smproxy, this);
  ui.gridLayout->addWidget(internals.OrderingArrays, 0, 1);
  internals.OrderingArrays->setArrayName("None");

  internals.NormalArrays =
    new pqArraySelectorPropertyWidget(smgroup->GetProperty("NormalArrays"), smproxy, this);
  ui.gridLayout->addWidget(internals.NormalArrays, 1, 1);
  internals.NormalArrays->setArrayName("None");

  this->connect(internals.OrderingArrays, SIGNAL(arrayChanged()), SLOT(onOrderingArraysChanged()));
  this->connect(internals.NormalArrays, SIGNAL(arrayChanged()), SLOT(onNormalArraysChanged()));

  this->links().addPropertyLink<pqYoungsMaterialPropertyLinksConnection>(this, "normalArrays",
    SIGNAL(normalArraysChanged()), smproxy, smgroup->GetProperty("NormalArrays"));
  this->links().addPropertyLink<pqYoungsMaterialPropertyLinksConnection>(this, "orderingArrays",
    SIGNAL(orderingArraysChanged()), smproxy, smgroup->GetProperty("OrderingArrays"));

  this->updateComboBoxes();
}

//-----------------------------------------------------------------------------
pqYoungsMaterialPropertyWidget::~pqYoungsMaterialPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqYoungsMaterialPropertyWidget::setOrderingArrays(const QList<QVariant>& values)
{
  pqInternals& internals = (*this->Internals);
  internals.OrderingArraysMap.clear();
  for (int cc = 0, max = values.size(); (cc + 1) < max; cc += 2)
  {
    internals.OrderingArraysMap[values[cc].toString()] = values[cc + 1].toString();
  }
  this->updateComboBoxes();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqYoungsMaterialPropertyWidget::orderingArrays() const
{
  pqInternals& internals = (*this->Internals);
  QList<QVariant> values;
  for (QMap<QString, QString>::const_iterator iter = internals.OrderingArraysMap.constBegin();
       iter != internals.OrderingArraysMap.constEnd(); ++iter)
  {
    values.push_back(iter.key());
    values.push_back(iter.value());
  }
  return values;
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialPropertyWidget::setNormalArrays(const QList<QVariant>& values)
{
  pqInternals& internals = (*this->Internals);
  internals.NormalArraysMap.clear();
  for (int cc = 0, max = values.size(); (cc + 1) < max; cc += 2)
  {
    internals.NormalArraysMap[values[cc].toString()] = values[cc + 1].toString();
  }
  this->updateComboBoxes();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqYoungsMaterialPropertyWidget::normalArrays() const
{
  pqInternals& internals = (*this->Internals);
  QList<QVariant> values;
  for (QMap<QString, QString>::const_iterator iter = internals.NormalArraysMap.constBegin();
       iter != internals.NormalArraysMap.constEnd(); ++iter)
  {
    values.push_back(iter.key());
    values.push_back(iter.value());
  }
  return values;
}

//-----------------------------------------------------------------------------
QStandardItem* pqYoungsMaterialPropertyWidget::currentItem()
{
  pqInternals& internals = (*this->Internals);
  QModelIndex index = internals.VolumeFractionArrays->selectionModel()->currentIndex();
  auto sortModel = qobject_cast<QSortFilterProxyModel*>(internals.VolumeFractionArrays->model());
  index = sortModel->mapToSource(index);
  auto model = qobject_cast<QStandardItemModel*>(sortModel->sourceModel());
  return model->itemFromIndex(index);
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialPropertyWidget::onNormalArraysChanged()
{
  pqInternals& internals = (*this->Internals);

  QString value = internals.NormalArrays->arrayName();
  value = (value == "None") ? "" : value;

  QStandardItem* currentItem = this->currentItem();
  if (currentItem)
  {
    QString key = currentItem->text();
    if (internals.NormalArraysMap.value(key, "__NO_VALUE__") != value)
    {
      internals.NormalArraysMap[key] = value;
      Q_EMIT this->normalArraysChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialPropertyWidget::onOrderingArraysChanged()
{
  pqInternals& internals = (*this->Internals);
  QString value = internals.OrderingArrays->arrayName();
  value = (value == "None") ? "" : value;

  QStandardItem* currentItem = this->currentItem();
  if (currentItem)
  {
    QString key = currentItem->text();
    if (internals.OrderingArraysMap.value(key, "__NO_VALUE__") != value)
    {
      internals.OrderingArraysMap[key] = value;
      Q_EMIT this->orderingArraysChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialPropertyWidget::updateComboBoxes()
{
  pqInternals& internals = (*this->Internals);

  // determine the volume fraction array currently selected.
  QStandardItem* currentItem = this->currentItem();
  if (!currentItem)
  {
    internals.OrderingArrays->setEnabled(false);
    internals.NormalArrays->setEnabled(false);
    return;
  }

  internals.OrderingArrays->setEnabled(true);
  internals.NormalArrays->setEnabled(true);

  QString label = currentItem->text();

  // check if there's a normal and ordering array already defined for this
  // volume-fraction array. If so, show it.
  const char* orderingArray = vtkSMUncheckedPropertyHelper(this->proxy(), "OrderingArrays")
                                .GetStatus(label.toLocal8Bit().data(), "");

  const char* normalArray = vtkSMUncheckedPropertyHelper(this->proxy(), "NormalArrays")
                              .GetStatus(label.toLocal8Bit().data(), "");

  if (orderingArray == nullptr || strlen(orderingArray) == 0)
  {
    orderingArray = "None";
  }
  if (normalArray == nullptr || strlen(normalArray) == 0)
  {
    normalArray = "None";
  }

  bool prev = internals.OrderingArrays->blockSignals(true);
  internals.OrderingArrays->setArrayName(orderingArray);
  internals.OrderingArrays->blockSignals(prev);

  prev = internals.NormalArrays->blockSignals(true);
  internals.NormalArrays->setArrayName(normalArray);
  internals.NormalArrays->blockSignals(prev);
}
