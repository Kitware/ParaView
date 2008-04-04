/*=========================================================================

   Program: ParaView
   Module:    pqChartCoordinate.h

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
 * \file pqChartCoordinate.h
 *
 * \brief
 *   The pqChartCoordinate class stores an x, y coordinate for a chart.
 *
 * \author Mark Richardson
 * \date   August 19, 2005
 */

#ifndef _pqChartCoordinate_h
#define _pqChartCoordinate_h

#include "QtChartExport.h"
#include "pqChartValue.h" // Needed for x and y members.


/// \class pqChartCoordinate
/// \brief
///   The pqChartCoordinate class is used to store a pair of values.
///
/// The x and y coordinates are stored as pqChartValue objects, which
/// allows any combination of int, float, or double. This is useful
/// when the chart axes use different types. If the axis uses a list
/// of string labels, the coordinate should use an int for the index.
class QTCHART_EXPORT pqChartCoordinate
{
public:
  pqChartCoordinate();

  /// \brief
  ///   Creates a coordinate from two pqChartValue objects.
  /// \param px The x coordinate of the point.
  /// \param py The y coordinate of the point.
  pqChartCoordinate(const pqChartValue &px, const pqChartValue &py);

  /// \brief
  ///   Makes a copy of an existing coordinate.
  /// \param other The coordinate to copy.
  pqChartCoordinate(const pqChartCoordinate &other);
  ~pqChartCoordinate() {}

  /// \name Operators
  //@{
  bool operator==(const pqChartCoordinate &other) const;
  bool operator!=(const pqChartCoordinate &other) const;
  pqChartCoordinate &operator=(const pqChartCoordinate &other);
  //@}

public:
  pqChartValue X; ///< Stores the x coordinate.
  pqChartValue Y; ///< Stores the y coordinate.
};

#endif
