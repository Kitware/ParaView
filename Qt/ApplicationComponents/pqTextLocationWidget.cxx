// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTextLocationWidget.h"
#include "ui_pqTextLocationWidget.h"

#include "pqPropertiesPanel.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

// Qt includes
#include <QString>

//-----------------------------------------------------------------------------
class pqTextLocationWidget::pqInternals
{
public:
  Ui::TextLocationWidget Ui;

  pqInternals(pqTextLocationWidget* self)
  {
    this->Ui.setupUi(self);
    this->Ui.gridLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    // Add location enum values to the buttons
    this->Ui.toolButtonLL->setProperty("location", QVariant("Lower Left Corner"));
    this->Ui.toolButtonLR->setProperty("location", QVariant("Lower Right Corner"));
    this->Ui.toolButtonLC->setProperty("location", QVariant("Lower Center"));
    this->Ui.toolButtonUL->setProperty("location", QVariant("Upper Left Corner"));
    this->Ui.toolButtonUR->setProperty("location", QVariant("Upper Right Corner"));
    this->Ui.toolButtonUC->setProperty("location", QVariant("Upper Center"));
  }

  QString windowLocation;
};

//-----------------------------------------------------------------------------
pqTextLocationWidget::pqTextLocationWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals(this))
{
  Ui::TextLocationWidget& ui = this->Internals->Ui;

  vtkSMProperty* smproperty = smgroup->GetProperty("WindowLocation");
  if (smproperty)
  {
    this->addPropertyLink(
      this, "windowLocation", SIGNAL(windowLocationChanged(QString&)), smproperty);
    QObject::connect(this, SIGNAL(windowLocationChanged(QString&)), this, SLOT(updateUI()));
    QObject::connect(
      ui.radioButtonLocation, SIGNAL(clicked()), this, SLOT(radioButtonLocationClicked()));
    QObject::connect(ui.buttonGroupLocation, SIGNAL(buttonClicked(QAbstractButton*)), this,
      SLOT(radioButtonLocationClicked()));
  }
  else
  {
    ui.radioButtonLocation->hide();
    ui.radioButtonPosition->setCheckable(false);
  }

  smproperty = smgroup->GetProperty("Position");
  if (smproperty)
  {
    QObject::connect(
      ui.radioButtonPosition, SIGNAL(clicked()), this, SLOT(radioButtonPositionClicked()));
    this->addPropertyLink(
      ui.pos1X, "text2", SIGNAL(textChangedAndEditingFinished()), smproperty, 0);
    this->addPropertyLink(
      ui.pos1Y, "text2", SIGNAL(textChangedAndEditingFinished()), smproperty, 1);
  }
  else
  {
    ui.radioButtonPosition->hide();
  }

  this->updateUI();
}

//-----------------------------------------------------------------------------
pqTextLocationWidget::~pqTextLocationWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqTextLocationWidget::setWindowLocation(QString& str)
{
  if (this->Internals->windowLocation == str)
  {
    return;
  }
  this->Internals->windowLocation = str;

  Q_EMIT this->windowLocationChanged(str);
}

//-----------------------------------------------------------------------------
QString pqTextLocationWidget::windowLocation() const
{
  return this->Internals->windowLocation;
}

//-----------------------------------------------------------------------------
void pqTextLocationWidget::radioButtonLocationClicked()
{
  auto button = this->Internals->Ui.buttonGroupLocation->checkedButton();
  if (button)
  {
    QString locationStr(button->property("location").toString());
    this->setWindowLocation(locationStr);
  }
}

//-----------------------------------------------------------------------------
void pqTextLocationWidget::radioButtonPositionClicked()
{
  QString str("Any Location");
  this->setWindowLocation(str);
}

//-----------------------------------------------------------------------------
void pqTextLocationWidget::updateUI()
{
  auto& ui = this->Internals->Ui;
  QAbstractButton* buttonToCheck = nullptr;
  const QString& windowLocation = this->Internals->windowLocation;

  bool anyLocation = (windowLocation == "Any Location");
  ui.radioButtonPosition->blockSignals(true);
  ui.radioButtonPosition->setChecked(anyLocation);
  ui.radioButtonPosition->blockSignals(false);
  ui.radioButtonLocation->blockSignals(true);
  ui.radioButtonLocation->setChecked(!anyLocation);
  ui.radioButtonLocation->blockSignals(false);

  if (anyLocation)
  {
    // If we have a PositionInfo property, update the property information so that we can
    // set the Position property values read from the current PositionInfo values.
    if (this->proxy()->GetProperty("PositionInfo") != nullptr)
    {
      this->proxy()->UpdatePropertyInformation();
      auto position = vtkSMPropertyHelper(this->proxy(), "PositionInfo").GetDoubleArray();
      vtkSMPropertyHelper(this->proxy(), "Position").Set(position.data(), 2);
      this->proxy()->UpdateVTKObjects();
    }
  }
  else
  {
    // Check the selected location button if location is anything other than "AnyLocation"
    QList<QAbstractButton*> toolButtons = this->Internals->Ui.buttonGroupLocation->buttons();
    for (QAbstractButton* toolButton : toolButtons)
    {
      if (toolButton->property("location") == windowLocation)
      {
        buttonToCheck = toolButton;
        break;
      }
    }

    if (buttonToCheck)
    {
      bool blocked = buttonToCheck->blockSignals(true);
      buttonToCheck->setChecked(true);
      buttonToCheck->blockSignals(blocked);
    }
  }
}
