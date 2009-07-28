/*=========================================================================

   Program: ParaView
   Module:    pqHistogramChartOptions.cxx

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

/// \file pqHistogramChartOptions.cxx
/// \date 2/14/2007

#include "pqHistogramChartOptions.h"


const QColor pqHistogramChartOptions::LightBlue = QColor(125, 165, 230);
pqHistogramColor pqHistogramChartOptions::ColorScheme;

pqHistogramChartOptions::pqHistogramChartOptions(QObject *parentObject)
  : QObject(parentObject), Highlight(pqHistogramChartOptions::LightBlue)
{
  this->Style = pqHistogramChartOptions::Fill;
  this->OutlineType = pqHistogramChartOptions::Darker;
  this->Colors = &pqHistogramChartOptions::ColorScheme;
}

pqHistogramChartOptions::pqHistogramChartOptions(
    const pqHistogramChartOptions &other)
  : QObject(other.parent()), Highlight(other.Highlight)
{
  this->Style = other.Style;
  this->OutlineType = other.OutlineType;
  this->Colors = other.Colors;
}

void pqHistogramChartOptions::setHighlightStyle(
    pqHistogramChartOptions::HighlightStyle style)
{
  if(this->Style != style)
    {
    this->Style = style;
    emit this->optionsChanged();
    }
}

void pqHistogramChartOptions::setBinOutlineStyle(
    pqHistogramChartOptions::OutlineStyle style)
{
  if(this->OutlineType != style)
    {
    this->OutlineType = style;
    emit this->optionsChanged();
    }
}

void pqHistogramChartOptions::setHighlightColor(const QColor &color)
{
  if(this->Highlight != color)
    {
    this->Highlight = color;
    emit this->optionsChanged();
    }
}

void pqHistogramChartOptions::setColorScheme(pqHistogramColor *scheme)
{
  if(!scheme && this->Colors == &pqHistogramChartOptions::ColorScheme)
    {
    return;
    }

  if(this->Colors != scheme)
    {
    if(scheme)
      {
      this->Colors = scheme;
      }
    else
      {
      this->Colors = &pqHistogramChartOptions::ColorScheme;
      }

    emit this->optionsChanged();
    }
}

pqHistogramChartOptions &pqHistogramChartOptions::operator=(
    const pqHistogramChartOptions &other)
{
  this->Style = other.Style;
  this->OutlineType = other.OutlineType;
  this->Highlight = other.Highlight;
  this->Colors = other.Colors;
  return *this;
}


