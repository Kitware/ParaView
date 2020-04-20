/*=========================================================================

   Program:   ParaView
   Module:    pqKeyFrameTypeWidget.cxx

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
#include "pqKeyFrameTypeWidget.h"
#include "ui_pqKeyFrameTypeWidget.h"

class pqKeyFrameTypeWidget::pqInternal : public Ui::pqKeyFrameTypeWidget
{
};

//-----------------------------------------------------------------------------
pqKeyFrameTypeWidget::pqKeyFrameTypeWidget(QWidget* p)
  : QWidget(p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);

  this->Internal->exponentialGroup->hide();
  this->Internal->sinusoidGroup->hide();

  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Internal->Base->setValidator(validator);
  this->Internal->StartPower->setValidator(validator);
  this->Internal->EndPower->setValidator(validator);
  this->Internal->Offset->setValidator(validator);
  this->Internal->Frequency->setValidator(validator);

  this->Internal->Type->addItem(QIcon(":pqWidgets/Icons/pqRamp24.png"), "Ramp", "Ramp");
  this->Internal->Type->addItem(
    QIcon(":pqWidgets/Icons/pqExponential24.png"), "Exponential", "Exponential");
  this->Internal->Type->addItem(
    QIcon(":pqWidgets/Icons/pqSinusoidal24.png"), "Sinusoid", "Sinusoid");
  this->Internal->Type->addItem(QIcon(":pqWidgets/Icons/pqStep24.png"), "Step", "Boolean");

  QObject::connect(
    this->Internal->Type, SIGNAL(currentIndexChanged(int)), this, SLOT(onTypeChanged()));

  QObject::connect(this->Internal->Base, SIGNAL(textChanged(const QString&)), this,
    SIGNAL(baseChanged(const QString&)));
  QObject::connect(this->Internal->StartPower, SIGNAL(textChanged(const QString&)), this,
    SIGNAL(startPowerChanged(const QString&)));
  QObject::connect(this->Internal->EndPower, SIGNAL(textChanged(const QString&)), this,
    SIGNAL(endPowerChanged(const QString&)));

  QObject::connect(this->Internal->Offset, SIGNAL(textChanged(const QString&)), this,
    SIGNAL(offsetChanged(const QString&)));
  QObject::connect(
    this->Internal->Phase, SIGNAL(valueChanged(double)), this, SIGNAL(phaseChanged(double)));
  QObject::connect(this->Internal->Frequency, SIGNAL(textChanged(const QString&)), this,
    SIGNAL(frequencyChanged(const QString&)));
}

//-----------------------------------------------------------------------------
pqKeyFrameTypeWidget::~pqKeyFrameTypeWidget()
{
  delete this->Internal;
}

void pqKeyFrameTypeWidget::setType(const QString& text)
{
  this->Internal->Type->setCurrentIndex(this->Internal->Type->findData(text));
}

void pqKeyFrameTypeWidget::setBase(const QString& text)
{
  this->Internal->Base->setText(text);
}

void pqKeyFrameTypeWidget::setStartPower(const QString& text)
{
  this->Internal->StartPower->setText(text);
}

void pqKeyFrameTypeWidget::setEndPower(const QString& text)
{
  this->Internal->EndPower->setText(text);
}

void pqKeyFrameTypeWidget::setPhase(double value)
{
  this->Internal->Phase->setValue(value);
}

void pqKeyFrameTypeWidget::setOffset(const QString& text)
{
  this->Internal->Offset->setText(text);
}

void pqKeyFrameTypeWidget::setFrequency(const QString& text)
{
  this->Internal->Frequency->setText(text);
}

QString pqKeyFrameTypeWidget::type() const
{
  int idx = this->Internal->Type->currentIndex();
  QAbstractItemModel* comboModel = this->Internal->Type->model();
  return comboModel->data(comboModel->index(idx, 0), Qt::UserRole).toString();
}

QComboBox* pqKeyFrameTypeWidget::typeComboBox() const
{
  return this->Internal->Type;
}

QString pqKeyFrameTypeWidget::base() const
{
  return this->Internal->Base->text();
}

QString pqKeyFrameTypeWidget::startPower() const
{
  return this->Internal->StartPower->text();
}

QString pqKeyFrameTypeWidget::endPower() const
{
  return this->Internal->EndPower->text();
}

double pqKeyFrameTypeWidget::phase() const
{
  return this->Internal->Phase->value();
}

QString pqKeyFrameTypeWidget::offset() const
{
  return this->Internal->Offset->text();
}

QString pqKeyFrameTypeWidget::frequency() const
{
  return this->Internal->Frequency->text();
}

//-----------------------------------------------------------------------------
void pqKeyFrameTypeWidget::onTypeChanged()
{
  QString text = this->type();

  // Hide all
  this->Internal->exponentialGroup->hide();
  this->Internal->sinusoidGroup->hide();

  if (text == "Exponential")
  {
    this->Internal->exponentialGroup->show();
  }
  else if (text == "Sinusoid")
  {
    this->Internal->sinusoidGroup->show();
  }

  Q_EMIT this->typeChanged(text);
}
