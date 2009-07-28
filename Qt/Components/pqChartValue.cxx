/*=========================================================================

   Program: ParaView
   Module:    pqChartValue.cxx

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

/*!
 * \file pqChartValue.cxx
 *
 * \author Mark Richardson
 * \date   May 10, 2005
 */

#include "pqChartValue.h"
#include <float.h>
#include <math.h>


pqChartValue::pqChartValue()
{
  this->Type = IntValue;
  this->Value.Int = 0;
}

pqChartValue::~pqChartValue()
{
}

pqChartValue::pqChartValue(const pqChartValue &value)
{
  this->Type = value.Type;
  if(this->Type == pqChartValue::IntValue)
    this->Value.Int = value.Value.Int;
  else if(this->Type == pqChartValue::FloatValue)
    this->Value.Float = value.Value.Float;
  else
    this->Value.Double = value.Value.Double;
}

pqChartValue::pqChartValue(int value)
{
  this->setValue(value);
}

pqChartValue::pqChartValue(float value)
{
  this->setValue(value);
}

pqChartValue::pqChartValue(double value)
{
  this->setValue(value);
}

void pqChartValue::convertTo(ValueType type)
{
  if(this->Type != type)
    {
    if(type == pqChartValue::IntValue)
      this->Value.Int = getIntValue();
    else if(type == pqChartValue::FloatValue)
      this->Value.Float = getFloatValue();
    else
      this->Value.Double = getDoubleValue();
    this->Type = type;
    }
}

void pqChartValue::setValue(int value)
{
  this->Type = IntValue;
  this->Value.Int = value;
}

void pqChartValue::setValue(float value)
{
  this->Type = FloatValue;
  this->Value.Float = value;
}

void pqChartValue::setValue(double value)
{
  this->Type = DoubleValue;
  this->Value.Double = value;
}

int pqChartValue::getIntValue() const
{
  if(this->Type == IntValue)
    return this->Value.Int;
  else if(this->Type == FloatValue)
    return static_cast<int>(this->Value.Float);
  else
    return static_cast<int>(this->Value.Double);
}

float pqChartValue::getFloatValue() const
{
  if(this->Type == IntValue)
    return static_cast<float>(this->Value.Int);
  else if(this->Type == FloatValue)
    return this->Value.Float;
  else
    return static_cast<float>(this->Value.Double);
}

double pqChartValue::getDoubleValue() const
{
  if(this->Type == IntValue)
    return static_cast<double>(this->Value.Int);
  else if(this->Type == FloatValue)
    return static_cast<double>(this->Value.Float);
  else
    return this->Value.Double;
}

QString pqChartValue::getString(int precision,
    pqChartValue::NotationType notation) const
{
  QString result;
  int exponent = 0;
  if(this->Type == IntValue)
    {
    result.setNum(this->Value.Int);
    }
  else
    {
    QString result2;
    if(this->Type == FloatValue)
      {
      result.setNum(this->Value.Float, 'f', precision);
      result2.setNum(this->Value.Float, 'e', precision);
      }
    else
      {
      result.setNum(this->Value.Double, 'f', precision);
      result2.setNum(this->Value.Double, 'e', precision);
      }

    // Extract the exponent from the exponential result.
    exponent = result2.mid(result2.indexOf('e')+1,result2.length()-1).toInt();

    // Use the notation flag to determine which result to use.
    if(notation == pqChartValue::Engineering)
      {
      int offset = exponent%3;
      if(offset<0)
        {
        offset+=3;
        }

      // if using engineering notation we may be moving decimal to right
      // get a new string representation with increased precision
      if(this->Type == FloatValue)
        {
        result2.setNum(this->Value.Float, 'e', precision+offset);
        }
      else
        {
        result2.setNum(this->Value.Double, 'e', precision+offset);
        }

      if(offset!=0)
        {
        // The string is not already in engineering notation so...
        // decrease the exponent.
        exponent -= offset;
        int eIdx = result2.indexOf('e');
        QString exponentString;
        exponentString.setNum(exponent);

        // Add a plus sign to the exponent if needed.
        if(exponent > 0)
          {
          exponentString.insert(0, '+');
          }

        result2.replace(eIdx + 1,
            result2.mid(eIdx + 1, result2.length() - 1).length(),
            exponentString);

        // Move the decimal point to the right (there's guaranteed to
        // be one since offset is non-zero even if precison==0).
        int idx = result2.indexOf('.');
        result2.remove(idx,1);

        // Only insert if we have a non-zero precision.
        if(precision > 0)
          {
          result2.insert(idx+offset, '.');
          }
        }

      result = result2;
      }
    else if(notation == pqChartValue::Exponential)
      {
      // Use the exponential notation regardless of the length.
      result = result2;
      }
    else if(notation == pqChartValue::StandardOrExponential)
      {
      // Use the shorter notation in this case. If the exponent is
      // negative, the length of the standard representation will
      // always be shorter. In that case, always use exponential
      // notation for negative exponents below a certain threshold (-2).
      if(exponent < -2 || result2.length() < result.length())
        {
        result = result2;
        }
      }
    //else if(notation == pqChartValue::Standard) use result as is.
    }

  return result;
}

