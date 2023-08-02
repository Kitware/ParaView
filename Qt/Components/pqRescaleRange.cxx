// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/// \file pqRescaleRange.cxx
/// \date 3/28/2007

#include "pqRescaleRange.h"
#include "ui_pqRescaleRangeDialog.h"

#include "pqCoreUtilities.h"

#include <algorithm> // for std::swap

class pqRescaleRangeForm : public Ui::pqRescaleRangeDialog
{
};

pqRescaleRange::pqRescaleRange(QWidget* widgetParent)
  : QDialog(widgetParent)
  , Lock(false)
{
  this->Form = new pqRescaleRangeForm();

  // Set up the ui.
  this->Form->setupUi(this);

  // Connect the gui elements.
  this->connect(
    this->Form->MinimumScalar, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));
  this->connect(
    this->Form->MaximumScalar, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));

  this->connect(this->Form->RescaleOnlyButton, SIGNAL(clicked()), SLOT(accept()));
  this->connect(this->Form->RescaleButton, SIGNAL(clicked()), SLOT(rescaleAndLock()));
  this->connect(this->Form->CancelButton, SIGNAL(clicked()), SLOT(reject()));
}

pqRescaleRange::~pqRescaleRange()
{
  delete this->Form;
}

void pqRescaleRange::setRange(double min, double max)
{
  if (min > max)
  {
    std::swap(min, max);
  }

  // Update the displayed range.
  this->Form->MinimumScalar->setText(pqCoreUtilities::formatFullNumber(min));
  this->Form->MaximumScalar->setText(pqCoreUtilities::formatFullNumber(max));
}

void pqRescaleRange::setOpacityRange(double min, double max)
{
  if (min > max)
  {
    std::swap(min, max);
  }

  // Update the displayed opacity range.
  this->Form->MinimumOpacityScalar->setText(pqCoreUtilities::formatFullNumber(min));
  this->Form->MaximumOpacityScalar->setText(pqCoreUtilities::formatFullNumber(max));
}

double pqRescaleRange::minimum() const
{
  return this->Form->MinimumScalar->text().toDouble();
}

double pqRescaleRange::maximum() const
{
  return this->Form->MaximumScalar->text().toDouble();
}

void pqRescaleRange::showOpacityControls(bool show)
{
  if (!show)
  {
    this->Form->OpacityLabel->setVisible(show);
    this->Form->MinimumOpacityScalar->setVisible(show);
    this->Form->OpacityHyphenLabel->setVisible(show);
    this->Form->MaximumOpacityScalar->setVisible(show);

    // Force the dialog to resize after changing widget visibilities
    this->resize(0, 0);
  }
}

double pqRescaleRange::opacityMinimum() const
{
  return this->Form->MinimumOpacityScalar->text().toDouble();
}

double pqRescaleRange::opacityMaximum() const
{
  return this->Form->MaximumOpacityScalar->text().toDouble();
}

void pqRescaleRange::validate()
{
  QString tmp1 = this->Form->MinimumScalar->text();
  QString tmp2 = this->Form->MaximumScalar->text();
  if (tmp1.toDouble() <= tmp2.toDouble())
  {
    this->Form->RescaleButton->setEnabled(true);
    this->Form->RescaleOnlyButton->setEnabled(true);
  }
  else
  {
    this->Form->RescaleButton->setEnabled(false);
    this->Form->RescaleOnlyButton->setEnabled(false);
  }
}

void pqRescaleRange::rescaleAndLock()
{
  this->Lock = true;
  this->accept();
}
