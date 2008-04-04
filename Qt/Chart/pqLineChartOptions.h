/*=========================================================================

   Program: ParaView
   Module:    pqLineChartOptions.h

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

/// \file pqLineChartOptions.h
/// \date 4/26/2007

#ifndef _pqLineChartOptions_h
#define _pqLineChartOptions_h


#include "QtChartExport.h"
#include <QObject>

class pqChartSeriesColorManager;
class pqChartSeriesOptionsGenerator;
class pqLineChartOptionsInternal;
class pqLineChartSeriesOptions;


/// \class pqLineChartOptions
/// \brief
///   The pqLineChartOptions class Stores the drawing options for a
///   line chart.
class QTCHART_EXPORT pqLineChartOptions : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line chart options instance.
  /// \param parent The parent object.
  pqLineChartOptions(QObject *parent=0);
  virtual ~pqLineChartOptions();

  /// \brief
  ///   Gets the series color manager.
  /// \return
  ///   A pointer to the series color manager.
  pqChartSeriesColorManager *getSeriesColorManager();

  /// \brief
  ///   Sets the series color manager.
  /// \param manager The new series color manager.
  void setSeriesColorManager(pqChartSeriesColorManager *manager);

  /// \brief
  ///   Gets the options generator.
  /// \return
  ///   A pointer to the options generator.
  pqChartSeriesOptionsGenerator *getGenerator();

  /// \brief
  ///   Sets the options generator.
  /// \param generator The new options generator.
  void setGenerator(pqChartSeriesOptionsGenerator *generator);

  /// \brief
  ///   Gets the number of plot options.
  /// \return
  ///   The number of plot options.
  int getNumberOfSeriesOptions() const;

  /// \brief
  ///   Gets the plot options for the specified model index.
  /// \param index The index of the plot in the model.
  /// \return
  ///   A pointer to the plot options.
  pqLineChartSeriesOptions *getSeriesOptions(int index);

  /// \brief
  ///   Sets the plot options for the specified model index.
  /// \param index The index of the plot in the model.
  /// \param options The new options for the line plot.
  void setSeriesOptions(int index, const pqLineChartSeriesOptions &options);

public slots:
  /// Removes all the plot options.
  void clearSeriesOptions();

  /// \brief
  ///   Inserts options for the given line series.
  ///
  /// When a new series options object is added, the options generator
  /// is used to initialize the options. The \c optionsInserted
  /// signal is emitted for each of the new options objects. This
  /// signal can be used to initialize the new plot options object.
  ///
  /// \param first The first index of the new line series.
  /// \param last The last index of the new line series.
  void insertSeriesOptions(int first, int last);

  /// \brief
  ///   Removes options for the given line series.
  ///
  /// When an line series is removed, its options generator index is
  /// saved so it can be used when a new series is added.
  ///
  /// \param first The first index of the options to remove.
  /// \param last The last index of the options to remove.
  void removeSeriesOptions(int first, int last);

  /// \brief
  ///   Moves the given series options to a new location.
  /// \param current The known index of the series options.
  /// \param index The new index of the series options.
  void moveSeriesOptions(int current, int index);

signals:
  /// Emitted when the drawing options for a series change.
  void optionsChanged();

  /// \brief
  ///   Emitted when generating new series options.
  ///
  /// This signal gives the observer an opportunity to modify the
  /// series options when they are generated before being used.
  ///
  /// \param index The index of the model's line series object.
  /// \param options The newly created line series options.
  void optionsInserted(int index, pqLineChartSeriesOptions *options);

private:
  pqLineChartOptionsInternal *Internal; ///< Stores the options.
};

#endif
