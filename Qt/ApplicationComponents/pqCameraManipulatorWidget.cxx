// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCameraManipulatorWidget.h"
#include "ui_pqCameraManipulatorWidget.h"

#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqPropertyLinksConnection.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cassert>

class pqCameraManipulatorWidget::PropertyLinksConnection : public pqPropertyLinksConnection
{
  typedef pqPropertyLinksConnection Superclass;

public:
  PropertyLinksConnection(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = nullptr)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }
  ~PropertyLinksConnection() override = default;

protected:
  /// Called to update the ServerManager Property due to UI change.
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    QList<QVariant> list = value.toList();

    assert(list.size() == 9);
    int values[9];
    for (int cc = 0; cc < 9; cc++)
    {
      values[cc] = list[cc].toInt();
    }
    if (use_unchecked)
    {
      vtkSMUncheckedPropertyHelper(this->propertySM()).Set(values, 9);
    }
    else
    {
      vtkSMPropertyHelper(this->propertySM()).Set(values, 9);
    }
  }

  /// called to get the current value for the ServerManager Property.
  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    int values[9];
    int max = 0;
    if (use_unchecked)
    {
      max = vtkSMUncheckedPropertyHelper(this->propertySM()).Get(values, 9);
    }
    else
    {
      max = vtkSMPropertyHelper(this->propertySM()).Get(values, 9);
    }

    QList<QVariant> val;
    for (int cc = 0; cc < max && cc < 9; cc++)
    {
      val << values[cc];
    }
    return val;
  }

private:
  Q_DISABLE_COPY(PropertyLinksConnection)
};

class pqCameraManipulatorWidget::pqInternals
{
public:
  Ui::CameraManipulatorWidget Ui;
  QPointer<QComboBox> Boxes[9];
};

//-----------------------------------------------------------------------------
pqCameraManipulatorWidget::pqCameraManipulatorWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals())
{
  Ui::CameraManipulatorWidget& ui = this->Internals->Ui;
  ui.setupUi(this);
  ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  ui.gridLayout->setContentsMargins(0, 0, 0, 0);

  QPointer<QComboBox>* boxes = this->Internals->Boxes;
  boxes[0] = ui.comboBox_1;
  boxes[1] = ui.comboBox_2;
  boxes[2] = ui.comboBox_3;
  boxes[3] = ui.comboBox_4;
  boxes[4] = ui.comboBox_5;
  boxes[5] = ui.comboBox_6;
  boxes[6] = ui.comboBox_7;
  boxes[7] = ui.comboBox_8;
  boxes[8] = ui.comboBox_9;

  auto domain = smproperty->FindDomain<vtkSMEnumerationDomain>();
  if (!domain)
  {
    qCritical("Developer error: pqCameraManipulatorWidget needs vtkSMEnumerationDomain.");
    return;
  }

  for (int cc = 0; cc < 9; cc++)
  {
    for (unsigned int kk = 0, max = domain->GetNumberOfEntries(); kk < max; kk++)
    {
      boxes[cc]->addItem(QCoreApplication::translate("ServerManagerXML", domain->GetEntryText(kk)),
        QVariant(domain->GetEntryValue(kk)));
    }
    this->connect(boxes[cc], SIGNAL(currentIndexChanged(int)), SIGNAL(manipulatorTypesChanged()));
  }

  this->links().addPropertyLink<pqCameraManipulatorWidget::PropertyLinksConnection>(
    this, "manipulatorTypes", SIGNAL(manipulatorTypesChanged()), smproxy, smproperty);
}

//-----------------------------------------------------------------------------
pqCameraManipulatorWidget::~pqCameraManipulatorWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqCameraManipulatorWidget::setManipulatorTypes(const QList<QVariant>& value)
{
  QPointer<QComboBox>* boxes = this->Internals->Boxes;
  for (int cc = 0, max = value.size(); cc < 9 && cc < max; cc++)
  {
    int index = boxes[cc]->findData(value[cc]);
    if (index != -1)
    {
      boxes[cc]->setCurrentIndex(index);
    }
    else
    {
      boxes[cc]->setCurrentIndex(1);
    }
  }
  for (int cc = value.size(); cc < 9; cc++)
  {
    boxes[cc]->setCurrentIndex(0);
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqCameraManipulatorWidget::manipulatorTypes() const
{
  QList<QVariant> value;

  QPointer<QComboBox>* boxes = this->Internals->Boxes;
  for (int cc = 0; cc < 9; cc++)
  {
    value.push_back(boxes[cc]->itemData(boxes[cc]->currentIndex()));
  }
  return value;
}
