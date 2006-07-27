/*=========================================================================

   Program: ParaView
   Module:    pqPixelTransferFunction.h

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

/// \file pqPixelTransferFunction.h
/// \date 7/25/2006

#ifndef _pqPixelTransferFunction_h
#define _pqPixelTransferFunction_h


#include "QtChartExport.h"
#include "pqChartValue.h" // Needed for min, max members.


/// \class pqPixelTransferFunction
class QTCHART_EXPORT pqPixelTransferFunction
{
public:
  enum ValueScale {
    Linear,
    Logarithmic
  };

public:
  pqPixelTransferFunction();
  ~pqPixelTransferFunction() {}

  /// \name Value Parameters
  //@{
  /// \brief
  ///   Sets the value range.
  /// \param min The minimum value.
  /// \param max The maximum value.
  void setValueRange(const pqChartValue &min, const pqChartValue &max);

  /// \brief
  ///   Gets the value range.
  /// \return 
  ///   The difference between the minimum and maximum values.
  pqChartValue getValueRange() const;

  /// \brief
  ///   Sets the minimum value.
  /// \param min The minimum value.
  void setMinValue(const pqChartValue &min);

  /// \brief
  ///   Gets the minimum value.
  /// \return
  ///   The minimum value.
  /// \sa pqPixelTransferFunction::getTrueMinValue()
  const pqChartValue &getMinValue() const {return this->ValueMin;}

  /// \brief
  ///   Sets the maximum value.
  /// \param max The maximum value.
  void setMaxValue(const pqChartValue &max);

  /// \brief
  ///   Gets the maximum value.
  /// \return
  ///   The maximum value.
  /// \sa pqPixelTransferFunction::getTrueMaxValue()
  const pqChartValue &getMaxValue() const {return this->ValueMax;}
  //@}

  /// \name Pixel Parameters
  //@{
  /// \brief
  ///   Sets the pixel range.
  /// \param min The minimum pixel location.
  /// \param max The maximum pixel location.
  void setPixelRange(int min, int max);

  /// \brief
  ///   Gets the pixel range.
  ///
  /// The pixel range is always returned as a positive number.
  ///
  /// \return 
  ///   The difference between the minimum and maximum pixels.
  int getPixelRange() const;

  /// \brief
  ///   Sets the minimum pixel location.
  /// \param min The minimum pixel location.
  void setMinPixel(int min);

  /// \brief
  ///   Gets the minimum pixel location.
  /// \return
  ///   The minimum pixel location.
  int getMinPixel() const {return this->PixelMin;}

  /// \brief
  ///   Sets the maximum pixel location.
  /// \param min The maximum pixel location.
  void setMaxPixel(int max);

  /// \brief
  ///   Gets the maximum pixel location.
  /// \return
  ///   The maximum pixel location.
  int getMaxPixel() const {return this->PixelMax;}
  //@}

  /// \name Pixel to Value Mapping
  //@{
  /// \brief
  ///   Maps a value to a pixel.
  /// \param value The data value.
  /// \return
  ///   The pixel location for the given value.
  /// \sa pqPixelTransferFunction::isValid(),
  ///     pqPixelTransferFunction::getValueFor(int)
  int getPixelFor(const pqChartValue &value) const;

  /// \brief
  ///   Maps a pixel to a value.
  /// \param pixel The pixel location.
  /// \return
  ///   The value equivalent to the pixel location.
  /// \sa pqPixelTransferFunction::isValid(),
  ///     pqPixelTransferFunction::getPixelFor(const pqChartValue &)
  pqChartValue getValueFor(int pixel);

  /// \brief
  ///   Used to determine if the pixel/value mapping is valid.
  /// \return
  ///   True if the pixel/value mapping is valid.
  bool isValid() const;

  /// \brief
  ///   Used to determine if zero is in the value range.
  /// \return
  ///   True if zero is in the value range.
  bool isZeroInRange() const;

  /// \brief
  ///   Sets the scale type.
  /// \param scale The new scale type.
  void setScaleType(ValueScale scale) {this->Scale = scale;}

  /// \brief
  ///   Gets the scale type.
  /// \return
  ///   The current scale type.
  ValueScale getScaleType() const {return this->Scale;}
  //@}

private:
  ValueScale Scale;      ///< Stores the scale type (linear or log10).
  pqChartValue ValueMin; ///< Stores the minimum value.
  pqChartValue ValueMax; ///< Stores the maximum value.
  int PixelMin;          ///< Stores the minimum pixel.
  int PixelMax;          ///< Stores the maximum pixel.

  static double MinLogValue; ///< Stores the log scale minimum.
};

#endif
