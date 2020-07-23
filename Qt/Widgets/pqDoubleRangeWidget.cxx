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
#if !defined(VTK_LEGACY_REMOVE)
  if (this->StrictRange)
  {
    this->setValidator(new QDoubleValidator(this->minimum(), this->maximum(), 100));
  }
  else
#endif
  {
    this->setValidator(new QDoubleValidator());
  }
}

#if !defined(VTK_LEGACY_REMOVE)
//-----------------------------------------------------------------------------
bool pqDoubleRangeWidget::strictRange() const
{
  const QDoubleValidator* dv = this->validator();
  return dv->bottom() == this->minimum() && dv->top() == this->maximum();
}
#endif

#if !defined(VTK_LEGACY_REMOVE)
//-----------------------------------------------------------------------------
void pqDoubleRangeWidget::setStrictRange(bool s)
{
  this->StrictRange = s;
  this->updateValidator();
}
#endif
