/*=========================================================================

   Program: ParaView
   Module:    pqChartPixelScale.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include <math.h>


static double MinIntLogPower = -1;
const double pqChartPixelScale::MinLogValue = 0.0001;


pqChartPixelScale::pqChartPixelScale()
  : ValueMin(), ValueMax()
{
  this->Scale = pqChartPixelScale::Linear;
  this->PixelMin = 0;
  this->PixelMax = 0;
}

bool pqChartPixelScale::setValueRange(const pqChartValue &min,
    const pqChartValue &max)
{
  pqChartValue prevMin = this->ValueMin;
  pqChartValue prevMax = this->ValueMax;
  this->ValueMin = min;
  this->ValueMax = max;
  if(this->Scale == pqChartPixelScale::Logarithmic)
    {
    // A logarithmic scale axis cannot contain zero because it is
    // undefined. If the range includes zero, set the scale to linear.
    if((min < 0 && max > 0) || (max < 0 && min > 0))
      {
      this->Scale = pqChartPixelScale::Linear;
      }
    }

  if(this->Scale == pqChartPixelScale::Logarithmic)
    {
    if(max < min)
      {
      this->ValueMin = max;
      this->ValueMax = min;
      }

    // Adjust the values that are close to zero if they are
    // below the minimum log value.
    if(this->ValueMin < 0)
      {
      if(this->ValueMax.getType() != pqChartValue::IntValue &&
          this->ValueMax > -pqChartPixelScale::MinLogValue)
        {
        this->ValueMax = -pqChartPixelScale::MinLogValue;
        if(this->ValueMin.getType() != pqChartValue::DoubleValue)
          {
          this->ValueMax.convertTo(pqChartValue::FloatValue);
          }
        }
      }
    else
      {
      if(this->ValueMin.getType() != pqChartValue::IntValue &&
          this->ValueMin < pqChartPixelScale::MinLogValue)
        {
        this->ValueMin = pqChartPixelScale::MinLogValue;
        if(this->ValueMax.getType() != pqChartValue::DoubleValue)
          {
          this->ValueMin.convertTo(pqChartValue::FloatValue);
          }
        }
      }
    }

  return prevMin != this->ValueMin || prevMax != this->ValueMax;
}

pqChartValue pqChartPixelScale::getValueRange() const
{
  return this->ValueMax - this->ValueMin;
}

bool pqChartPixelScale::setMinValue(const pqChartValue &min)
{
  return this->setValueRange(min, this->ValueMax);
}

bool pqChartPixelScale::setMaxValue(const pqChartValue &max)
{
  return this->setValueRange(this->ValueMin, max);
}

bool pqChartPixelScale::setPixelRange(int min, int max)
{
  if(this->PixelMin != min || this->PixelMax != max)
    {
    this->PixelMin = min;
    this->PixelMax = max;
    return true;
    }

  return false;
}

int pqChartPixelScale::getPixelRange() const
{
  // TODO: Is the true (max - min) ever needed?
  if(this->PixelMax > this->PixelMin)
    {
    return this->PixelMax - this->PixelMin;
    }
  else
    {
    return this->PixelMin - this->PixelMax;
    }
}

int pqChartPixelScale::getPixelFor(const pqChartValue &value) const
{
  // Convert the value to a pixel location using:
  // px = ((vx - v1)*(p2 - p1))/(v2 - v1) + p1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue result;
  pqChartValue valueRange;
  if(this->Scale == pqChartPixelScale::Logarithmic)
    {
    // If the value is less than the minimum log number, return
    // the minimum pixel value.
    bool reversed = this->ValueMin < 0;
    if(reversed)
      {
      if(value >= -pqChartPixelScale::MinLogValue)
        {
        return this->PixelMax;
        }
      }
    else
      {
      if(value <= pqChartPixelScale::MinLogValue)
        {
        return this->PixelMin;
        }
      }

    // If the log scale uses integers, the first value may be zero.
    // In that case, use -1 instead of taking the log of zero.
    pqChartValue v1;
    if(this->ValueMin.getType() == pqChartValue::IntValue &&
        this->ValueMin == 0)
      {
      v1 = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        v1 = log10(-this->ValueMin.getDoubleValue());
        }
      else
        {
        v1 = log10(this->ValueMin.getDoubleValue());
        }
      }

    if(this->ValueMin.getType() == pqChartValue::IntValue &&
        this->ValueMax == 0)
      {
      valueRange = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        valueRange = log10(-this->ValueMax.getDoubleValue());
        }
      else
        {
        valueRange = log10(this->ValueMax.getDoubleValue());
        }
      }

    if(reversed)
      {
      result = log10(-value.getDoubleValue());
      }
    else
      {
      result = log10(value.getDoubleValue());
      }

    result -= v1;
    valueRange -= v1;
    }
  else
    {
    result = value - this->ValueMin;
    valueRange = this->ValueMax - this->ValueMin;
    }

  result *= this->PixelMax - this->PixelMin;
  if(valueRange != 0)
    {
    result /= valueRange;
    }

  return result.getIntValue() + this->PixelMin;
}

pqChartValue pqChartPixelScale::getValueFor(int pixel) const
{
  // Convert the pixel location to a value using:
  // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue v1;
  pqChartValue result;
  bool reversed = false;
  if(this->Scale == pqChartPixelScale::Logarithmic)
    {
    // If the log scale uses integers, the first value may be zero.
    // In that case, use -1 instead of taking the log of zero.
    reversed = this->ValueMin < 0;
    if(this->ValueMin.getType() == pqChartValue::IntValue &&
        this->ValueMin == 0)
      {
      v1 = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        v1 = log10(-this->ValueMin.getDoubleValue());
        }
      else
        {
        v1 = log10(this->ValueMin.getDoubleValue());
        }
      }

    if(this->ValueMin.getType() == pqChartValue::IntValue &&
        this->ValueMax == 0)
      {
      result = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        result = log10(-this->ValueMax.getDoubleValue());
        }
      else
        {
        result = log10(this->ValueMax.getDoubleValue());
        }
      }

    result -= v1;
    }
  else
    {
    v1 = this->ValueMin;
    result = this->ValueMax - this->ValueMin;
    }

  result *= pixel - this->PixelMin;
  int pixelRange = this->PixelMax - this->PixelMin;
  if(pixelRange != 0)
    {
    result /= pixelRange;
    }

  result += v1;
  if(this->Scale == pqChartPixelScale::Logarithmic)
    {
    result = pow((double)10.0, result.getDoubleValue());
    if(reversed)
      {
      result *= -1;
      }
    if(this->ValueMin.getType() != pqChartValue::DoubleValue)
      {
      result.convertTo(pqChartValue::FloatValue);
      }
    }

  return result;
}

bool pqChartPixelScale::isValid() const
{
  if(this->ValueMax == this->ValueMin)
    return false;
  if(this->PixelMax == this->PixelMin)
    return false;
  return true;
}

bool pqChartPixelScale::isZeroInRange() const
{
  return (this->ValueMax >= 0 && this->ValueMin <= 0) ||
      (this->ValueMax <= 0 && this->ValueMin >= 0);
}


