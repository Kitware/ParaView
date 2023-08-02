// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqRescaleScalarRangeToCustomDialog.h"
#include "ui_pqRescaleScalarRangeToCustomDialog.h"

#include "pqCoreUtilities.h"

#include <algorithm> // for std::swap

class pqRescaleScalarRangeToCustomDialogForm : public Ui::RescaleScalarRangeToCustomDialog
{
};

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToCustomDialog::pqRescaleScalarRangeToCustomDialog(QWidget* widgetParent)
  : QDialog(widgetParent)
  , Form(new pqRescaleScalarRangeToCustomDialogForm())
{
  // Set up the ui.
  this->Form->setupUi(this);

  // Connect the gui elements.
  this->connect(
    this->Form->MinimumScalar, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));
  this->connect(
    this->Form->MaximumScalar, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));

  this->connect(this->Form->ApplyButton, SIGNAL(clicked()), SIGNAL(apply()));
  this->connect(this->Form->RescaleButton, SIGNAL(clicked()), SLOT(rescale()));
  this->connect(this->Form->CancelButton, SIGNAL(clicked()), SLOT(reject()));
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToCustomDialog::~pqRescaleScalarRangeToCustomDialog() = default;

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeToCustomDialog::setRange(double min, double max)
{
  if (min > max)
  {
    std::swap(min, max);
  }

  // Update the displayed range.
  this->Form->MinimumScalar->setText(pqCoreUtilities::formatFullNumber(min));
  this->Form->MaximumScalar->setText(pqCoreUtilities::formatFullNumber(max));
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeToCustomDialog::setOpacityRange(double min, double max)
{
  if (min > max)
  {
    std::swap(min, max);
  }

  // Update the displayed opacity range.
  this->Form->MinimumOpacityScalar->setText(pqCoreUtilities::formatFullNumber(min));
  this->Form->MaximumOpacityScalar->setText(pqCoreUtilities::formatFullNumber(max));
}

//-----------------------------------------------------------------------------
double pqRescaleScalarRangeToCustomDialog::minimum() const
{
  return this->Form->MinimumScalar->text().toDouble();
}

//-----------------------------------------------------------------------------
double pqRescaleScalarRangeToCustomDialog::maximum() const
{
  return this->Form->MaximumScalar->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeToCustomDialog::showOpacityControls(bool show)
{
  this->Form->OpacityLabel->setVisible(show);
  this->Form->MinimumOpacityScalar->setVisible(show);
  this->Form->OpacityHyphenLabel->setVisible(show);
  this->Form->MaximumOpacityScalar->setVisible(show);

  // Force the dialog to resize after changing widget visibilities
  this->resize(0, 0);
}

//-----------------------------------------------------------------------------
double pqRescaleScalarRangeToCustomDialog::opacityMinimum() const
{
  return this->Form->MinimumOpacityScalar->text().toDouble();
}

//-----------------------------------------------------------------------------
double pqRescaleScalarRangeToCustomDialog::opacityMaximum() const
{
  return this->Form->MaximumOpacityScalar->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeToCustomDialog::validate()
{
  QString tmp1 = this->Form->MinimumScalar->text();
  QString tmp2 = this->Form->MaximumScalar->text();
  if (tmp1.toDouble() <= tmp2.toDouble())
  {
    this->Form->RescaleButton->setEnabled(true);
    this->Form->ApplyButton->setEnabled(true);
  }
  else
  {
    this->Form->RescaleButton->setEnabled(false);
    this->Form->ApplyButton->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeToCustomDialog::setLock(bool lock)
{
  this->Form->AutomaticRescaling->setChecked(!lock);
}

//-----------------------------------------------------------------------------
bool pqRescaleScalarRangeToCustomDialog::doLock() const
{
  return !this->Form->AutomaticRescaling->isChecked();
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeToCustomDialog::rescale()
{
  Q_EMIT(this->apply());
  this->accept();
}