pqChartValue &pqChartValue::operator=(const pqChartValue &value)
{
  this->Type = value.Type;
  if(this->Type == IntValue)
    this->Value.Int = value.Value.Int;
  else if(this->Type == FloatValue)
    this->Value.Float = value.Value.Float;
  else
    this->Value.Double = value.Value.Double;
  return *this;
}

pqChartValue &pqChartValue::operator=(int value)
{
  this->setValue(value);
  return *this;
}

pqChartValue &pqChartValue::operator=(float value)
{
  this->setValue(value);
  return *this;
}

pqChartValue &pqChartValue::operator=(double value)
{
  this->setValue(value);
  return *this;
}

pqChartValue &pqChartValue::operator++()
{
  if(this->Type == pqChartValue::IntValue)
    this->Value.Int += 1;
  else if(this->Type == pqChartValue::FloatValue)
    this->Value.Float += FLT_EPSILON;
  else
    this->Value.Double += DBL_EPSILON;
  return *this;
}

pqChartValue pqChartValue::operator++(int)
{
  pqChartValue result(*this);
  if(this->Type == pqChartValue::IntValue)
    this->Value.Int += 1;
  else if(this->Type == pqChartValue::FloatValue)
    this->Value.Float += FLT_EPSILON;
  else
    this->Value.Double += DBL_EPSILON;
  return result;
}

pqChartValue &pqChartValue::operator--()
{
  if(this->Type == pqChartValue::IntValue)
    this->Value.Int -= 1;
  else if(this->Type == pqChartValue::FloatValue)
    this->Value.Float -= FLT_EPSILON;
  else
    this->Value.Double -= DBL_EPSILON;
  return *this;
}

pqChartValue pqChartValue::operator--(int)
{
  pqChartValue result(*this);
  if(this->Type == pqChartValue::IntValue)
    this->Value.Int -= 1;
  else if(this->Type == pqChartValue::FloatValue)
    this->Value.Float -= FLT_EPSILON;
  else
    this->Value.Double -= DBL_EPSILON;
  return result;
}

pqChartValue pqChartValue::operator+(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this + value.getIntValue();
  else if(value.Type == FloatValue)
    return *this + value.getFloatValue();
  else
    return *this + value.getDoubleValue();
}

pqChartValue pqChartValue::operator+(int value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int + value);
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float + static_cast<float>(value));
  else
    return pqChartValue(this->Value.Double + static_cast<double>(value));
}

pqChartValue pqChartValue::operator+(float value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int + static_cast<int>(value));
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float + value);
  else
    return pqChartValue(this->Value.Double + static_cast<double>(value));
}

pqChartValue pqChartValue::operator+(double value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int + static_cast<int>(value));
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float + static_cast<float>(value));
  else
    return pqChartValue(this->Value.Double + value);
}

