/*=========================================================================

   Program: ParaView
   Module:    pqPixelTransferFunction.cxx

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

/// \file pqPixelTransferFunction.cxx
/// \date 7/25/2006

#include "pqPixelTransferFunction.h"

#include <math.h>


static double MinIntLogPower = -1;
double pqPixelTransferFunction::MinLogValue = 0.0001;


pqPixelTransferFunction::pqPixelTransferFunction()
  : ValueMin(), ValueMax()
{
  this->PixelMin = 0;
  this->PixelMax = 0;
}

void pqPixelTransferFunction::setValueRange(const pqChartValue &min,
    const pqChartValue &max)
{
  this->ValueMin = min;
  this->ValueMax = max;
  if(this->Scale == pqPixelTransferFunction::Logarithmic)
    {
    // A logarithmic scale axis cannot contain zero because it is
    // undefined. If the range includes zero, set the scale to linear.
    if((min < 0 && max > 0) || (max < 0 && min > 0))
      {
      this->Scale = pqPixelTransferFunction::Linear;
      }
    }

  if(this->Scale == pqPixelTransferFunction::Logarithmic)
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
          this->ValueMax > -pqPixelTransferFunction::MinLogValue)
        {
        this->ValueMax = -pqPixelTransferFunction::MinLogValue;
        if(this->ValueMin.getType() != pqChartValue::DoubleValue)
          {
          this->ValueMax.convertTo(pqChartValue::FloatValue);
          }
        }
      }
    else
      {
      if(this->ValueMin.getType() != pqChartValue::IntValue &&
          this->ValueMin < pqPixelTransferFunction::MinLogValue)
        {
        this->ValueMin = pqPixelTransferFunction::MinLogValue;
        if(this->ValueMax.getType() != pqChartValue::DoubleValue)
          {
          this->ValueMin.convertTo(pqChartValue::FloatValue);
          }
        }
      }
    }
}

pqChartValue pqPixelTransferFunction::getValueRange() const
{
  return this->ValueMax - this->ValueMin;
}

void pqPixelTransferFunction::setMinValue(const pqChartValue &min)
{
  this->setValueRange(min, this->ValueMax);
}

void pqPixelTransferFunction::setMaxValue(const pqChartValue &max)
{
  this->setValueRange(this->ValueMin, max);
}

void pqPixelTransferFunction::setPixelRange(int min, int max)
{
  if(min > max)
    {
    this->setMinPixel(max);
    this->setMaxPixel(min);
    }
  else
    {
    this->setMinPixel(min);
    this->setMaxPixel(max);
    }
}

int pqPixelTransferFunction::getPixelRange() const
{
  if(this->PixelMax > this->PixelMin)
    return this->PixelMax - this->PixelMin;
  else
    return this->PixelMin - this->PixelMax;
}

void pqPixelTransferFunction::setMinPixel(int min)
{
  if(min < 0)
    {
    this->PixelMin = 0;
    }
  else
    {
    this->PixelMin = min;
    }
}

void pqPixelTransferFunction::setMaxPixel(int max)
{
  if(max < 0)
    {
    this->PixelMax = 0;
    }
  else
    {
    this->PixelMax = max;
    }
}

int pqPixelTransferFunction::getPixelFor(const pqChartValue &value) const
{
  // Convert the value to a pixel location using:
  // px = ((vx - v1)*(p2 - p1))/(v2 - v1) + p1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue result;
  pqChartValue valueRange;
  if(this->Scale == pqPixelTransferFunction::Logarithmic)
    {
    // If the value is less than the minimum log number, return
    // the minimum pixel value.
    bool reversed = this->ValueMin < 0;
    if(reversed)
      {
      if(value >= -pqPixelTransferFunction::MinLogValue)
        {
        return this->PixelMax;
        }
      }
    else
      {
      if(value <= pqPixelTransferFunction::MinLogValue)
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

pqChartValue pqPixelTransferFunction::getValueFor(int pixel)
{
  // Convert the pixel location to a value using:
  // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue v1;
  pqChartValue result;
  bool reversed = false;
  if(this->Scale == pqPixelTransferFunction::Logarithmic)
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
  if(this->Scale == pqPixelTransferFunction::Logarithmic)
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

bool pqPixelTransferFunction::isValid() const
{
  if(this->ValueMax == this->ValueMin)
    return false;
  if(this->PixelMax == this->PixelMin)
    return false;
  return true;
}

bool pqPixelTransferFunction::isZeroInRange() const
{
  return (this->ValueMax >= 0 && this->ValueMin <= 0) ||
      (this->ValueMax <= 0 && this->ValueMin >= 0);
}


