/*=========================================================================

   Program: ParaView
   Module:    pqCameraWidget.cxx

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
#include "pqCameraWidget.h"
#include "ui_pqCameraWidget.h"

//-----------------------------------------------------------------------------
class pqCameraWidget::pqInternal: public Ui::pqCameraWidget { };

//-----------------------------------------------------------------------------
pqCameraWidget::pqCameraWidget(QWidget* _p): QWidget(_p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);

  this->connect(this->Internal->position0,
                SIGNAL(valueChanged(double)),
                SIGNAL(positionChanged()));
  this->connect(this->Internal->position1,
                SIGNAL(valueChanged(double)),
                SIGNAL(positionChanged()));
  this->connect(this->Internal->position2,
                SIGNAL(valueChanged(double)),
                SIGNAL(positionChanged()));
  
  this->connect(this->Internal->focalPoint0,
                SIGNAL(valueChanged(double)),
                SIGNAL(focalPointChanged()));
  this->connect(this->Internal->focalPoint1,
                SIGNAL(valueChanged(double)),
                SIGNAL(focalPointChanged()));
  this->connect(this->Internal->focalPoint2,
                SIGNAL(valueChanged(double)),
                SIGNAL(focalPointChanged()));
  
  this->connect(this->Internal->viewUp0,
                SIGNAL(valueChanged(double)),
                SIGNAL(viewUpChanged()));
  this->connect(this->Internal->viewUp1,
                SIGNAL(valueChanged(double)),
                SIGNAL(viewUpChanged()));
  this->connect(this->Internal->viewUp2,
                SIGNAL(valueChanged(double)),
                SIGNAL(viewUpChanged()));
  
  this->connect(this->Internal->viewAngle,
                SIGNAL(valueChanged(double)),
                SIGNAL(viewAngleChanged()));
}

//-----------------------------------------------------------------------------
pqCameraWidget::~pqCameraWidget()
{
  delete this->Internal;
}

void pqCameraWidget::setPosition(QList<QVariant> v)
{
  if(v.size() == 3 && this->position() != v)
    {
    this->blockSignals(true);
    this->Internal->position0->setValue(v[0].toDouble());
    this->Internal->position1->setValue(v[1].toDouble());
    this->Internal->position2->setValue(v[2].toDouble());
    this->blockSignals(false);
    emit this->positionChanged();
    }
}

void pqCameraWidget::setFocalPoint(QList<QVariant> v)
{
  if(v.size() == 3 && this->focalPoint() != v)
    {
    this->blockSignals(true);
    this->Internal->focalPoint0->setValue(v[0].toDouble());
    this->Internal->focalPoint1->setValue(v[1].toDouble());
    this->Internal->focalPoint2->setValue(v[2].toDouble());
    this->blockSignals(false);
    emit this->focalPointChanged();
    }
}

void pqCameraWidget::setViewUp(QList<QVariant> v)
{
  if(v.size() == 3 && this->viewUp() != v)
    {
    this->blockSignals(true);
    this->Internal->viewUp0->setValue(v[0].toDouble());
    this->Internal->viewUp1->setValue(v[1].toDouble());
    this->Internal->viewUp2->setValue(v[2].toDouble());
    this->blockSignals(false);
    emit this->viewUpChanged();
    }
}

void pqCameraWidget::setViewAngle(QVariant v)
{
  if(this->viewAngle() != v)
    {
    this->Internal->viewAngle->setValue(v.toDouble());
    }
}

QList<QVariant> pqCameraWidget::position() const
{
  QList<QVariant> ret;
  ret.append(this->Internal->position0->value());
  ret.append(this->Internal->position1->value());
  ret.append(this->Internal->position2->value());
  return ret;
}

QList<QVariant> pqCameraWidget::focalPoint() const
{
  QList<QVariant> ret;
  ret.append(this->Internal->focalPoint0->value());
  ret.append(this->Internal->focalPoint1->value());
  ret.append(this->Internal->focalPoint2->value());
  return ret;
}

QList<QVariant> pqCameraWidget::viewUp() const
{
  QList<QVariant> ret;
  ret.append(this->Internal->viewUp0->value());
  ret.append(this->Internal->viewUp1->value());
  ret.append(this->Internal->viewUp2->value());
  return ret;
}

QVariant pqCameraWidget::viewAngle() const
{
  return this->Internal->viewAngle->value();
}