pqChartValue pqChartValue::operator-(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this - value.getIntValue();
  else if(value.Type == FloatValue)
    return *this - value.getFloatValue();
  else
    return *this - value.getDoubleValue();
}

pqChartValue pqChartValue::operator-(int value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int - value);
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float - static_cast<float>(value));
  else
    return pqChartValue(this->Value.Double - static_cast<double>(value));
}

pqChartValue pqChartValue::operator-(float value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int - static_cast<int>(value));
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float - value);
  else
    return pqChartValue(this->Value.Double - static_cast<double>(value));
}

pqChartValue pqChartValue::operator-(double value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int - static_cast<int>(value));
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float - static_cast<float>(value));
  else
    return pqChartValue(this->Value.Double - value);
}

pqChartValue pqChartValue::operator*(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this * value.getIntValue();
  else if(value.Type == FloatValue)
    return *this * value.getFloatValue();
  else
    return *this * value.getDoubleValue();
}

pqChartValue pqChartValue::operator*(int value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int * value);
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float * static_cast<float>(value));
  else
    return pqChartValue(this->Value.Double * static_cast<double>(value));
}

pqChartValue pqChartValue::operator*(float value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int * static_cast<int>(value));
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float * value);
  else
    return pqChartValue(this->Value.Double * static_cast<double>(value));
}

pqChartValue pqChartValue::operator*(double value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int * static_cast<int>(value));
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float * static_cast<float>(value));
  else
    return pqChartValue(this->Value.Double * value);
}

pqChartValue pqChartValue::operator/(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this / value.getIntValue();
  else if(value.Type == FloatValue)
    return *this / value.getFloatValue();
  else
    return *this / value.getDoubleValue();
}

pqChartValue pqChartValue::operator/(int value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int / value);
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float / static_cast<float>(value));
  else
    return pqChartValue(this->Value.Double / static_cast<double>(value));
}

pqChartValue pqChartValue::operator/(float value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int / static_cast<int>(value));
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float / value);
  else
    return pqChartValue(this->Value.Double / static_cast<double>(value));
}

pqChartValue pqChartValue::operator/(double value) const
{
  if(this->Type == IntValue)
    return pqChartValue(this->Value.Int / static_cast<int>(value));
  else if(this->Type == FloatValue)
    return pqChartValue(this->Value.Float / static_cast<float>(value));
  else
    return pqChartValue(this->Value.Double / value);
}

pqChartValue &pqChartValue::operator+=(const pqChartValue &value)
{
  if(value.Type == IntValue)
    return *this += value.getIntValue();
  else if(value.Type == FloatValue)
    return *this += value.getFloatValue();
  else
    return *this += value.getDoubleValue();
}

pqChartValue &pqChartValue::operator+=(int value)
{
  if(this->Type == IntValue)
    this->Value.Int += value;
  else if(this->Type == FloatValue)
    this->Value.Float += static_cast<float>(value);
  else
    this->Value.Double += static_cast<double>(value);
  return *this;
}

pqChartValue &pqChartValue::operator+=(float value)
{
  if(this->Type == IntValue)
    this->Value.Int += static_cast<int>(value);
  else if(this->Type == FloatValue)
    this->Value.Float += value;
  else
    this->Value.Double += static_cast<double>(value);
  return *this;
}

pqChartValue &pqChartValue::operator+=(double value)
{
  if(this->Type == IntValue)
    this->Value.Int += static_cast<int>(value);
  else if(this->Type == FloatValue)
    this->Value.Float += static_cast<float>(value);
  else
    this->Value.Double += value;
  return *this;
}

pqChartValue &pqChartValue::operator-=(const pqChartValue &value)
{
  if(value.Type == IntValue)
    return *this -= value.getIntValue();
  else if(value.Type == FloatValue)
    return *this -= value.getFloatValue();
  else
    return *this -= value.getDoubleValue();
}

