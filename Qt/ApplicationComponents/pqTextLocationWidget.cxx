/*=========================================================================

   Program: ParaView
   Module:  pqTextLocationWidget.cxx

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
    this->Ui.gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    // Add location enum values to the buttons
    this->Ui.toolButtonLL->setProperty("location", QVariant("LowerLeftCorner"));
    this->Ui.toolButtonLR->setProperty("location", QVariant("LowerRightCorner"));
    this->Ui.toolButtonLC->setProperty("location", QVariant("LowerCenter"));
    this->Ui.toolButtonUL->setProperty("location", QVariant("UpperLeftCorner"));
    this->Ui.toolButtonUR->setProperty("location", QVariant("UpperRightCorner"));
    this->Ui.toolButtonUC->setProperty("location", QVariant("UpperCenter"));
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
  QString locationStr(button->property("location").toString());
  this->setWindowLocation(locationStr);
}

//-----------------------------------------------------------------------------
void pqTextLocationWidget::radioButtonPositionClicked()
{
  QString str("AnyLocation");
  this->setWindowLocation(str);
}

//-----------------------------------------------------------------------------
void pqTextLocationWidget::updateUI()
{
  auto& ui = this->Internals->Ui;
  QAbstractButton* buttonToCheck = nullptr;
  const QString& windowLocation = this->Internals->windowLocation;

  bool anyLocation = (windowLocation == "AnyLocation");
  ui.radioButtonPosition->blockSignals(true);
  ui.radioButtonPosition->setChecked(anyLocation);
  ui.radioButtonPosition->blockSignals(false);
  ui.radioButtonLocation->blockSignals(true);
  ui.radioButtonLocation->setChecked(!anyLocation);
  ui.radioButtonLocation->blockSignals(false);

  // Check the selected location button if location is anything other than "AnyLocation"
  if (!anyLocation)
  {
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
