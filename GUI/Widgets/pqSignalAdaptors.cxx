/*=========================================================================

   Program:   ParaQ
   Module:    pqSignalAdaptors.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

// self includes
#include "pqSignalAdaptors.h"

// Qt includes
#include <QComboBox>
#include <QSlider>


pqSignalAdaptorComboBox::pqSignalAdaptorComboBox(QComboBox* p)
  : QObject(p)
{
  QObject::connect(p, SIGNAL(currentIndexChanged(const QString&)), 
                   this, SIGNAL(currentTextChanged(const QString&)));
}

QString pqSignalAdaptorComboBox::currentText() const
{
  return static_cast<QComboBox*>(this->parent())->currentText();
}

void pqSignalAdaptorComboBox::setCurrentText(const QString& text)
{
  QComboBox* combo = static_cast<QComboBox*>(this->parent());
  combo->setCurrentIndex(combo->findText(text));
}

pqSignalAdaptorColor::pqSignalAdaptorColor(QObject* p, 
              const char* colorProperty, const char* signal)
  : QObject(p), PropertyName(colorProperty)
{
  // assumes signal named after property 
  QObject::connect(p, signal,
                   this, SLOT(handleColorChanged()));
}

QVariant pqSignalAdaptorColor::color() const
{
  QColor col = this->parent()->property(this->PropertyName).value<QColor>();
  QList<QVariant> rgba;
  if(col.isValid())
    {
    rgba.append(col.red() / 255.0);
    rgba.append(col.green() / 255.0);
    rgba.append(col.blue() / 255.0);
    rgba.append(col.alpha() / 255.0);
    }
  return rgba;
}

void pqSignalAdaptorColor::setColor(const QVariant& var)
{
  QColor col;
  QList<QVariant> rgba = var.toList();
  if(rgba.size() >= 3)
    {
    int r = qRound(rgba[0].toDouble() * 255.0);
    int g = qRound(rgba[1].toDouble() * 255.0);
    int b = qRound(rgba[2].toDouble() * 255.0);
    int a = 255;
    if(rgba.size() == 4)
      {
      a = qRound(rgba[3].toDouble() * 255.0);
      }
    QColor newColor(r,g,b,a);
    if(this->parent()->property(this->PropertyName) != newColor)
      {
      this->parent()->setProperty(this->PropertyName, QColor(r,g,b,a));
      }
    }
}

void pqSignalAdaptorColor::handleColorChanged()
{
  QVariant col = this->color();
  emit this->colorChanged(col);
}

pqSignalAdaptorSliderRange::pqSignalAdaptorSliderRange(QSlider* p)
  : QObject(p)
{
  QObject::connect(p, SIGNAL(valueChanged(int)), 
                   this, SLOT(handleValueChanged()));
}

double pqSignalAdaptorSliderRange::value() const
{
  QSlider* slider = static_cast<QSlider*>(this->parent());
  double factor = slider->maximum() - slider->minimum();
  return slider->value() / factor;
}

void pqSignalAdaptorSliderRange::setValue(double val)
{
  QSlider* slider = static_cast<QSlider*>(this->parent());
  double factor = slider->maximum() - slider->minimum();
  slider->setValue(qRound(val * factor));
}

void pqSignalAdaptorSliderRange::handleValueChanged()
{
  emit this->valueChanged(this->value());
}

