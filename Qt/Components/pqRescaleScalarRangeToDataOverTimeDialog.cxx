// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqRescaleScalarRangeToDataOverTimeDialog.h"
#include "ui_pqRescaleScalarRangeToDataOverTimeDialog.h"

class pqRescaleScalarRangeToDataOverTimeDialogForm
  : public Ui::RescaleScalarRangeToDataOverTimeDialog
{
};

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToDataOverTimeDialog::pqRescaleScalarRangeToDataOverTimeDialog(
  QWidget* widgetParent)
  : QDialog(widgetParent)
  , Form(new pqRescaleScalarRangeToDataOverTimeDialogForm())
{
  // Set up the ui.
  this->Form->setupUi(this);

  this->connect(this->Form->ApplyButton, SIGNAL(clicked()), SIGNAL(apply()));
  this->connect(this->Form->RescaleButton, SIGNAL(clicked()), SLOT(rescale()));
  this->connect(this->Form->CancelButton, SIGNAL(clicked()), SLOT(reject()));
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToDataOverTimeDialog::~pqRescaleScalarRangeToDataOverTimeDialog() = default;

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeToDataOverTimeDialog::setLock(bool lock)
{
  this->Form->AutomaticRescaling->setChecked(!lock);
}

//-----------------------------------------------------------------------------
bool pqRescaleScalarRangeToDataOverTimeDialog::doLock() const
{
  return !this->Form->AutomaticRescaling->isChecked();
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeToDataOverTimeDialog::rescale()
{
  Q_EMIT(this->apply());
  this->accept();
}
