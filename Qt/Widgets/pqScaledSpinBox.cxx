// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqScaledSpinBox.h"

#include <QKeyEvent>

#include <cmath>

// constructor
pqScaledSpinBox::pqScaledSpinBox(QWidget* parent)
  : QDoubleSpinBox(parent)
{
  this->initialize();
}

// constructor
pqScaledSpinBox::pqScaledSpinBox(QDoubleSpinBox* other)
  : QDoubleSpinBox(other->parentWidget())
{
  this->initialize();

  this->setValue(other->value());
  this->setSingleStep(other->singleStep());
  this->setSuffix(other->suffix());
  this->setMinimum(other->minimum());
  this->setMaximum(other->maximum());
}

void pqScaledSpinBox::initialize()
{
  this->LastValue = 1.0;
  this->ScaleFactor = 2.0;
  this->setValue(this->LastValue);
  // need a small step that isn't visible, but still works for 'LastValue' value tests
  this->setSingleStep(0.00001);

  // itrack valueChanged signal to apply scaleFactor multiplier
  QDoubleSpinBox* pThisBaseClass = qobject_cast<QDoubleSpinBox*>(this);
  QObject::connect(
    pThisBaseClass, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
}

// destructor
pqScaledSpinBox::~pqScaledSpinBox()
{
  ;
}

void pqScaledSpinBox::setScalingFactor(double scaleFactor)
{
  this->ScaleFactor = scaleFactor;
}

void pqScaledSpinBox::onValueChanged(double newValue)
{
  // don't apply the scaled step if this is a manually entered value (via keyboard)
  if (fabs(newValue - this->LastValue) > 1.01 * this->singleStep())
  {
    this->LastValue = this->value();
    return;
  }

  if (newValue > this->LastValue)
  {
    this->scaledStepUp();
  }
  else if (newValue < this->LastValue)
  {
    this->scaledStepDown();
  }
  this->LastValue = this->value();
}

void pqScaledSpinBox::setValue(double val)
{
  this->LastValue = val;
  QDoubleSpinBox::setValue(val);
}

void pqScaledSpinBox::scaledStepUp()
{
  this->blockSignals(true);
  this->setValue(this->value() * this->ScaleFactor);
  this->blockSignals(false);
}

void pqScaledSpinBox::scaledStepDown()
{
  this->blockSignals(true);
  this->setValue(this->value() / this->ScaleFactor);
  this->blockSignals(false);
}

void pqScaledSpinBox::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Up)
  {
    this->scaledStepUp();
    event->accept();
    return;
  }
  else if (event->key() == Qt::Key_Down)
  {
    this->scaledStepDown();
    event->accept();
    return;
  }

  // pass all other inputs to the base class
  QAbstractSpinBox::keyPressEvent(event);
}
