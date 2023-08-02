// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDoubleLineEdit.h"

// Qt Includes.
#include <QDoubleValidator>
#include <QFocusEvent>
#include <QPointer>
#include <QTextStream>

// VTK includes
#include "vtkNumberToString.h"

// System includes
#include <cassert>

//=============================================================================
namespace
{
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
  if (InstanceTracker->empty())
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
  bool AlwaysUseFullPrecision = false;
  int Precision = 2;
  pqDoubleLineEdit::RealNumberNotation Notation = pqDoubleLineEdit::FixedNotation;
  bool UseGlobalPrecisionAndNotation = true;
  QPointer<QLineEdit> InactiveLineEdit = nullptr;

  bool useFullPrecision(const pqDoubleLineEdit* self) const
  {
    const auto realNotation =
      this->UseGlobalPrecisionAndNotation ? pqDoubleLineEdit::globalNotation() : this->Notation;

    return this->AlwaysUseFullPrecision || self->hasFocus() ||
      realNotation == RealNumberNotation::FullNotation;
  }

  void sync(pqDoubleLineEdit* self)
  {
    const auto realPrecision =
      this->UseGlobalPrecisionAndNotation ? pqDoubleLineEdit::globalPrecision() : this->Precision;
    const auto realNotation =
      this->UseGlobalPrecisionAndNotation ? pqDoubleLineEdit::globalNotation() : this->Notation;

    const QString limited = self->text().isEmpty()
      ? self->text()
      : pqDoubleLineEdit::formatDouble(self->text().toDouble(), realNotation, realPrecision);

    const bool changed = (limited != this->InactiveLineEdit->text());
    this->InactiveLineEdit->setText(limited);

    if (changed)
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
QString pqDoubleLineEdit::formatDouble(double value, pqDoubleLineEdit::RealNumberNotation notation,
  int precision, int fullLowExponent, int fullHighExponent)
{
  switch (notation)
  {
    case RealNumberNotation::ScientificNotation:
      return QString::number(value, 'e', precision);
      break;
    case RealNumberNotation::FixedNotation:
      return QString::number(value, 'f', precision);
      break;
    case RealNumberNotation::MixedNotation:
      return QString::number(value, 'g', precision);
      break;
    case RealNumberNotation::FullNotation:
    {
      vtkNumberToString converter;
      converter.SetLowExponent(fullLowExponent);
      converter.SetHighExponent(fullHighExponent);
      return QString::fromStdString(converter.Convert(value));
    }
    break;
    default:
      return "";
      break;
  };
  return "";
}

//-----------------------------------------------------------------------------
QString pqDoubleLineEdit::formatDoubleUsingGlobalPrecisionAndNotation(double value)
{
  return pqDoubleLineEdit::formatDouble(
    value, pqDoubleLineEdit::globalNotation(), pqDoubleLineEdit::globalPrecision());
}

//-----------------------------------------------------------------------------
void pqDoubleLineEdit::setAlwaysUseFullPrecision(bool value)
{
  this->Internals->AlwaysUseFullPrecision = value;
}
