/*=========================================================================

   Program: ParaView
   Module:    pqDoubleRangeDialog.cxx

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

#include "pqDoubleRangeDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "pqDoubleRangeWidget.h"

pqDoubleRangeDialog::pqDoubleRangeDialog(
  const QString& label, double minimum, double maximum, QWidget* parent_)
  : QDialog(parent_)
{
  this->Widget = new pqDoubleRangeWidget(this);
  this->Widget->setMinimum(minimum);
  this->Widget->setMaximum(maximum);

  QDialogButtonBox* buttonBox =
    new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QHBoxLayout* widgetLayout = new QHBoxLayout;
  widgetLayout->addWidget(new QLabel(label, this));
  widgetLayout->addWidget(this->Widget);

  QVBoxLayout* layout_ = new QVBoxLayout;
  layout_->addLayout(widgetLayout);
  layout_->addWidget(buttonBox);
  this->setLayout(layout_);
}

pqDoubleRangeDialog::~pqDoubleRangeDialog() = default;

void pqDoubleRangeDialog::setValue(double value_)
{
  this->Widget->setValue(value_);
}

double pqDoubleRangeDialog::value() const
{
  return this->Widget->value();
}
