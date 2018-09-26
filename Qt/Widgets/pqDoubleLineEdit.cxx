/*=========================================================================

   Program: ParaView
   Module:  pqDoubleLineEdit.cxx

   Copyright (c) 2005-2018 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqDoubleLineEdit.h"

// Qt Includes.
#include <QFocusEvent>
#include <QPointer>
#include <QTextStream>

namespace
{
//-----------------------------------------------------------------------------
QTextStream::RealNumberNotation toTextStreamNotation(pqDoubleLineEdit::RealNumberNotation notation)
{
  if (notation == pqDoubleLineEdit::FixedNotation)
  {
    return QTextStream::FixedNotation;
  }
  else if (notation == pqDoubleLineEdit::ScientificNotation)
  {
    return QTextStream::ScientificNotation;
  }
  else
  {
    return QTextStream::SmartNotation;
  }
}

//-----------------------------------------------------------------------------
QDoubleValidator::Notation toValidatorNotation(pqDoubleLineEdit::RealNumberNotation notation)
{
  if (notation != pqDoubleLineEdit::RealNumberNotation::FixedNotation)
  {
    return QDoubleValidator::ScientificNotation;
  }
  else
  {
    return QDoubleValidator::StandardNotation;
  }
}

using InstanceTrackerType = QList<QPointer<pqDoubleLineEdit> >;
static InstanceTrackerType* InstanceTracker = nullptr;
}

int pqDoubleLineEdit::GlobalPrecision = 2;
pqDoubleLineEdit::RealNumberNotation pqDoubleLineEdit::GlobalNotation =
  pqDoubleLineEdit::FixedNotation;

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setGlobalPrecisionAndNotation(int precision, RealNumberNotation notation)
{
  bool modified = false;
  if (precision != pqDoubleLineEdit::GlobalPrecision)
  {
    pqDoubleLineEdit::GlobalPrecision = precision;
    modified = true;
  }

  if (pqDoubleLineEdit::GlobalNotation != notation)
  {
    pqDoubleLineEdit::GlobalNotation = notation;
    modified = true;
  }

  if (modified && InstanceTracker != nullptr)
  {
    for (const auto& instance : *InstanceTracker)
    {
      if (instance && instance->useGlobalPrecisionAndNotation())
      {
        instance->updateFullPrecisionText();
        instance->updateLimitedPrecisionText();
      }
    }
  }
}

//-----------------------------------------------------------------------------
pqDoubleLineEdit::RealNumberNotation pqDoubleLineEdit::globalNotation()
{
  return pqDoubleLineEdit::GlobalNotation;
}

//-----------------------------------------------------------------------------
int pqDoubleLineEdit::globalPrecision()
{
  return pqDoubleLineEdit::GlobalPrecision;
}

//-----------------------------------------------------------------------------
pqDoubleLineEdit::pqDoubleLineEdit(QWidget* _parent)
  : Superclass(_parent)
  , Precision(2)
  , UseGlobalPrecisionAndNotation(true)
{
  this->DoubleValidator = new QDoubleValidator(this);
  this->setValidator(this->DoubleValidator);
  this->setNotation(pqDoubleLineEdit::FixedNotation);
  if (InstanceTracker == nullptr)
  {
    InstanceTracker = new InstanceTrackerType();
  }
  InstanceTracker->push_back(this);
}

//-----------------------------------------------------------------------------
pqDoubleLineEdit::~pqDoubleLineEdit()
{
  if (InstanceTracker)
  {
    InstanceTracker->removeAll(this);
    if (InstanceTracker->size() == 0)
    {
      delete InstanceTracker;
      InstanceTracker = nullptr;
    }
  }
}

//-----------------------------------------------------------------------------
QString pqDoubleLineEdit::fullPrecisionText() const
{
  return this->FullPrecisionText;
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setFullPrecisionText(const QString& _text)
{
  if (this->FullPrecisionText == _text)
  {
    return;
  }
  this->FullPrecisionText = _text;
  this->updateFullPrecisionText();
  this->updateLimitedPrecisionText();
  emit fullPrecisionTextChanged(this->FullPrecisionText);
}

//-----------------------------------------------------------------------------
pqDoubleLineEdit::RealNumberNotation pqDoubleLineEdit::notation() const
{
  return this->Notation;
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setNotation(pqDoubleLineEdit::RealNumberNotation _notation)
{
  if (this->Notation == _notation)
  {
    return;
  }
  this->Notation = _notation;
  this->DoubleValidator->setNotation(toValidatorNotation(_notation));
  this->updateLimitedPrecisionText();
}

//-----------------------------------------------------------------------------
int pqDoubleLineEdit::precision() const
{
  return this->Precision;
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setPrecision(int _precision)
{
  if (this->Precision == _precision)
  {
    return;
  }
  this->Precision = _precision;
  this->updateFullPrecisionText();
  this->updateLimitedPrecisionText();
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setDoubleValidator(QDoubleValidator* validator)
{
  if (validator == nullptr)
  {
    return;
  }
  validator->setParent(this);
  this->setValidator(validator);
  delete this->DoubleValidator;
  this->DoubleValidator = validator;
  this->DoubleValidator->setNotation(toValidatorNotation(this->notation()));
}

//-----------------------------------------------------------------------------
const QDoubleValidator* pqDoubleLineEdit::doubleValidator() const
{
  return this->DoubleValidator;
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::focusInEvent(QFocusEvent* event)
{
  if (event->gotFocus())
  {
    this->onEditingStarted();
  }
  return this->Superclass::focusInEvent(event);
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::updateFullPrecisionText()
{
  const int real_precision =
    this->UseGlobalPrecisionAndNotation ? pqDoubleLineEdit::GlobalPrecision : this->Precision;

  int dotIndex = this->FullPrecisionText.indexOf(".");
  if (dotIndex >= 0)
  {
    int digits = this->FullPrecisionText.length() - 1 - dotIndex;
    if (digits <= real_precision)
    {
      // If it applies, appends "0"
      this->FullPrecisionText += QString("0").repeated(real_precision - digits);
    }
    else
    {
      // If it applies, removes extra "0"
      while (digits > this->Precision &&
        this->FullPrecisionText[this->FullPrecisionText.length() - 1] == '0')
      {
        this->FullPrecisionText.chop(1);
        digits = this->FullPrecisionText.length() - 1 - dotIndex;
      }
    }
  }
  else
  {
    this->FullPrecisionText += QString(".") + QString("0").repeated(real_precision);
  }
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::updateLimitedPrecisionText()
{
  const auto real_precision =
    this->UseGlobalPrecisionAndNotation ? pqDoubleLineEdit::GlobalPrecision : this->Precision;
  const auto real_notation =
    this->UseGlobalPrecisionAndNotation ? pqDoubleLineEdit::GlobalNotation : this->Notation;

  QString limited;
  QTextStream converter(&limited);
  converter.setRealNumberNotation(toTextStreamNotation(real_notation));
  converter.setRealNumberPrecision(real_precision);
  converter << this->FullPrecisionText.toDouble();
  this->setText(limited);
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::onEditingStarted()
{
  this->setText(this->FullPrecisionText);
  connect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::onEditingFinished()
{
  disconnect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
  QString previousFullPrecisionText = this->FullPrecisionText;
  this->setFullPrecisionText(this->text());
  this->updateLimitedPrecisionText();
  this->clearFocus();
  if (previousFullPrecisionText != this->FullPrecisionText)
  {
    emit fullPrecisionTextChangedAndEditingFinished();
  }
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::triggerFullPrecisionTextChangedAndEditingFinished()
{
  emit fullPrecisionTextChangedAndEditingFinished();
}

//-----------------------------------------------------------------------------
bool pqDoubleLineEdit::useGlobalPrecisionAndNotation() const
{
  return this->UseGlobalPrecisionAndNotation;
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setUseGlobalPrecisionAndNotation(bool value)
{
  this->UseGlobalPrecisionAndNotation = value;
  this->updateLimitedPrecisionText();
}