pqChartValue &pqChartValue::operator-=(int value)
{
  if(this->Type == IntValue)
    this->Value.Int -= value;
  else if(this->Type == FloatValue)
    this->Value.Float -= static_cast<float>(value);
  else
    this->Value.Double -= static_cast<double>(value);
  return *this;
}

pqChartValue &pqChartValue::operator-=(float value)
{
  if(this->Type == IntValue)
    this->Value.Int -= static_cast<int>(value);
  else if(this->Type == FloatValue)
    this->Value.Float -= value;
  else
    this->Value.Double -= static_cast<double>(value);
  return *this;
}

pqChartValue &pqChartValue::operator-=(double value)
{
  if(this->Type == IntValue)
    this->Value.Int -= static_cast<int>(value);
  else if(this->Type == FloatValue)
    this->Value.Float -= static_cast<float>(value);
  else
    this->Value.Double -= value;
  return *this;
}

pqChartValue &pqChartValue::operator*=(const pqChartValue &value)
{
  if(value.Type == IntValue)
    return *this *= value.getIntValue();
  else if(value.Type == FloatValue)
    return *this *= value.getFloatValue();
  else
    return *this *= value.getDoubleValue();
}

pqChartValue &pqChartValue::operator*=(int value)
{
  if(this->Type == IntValue)
    this->Value.Int *= value;
  else if(this->Type == FloatValue)
    this->Value.Float *= static_cast<float>(value);
  else
    this->Value.Double *= static_cast<double>(value);
  return *this;
}

pqChartValue &pqChartValue::operator*=(float value)
{
  if(this->Type == IntValue)
    this->Value.Int *= static_cast<int>(value);
  else if(this->Type == FloatValue)
    this->Value.Float *= value;
  else
    this->Value.Double *= static_cast<double>(value);
  return *this;
}

pqChartValue &pqChartValue::operator*=(double value)
{
  if(this->Type == IntValue)
    this->Value.Int *= static_cast<int>(value);
  else if(this->Type == FloatValue)
    this->Value.Float *= static_cast<float>(value);
  else
    this->Value.Double *= value;
  return *this;
}

pqChartValue &pqChartValue::operator/=(const pqChartValue &value)
{
  if(value.Type == IntValue)
    return *this /= value.getIntValue();
  else if(value.Type == FloatValue)
    return *this /= value.getFloatValue();
  else
    return *this /= value.getDoubleValue();
}

pqChartValue &pqChartValue::operator/=(int value)
{
  if(this->Type == IntValue)
    this->Value.Int /= value;
  else if(this->Type == FloatValue)
    this->Value.Float /= static_cast<float>(value);
  else
    this->Value.Double /= static_cast<double>(value);
  return *this;
}

pqChartValue &pqChartValue::operator/=(float value)
{
  if(this->Type == IntValue)
    this->Value.Int /= static_cast<int>(value);
  else if(this->Type == FloatValue)
    this->Value.Float /= value;
  else
    this->Value.Double /= static_cast<double>(value);
  return *this;
}

pqChartValue &pqChartValue::operator/=(double value)
{
  if(this->Type == IntValue)
    this->Value.Int /= static_cast<int>(value);
  else if(this->Type == FloatValue)
    this->Value.Float /= static_cast<float>(value);
  else
    this->Value.Double /= value;
  return *this;
}

bool pqChartValue::operator==(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this == value.getIntValue();
  else if(value.Type == FloatValue)
    return *this == value.getFloatValue();
  else
    return *this == value.getDoubleValue();
}

bool pqChartValue::operator==(int value) const
{
  if(this->Type == IntValue)
    return this->Value.Int == value;
  else if(this->Type == FloatValue)
    return this->Value.Float == static_cast<float>(value);
  else
    return this->Value.Double == static_cast<double>(value);
}

bool pqChartValue::operator==(float value) const
{
  if(this->Type == IntValue)
    return this->Value.Int == static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float == value;
  else
    return this->Value.Double == static_cast<double>(value);
}

