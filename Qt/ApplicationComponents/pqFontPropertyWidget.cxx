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
#include "pqFontPropertyWidget.h"
#include "ui_pqFontPropertyWidget.h"

#include "pqComboBoxDomain.h"
#include "pqPropertiesPanel.h"
#include "pqSignalAdaptors.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

// Qt includes
#include <QActionGroup>
#include <QMenu>

class pqFontPropertyWidget::pqInternals
{
public:
  Ui::FontPropertyWidget Ui;

  pqInternals(pqFontPropertyWidget* self)
  {
    this->Ui.setupUi(self);
    this->Ui.mainLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.mainLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  }

  QString justification;
};

//-----------------------------------------------------------------------------
pqFontPropertyWidget::pqFontPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, smgroup, parentObject)
  , Internals(new pqInternals(this))
{
  Ui::FontPropertyWidget& ui = this->Internals->Ui;

  vtkSMProperty* smproperty = smgroup->GetProperty("Family");
  if (smproperty)
  {
    new pqComboBoxDomain(ui.FontFamily, smproperty);
    pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(ui.FontFamily);
    this->addPropertyLink(adaptor, "currentText", SIGNAL(currentTextChanged(QString)), smproperty);
  }
  else
  {
    ui.FontFamily->hide();
  }
  QObject::connect(
    ui.FontFamily, SIGNAL(currentIndexChanged(int)), this, SLOT(onFontFamilyChanged()));

  ui.FontFile->setServer(nullptr);
  ui.FontFile->setForceSingleFile(true);
  ui.FontFile->setExtension("TrueType Font Files (*.ttf *.TTF)");

  smproperty = smgroup->GetProperty("File");
  if (smproperty)
  {
    this->addPropertyLink(
      ui.FontFile, "filenames", SIGNAL(filenameChanged(const QString&)), smproperty);
  }
  else
  {
    ui.FontFile->hide();
  }

  smproperty = smgroup->GetProperty("Size");
  if (smproperty)
  {
    this->addPropertyLink(ui.FontSize, "value", SIGNAL(valueChanged(int)), smproperty);
  }
  else
  {
    ui.FontSize->hide();
  }

  smproperty = smgroup->GetProperty("Color");
  if (smproperty)
  {
    this->addPropertyLink(
      ui.FontColor, "chosenColorRgbF", SIGNAL(chosenColorChanged(const QColor&)), smproperty);

    // pqColorPaletteLinkHelper makes it possible to set this color to one of
    // the color palette colors.
    new pqColorPaletteLinkHelper(ui.FontColor, smproxy, smproxy->GetPropertyName(smproperty));
  }
  else
  {
    ui.FontColor->hide();
  }

  smproperty = smgroup->GetProperty("Opacity");
  if (smproperty)
  {
    this->addPropertyLink(ui.Opacity, "value", SIGNAL(valueChanged(double)), smproperty);
  }
  else
  {
    ui.Opacity->hide();
  }

  smproperty = smgroup->GetProperty("Bold");
  if (smproperty)
  {
    this->addPropertyLink(ui.Bold, "checked", SIGNAL(toggled(bool)), smproperty);
  }
  else
  {
    ui.Bold->hide();
  }

  smproperty = smgroup->GetProperty("Italics");
  if (smproperty)
  {
    this->addPropertyLink(ui.Italics, "checked", SIGNAL(toggled(bool)), smproperty);
  }
  else
  {
    ui.Italics->hide();
  }

  smproperty = smgroup->GetProperty("Shadow");
  if (smproperty)
  {
    this->addPropertyLink(ui.Shadow, "checked", SIGNAL(toggled(bool)), smproperty);
  }
  else
  {
    ui.Shadow->hide();
  }

  smproperty = smgroup->GetProperty("Justification");
  if (smproperty)
  {
    this->setupJustificationButton();
    this->addPropertyLink(
      this, "justification", SIGNAL(justificationChanged(QString&)), smproperty);
  }
  else
  {
    ui.Justification->hide();
  }

  onFontFamilyChanged();
}

//-----------------------------------------------------------------------------
pqFontPropertyWidget::~pqFontPropertyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::setJustification(QString& str)
{
  if (this->Internals->justification == str)
  {
    return;
  }
  this->Internals->justification = str;
  // Change toolbutton icon
  QList<QAction*> acts = this->Internals->Ui.Justification->menu()->actions();
  for (QList<QAction*>::iterator i = acts.begin(); i != acts.end(); ++i)
  {
    if ((*i)->text() == str)
    {
      this->Internals->Ui.Justification->setIcon((*i)->icon());
      break;
    }
  }

  Q_EMIT this->justificationChanged(str);
}

//-----------------------------------------------------------------------------
QString pqFontPropertyWidget::justification() const
{
  return this->Internals->justification;
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::setupJustificationButton()
{
  Ui::FontPropertyWidget& ui = this->Internals->Ui;
  QActionGroup* actionGroup = new QActionGroup(this);
  actionGroup->setExclusive(true);
  QAction* leftAlign =
    new QAction(QIcon(":/pqWidgets/Icons/pqTextAlignLeft16.png"), tr("Left"), actionGroup);
  leftAlign->setIconVisibleInMenu(true);
  QAction* rightAlign =
    new QAction(QIcon(":/pqWidgets/Icons/pqTextAlignRight16.png"), tr("Right"), actionGroup);
  rightAlign->setIconVisibleInMenu(true);
  QAction* centerAlign =
    new QAction(QIcon(":/pqWidgets/Icons/pqTextAlignCenter16.png"), tr("Center"), actionGroup);
  centerAlign->setIconVisibleInMenu(true);
  QMenu* popup = new QMenu(this);
  popup->addAction(leftAlign);
  popup->addAction(rightAlign);
  popup->addAction(centerAlign);
  ui.Justification->setMenu(popup);
  QObject::connect(
    actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeJustificationIcon(QAction*)));
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::changeJustificationIcon(QAction* action)
{
  QString str = action->text();
  this->setJustification(str);
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::onFontFamilyChanged()
{
  Ui::FontPropertyWidget& ui = this->Internals->Ui;
  ui.FontFile->setVisible(ui.FontFamily->currentIndex() == 3);
}
