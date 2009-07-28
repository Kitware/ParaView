/*=========================================================================

   Program: ParaView
   Module:    pqChartAxisModel.h

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

/// \file pqChartAxisModel.h
/// \date 12/1/2006

#ifndef _pqChartAxisModel_h
#define _pqChartAxisModel_h


#include "QtChartExport.h"
#include <QObject>

class pqChartAxisModelInternal;
class pqChartValue;


/// \class pqChartAxisModel
/// \brief
///   The pqChartAxisModel class stores the labels for a chart axis.
class QTCHART_EXPORT pqChartAxisModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart axis model.
  /// \param parent The parent object.
  pqChartAxisModel(QObject *parent=0);
  virtual ~pqChartAxisModel();

  /// \brief
  ///   Adds a label to the chart axis.
  /// \param label The label to add.
  void addLabel(const pqChartValue &label);

  /// \brief
  ///   Removes a label from the chart axis.
  /// \param index The index of the label to remove.
  void removeLabel(int index);

  /// Removes all the labels from the chart axis.
  void removeAllLabels();

  /// \brief
  ///   Blocks the model modification signals.
  ///
  /// This method should be called before making multiple changes to
  /// the model. It will prevent the view from updating before the
  /// changes are complete. Once all the changes are made, the
  /// \c finishModifyingData method should be called to notify the
  /// view of the changes.
  ///
  /// \sa pqChartAxisModel::finishModifyingData()
  void startModifyingData();

  /// \brief
  ///   Unblocks the model modification signals.
  ///
  /// The \c labelsReset signal is emitted to synchronize the view.
  ///
  /// \sa pqChartAxisModel::startModifyingData()
  void finishModifyingData();

  /// \brief
  ///   Gets the number of labels in the chart axis.
  /// \return
  ///   The number of labels in the chart axis.
  int getNumberOfLabels() const;

  /// \brief
  ///   Gets a specified chart axis label.
  /// \param index Which chart axis to get.
  /// \param label Used to return the label.
  void getLabel(int index, pqChartValue &label) const;

signals:
  /// \brief
  ///   Emitted when a new label is added.
  /// \param index Where the label was added.
  void labelInserted(int index);

  /// \brief
  ///   Emitted before a label is removed.
  /// \param index The index being removed.
  void removingLabel(int index);

  /// \brief
  ///   Emitted after a label is removed.
  /// \param index The index being removed.
  void labelRemoved(int index);

  /// Emitted when the axis labels are reset.
  void labelsReset();

private:
  pqChartAxisModelInternal *Internal; ///< Stores the list of labels.
  bool InModify;                      ///< True when blocking signals.
};

#endif
