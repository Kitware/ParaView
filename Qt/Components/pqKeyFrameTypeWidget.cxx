// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqKeyFrameTypeWidget.h"
#include "ui_pqKeyFrameTypeWidget.h"

#include "vtk_jsoncpp.h"

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

  this->Internal->Type->addItem(QIcon(":pqWidgets/Icons/pqRamp24.png"), "Ramp", tr("Ramp"));
  this->Internal->Type->addItem(
    QIcon(":pqWidgets/Icons/pqExponential24.png"), "Exponential", tr("Exponential"));
  this->Internal->Type->addItem(
    QIcon(":pqWidgets/Icons/pqSinusoidal24.png"), "Sinusoid", tr("Sinusoid"));
  this->Internal->Type->addItem(QIcon(":pqWidgets/Icons/pqStep24.png"), "Step", tr("Boolean"));

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

//-----------------------------------------------------------------------------
void pqKeyFrameTypeWidget::initializeUsingJSON(const Json::Value& json)
{
  if (json["type"].isString())
  {
    this->setType(json["type"].asCString());
  }

  QString interpolationType = this->type();
  if (interpolationType == "Exponential")
  {
    if (json["base"].isInt())
    {
      this->setBase(QString::number(json["base"].asInt()));
    }
    if (json["startPower"].isInt())
    {
      this->setStartPower(QString::number(json["startPower"].asInt()));
    }
    if (json["endPower"].isInt())
    {
      this->setEndPower(QString::number(json["endPower"].asInt()));
    }
  }
  else if (interpolationType == "Sinusoid")
  {
    if (json["phase"].isDouble())
    {
      this->setPhase(json["phase"].asDouble());
    }
    if (json["frequency"].isInt())
    {
      this->setFrequency(QString::number(json["frequency"].asInt()));
    }
    if (json["offset"].isInt())
    {
      this->setOffset(QString::number(json["offset"].asInt()));
    }
  }
}

//-----------------------------------------------------------------------------
Json::Value pqKeyFrameTypeWidget::serializeToJSON() const
{
  Json::Value keyFrame;

  QString interpolationType = this->type();
  keyFrame["type"] = interpolationType.toStdString();
  if (interpolationType == "Exponential")
  {
    keyFrame["base"] = this->base().toInt();
    keyFrame["startPower"] = this->startPower().toInt();
    keyFrame["endPower"] = this->endPower().toInt();
  }
  else if (interpolationType == "Sinusoid")
  {
    keyFrame["phase"] = this->phase();
    keyFrame["frequency"] = this->frequency().toInt();
    keyFrame["offset"] = this->offset().toInt();
  }
  return keyFrame;
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
