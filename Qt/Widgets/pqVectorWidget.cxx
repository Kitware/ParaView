// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVectorWidget.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>

//-----------------------------------------------------------------------------
pqVectorWidget::pqVectorWidget(const QVariant& value, QWidget* parent)
  : QWidget(parent)
{
  this->Vector = value;
}

//-----------------------------------------------------------------------------
void pqVectorWidget::CreateUI(unsigned int nbElem)
{
  this->setAutoFillBackground(true);

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(1, 1, 1, 1);
  layout->setSpacing(1);
  this->setLayout(layout);

  for (unsigned int s = 0; s < nbElem; ++s)
  {
    QDoubleSpinBox* valEdit = new QDoubleSpinBox(this);
    valEdit->setValue(this->getValue(s));
    layout->addWidget(valEdit);

    QObject::connect(valEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
      [=](double val) { this->setValue(s, val); });
  }
}
