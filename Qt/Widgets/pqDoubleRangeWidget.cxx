/*=========================================================================

   Program:   ParaView
   Module:    pqDoubleRangeWidget.cxx

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

#include "pqDoubleRangeWidget.h"

#include "pqLineEdit.h"
#include "vtkPVConfig.h"

// Qt includes
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QSlider>

pqDoubleRangeWidget::pqDoubleRangeWidget(QWidget* p)
  : QWidget(p)
{
  this->BlockUpdate = false;
  this->Value = 0;
  this->Minimum = 0;
  this->Maximum = 1;
  this->Resolution = 100;
  this->StrictRange = false;
  this->InteractingWithSlider = false;
  this->DeferredValueEdited = false;

  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(0);
  this->Slider = new QSlider(Qt::Horizontal, this);
  this->Slider->setRange(0, this->Resolution);
  l->addWidget(this->Slider);
  this->Slider->setObjectName("Slider");
  this->LineEdit = new pqLineEdit(this);
  l->addWidget(this->LineEdit);
  this->LineEdit->setObjectName("LineEdit");
  this->LineEdit->setValidator(new QDoubleValidator(this->LineEdit));
  this->LineEdit->setTextAndResetCursor(QString().setNum(this->Value));

  QObject::connect(this->Slider, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
  QObject::connect(
    this->LineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
  QObject::connect(
    this->LineEdit, SIGNAL(textChangedAndEditingFinished()), this, SLOT(editingFinished()));

  // let's avoid firing `valueChanged` events until the user has released the
  // slider.
  this->connect(this->Slider, SIGNAL(sliderPressed()), SLOT(sliderPressed()));
  this->connect(this->Slider, SIGNAL(sliderReleased()), SLOT(sliderReleased()));
}

//-----------------------------------------------------------------------------
pqDoubleRangeWidget::~pqDoubleRangeWidget()
{
}

//-----------------------------------------------------------------------------
int pqDoubleRangeWidget::resolution() const
{
  return this->Resolution;
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::setResolution(int val)
{
  this->Resolution = val;
  this->Slider->setRange(0, this->Resolution);
  this->updateSlider();
}

//-----------------------------------------------------------------------------
double pqDoubleRangeWidget::value() const
{
  return this->Value;
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::setValue(double val)
{
  if (this->Value == val)
  {
    return;
  }

  this->Value = val;

  if (!this->BlockUpdate)
  {
    // set the slider
    this->updateSlider();

    // set the text
    this->BlockUpdate = true;
    this->LineEdit->setTextAndResetCursor(
      QString().setNum(val, 'g', DEFAULT_DOUBLE_PRECISION_VALUE));
    this->BlockUpdate = false;
  }

  emit this->valueChanged(this->Value);
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
  if (this->StrictRange)
  {
    this->LineEdit->setValidator(
      new QDoubleValidator(this->minimum(), this->maximum(), 100, this->LineEdit));
  }
  else
  {
    this->LineEdit->setValidator(new QDoubleValidator(this->LineEdit));
  }
}

//-----------------------------------------------------------------------------
bool pqDoubleRangeWidget::strictRange() const
{
  const QDoubleValidator* dv = qobject_cast<const QDoubleValidator*>(this->LineEdit->validator());
  return dv->bottom() == this->minimum() && dv->top() == this->maximum();
}

void pqDoubleRangeWidget::setStrictRange(bool s)
{
  this->StrictRange = s;
  this->updateValidator();
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::sliderChanged(int val)
{
  if (!this->BlockUpdate)
  {
    double fraction = val / static_cast<double>(this->Resolution);
    double range = this->Maximum - this->Minimum;
    double v = (fraction * range) + this->Minimum;
    this->BlockUpdate = true;
    this->LineEdit->setTextAndResetCursor(QString().setNum(v));
    this->setValue(v);
    this->emitValueEdited();
    this->BlockUpdate = false;
  }
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::textChanged(const QString& text)
{
  if (!this->BlockUpdate)
  {
    double val = text.toDouble();
    this->BlockUpdate = true;
    double range = this->Maximum - this->Minimum;
    double fraction = (val - this->Minimum) / range;
    int sliderVal = qRound(fraction * static_cast<double>(this->Resolution));
    this->Slider->setValue(sliderVal);
    this->setValue(val);
    this->BlockUpdate = false;
  }
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::editingFinished()
{
  this->emitValueEdited();
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::emitValueEdited()
{
  if (this->InteractingWithSlider == false)
  {
    emit this->valueEdited(this->Value);
  }
  else
  {
    this->DeferredValueEdited = true;
  }
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::emitIfDeferredValueEdited()
{
  if (this->DeferredValueEdited)
  {
    this->DeferredValueEdited = false;
    this->emitValueEdited();
  }
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::updateSlider()
{
  this->Slider->blockSignals(true);
  double range = this->Maximum - this->Minimum;
  double fraction = (this->Value - this->Minimum) / range;
  int v = qRound(fraction * static_cast<double>(this->Resolution));
  this->Slider->setValue(v);
  this->Slider->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::sliderPressed()
{
  this->InteractingWithSlider = true;
}

//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::sliderReleased()
{
  this->InteractingWithSlider = false;
  this->emitIfDeferredValueEdited();
}
