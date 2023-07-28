// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
    this->Ui.mainLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin());
    this->Ui.mainLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  }

  QString HorizontalJustification;
  QString VerticalJustification;
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
    this->setupHorizontalJustificationButton();
    this->addPropertyLink(this, "HorizontalJustification",
      SIGNAL(horizontalJustificationChanged(QString&)), smproperty);
  }
  else
  {
    ui.HorizontalJustification->hide();
  }

  smproperty = smgroup->GetProperty("VerticalJustification");
  if (smproperty)
  {
    this->setupVerticalJustificationButton();
    this->addPropertyLink(
      this, "VerticalJustification", SIGNAL(verticalJustificationChanged(QString&)), smproperty);
  }
  else
  {
    ui.VerticalJustification->hide();
  }

  onFontFamilyChanged();
}

//-----------------------------------------------------------------------------
pqFontPropertyWidget::~pqFontPropertyWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::setHorizontalJustification(QString& str)
{
  if (this->Internals->HorizontalJustification == str)
  {
    return;
  }
  this->Internals->HorizontalJustification = str;

  this->UpdateToolButtonIcon(str, this->Internals->Ui.HorizontalJustification);

  Q_EMIT this->horizontalJustificationChanged(str);
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::setVerticalJustification(QString& str)
{
  if (this->Internals->VerticalJustification == str)
  {
    return;
  }
  this->Internals->VerticalJustification = str;

  this->UpdateToolButtonIcon(str, this->Internals->Ui.VerticalJustification);

  Q_EMIT this->verticalJustificationChanged(str);
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::UpdateToolButtonIcon(QString& str, QToolButton* justification)
{
  // Change toolbutton icon
  QList<QAction*> acts = justification->menu()->actions();
  for (QList<QAction*>::iterator i = acts.begin(); i != acts.end(); ++i)
  {
    if ((*i)->text() == str)
    {
      justification->setIcon((*i)->icon());
      break;
    }
  }
}

//-----------------------------------------------------------------------------
QString pqFontPropertyWidget::HorizontalJustification() const
{
  return this->Internals->HorizontalJustification;
}

//-----------------------------------------------------------------------------
QString pqFontPropertyWidget::VerticalJustification() const
{
  return this->Internals->VerticalJustification;
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::setupHorizontalJustificationButton()
{
  Ui::FontPropertyWidget& ui = this->Internals->Ui;
  QActionGroup* actionGroup = new QActionGroup(this);
  actionGroup->setExclusive(true);
  QAction* leftAlign =
    new QAction(QIcon(":/pqWidgets/Icons/pqTextAlignLeft.svg"), tr("Left"), actionGroup);
  leftAlign->setIconVisibleInMenu(true);
  QAction* centerAlign =
    new QAction(QIcon(":/pqWidgets/Icons/pqTextAlignCenter.svg"), tr("Center"), actionGroup);
  centerAlign->setIconVisibleInMenu(true);
  QAction* rightAlign =
    new QAction(QIcon(":/pqWidgets/Icons/pqTextAlignRight.svg"), tr("Right"), actionGroup);
  rightAlign->setIconVisibleInMenu(true);
  QMenu* popup = new QMenu(this);
  popup->addAction(leftAlign);
  popup->addAction(centerAlign);
  popup->addAction(rightAlign);
  ui.HorizontalJustification->setMenu(popup);
  QObject::connect(actionGroup, SIGNAL(triggered(QAction*)), this,
    SLOT(changeHorizontalJustificationIcon(QAction*)));
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::setupVerticalJustificationButton()
{
  Ui::FontPropertyWidget& ui = this->Internals->Ui;
  QActionGroup* actionGroup = new QActionGroup(this);
  actionGroup->setExclusive(true);
  QAction* topAlign =
    new QAction(QIcon(":/pqWidgets/Icons/pqTextVerticalAlignTop.svg"), tr("Top"), actionGroup);
  topAlign->setIconVisibleInMenu(true);
  QAction* centerAlign = new QAction(
    QIcon(":/pqWidgets/Icons/pqTextVerticalAlignCenter.svg"), tr("Center"), actionGroup);
  centerAlign->setIconVisibleInMenu(true);
  QAction* bottomAlign = new QAction(
    QIcon(":/pqWidgets/Icons/pqTextVerticalAlignBottom.svg"), tr("Bottom"), actionGroup);
  bottomAlign->setIconVisibleInMenu(true);
  QMenu* popup = new QMenu(this);
  popup->addAction(topAlign);
  popup->addAction(centerAlign);
  popup->addAction(bottomAlign);
  ui.VerticalJustification->setMenu(popup);
  QObject::connect(actionGroup, SIGNAL(triggered(QAction*)), this,
    SLOT(changeVerticalJustificationIcon(QAction*)));
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::changeHorizontalJustificationIcon(QAction* action)
{
  QString str = action->text();
  this->setHorizontalJustification(str);
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::changeVerticalJustificationIcon(QAction* action)
{
  QString str = action->text();
  this->setVerticalJustification(str);
}

//-----------------------------------------------------------------------------
void pqFontPropertyWidget::onFontFamilyChanged()
{
  Ui::FontPropertyWidget& ui = this->Internals->Ui;
  ui.FontFile->setVisible(ui.FontFamily->currentIndex() == 3);
}
