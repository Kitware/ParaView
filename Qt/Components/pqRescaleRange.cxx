/*=========================================================================

   Program: ParaView
   Module:    pqRescaleRange.cxx

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

/// \file pqRescaleRange.cxx
/// \date 3/28/2007

#include "pqRescaleRange.h"
#include "ui_pqRescaleRangeDialog.h"

#include "pqLineEditNumberValidator.h"
#include <QTimer>


class pqRescaleRangeForm : public Ui::pqRescaleRangeDialog {};


pqRescaleRange::pqRescaleRange(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Form = new pqRescaleRangeForm();
  this->EditDelay = new QTimer(this);
  this->Minimum = 0.0;
  this->Maximum = 0.0;
  this->MinChanged = false;
  this->MaxChanged = false;

  // Set up the ui.
  this->Form->setupUi(this);
  this->EditDelay->setSingleShot(true);

  // Make sure the line edits only allow number inputs.
  pqLineEditNumberValidator *validator =
      new pqLineEditNumberValidator(true, this);
  this->Form->MinimumScalar->installEventFilter(validator);
  this->Form->MaximumScalar->installEventFilter(validator);

  // Connect the gui elements.
  this->connect(this->Form->MinimumScalar, SIGNAL(textEdited(const QString &)),
      this, SLOT(handleMinimumEdited()));
  this->connect(this->Form->MaximumScalar, SIGNAL(textEdited(const QString &)),
      this, SLOT(handleMaximumEdited()));
  this->connect(this->EditDelay, SIGNAL(timeout()),
      this, SLOT(applyTextChanges()));
  this->connect(this->Form->RescaleButton, SIGNAL(clicked()),
      this, SLOT(accept()));
  this->connect(this->Form->CancelButton, SIGNAL(clicked()),
      this, SLOT(reject()));
}

pqRescaleRange::~pqRescaleRange()
{
  delete this->Form;
  delete this->EditDelay;
}

void pqRescaleRange::setRange(double min, double max)
{
  if(min > max)
    {
    this->Minimum = max;
    this->Maximum = min;
    }
  else
    {
    this->Minimum = min;
    this->Maximum = max;
    }

  // Update the displayed range.
  this->Form->MinimumScalar->setText(QString::number(this->Minimum, 'g', 6));
  this->Form->MaximumScalar->setText(QString::number(this->Maximum, 'g', 6));
}

void pqRescaleRange::hideEvent(QHideEvent *e)
{
  // If the edit delay timer is active, set the final user entry.
  if(this->EditDelay->isActive())
    {
    this->EditDelay->stop();
    this->applyTextChanges();
    }

  QDialog::hideEvent(e);
}

void pqRescaleRange::handleMinimumEdited()
{
  this->MinChanged = true;
  this->EditDelay->start(600);
}

void pqRescaleRange::handleMaximumEdited()
{
  this->MaxChanged = true;
  this->EditDelay->start(600);
}

void pqRescaleRange::applyTextChanges()
{
  if(this->MinChanged)
    {
    this->MinChanged = true;
    this->setMinimum();
    }

  if(this->MaxChanged)
    {
    this->MaxChanged = true;
    this->setMaximum();
    }
}

void pqRescaleRange::setMinimum()
{
  // Get the value from the line edit.
  double value = this->Form->MinimumScalar->text().toDouble();

  // Make sure the value is less than the maximum.
  if(value > this->Maximum)
    {
    this->Minimum = this->Maximum;
    this->Form->MinimumScalar->setText(this->Form->MaximumScalar->text());
    }
  else
    {
    this->Minimum = value;
    }
}

void pqRescaleRange::setMaximum()
{
  // Get the value from the line edit.
  double value = this->Form->MaximumScalar->text().toDouble();

  // Make sure the value is greater than the maximum.
  if(value < this->Minimum)
    {
    this->Maximum = this->Minimum;
    this->Form->MaximumScalar->setText(this->Form->MinimumScalar->text());
    }
  else
    {
    this->Maximum = value;
    }
}


