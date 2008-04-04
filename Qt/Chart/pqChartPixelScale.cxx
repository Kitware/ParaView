/*=========================================================================

   Program: ParaView
   Module:    pqChartPixelScale.cxx

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

/// \file pqChartPixelScale.cxx
/// \date 7/25/2006

#include "pqChartPixelScale.h"

#include "pqChartValue.h"
#include <math.h>


class pqChartPixelScaleInternal
{
public:
  pqChartPixelScaleInternal();
  ~pqChartPixelScaleInternal() {}

  /// Stores the scale type (linear or log10).
  pqChartPixelScale::ValueScale Scale;
  pqChartValue ValueMin; ///< Stores the minimum value.
  pqChartValue ValueMax; ///< Stores the maximum value.
  int PixelMin;          ///< Stores the minimum pixel.
  int PixelMax;          ///< Stores the maximum pixel.
  bool LogAvailable;     ///< True if log10 scale is valid.
};


static double MinIntLogPower = -1;
const double pqChartPixelScale::MinLogValue = 0.0001;


//----------------------------------------------------------------------------
pqChartPixelScaleInternal::pqChartPixelScaleInternal()
  : ValueMin(), ValueMax()
{
  this->Scale = pqChartPixelScale::Linear;
  this->PixelMin = 0;
  this->PixelMax = 0;
  this->LogAvailable = false;
}


//----------------------------------------------------------------------------
pqChartPixelScale::pqChartPixelScale()
{
  this->Internal = new pqChartPixelScaleInternal();
}

pqChartPixelScale::~pqChartPixelScale()
{
  delete this->Internal;
}

bool pqChartPixelScale::setValueRange(const pqChartValue &min,
    const pqChartValue &max)
{
  if(min != this->Internal->ValueMin || max != this->Internal->ValueMax)
    {
    this->Internal->ValueMin = min;
    this->Internal->ValueMax = max;

    // If the value range changed, determine if log scale can be used.
    this->Internal->LogAvailable = pqChartPixelScale::isLogScaleValid(
        this->Internal->ValueMin, this->Internal->ValueMax);
    return true;
    }

  return false;
}

void pqChartPixelScale::getValueRange(pqChartValue &range) const
{
  range = this->Internal->ValueMax - this->Internal->ValueMin;
}

bool pqChartPixelScale::setMinValue(const pqChartValue &min)
{
  return this->setValueRange(min, this->Internal->ValueMax);
}

const pqChartValue &pqChartPixelScale::getMinValue() const
{
  return this->Internal->ValueMin;
}

bool pqChartPixelScale::setMaxValue(const pqChartValue &max)
{
  return this->setValueRange(this->Internal->ValueMin, max);
}

const pqChartValue &pqChartPixelScale::getMaxValue() const
{
  return this->Internal->ValueMax;
}

bool pqChartPixelScale::setPixelRange(int min, int max)
{
  if(this->Internal->PixelMin != min || this->Internal->PixelMax != max)
    {
    this->Internal->PixelMin = min;
    this->Internal->PixelMax = max;
    return true;
    }

  return false;
}

int pqChartPixelScale::getPixelRange() const
{
  // TODO: Is the true (max - min) ever needed?
  if(this->Internal->PixelMax > this->Internal->PixelMin)
    {
    return this->Internal->PixelMax - this->Internal->PixelMin;
    }
  else
    {
    return this->Internal->PixelMin - this->Internal->PixelMax;
    }
}

bool pqChartPixelScale::setMinPixel(int min)
{
  return this->setPixelRange(min, this->Internal->PixelMin);
}

int pqChartPixelScale::getMinPixel() const
{
  return this->Internal->PixelMin;
}

bool pqChartPixelScale::setMaxPixel(int max)
{
  return this->setPixelRange(this->Internal->PixelMin, max);
}

int pqChartPixelScale::getMaxPixel() const
{
  return this->Internal->PixelMax;
}

int pqChartPixelScale::getPixel(const pqChartValue &value) const
{
  // Convert the value to a pixel location using:
  // px = ((vx - v1)*(p2 - p1))/(v2 - v1) + p1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue result;
  pqChartValue valueRange;
  if(this->Internal->Scale == pqChartPixelScale::Logarithmic &&
      this->Internal->LogAvailable)
    {
    // If the value is too small, return the minimum pixel.
    if(value <= pqChartPixelScale::MinLogValue)
      {
      return this->Internal->PixelMin;
      }

    // If the log scale uses integers, the first value may be zero.
    // In that case, use -1 instead of taking the log of zero.
    pqChartValue v1;
    if(this->Internal->ValueMin.getType() == pqChartValue::IntValue &&
        this->Internal->ValueMin == 0)
      {
      v1 = MinIntLogPower;
      }
    else
      {
      v1 = log10(this->Internal->ValueMin.getDoubleValue());
      }

    if(this->Internal->ValueMin.getType() == pqChartValue::IntValue &&
        this->Internal->ValueMax == 0)
      {
      valueRange = MinIntLogPower;
      }
    else
      {
      valueRange = log10(this->Internal->ValueMax.getDoubleValue());
      }

    result = log10(value.getDoubleValue());
    result -= v1;
    valueRange -= v1;
    }
  else
    {
    result = value - this->Internal->ValueMin;
    valueRange = this->Internal->ValueMax - this->Internal->ValueMin;
    }

  result *= this->Internal->PixelMax - this->Internal->PixelMin;
  if(valueRange != 0)
    {
    result /= valueRange;
    }

  return result.getIntValue() + this->Internal->PixelMin;
}

float pqChartPixelScale::getPixelF(const pqChartValue &value) const
{
  // Convert the value to a pixel location using:
  // px = ((vx - v1)*(p2 - p1))/(v2 - v1) + p1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue result;
  pqChartValue valueRange;
  if(this->Internal->Scale == pqChartPixelScale::Logarithmic &&
      this->Internal->LogAvailable)
    {
    // If the value is too small, return the minimum pixel.
    if(value <= pqChartPixelScale::MinLogValue)
      {
      return (float)this->Internal->PixelMin;
      }

    // If the log scale uses integers, the first value may be zero.
    // In that case, use -1 instead of taking the log of zero.
    pqChartValue v1;
    if(this->Internal->ValueMin.getType() == pqChartValue::IntValue &&
        this->Internal->ValueMin == 0)
      {
      v1 = MinIntLogPower;
      }
    else
      {
      v1 = log10(this->Internal->ValueMin.getDoubleValue());
      }

    if(this->Internal->ValueMin.getType() == pqChartValue::IntValue &&
        this->Internal->ValueMax == 0)
      {
      valueRange = MinIntLogPower;
      }
    else
      {
      valueRange = log10(this->Internal->ValueMax.getDoubleValue());
      }

    result = log10(value.getDoubleValue());
    result -= v1;
    valueRange -= v1;
    }
  else
    {
    result = value - this->Internal->ValueMin;
    result.convertTo(pqChartValue::FloatValue);
    valueRange = this->Internal->ValueMax - this->Internal->ValueMin;
    }

  result *= this->Internal->PixelMax - this->Internal->PixelMin;
  if(valueRange != 0)
    {
    result /= valueRange;
    }

  return result.getFloatValue() + (float)this->Internal->PixelMin;
}

void pqChartPixelScale::getValue(int pixel, pqChartValue &value) const
{
  // Convert the pixel location to a value using:
  // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue v1;
  if(this->Internal->Scale == pqChartPixelScale::Logarithmic &&
      this->Internal->LogAvailable)
    {
    // If the log scale uses integers, the first value may be zero.
    // In that case, use -1 instead of taking the log of zero.
    if(this->Internal->ValueMin.getType() == pqChartValue::IntValue &&
        this->Internal->ValueMin == 0)
      {
      v1 = MinIntLogPower;
      }
    else
      {
      v1 = log10(this->Internal->ValueMin.getDoubleValue());
      }

    if(this->Internal->ValueMin.getType() == pqChartValue::IntValue &&
        this->Internal->ValueMax == 0)
      {
      value = MinIntLogPower;
      }
    else
      {
      value = log10(this->Internal->ValueMax.getDoubleValue());
      }

    value -= v1;
    }
  else
    {
    v1 = this->Internal->ValueMin;
    value = this->Internal->ValueMax - this->Internal->ValueMin;
    }

  value *= pixel - this->Internal->PixelMin;
  int pixelRange = this->Internal->PixelMax - this->Internal->PixelMin;
  if(pixelRange != 0)
    {
    value /= pixelRange;
    }

  value += v1;
  if(this->Internal->Scale == pqChartPixelScale::Logarithmic &&
      this->Internal->LogAvailable)
    {
    value = pow((double)10.0, value.getDoubleValue());
    if(this->Internal->ValueMin.getType() != pqChartValue::DoubleValue)
      {
      value.convertTo(pqChartValue::FloatValue);
      }
    }
}

bool pqChartPixelScale::isValid() const
{
  if(this->Internal->ValueMax == this->Internal->ValueMin)
    {
    return false;
    }

  if(this->Internal->PixelMax == this->Internal->PixelMin)
    {
    return false;
    }

  return true;
}

bool pqChartPixelScale::isZeroInRange() const
{
  return (this->Internal->ValueMax >= 0 && this->Internal->ValueMin <= 0) ||
      (this->Internal->ValueMax <= 0 && this->Internal->ValueMin >= 0);
}

void pqChartPixelScale::setScaleType(pqChartPixelScale::ValueScale scale)
{
  this->Internal->Scale = scale;
}

pqChartPixelScale::ValueScale pqChartPixelScale::getScaleType() const
{
  return this->Internal->Scale;
}

bool pqChartPixelScale::isLogScaleAvailable() const
{
  return this->Internal->LogAvailable;
}

bool pqChartPixelScale::isLogScaleValid(const pqChartValue &min,
    const pqChartValue &max)
{
  bool available = min > 0 && max > 0;
  if(!available && max.getType() == pqChartValue::IntValue)
    {
    available = (min == 0 && min < max) || (max == 0 && max < min);
    }

  return available;
}


