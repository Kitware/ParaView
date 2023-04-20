/*=========================================================================

   Program: ParaView
   Module:  pqRescaleScalarRangeToCustomDialog.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

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