bool pqChartValue::operator==(double value) const
{
  if(this->Type == IntValue)
    return this->Value.Int == static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float == static_cast<float>(value);
  else
    return this->Value.Double == value;
}

bool pqChartValue::operator!=(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this != value.getIntValue();
  else if(value.Type == FloatValue)
    return *this != value.getFloatValue();
  else
    return *this != value.getDoubleValue();
}

bool pqChartValue::operator!=(int value) const
{
  if(this->Type == IntValue)
    return this->Value.Int != value;
  else if(this->Type == FloatValue)
    return this->Value.Float != static_cast<float>(value);
  else
    return this->Value.Double != static_cast<double>(value);
}

bool pqChartValue::operator!=(float value) const
{
  if(this->Type == IntValue)
    return this->Value.Int != static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float != value;
  else
    return this->Value.Double != static_cast<double>(value);
}

bool pqChartValue::operator!=(double value) const
{
  if(this->Type == IntValue)
    return this->Value.Int != static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float != static_cast<float>(value);
  else
    return this->Value.Double != value;
}

bool pqChartValue::operator>(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this > value.getIntValue();
  else if(value.Type == FloatValue)
    return *this > value.getFloatValue();
  else
    return *this > value.getDoubleValue();
}

bool pqChartValue::operator>(int value) const
{
  if(this->Type == IntValue)
    return this->Value.Int > value;
  else if(this->Type == FloatValue)
    return this->Value.Float > static_cast<float>(value);
  else
    return this->Value.Double > static_cast<double>(value);
}

bool pqChartValue::operator>(float value) const
{
  if(this->Type == IntValue)
    return this->Value.Int > static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float > value;
  else
    return this->Value.Double > static_cast<double>(value);
}

bool pqChartValue::operator>(double value) const
{
  if(this->Type == IntValue)
    return this->Value.Int > static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float > static_cast<float>(value);
  else
    return this->Value.Double > value;
}

bool pqChartValue::operator<(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this < value.getIntValue();
  else if(value.Type == FloatValue)
    return *this < value.getFloatValue();
  else
    return *this < value.getDoubleValue();
}

bool pqChartValue::operator<(int value) const
{
  if(this->Type == IntValue)
    return this->Value.Int < value;
  else if(this->Type == FloatValue)
    return this->Value.Float < static_cast<float>(value);
  else
    return this->Value.Double < static_cast<double>(value);
}

bool pqChartValue::operator<(float value) const
{
  if(this->Type == IntValue)
    return this->Value.Int < static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float < value;
  else
    return this->Value.Double < static_cast<double>(value);
}

bool pqChartValue::operator<(double value) const
{
  if(this->Type == IntValue)
    return this->Value.Int < static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float < static_cast<float>(value);
  else
    return this->Value.Double < value;
}

bool pqChartValue::operator>=(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this >= value.getIntValue();
  else if(value.Type == FloatValue)
    return *this >= value.getFloatValue();
  else
    return *this >= value.getDoubleValue();
}

bool pqChartValue::operator>=(int value) const
{
  if(this->Type == IntValue)
    return this->Value.Int >= value;
  else if(this->Type == FloatValue)
    return this->Value.Float >= static_cast<float>(value);
  else
    return this->Value.Double >= static_cast<double>(value);
}

bool pqChartValue::operator>=(float value) const
{
  if(this->Type == IntValue)
    return this->Value.Int >= static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float >= value;
  else
    return this->Value.Double >= static_cast<double>(value);
}

bool pqChartValue::operator>=(double value) const
{
  if(this->Type == IntValue)
    return this->Value.Int >= static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float >= static_cast<float>(value);
  else
    return this->Value.Double >= value;
}

bool pqChartValue::operator<=(const pqChartValue &value) const
{
  if(value.Type == IntValue)
    return *this <= value.getIntValue();
  else if(value.Type == FloatValue)
    return *this <= value.getFloatValue();
  else
    return *this <= value.getDoubleValue();
}

