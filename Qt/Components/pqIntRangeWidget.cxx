// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqIntRangeWidget.h"

// Qt includes
#include "pqLineEdit.h"
#include <QHBoxLayout>
#include <QIntValidator>
#include <QSlider>

#include "vtkEventQtSlotConnect.h"
#include "vtkSMIntRangeDomain.h"

pqIntRangeWidget::pqIntRangeWidget(QWidget* p)
  : QWidget(p)
{
  this->BlockUpdate = false;
  this->Value = 0;
  this->Minimum = 0;
  this->Maximum = 1;
  this->Domain = nullptr;
  this->DomainConnection = nullptr;
  this->InteractingWithSlider = false;
  this->DeferredValueEdited = false;

  QHBoxLayout* l = new QHBoxLayout(this);
  l->setContentsMargins(0, 0, 0, 0);
  this->Slider = new QSlider(Qt::Horizontal, this);
  this->Slider->setRange(0, 1);
  l->addWidget(this->Slider);
  this->Slider->setObjectName("Slider");
  this->Slider->setFocusPolicy(Qt::StrongFocus); // change from the default Qt::WheelFocus
  this->LineEdit = new pqLineEdit(this);
  l->addWidget(this->LineEdit);
  this->LineEdit->setObjectName("LineEdit");
  this->LineEdit->setValidator(new QIntValidator(this->LineEdit));
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
pqIntRangeWidget::~pqIntRangeWidget()
{
  if (this->DomainConnection)
  {
    this->DomainConnection->Delete();
  }
}

//-----------------------------------------------------------------------------
int pqIntRangeWidget::value() const
{
  return this->Value;
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::setValue(int val)
{
  if (this->Value == val)
  {
    return;
  }

  if (!this->BlockUpdate)
  {
    // set the slider
    this->Slider->blockSignals(true);
    this->Slider->setValue(val);
    this->Slider->blockSignals(false);

    // set the text
    this->LineEdit->blockSignals(true);
    this->LineEdit->setTextAndResetCursor(QString().setNum(val));
    this->LineEdit->blockSignals(false);
  }

  this->Value = val;
  Q_EMIT this->valueChanged(this->Value);
}

//-----------------------------------------------------------------------------
int pqIntRangeWidget::maximum() const
{
  return this->Maximum;
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::setMaximum(int val)
{
  this->Maximum = val;
  this->Slider->setMaximum(val);
  this->updateValidator();
}

//-----------------------------------------------------------------------------
int pqIntRangeWidget::minimum() const
{
  return this->Minimum;
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::setMinimum(int val)
{
  this->Minimum = val;
  this->Slider->setMinimum(val);
  this->updateValidator();
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::updateValidator()
{
  this->LineEdit->setValidator(new QIntValidator(this->LineEdit));
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::setDomain(vtkSMIntRangeDomain* domain)
{
  if (this->Domain == domain)
  {
    return;
  }

  this->Domain = domain;

  if (this->Domain)
  {
    if (this->DomainConnection)
    {
      this->DomainConnection->Delete();
      this->DomainConnection = nullptr;
    }

    this->DomainConnection = vtkEventQtSlotConnect::New();
    this->DomainConnection->Connect(
      this->Domain, vtkCommand::DomainModifiedEvent, this, SLOT(domainChanged()));
    this->setMinimum(this->Domain->GetMinimum(0));
    this->setMaximum(this->Domain->GetMaximum(0));
  }
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::sliderChanged(int val)
{
  if (!this->BlockUpdate)
  {
    this->BlockUpdate = true;
    this->LineEdit->setTextAndResetCursor(QString().setNum(val));
    this->setValue(val);
    this->emitValueEdited();
    this->BlockUpdate = false;
  }
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::textChanged(const QString& text)
{
  if (!this->BlockUpdate)
  {
    int val = text.toInt();
    this->BlockUpdate = true;
    this->Slider->setValue(val);
    this->setValue(val);
    this->BlockUpdate = false;
  }
}
//-----------------------------------------------------------------------------
void pqIntRangeWidget::editingFinished()
{
  this->emitValueEdited();
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::emitValueEdited()
{
  if (this->InteractingWithSlider == false)
  {
    Q_EMIT this->valueEdited(this->Value);
  }
  else
  {
    this->DeferredValueEdited = true;
  }
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::emitIfDeferredValueEdited()
{
  if (this->DeferredValueEdited)
  {
    this->DeferredValueEdited = false;
    this->emitValueEdited();
  }
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::domainChanged()
{
  if (this->Domain)
  {
    this->setMinimum(this->Domain->GetMinimum(0));
    this->setMaximum(this->Domain->GetMaximum(0));
  }
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::sliderPressed()
{
  this->InteractingWithSlider = true;
}

//-----------------------------------------------------------------------------
void pqIntRangeWidget::sliderReleased()
{
  this->InteractingWithSlider = false;
  this->emitIfDeferredValueEdited();
}
