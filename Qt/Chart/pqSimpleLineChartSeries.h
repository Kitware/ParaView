/*=========================================================================

   Program: ParaView
   Module:    pqSimpleLineChartSeries.h

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

#ifndef _pqSimpleLineChartSeries_h
#define _pqSimpleLineChartSeries_h

#include "QtChartExport.h"
#include "pqLineChartSeries.h"

class pqChartCoordinate;
class pqChartValue;
class pqSimpleLineChartSeriesInternal;

/// \class pqSimpleLineChartSeries
/// \brief
///   The pqSimpleLineChartSeries class is a generic line chart series.
///
/// The simple line chart series can be used to make a scatter plot, a
/// line plot, or an error line plot. It stores each sequence in a
/// separate list and doesn't take advantage of the multi-sequence
/// change feature.
class QTCHART_EXPORT pqSimpleLineChartSeries : public pqLineChartSeries
{
public:
  /// \brief
  ///   Creates a line chart series object.
  /// \param parent The parent object.
  pqSimpleLineChartSeries(QObject *parent=0);
  virtual ~pqSimpleLineChartSeries();

  /// \name pqLineChartSeries Methods
  //@{
  virtual int getNumberOfSequences() const;
  virtual int getTotalNumberOfPoints() const;
  virtual pqLineChartSeries::SequenceType getSequenceType(int sequence) const;
  virtual int getNumberOfPoints(int sequence) const;
  virtual bool getPoint(int sequence, int index,
      pqChartCoordinate &coord) const;
  virtual void getErrorBounds(int sequence, int index, pqChartValue &upper,
      pqChartValue &lower) const;
  virtual void getErrorWidth(int sequence, pqChartValue &width) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;
  //@}

  /// \name Data Setup Methods
  //@{
  /// Clears all the point sequences from the series.
  void clearSeries();

  /// \brief
  ///   Adds a new sequence to the series.
  /// \param type The type of sequence to create.
  void addSequence(pqLineChartSeries::SequenceType type);

  /// \brief
  ///   Inserts a new sequence in the series list.
  /// \param index Where to insert the sequence.
  /// \param type The type of sequence to create.
  void insertSequence(int index, pqLineChartSeries::SequenceType type);

  /// \brief
  ///   Sets the type for the given sequence.
  /// \param sequence The index of the sequence to change.
  /// \param type The type of sequence.
  void setSequenceType(int sequence, pqLineChartSeries::SequenceType type);

  /// \brief
  ///   Removes a sequence from the series.
  /// \param sequence The index of the sequence to remove.
  void removeSequence(int sequence);

  /// \brief
  ///   Copies the point data from one sequence to another.
  /// \param source The index of the source sequence.
  /// \param destination The index of the destination sequence.
  void copySequencePoints(int source, int destination);

  /// \brief
  ///   Adds a point to the given sequence.
  /// \param sequence The index of the sequence.
  /// \param coord The coordinates of the new point.
  void addPoint(int sequence, const pqChartCoordinate &coord);

  /// \brief
  ///   Inserts a point in the given sequence.
  /// \param sequence The index of the sequence.
  /// \param index Where to insert the point.
  /// \param coord The coordinates of the new point.
  void insertPoint(int sequence, int index, const pqChartCoordinate &coord);

  /// \brief
  ///   Removes the indicated point from the given sequence.
  /// \param sequence The index of the sequence.
  /// \param index The index of the point to remove.
  void removePoint(int sequence, int index);

  /// \brief
  ///   Removes all the points for a given sequence.
  /// \param sequence The index of the sequence to clear.
  void clearPoints(int sequence);

  /// \brief
  ///   Sets the error bounds for an error sequence point.
  /// \param sequence The index of the sequence.
  /// \param index The index of the point in the sequence.
  /// \param upper The upper error bound.
  /// \param lower The lower error bound.
  /// \sa pqLineChartSeries::getErrorBounds(int, int, pqChartValue &,
  ///         pqChartValue &)
  void setErrorBounds(int sequence, int index, const pqChartValue &upper,
      const pqChartValue &lower);

  /// \brief
  ///   Sets the error bar width for a sequence.
  /// \param sequence The index of the sequence.
  /// \param width The width of the error bar from the vertical line
  ///   to the end.
  /// \sa pqLineChartSeries::getErrorWidth(int, pqChartValue &)
  void setErrorWidth(int sequence, const pqChartValue &width);
  //@}

private:
  /// Compiles the overall series range from the points.
  void updateSeriesRanges();

  /// \brief
  ///   Updates the series range after a point addition or insertion.
  ///
  /// This method can only be called after a adding a point to the
  /// series. In this case, the series ranges can only grow not shrink.
  ///
  /// \param coord The newly added point.
  void updateSeriesRanges(const pqChartCoordinate &coord);

private:
  pqSimpleLineChartSeriesInternal *Internal; ///< Stores the series data.
};

#endif