bool pqChartValue::operator<=(int value) const
{
  if(this->Type == IntValue)
    return this->Value.Int <= value;
  else if(this->Type == FloatValue)
    return this->Value.Float <= static_cast<float>(value);
  else
    return this->Value.Double <= static_cast<double>(value);
}

bool pqChartValue::operator<=(float value) const
{
  if(this->Type == IntValue)
    return this->Value.Int <= static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float <= value;
  else
    return this->Value.Double <= static_cast<double>(value);
}

bool pqChartValue::operator<=(double value) const
{
  if(this->Type == IntValue)
    return this->Value.Int <= static_cast<int>(value);
  else if(this->Type == FloatValue)
    return this->Value.Float <= static_cast<float>(value);
  else
    return this->Value.Double <= value;
}


int operator+(int value1, const pqChartValue &value2)
{
  return value1 + value2.getIntValue();
}

float operator+(float value1, const pqChartValue &value2)
{
  return value1 + value2.getFloatValue();
}

double operator+(double value1, const pqChartValue &value2)
{
  return value1 + value2.getDoubleValue();
}

int operator-(int value1, const pqChartValue &value2)
{
  return value1 - value2.getIntValue();
}

float operator-(float value1, const pqChartValue &value2)
{
  return value1 - value2.getFloatValue();
}

double operator-(double value1, const pqChartValue &value2)
{
  return value1 - value2.getDoubleValue();
}

int operator*(int value1, const pqChartValue &value2)
{
  return value1 * value2.getIntValue();
}

float operator*(float value1, const pqChartValue &value2)
{
  return value1 * value2.getFloatValue();
}

double operator*(double value1, const pqChartValue &value2)
{
  return value1 * value2.getDoubleValue();
}

int operator/(int value1, const pqChartValue &value2)
{
  return value1 / value2.getIntValue();
}

float operator/(float value1, const pqChartValue &value2)
{
  return value1 / value2.getFloatValue();
}

double operator/(double value1, const pqChartValue &value2)
{
  return value1 / value2.getDoubleValue();
}

bool operator==(int value1, const pqChartValue &value2)
{
  return value1 == value2.getIntValue();
}

bool operator==(float value1, const pqChartValue &value2)
{
  return value1 == value2.getFloatValue();
}

bool operator==(double value1, const pqChartValue &value2)
{
  return value1 == value2.getDoubleValue();
}

bool operator!=(int value1, const pqChartValue &value2)
{
  return value1 != value2.getIntValue();
}

bool operator!=(float value1, const pqChartValue &value2)
{
  return value1 != value2.getFloatValue();
}

bool operator!=(double value1, const pqChartValue &value2)
{
  return value1 != value2.getDoubleValue();
}

bool operator>(int value1, const pqChartValue &value2)
{
  return value1 > value2.getIntValue();
}

bool operator>(float value1, const pqChartValue &value2)
{
  return value1 > value2.getFloatValue();
}

bool operator>(double value1, const pqChartValue &value2)
{
  return value1 > value2.getDoubleValue();
}

bool operator<(int value1, const pqChartValue &value2)
{
  return value1 < value2.getIntValue();
}

bool operator<(float value1, const pqChartValue &value2)
{
  return value1 < value2.getFloatValue();
}

bool operator<(double value1, const pqChartValue &value2)
{
  return value1 < value2.getDoubleValue();
}

bool operator>=(int value1, const pqChartValue &value2)
{
  return value1 >= value2.getIntValue();
}

bool operator>=(float value1, const pqChartValue &value2)
{
  return value1 >= value2.getFloatValue();
}

bool operator>=(double value1, const pqChartValue &value2)
{
  return value1 >= value2.getDoubleValue();
}

bool operator<=(int value1, const pqChartValue &value2)
{
  return value1 <= value2.getIntValue();
}

bool operator<=(float value1, const pqChartValue &value2)
{
  return value1 <= value2.getFloatValue();
}

bool operator<=(double value1, const pqChartValue &value2)
{
  return value1 <= value2.getDoubleValue();
}


