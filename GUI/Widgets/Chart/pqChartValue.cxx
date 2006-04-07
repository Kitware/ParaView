/*=========================================================================

   Program:   ParaQ
   Module:    pqChartValue.cxx

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

/*!
 * \file pqChartValue.cxx
 *
 * \brief
 *   The pqChartValue and pqChartValueList classes are used to define
 *   the data for a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   May 10, 2005
 */

#include "pqChartValue.h"
#include <vtkstd/vector>
#include <float.h>


/// \class pqChartValueIteratorData
/// \brief
///   The pqChartValueIteratorData class hides the private data of
///   the pqChartValueIterator class.
class pqChartValueIteratorData : public vtkstd::vector<pqChartValue>::iterator
{
public:
  pqChartValueIteratorData& operator=(
      const vtkstd::vector<pqChartValue>::iterator& iter)
    {
    vtkstd::vector<pqChartValue>::iterator::operator=(iter);
    return *this;
    }
};


/// \class pqChartValueConstIteratorData
/// \brief
///   The pqChartValueConstIteratorData class hides the private
///   data of the pqChartValueConstIterator class.
class pqChartValueConstIteratorData :
    public vtkstd::vector<pqChartValue>::const_iterator
{
public:
  pqChartValueConstIteratorData& operator=(
      const vtkstd::vector<pqChartValue>::iterator& iter)
    {
    vtkstd::vector<pqChartValue>::const_iterator::operator=(iter);
    return *this;
    }

  pqChartValueConstIteratorData& operator=(
      const vtkstd::vector<pqChartValue>::const_iterator& iter)
    {
    vtkstd::vector<pqChartValue>::const_iterator::operator=(iter);
    return *this;
    }
};


/// \class pqChartValueListData
/// \brief
///   The pqChartValueListData class hides the private data of the
///   pqChartValueList class.
class pqChartValueListData : public vtkstd::vector<pqChartValue> {};


pqChartValue::pqChartValue()
{
  this->Type = IntValue;
  this->Value.Int = 0;
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

QString pqChartValue::getString(int precision) const
{
  QString result;
  if(this->Type == IntValue)
    result.setNum(this->Value.Int);
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

    if(result2.length() < result.length())
      result = result2;
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


pqChartValueIterator::pqChartValueIterator()
{
  this->Data = new pqChartValueIteratorData();
}

pqChartValueIterator::pqChartValueIterator(const pqChartValueIterator &iter)
{
  this->Data = new pqChartValueIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqChartValueIterator::~pqChartValueIterator()
{
  if(this->Data)
    delete this->Data;
}

bool pqChartValueIterator::operator==(const pqChartValueIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqChartValueIterator::operator!=(const pqChartValueIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

bool pqChartValueIterator::operator==(
    const pqChartValueConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqChartValueIterator::operator!=(
    const pqChartValueConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

const pqChartValue &pqChartValueIterator::operator*() const
{
  return *(*this->Data);
}

pqChartValue &pqChartValueIterator::operator*()
{
  return *(*this->Data);
}

pqChartValue *pqChartValueIterator::operator->()
{
  if(this->Data)
    return &(*(*this->Data));
  return 0;
}

pqChartValueIterator &pqChartValueIterator::operator++()
{
  if(this->Data)
    ++(*this->Data);
  return *this;
}

pqChartValueIterator pqChartValueIterator::operator++(int)
{
  pqChartValueIterator result(*this);
  if(this->Data)
    ++(this->Data);
  return result;
}

pqChartValueIterator &pqChartValueIterator::operator=(
    const pqChartValueIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}


pqChartValueConstIterator::pqChartValueConstIterator()
{
  this->Data = new pqChartValueConstIteratorData();
}

pqChartValueConstIterator::pqChartValueConstIterator(
    const pqChartValueIterator &iter)
{
  this->Data = new pqChartValueConstIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqChartValueConstIterator::pqChartValueConstIterator(
    const pqChartValueConstIterator &iter)
{
  this->Data = new pqChartValueConstIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqChartValueConstIterator::~pqChartValueConstIterator()
{
  if(this->Data)
    delete this->Data;
}

bool pqChartValueConstIterator::operator==(
    const pqChartValueConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqChartValueConstIterator::operator!=(
    const pqChartValueConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

bool pqChartValueConstIterator::operator==(
    const pqChartValueIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqChartValueConstIterator::operator!=(
    const pqChartValueIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

const pqChartValue &pqChartValueConstIterator::operator*() const
{
  return *(*this->Data);
}

const pqChartValue *pqChartValueConstIterator::operator->() const
{
  if(this->Data)
    return &(*(*this->Data));
  return 0;
}

pqChartValueConstIterator &pqChartValueConstIterator::operator++()
{
  if(this->Data)
    ++(*this->Data);
  return *this;
}

pqChartValueConstIterator pqChartValueConstIterator::operator++(int)
{
  pqChartValueConstIterator result(*this);
  if(this->Data)
    ++(this->Data);
  return result;
}

pqChartValueConstIterator &pqChartValueConstIterator::operator=(
    const pqChartValueConstIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}

pqChartValueConstIterator &pqChartValueConstIterator::operator=(
    const pqChartValueIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}


pqChartValueList::pqChartValueList()
{
  this->Data = new pqChartValueListData();
}

pqChartValueList::~pqChartValueList()
{
  if(this->Data)
    delete this->Data;
}

pqChartValueList::Iterator pqChartValueList::begin()
{
  pqChartValueIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqChartValueList::Iterator pqChartValueList::end()
{
  pqChartValueIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

pqChartValueList::ConstIterator pqChartValueList::begin() const
{
  pqChartValueConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqChartValueList::ConstIterator pqChartValueList::end() const
{
  pqChartValueConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

pqChartValueList::ConstIterator pqChartValueList::constBegin() const
{
  pqChartValueConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqChartValueList::ConstIterator pqChartValueList::constEnd() const
{
  pqChartValueConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

bool pqChartValueList::isEmpty() const
{
  if(this->Data)
    return this->Data->size() == 0;
  return true;
}

int pqChartValueList::getSize() const
{
  if(this->Data)
    return static_cast<int>(this->Data->size());
  return 0;
}

void pqChartValueList::clear()
{
  if(this->Data)
    this->Data->clear();
}

void pqChartValueList::pushBack(const pqChartValue &value)
{
  if(this->Data)
    this->Data->push_back(value);
}

pqChartValueList &pqChartValueList::operator=(
    const pqChartValueList &list)
{
  this->clear();
  if(this->Data && list.Data)
    {
    vtkstd::vector<pqChartValue>::iterator iter = list.Data->begin();
    for( ; iter != list.Data->end(); iter++)
      this->Data->push_back(*iter);
    }

  return *this;
}

pqChartValueList &pqChartValueList::operator+=(
    const pqChartValueList &list)
{
  if(this->Data && list.Data)
    {
    vtkstd::vector<pqChartValue>::iterator iter = list.Data->begin();
    for( ; iter != list.Data->end(); iter++)
      this->Data->push_back(*iter);
    }

  return *this;
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


