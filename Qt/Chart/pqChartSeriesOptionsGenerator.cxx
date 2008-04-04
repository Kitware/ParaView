/*=========================================================================

   Program: ParaView
   Module:    pqChartSeriesOptionsGenerator.cxx

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

/// \file pqChartSeriesOptionsGenerator.cxx
/// \date 4/26/2007

#include "pqChartSeriesOptionsGenerator.h"

#include <QColor>
#include <QPen>
#include <QVector>


class pqChartSeriesOptionsGeneratorInternal
{
public:
  pqChartSeriesOptionsGeneratorInternal();
  ~pqChartSeriesOptionsGeneratorInternal() {}

  QVector<QColor> Colors;       ///< Stores the list of colors.
  QVector<Qt::PenStyle> Styles; ///< Stores the list of pen styles.
};


//----------------------------------------------------------------------------
pqChartSeriesOptionsGeneratorInternal::pqChartSeriesOptionsGeneratorInternal()
  : Colors(), Styles()
{
}


//----------------------------------------------------------------------------
pqChartSeriesOptionsGenerator::pqChartSeriesOptionsGenerator(
    pqChartSeriesOptionsGenerator::ColorScheme scheme)
{
  this->Internal = new pqChartSeriesOptionsGeneratorInternal();
  this->Scheme = pqChartSeriesOptionsGenerator::Custom;

  // Set the requested color scheme and the default pen styles.
  this->setColorScheme(scheme);
  this->Internal->Styles.append(Qt::SolidLine);
  this->Internal->Styles.append(Qt::DashLine);
  this->Internal->Styles.append(Qt::DotLine);
  this->Internal->Styles.append(Qt::DashDotLine);
  this->Internal->Styles.append(Qt::DashDotDotLine);
}

pqChartSeriesOptionsGenerator::~pqChartSeriesOptionsGenerator()
{
  delete this->Internal;
}

int pqChartSeriesOptionsGenerator::getNumberOfColors() const
{
  return this->Internal->Colors.size();
}

int pqChartSeriesOptionsGenerator::getNumberOfStyles() const
{
  return this->Internal->Styles.size();
}

void pqChartSeriesOptionsGenerator::getColor(int index, QColor &color) const
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    color = this->Internal->Colors[index];
    }
}

Qt::PenStyle pqChartSeriesOptionsGenerator::getPenStyle(int index) const
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    return this->Internal->Styles[index];
    }

  return Qt::SolidLine;
}

void pqChartSeriesOptionsGenerator::getSeriesColor(int index,
    QColor &color) const
{
  if(this->Internal->Colors.size() > 0)
    {
    index = index % this->Internal->Colors.size();
    color = this->Internal->Colors[index];
    }
}

void pqChartSeriesOptionsGenerator::getSeriesPen(int index, QPen &pen) const
{
  if(this->Internal->Colors.size() > 0)
    {
    QColor color;
    this->getSeriesColor(index, color);
    pen.setColor(color);
    index /= this->Internal->Colors.size();
    }

  if(this->Internal->Styles.size() > 0)
    {
    index = index % this->Internal->Styles.size();
    pen.setStyle(this->Internal->Styles[index]);
    }
}

void pqChartSeriesOptionsGenerator::setColorScheme(
    pqChartSeriesOptionsGenerator::ColorScheme scheme)
{
  if(this->Scheme == scheme)
    {
    return;
    }

  // Clear the list of previous colors.
  this->Internal->Colors.clear();

  // Save the new scheme type and load the new colors.
  this->Scheme = scheme;
  if(this->Scheme == pqChartSeriesOptionsGenerator::Spectrum)
    {
    this->Internal->Colors.append(QColor(0, 0, 0));
    this->Internal->Colors.append(QColor(228, 26, 28));
    this->Internal->Colors.append(QColor(55, 126, 184));
    this->Internal->Colors.append(QColor(77, 175, 74));
    this->Internal->Colors.append(QColor(152, 78, 163));
    this->Internal->Colors.append(QColor(255, 127, 0));
    this->Internal->Colors.append(QColor(166, 86, 40));
    }
  else if(this->Scheme == pqChartSeriesOptionsGenerator::Warm)
    {
    this->Internal->Colors.append(QColor(121, 23, 23));
    this->Internal->Colors.append(QColor(181, 1, 1));
    this->Internal->Colors.append(QColor(239, 71, 25));
    this->Internal->Colors.append(QColor(249, 131, 36));
    this->Internal->Colors.append(QColor(255, 180, 0));
    this->Internal->Colors.append(QColor(255, 229, 6));
    }
  else if(this->Scheme == pqChartSeriesOptionsGenerator::Cool)
    {
    this->Internal->Colors.append(QColor(117, 177, 1));
    this->Internal->Colors.append(QColor(88, 128, 41));
    this->Internal->Colors.append(QColor(80, 215, 191));
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(154, 104, 255));
    this->Internal->Colors.append(QColor(95, 51, 128));
    }
  else if(this->Scheme == pqChartSeriesOptionsGenerator::Blues)
    {
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(78, 217, 234));
    this->Internal->Colors.append(QColor(115, 154, 213));
    this->Internal->Colors.append(QColor(66, 61, 169));
    this->Internal->Colors.append(QColor(80, 84, 135));
    this->Internal->Colors.append(QColor(16, 42, 82));
    }
  else if(this->Scheme == pqChartSeriesOptionsGenerator::WildFlower)
    {
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(102, 62, 183));
    this->Internal->Colors.append(QColor(162, 84, 207));
    this->Internal->Colors.append(QColor(222, 97, 206));
    this->Internal->Colors.append(QColor(220, 97, 149));
    this->Internal->Colors.append(QColor(61, 16, 82));
    }
  else if(this->Scheme == pqChartSeriesOptionsGenerator::Citrus)
    {
    this->Internal->Colors.append(QColor(101, 124, 55));
    this->Internal->Colors.append(QColor(117, 177, 1));
    this->Internal->Colors.append(QColor(178, 186, 48));
    this->Internal->Colors.append(QColor(255, 229, 6));
    this->Internal->Colors.append(QColor(255, 180, 0));
    this->Internal->Colors.append(QColor(249, 131, 36));
    }
}

void pqChartSeriesOptionsGenerator::clearColors()
{
  this->Scheme = pqChartSeriesOptionsGenerator::Custom;
  this->Internal->Colors.clear();
}

void pqChartSeriesOptionsGenerator::addColor(const QColor &color)
{
  this->Scheme = pqChartSeriesOptionsGenerator::Custom;
  this->Internal->Colors.append(color);
}

void pqChartSeriesOptionsGenerator::insertColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = pqChartSeriesOptionsGenerator::Custom;
    this->Internal->Colors.insert(index, color);
    }
}

void pqChartSeriesOptionsGenerator::setColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = pqChartSeriesOptionsGenerator::Custom;
    this->Internal->Colors[index] = color;
    }
}

void pqChartSeriesOptionsGenerator::removeColor(int index)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = pqChartSeriesOptionsGenerator::Custom;
    this->Internal->Colors.remove(index);
    }
}

void pqChartSeriesOptionsGenerator::clearPenStyles()
{
  this->Internal->Styles.clear();
}

void pqChartSeriesOptionsGenerator::addPenStyle(Qt::PenStyle style)
{
  this->Internal->Styles.append(style);
}

void pqChartSeriesOptionsGenerator::insertPenStyle(int index,
    Qt::PenStyle style)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles.insert(index, style);
    }
}

void pqChartSeriesOptionsGenerator::setPenStyle(int index, Qt::PenStyle style)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles[index] = style;
    }
}

void pqChartSeriesOptionsGenerator::removePenStyle(int index)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles.remove(index);
    }
}


