// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// self includes
#include "pqSignalAdaptors.h"

// Qt includes
#include <QColor>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>

//----------------------------------------------------------------------------
pqSignalAdaptorComboBox::pqSignalAdaptorComboBox(QComboBox* p)
  : QObject(p)
{
  QObject::connect(
    p, &QComboBox::currentTextChanged, this, &pqSignalAdaptorComboBox::currentTextChanged);

  QObject::connect(p, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &pqSignalAdaptorComboBox::currentIndexChanged);
}

//----------------------------------------------------------------------------
QString pqSignalAdaptorComboBox::currentText() const
{
  return static_cast<QComboBox*>(this->parent())->currentText();
}

//----------------------------------------------------------------------------
void pqSignalAdaptorComboBox::setCurrentText(const QString& text)
{
  QComboBox* combo = static_cast<QComboBox*>(this->parent());
  int idx = combo->findText(text);
  combo->setCurrentIndex(idx);
  if (idx == -1 && combo->count() > 0)
  {
    combo->setCurrentIndex(0);
  }
}

//----------------------------------------------------------------------------
int pqSignalAdaptorComboBox::currentIndex() const
{
  return static_cast<QComboBox*>(this->parent())->currentIndex();
}

//----------------------------------------------------------------------------
void pqSignalAdaptorComboBox::setCurrentIndex(int index)
{
  QComboBox* combo = static_cast<QComboBox*>(this->parent());
  combo->setCurrentIndex(index);
}

//----------------------------------------------------------------------------
void pqSignalAdaptorComboBox::setCurrentData(const QVariant& data)
{
  QComboBox* combo = static_cast<QComboBox*>(this->parent());
  int idx = combo->findData(data);
  combo->setCurrentIndex(idx);
  if (idx == -1 && combo->count() > 0)
  {
    combo->setCurrentIndex(0);
  }
}

//----------------------------------------------------------------------------
QVariant pqSignalAdaptorComboBox::currentData() const
{
  int index = this->currentIndex();
  QComboBox* combo = static_cast<QComboBox*>(this->parent());
  return combo->itemData(index);
}

//----------------------------------------------------------------------------
pqSignalAdaptorColor::pqSignalAdaptorColor(
  QObject* p, const char* colorProperty, const char* signal, bool enableAlpha)
  : QObject(p)
  , PropertyName(colorProperty)
  , EnableAlpha(enableAlpha)
{
  // assumes signal named after property
  QObject::connect(p, signal, this, SLOT(handleColorChanged()));
  qDebug("Changes in pqColorChooserButton API have made pqSignalAdaptorColor unnecessary. "
         "Please update the code. pqSignalAdaptorColor will soon be deprecated.");
}

//----------------------------------------------------------------------------
QVariant pqSignalAdaptorColor::color() const
{
  QColor col = this->parent()->property(this->PropertyName).value<QColor>();
  QList<QVariant> rgba;
  if (col.isValid())
  {
    rgba.append(col.red() / 255.0);
    rgba.append(col.green() / 255.0);
    rgba.append(col.blue() / 255.0);
    if (this->EnableAlpha)
    {
      rgba.append(col.alpha() / 255.0);
    }
  }
  return rgba;
}

//----------------------------------------------------------------------------
void pqSignalAdaptorColor::setColor(const QVariant& var)
{
  QColor col;
  QList<QVariant> rgba = var.toList();
  if (rgba.size() >= 3)
  {
    int r = qRound(rgba[0].toDouble() * 255.0);
    int g = qRound(rgba[1].toDouble() * 255.0);
    int b = qRound(rgba[2].toDouble() * 255.0);
    int a = 255;
    if (rgba.size() == 4 && this->EnableAlpha)
    {
      a = qRound(rgba[3].toDouble() * 255.0);
    }
    QColor newColor(r, g, b, a);
    if (this->parent()->property(this->PropertyName) != newColor)
    {
      this->parent()->setProperty(this->PropertyName, QColor(r, g, b, a));
    }
  }
}

//----------------------------------------------------------------------------
void pqSignalAdaptorColor::handleColorChanged()
{
  QVariant col = this->color();
  Q_EMIT this->colorChanged(col);
}

//----------------------------------------------------------------------------
pqSignalAdaptorSliderRange::pqSignalAdaptorSliderRange(QSlider* p)
  : QObject(p)
{
  QObject::connect(p, SIGNAL(valueChanged(int)), this, SLOT(handleValueChanged()));
}

//----------------------------------------------------------------------------
double pqSignalAdaptorSliderRange::value() const
{
  QSlider* slider = static_cast<QSlider*>(this->parent());
  double factor = slider->maximum() - slider->minimum();
  return slider->value() / factor;
}

//----------------------------------------------------------------------------
void pqSignalAdaptorSliderRange::setValue(double val)
{
  QSlider* slider = static_cast<QSlider*>(this->parent());
  double factor = slider->maximum() - slider->minimum();
  slider->setValue(qRound(val * factor));
}

//----------------------------------------------------------------------------
void pqSignalAdaptorSliderRange::handleValueChanged()
{
  Q_EMIT this->valueChanged(this->value());
}

//----------------------------------------------------------------------------
pqSignalAdaptorTextEdit::pqSignalAdaptorTextEdit(QTextEdit* p)
  : QObject(p)
{
  QObject::connect(p, SIGNAL(textChanged()), this, SIGNAL(textChanged()));
}

QString pqSignalAdaptorTextEdit::text() const
{
  return static_cast<QTextEdit*>(this->parent())->toPlainText();
}

void pqSignalAdaptorTextEdit::setText(const QString& ptext)
{
  QTextEdit* combo = static_cast<QTextEdit*>(this->parent());
  combo->setPlainText(ptext);
}

//----------------------------------------------------------------------------
pqSignalAdaptorSpinBox::pqSignalAdaptorSpinBox(QSpinBox* p)
  : QObject(p)
{
  QObject::connect(p, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));
}

int pqSignalAdaptorSpinBox::value() const
{
  return static_cast<QSpinBox*>(this->parent())->value();
}

void pqSignalAdaptorSpinBox::setValue(int val)
{
  QSpinBox* widget = static_cast<QSpinBox*>(this->parent());
  widget->setValue(val);
}
