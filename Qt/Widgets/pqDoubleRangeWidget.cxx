// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqDoubleRangeWidget.h"

// Qt includes
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QSlider>

pqDoubleRangeWidget::pqDoubleRangeWidget(QWidget* p)
  : Superclass(p)
{
  this->Minimum = 0;
  this->Maximum = 1;
  this->Resolution = 100;
}

//-----------------------------------------------------------------------------
pqDoubleRangeWidget::~pqDoubleRangeWidget() = default;

//-----------------------------------------------------------------------------
int pqDoubleRangeWidget::resolution() const
{
  return this->Resolution;
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::setResolution(int val)
{
  this->Resolution = val;
  this->setSliderRange(0, this->Resolution);
}

//-----------------------------------------------------------------------------
int pqDoubleRangeWidget::valueToSliderPos(double val)
{
  double range = this->Maximum - this->Minimum;
  double fraction = range != 0 ? (val - this->Minimum) / range : 0;
  int sliderVal = qRound(fraction * this->Resolution);
  return sliderVal;
}

//-----------------------------------------------------------------------------
double pqDoubleRangeWidget::sliderPosToValue(int pos)
{
  double fraction = this->Resolution > 0 ? pos / static_cast<double>(this->Resolution) : 0;
  double range = this->Maximum - this->Minimum;
  double v = (fraction * range) + this->Minimum;
  return v;
}

//-----------------------------------------------------------------------------
double pqDoubleRangeWidget::maximum() const
{
  return this->Maximum;
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::setMaximum(double val)
{
  this->Maximum = val;
  this->updateValidator();
  this->updateSlider();
}

//-----------------------------------------------------------------------------
double pqDoubleRangeWidget::minimum() const
{
  return this->Minimum;
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::setMinimum(double val)
{
  this->Minimum = val;
  this->updateValidator();
  this->updateSlider();
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::updateValidator()
{
  this->setValidator(new QDoubleValidator(this));
}
