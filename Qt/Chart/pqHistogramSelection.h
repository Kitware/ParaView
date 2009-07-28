/*=========================================================================

   Program: ParaView
   Module:    pqHistogramSelection.h

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
 * \file pqHistogramSelection.h
 *
 * \brief
 *   The pqHistogramSelection and pqHistogramSelectionList classes are
 *   used to define the selection for a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   June 2, 2005
 */

#ifndef _pqHistogramSelection_h
#define _pqHistogramSelection_h

#include "QtChartExport.h"
#include "pqChartValue.h" // Needed for first and second members.


/// \class pqHistogramSelection
/// \brief
///   The pqHistogramSelection class is used to store a selection range.
///
/// The selection range includes the first and last values. The
/// selection type is used to distinguish between bin indexes and
/// integer value types.
class QTCHART_EXPORT pqHistogramSelection
{
public:
  enum SelectionType {
    None,  ///< No selection type has been defined.
    Value, ///< The selection consists of values.
    Bin    ///< The selection consists of bin indexes.
  };

public:
  /// Creates an empty selection object.
  pqHistogramSelection();

  /// \brief
  ///   Creates a copy of another selection object.
  /// \param other The selection to copy.
  pqHistogramSelection(const pqHistogramSelection &other);

  /// \brief
  ///   Creates a selection object with a given range.
  ///
  /// The \c first and \c second values will be included in the
  /// selection range.
  ///
  /// \param first The beginning of the selection range.
  /// \param second The end of the selection range.
  pqHistogramSelection(const pqChartValue &first, const pqChartValue &second);
  virtual ~pqHistogramSelection() {}

  /// \brief
  ///   Sets the selection type.
  /// \param type The new selection type.
  void setType(SelectionType type) {this->Type = type;}

  /// \brief
  ///   Gets the selection type.
  /// \return
  ///   The selection type.
  SelectionType getType() const {return this->Type;}

  /// Switches the direction of the selection range.
  void reverse();

  /// \brief
  ///   Adjusts the selection range to be within the limits.
  /// \param min The minimum selection value allowed.
  /// \param max The maximum selection value allowed.
  void adjustRange(const pqChartValue &min, const pqChartValue &max);

  /// \brief
  ///   Adds the offset to both ends of the range.
  /// \param offset The amount to move the selection range.
  void moveRange(const pqChartValue &offset);

  /// \brief
  ///   Sets the selection range.
  ///
  /// The \c first and \c last values will be included in the
  /// selection range.
  ///
  /// \param first The beginning of the selection range.
  /// \param last The end of the selection range.
  /// \sa pqHistogramSelection::setFirst(const pqChartValue &),
  ///     pqHistogramSelection::setSecond(const pqChartValue &)
  void setRange(const pqChartValue &first, const pqChartValue &last);

  /// \brief
  ///   Sets the beginning of the selection range.
  /// \param first The beginning of the selection range.
  /// \sa pqHistogramSelection::setRange(const pqChartValue &,
  ///       const pqChartValue &),
  ///     pqHistogramSelection::setSecond(const pqChartValue &)
  void setFirst(const pqChartValue &first) {this->First = first;}

  /// \brief
  ///   Sets the end of the selection range.
  /// \param second The end of the selection range.
  /// \sa pqHistogramSelection::setRange(const pqChartValue &,
  ///       const pqChartValue &),
  ///     pqHistogramSelection::setSecond(const pqChartValue &)
  void setSecond(const pqChartValue &second) {this->Second = second;}

  /// \brief
  ///   Gets the beginning of the selection range.
  /// \return
  ///   The beginning of the selection range.
  const pqChartValue &getFirst() const {return this->First;}

  /// \brief
  ///   Gets the end of the selection range.
  /// \return
  ///   The end of the selection range.
  const pqChartValue &getSecond() const {return this->Second;}

  /// \brief
  ///   Makes a copy of another selection.
  /// \param other The selection range to copy.
  /// \return
  ///   A reference to the object being assigned.
  pqHistogramSelection &operator=(const pqHistogramSelection &other);

  /// \brief
  ///   Tests whether two histogram selection ranges are equal.
  /// \param other The selection range to compare to this.
  /// \return
  ///   True if the two histogram selection ranges are equal.
  bool operator==(const pqHistogramSelection &other) const;

private:
  SelectionType Type;  ///< Stores the type of selection.
  pqChartValue First;  ///< Stores the beginning of the range.
  pqChartValue Second; ///< Stores the end of the range.
};

#endif
