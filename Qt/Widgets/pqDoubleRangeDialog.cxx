// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
