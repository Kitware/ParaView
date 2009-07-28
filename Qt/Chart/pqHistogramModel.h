/*=========================================================================

   Program: ParaView
   Module:    pqHistogramModel.h

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

/// \file pqHistogramModel.h
/// \date 8/15/2006

#ifndef _pqHistogramModel_h
#define _pqHistogramModel_h


#include "QtChartExport.h"
#include <QObject>

class pqChartValue;


/// \class pqHistogramModel
/// \brief
///   The pqHistogramModel class is the histogram chart's interface
///   to the histogram data.
class QTCHART_EXPORT pqHistogramModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a histogram model object.
  /// \param parent The parent object.
  pqHistogramModel(QObject *parent=0);
  virtual ~pqHistogramModel() {}

  /// \name Histogram Data Methods
  //@{
  /// \brief
  ///   Gets the number of bins in the model.
  /// \return
  ///   The number of bins in the model.
  virtual int getNumberOfBins() const=0;

  /// \brief
  ///   Gets the value for a particular bin.
  /// \param index The index of the bin.
  /// \param bin Used to return the bin value.
  virtual void getBinValue(int index, pqChartValue &bin) const=0;

  /// \brief
  ///   Gets the x-axis range for the particular bin.
  /// \param index The index of the bin.
  /// \param min Used to return the minumum.
  /// \param max Used to return the maxumum.
  virtual void getBinRange(int index, pqChartValue &min,
      pqChartValue &max) const=0;
  //@}

  /// \name Histogram Range Methods
  //@{
  /// \brief
  ///   Gets the x-axis range for the histogram.
  /// \param min Used to return the minimum x-axis value.
  /// \param max Used to return the maximum x-axis value.
  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const=0;

  /// \brief
  ///   Gets the y-axis range for the histogram.
  /// \param min Used to return the minimum y-axis value.
  /// \param max Used to return the maximum y-axis value.
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const=0;
  //@}

signals:
  /// Emitted when the histogram has been reset or changed dramatically.
  void histogramReset();

  /// \brief
  ///   Emitted when new bins will be inserted.
  /// \param first The first index of the bin insertion.
  /// \param last The last index of the bin insertion.
  void aboutToInsertBins(int first, int last);

  /// Emitted when new points have been inserted.
  void binsInserted();

  /// \brief
  ///   Emitted when bins will be removed.
  /// \param first The first index of the bin removal.
  /// \param last The last index of the bin removal.
  void aboutToRemoveBins(int first, int last);

  /// Emitted when bins have been removed.
  void binsRemoved();

  /// \brief
  ///   Emitted when bin values have changed.
  /// \param first The first index of the bin change.
  /// \param last The last index of the bin change.
  void binValuesChanged(int first, int last);

  /// \brief
  ///   Emitted when bin ranges have changed.
  /// \param first The first index of the bin range change.
  /// \param last The last index of the bin range change.
  void binRangesChanged(int first, int last);

  /// \brief
  ///   Emitted when the histogram range in x and/or y changes.
  /// \note
  ///   This signal is emitted during bin insertion or removal.
  void histogramRangeChanged();

protected:
  /// \brief
  ///   Called to begin the bin insertion process.
  /// \param first The first index of the bin insertion.
  /// \param last The last index of the bin insertion.
  void beginInsertBins(int first, int last);

  /// Called to end the bin insertion process.
  void endInsertBins();

  /// \brief
  ///   Called to begin the bin removal process.
  /// \param first The first index of the bin removal.
  /// \param last The last index of the bin removal.
  void beginRemoveBins(int first, int last);

  /// Called to end the bin removal process.
  void endRemoveBins();
};

#endif
