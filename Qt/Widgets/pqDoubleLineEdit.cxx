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
#include <QDoubleValidator>
#include <QFocusEvent>
#include <QPointer>
#include <QTextStream>

#include <cassert>

//=============================================================================
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
using InstanceTrackerType = QList<pqDoubleLineEdit*>;
static InstanceTrackerType* InstanceTracker = nullptr;

//-----------------------------------------------------------------------------
void register_dle_instance(pqDoubleLineEdit* dle)
{
  if (InstanceTracker == nullptr)
  {
    InstanceTracker = new InstanceTrackerType();
  }
  InstanceTracker->push_back(dle);
}

void unregister_dle_instance(pqDoubleLineEdit* dle)
{
  assert(InstanceTracker != nullptr);
  InstanceTracker->removeOne(dle);
  if (InstanceTracker->size() == 0)
  {
    delete InstanceTracker;
    InstanceTracker = nullptr;
  }
}
}

//=============================================================================
class pqDoubleLineEdit::pqInternals
{
public:
  int Precision = 2;
  pqDoubleLineEdit::RealNumberNotation Notation = pqDoubleLineEdit::FixedNotation;
  bool UseGlobalPrecisionAndNotation = true;
  QPointer<QLineEdit> InactiveLineEdit = nullptr;

  bool useFullPrecision(const pqDoubleLineEdit* self) const { return self->hasFocus(); }

  void sync(pqDoubleLineEdit* self)
  {
    const auto real_precision =
      this->UseGlobalPrecisionAndNotation ? pqDoubleLineEdit::globalPrecision() : this->Precision;
    const auto real_notation =
      this->UseGlobalPrecisionAndNotation ? pqDoubleLineEdit::globalNotation() : this->Notation;

    const QString limited =
      self->text().isEmpty() ? QString() : pqDoubleLineEdit::formatDouble(self->text().toDouble(),
                                             toTextStreamNotation(real_notation), real_precision);

    const bool changed = (limited != this->InactiveLineEdit->text());
    this->InactiveLineEdit->setText(limited);

    if (changed & !this->useFullPrecision(self))
    {
      // ensures that if the low precision text changed and it was being shown on screen,
      // we repaint it.
      self->update();
    }
  }

  void renderSimplified(pqDoubleLineEdit* self)
  {
    if (this->InactiveLineEdit)
    {
      // sync some state with inactive-line edit.
      this->InactiveLineEdit->setEnabled(self->isEnabled());
      this->InactiveLineEdit->setPlaceholderText(self->placeholderText());
      this->InactiveLineEdit->render(self, self->mapTo(self->window(), QPoint(0, 0)));
    }
  }
};

//=============================================================================
int pqDoubleLineEdit::GlobalPrecision = 6;
pqDoubleLineEdit::RealNumberNotation pqDoubleLineEdit::GlobalNotation =
  pqDoubleLineEdit::MixedNotation;

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
        instance->Internals->sync(instance);
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
  , Internals(new pqDoubleLineEdit::pqInternals())
{
  this->setValidator(new QDoubleValidator(this));
  register_dle_instance(this);

  auto& internals = (*this->Internals);
  internals.InactiveLineEdit = new QLineEdit();
  internals.InactiveLineEdit->hide();
  internals.sync(this);

  QObject::connect(
    this, &QLineEdit::textChanged, [this](const QString&) { this->Internals->sync(this); });
}

//-----------------------------------------------------------------------------
pqDoubleLineEdit::~pqDoubleLineEdit()
{
  unregister_dle_instance(this);
  auto& internals = (*this->Internals);
  delete internals.InactiveLineEdit;
  internals.InactiveLineEdit = nullptr;
}

//-----------------------------------------------------------------------------
pqDoubleLineEdit::RealNumberNotation pqDoubleLineEdit::notation() const
{
  auto& internals = (*this->Internals);
  return internals.Notation;
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setNotation(pqDoubleLineEdit::RealNumberNotation _notation)
{
  auto& internals = (*this->Internals);
  if (internals.Notation != _notation)
  {
    internals.Notation = _notation;
    internals.sync(this);
  }
}

//-----------------------------------------------------------------------------
int pqDoubleLineEdit::precision() const
{
  auto& internals = (*this->Internals);
  return internals.Precision;
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setPrecision(int _precision)
{
  auto& internals = (*this->Internals);
  if (internals.Precision != _precision)
  {
    internals.Precision = _precision;
    internals.sync(this);
  }
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::resizeEvent(QResizeEvent* evt)
{
  this->Superclass::resizeEvent(evt);
  auto& internals = (*this->Internals);
  internals.InactiveLineEdit->resize(this->size());
}

//-----------------------------------------------------------------------------
bool pqDoubleLineEdit::useGlobalPrecisionAndNotation() const
{
  auto& internals = (*this->Internals);
  return internals.UseGlobalPrecisionAndNotation;
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setUseGlobalPrecisionAndNotation(bool value)
{
  auto& internals = (*this->Internals);
  if (internals.UseGlobalPrecisionAndNotation != value)
  {
    internals.UseGlobalPrecisionAndNotation = value;
    internals.sync(this);
  }
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::paintEvent(QPaintEvent* evt)
{
  auto& internals = (*this->Internals);
  if (internals.useFullPrecision(this))
  {
    this->Superclass::paintEvent(evt);
  }
  else
  {
    internals.renderSimplified(this);
  }
}

//-----------------------------------------------------------------------------
QString pqDoubleLineEdit::simplifiedText() const
{
  auto& internals = (*this->Internals);
  return internals.InactiveLineEdit->text();
}

//-----------------------------------------------------------------------------
QString pqDoubleLineEdit::formatDouble(
  double value, QTextStream::RealNumberNotation notation, int precision)
{
  QString text;
  QTextStream converter(&text);
  converter.setRealNumberNotation(notation);
  converter.setRealNumberPrecision(precision);
  converter << value;

  return text;
}

//-----------------------------------------------------------------------------
QString pqDoubleLineEdit::formatDouble(
  double value, pqDoubleLineEdit::RealNumberNotation notation, int precision)
{
  return pqDoubleLineEdit::formatDouble(value, toTextStreamNotation(notation), precision);
}

//-----------------------------------------------------------------------------
QString pqDoubleLineEdit::formatDoubleUsingGlobalPrecisionAndNotation(double value)
{
  return pqDoubleLineEdit::formatDouble(
    value, pqDoubleLineEdit::globalNotation(), pqDoubleLineEdit::globalPrecision());
}
