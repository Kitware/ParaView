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

#include "pqComboBoxDomain.h"
#include "pqPropertyLinks.h"
#include "pqPropertyLinksConnection.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidget.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QMap>

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
  ~pqYoungsMaterialPropertyLinksConnection() override {}

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
  QPointer<QTreeWidget> VolumeFractionArrays;
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

  assert(smproxy != NULL);
  assert(smgroup->GetProperty("VolumeFractionArrays"));
  assert(smgroup->GetProperty("OrderingArrays"));
  assert(smgroup->GetProperty("NormalArrays"));

  QTreeWidget* volumeFractionArraysWidget = this->findChild<QTreeWidget*>("ArraySelectionWidget");
  assert(volumeFractionArraysWidget);
  internals.VolumeFractionArrays = volumeFractionArraysWidget;

  QObject::connect(volumeFractionArraysWidget,
    SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(updateComboBoxes()));

  pqComboBoxDomain* domain1 =
    new pqComboBoxDomain(ui.OrderingArrays, smgroup->GetProperty("OrderingArrays"));
  domain1->setObjectName("OrderingArraysDomain");
  domain1->addString("None");

  pqComboBoxDomain* domain2 =
    new pqComboBoxDomain(ui.NormalArrays, smproxy->GetProperty("NormalArrays"));
  domain2->setObjectName("NormalArraysDomain");
  domain2->addString("None");

  this->connect(ui.OrderingArrays, SIGNAL(currentIndexChanged(const QString&)),
    SLOT(orderingArraysChanged(const QString&)));
  this->connect(ui.NormalArrays, SIGNAL(currentIndexChanged(const QString&)),
    SLOT(normalArraysChanged(const QString&)));

  this->links().addPropertyLink<pqYoungsMaterialPropertyLinksConnection>(this, "normalArrays",
    SIGNAL(normalArraysChanged()), smproxy, smgroup->GetProperty("NormalArrays"));
  this->links().addPropertyLink<pqYoungsMaterialPropertyLinksConnection>(this, "orderingArrays",
    SIGNAL(orderingArraysChanged()), smproxy, smgroup->GetProperty("OrderingArrays"));

  this->updateComboBoxes();
}

//-----------------------------------------------------------------------------
pqYoungsMaterialPropertyWidget::~pqYoungsMaterialPropertyWidget()
{
}

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
void pqYoungsMaterialPropertyWidget::normalArraysChanged(const QString& val)
{
  pqInternals& internals = (*this->Internals);
  const QString value = (val == "None") ? "" : val;

  QTreeWidgetItem* currentItem = internals.VolumeFractionArrays->currentItem();
  if (currentItem)
  {
    QString key = currentItem->text(0);
    if (internals.NormalArraysMap.value(key, "__NO_VALUE__") != value)
    {
      internals.NormalArraysMap[key] = value;
      emit this->normalArraysChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialPropertyWidget::orderingArraysChanged(const QString& val)
{
  pqInternals& internals = (*this->Internals);
  const QString value = (val == "None") ? "" : val;

  QTreeWidgetItem* currentItem = internals.VolumeFractionArrays->currentItem();
  if (currentItem)
  {
    QString key = currentItem->text(0);
    if (internals.OrderingArraysMap.value(key, "__NO_VALUE__") != value)
    {
      internals.OrderingArraysMap[key] = value;
      emit this->orderingArraysChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialPropertyWidget::updateComboBoxes()
{
  pqInternals& internals = (*this->Internals);
  Ui::YoungsMaterialPropertyWidget& ui = internals.Ui;

  // determine the volume fraction array currently selected.
  QTreeWidgetItem* currentItem = internals.VolumeFractionArrays->currentItem();
  if (!currentItem)
  {
    ui.OrderingArrays->setEnabled(false);
    ui.NormalArrays->setEnabled(false);
    return;
  }

  ui.OrderingArrays->setEnabled(true);
  ui.NormalArrays->setEnabled(true);

  QString label = currentItem->text(0);

  // check if there's a normal and ordering array already defined for this
  // volume-fraction array. If so, show it.
  const char* ordering_array = vtkSMUncheckedPropertyHelper(this->proxy(), "OrderingArrays")
                                 .GetStatus(label.toLocal8Bit().data(), "");

  const char* normal_array = vtkSMUncheckedPropertyHelper(this->proxy(), "NormalArrays")
                               .GetStatus(label.toLocal8Bit().data(), "");

  if (ordering_array == NULL || strlen(ordering_array) == 0)
  {
    ordering_array = "None";
  }
  if (normal_array == NULL || strlen(normal_array) == 0)
  {
    normal_array = "None";
  }

  bool prev = ui.OrderingArrays->blockSignals(true);
  ui.OrderingArrays->setCurrentIndex(ui.OrderingArrays->findText(ordering_array));
  ui.OrderingArrays->blockSignals(prev);

  prev = ui.NormalArrays->blockSignals(true);
  ui.NormalArrays->setCurrentIndex(ui.NormalArrays->findText(normal_array));
  ui.NormalArrays->blockSignals(prev);
}
