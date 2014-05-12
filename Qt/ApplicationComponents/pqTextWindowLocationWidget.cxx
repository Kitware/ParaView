/*=========================================================================

   Program: ParaView
   Module:  pqTextWindowLocationWidget.cxx

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

#include "pqTextWindowLocationWidget.h"
#include "ui_pqTextWindowLocationWidget.h"

#include "pqPropertiesPanel.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

// Qt includes
#include <QString>

//-----------------------------------------------------------------------------
class pqTextWindowLocationWidget::pqInternals
{
public:
  Ui::TextWindowLocationWidget Ui;

  pqInternals(pqTextWindowLocationWidget* self)
    : windowLocation(QString("AnyLocation"))
    {
    this->Ui.setupUi(self);
    this->Ui.gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    }

  QString windowLocation;
};

//-----------------------------------------------------------------------------
pqTextWindowLocationWidget::pqTextWindowLocationWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject),
  Internals(new pqInternals(this))
{
  Ui::TextWindowLocationWidget &ui = this->Internals->Ui;

  vtkSMProperty* smproperty = smgroup->GetProperty("WindowLocation");
  if(smproperty)
    {
    this->addPropertyLink(this, "windowLocation",
      SIGNAL(windowLocationChanged(QString&)), smproperty);
    QObject::connect(ui.groupBoxLocation, SIGNAL(clicked(bool)),
      this, SLOT(groupBoxLocationClicked(bool)));
    QObject::connect(ui.buttonGroupLocation, SIGNAL(
        buttonClicked(QAbstractButton*)),
      this, SLOT(emitWindowLocationChangedSignal()));
    }
  else
    {
    ui.groupBoxLocation->hide();
    ui.groupBoxPosition->setCheckable(false);
    }
  
  smproperty = smgroup->GetProperty("Position");
  if(smproperty)
    {
    QObject::connect(ui.groupBoxPosition, SIGNAL(clicked(bool)),
      this, SLOT(groupBoxPositionClicked(bool)));
    this->addPropertyLink(ui.doubleSpinBox_Pos1X, "value",
      SIGNAL(valueChanged(double)), smproperty, 0);
    this->addPropertyLink(ui.doubleSpinBox_Pos1Y, "value",
      SIGNAL(valueChanged(double)), smproperty, 1);
    }
  else
    {
    ui.groupBoxPosition->hide();
    }
}

//-----------------------------------------------------------------------------
pqTextWindowLocationWidget::~pqTextWindowLocationWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqTextWindowLocationWidget::setWindowLocation(QString& str)
{
  if(this->Internals->windowLocation == str)
    {
    return;
    }
  this->Internals->windowLocation = str;
  emit this->windowLocationChanged(str);
}

//-----------------------------------------------------------------------------
QString pqTextWindowLocationWidget::windowLocation() const
{
  return this->Internals->windowLocation;
}

//-----------------------------------------------------------------------------
void pqTextWindowLocationWidget::groupBoxLocationClicked(bool enable)
{
  this->Internals->Ui.groupBoxPosition->setChecked(!enable);
  if(!enable)
    {
    QString str("AnyLocation");
    this->setWindowLocation(str);
    }
  else
    {
    this->emitWindowLocationChangedSignal();
    }
}

//-----------------------------------------------------------------------------
void pqTextWindowLocationWidget::groupBoxPositionClicked(bool enable)
{
  this->Internals->Ui.groupBoxLocation->setChecked(!enable);
  if(enable)
    {
    QString str("AnyLocation");
    this->setWindowLocation(str);
    }
  else
    {
    this->emitWindowLocationChangedSignal();
    }
}

//-----------------------------------------------------------------------------
void pqTextWindowLocationWidget::emitWindowLocationChangedSignal()
{
  Ui::TextWindowLocationWidget &ui = this->Internals->Ui;
  if(ui.toolButtonLL->isChecked())
    {
    QString str("LowerLeftCorner");
    this->setWindowLocation(str);
    }
  else if(ui.toolButtonLC->isChecked())
    {
    QString str("LowerCenter");
    this->setWindowLocation(str);
    }
  else if(ui.toolButtonLR->isChecked())
    {
    QString str("LowerRightCorner");
    this->setWindowLocation(str);
    }
  else if(ui.toolButtonUL->isChecked())
    {
    QString str("UpperLeftCorner");
    this->setWindowLocation(str);
    }
  else if(ui.toolButtonUC->isChecked())
    {
    QString str("UpperCenter");
    this->setWindowLocation(str);
    }
  else if(ui.toolButtonUR->isChecked())
    {
    QString str("UpperRightCorner");
    this->setWindowLocation(str);
    }
}
