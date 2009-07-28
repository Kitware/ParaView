/*=========================================================================

   Program: ParaView
   Module:    pqHistogramSelectionModel.h

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

/// \file pqHistogramSelectionModel.h
/// \date 8/17/2006

#ifndef _pqHistogramSelectionModel_h
#define _pqHistogramSelectionModel_h


#include "QtChartExport.h"
#include <QObject>

#include "pqHistogramSelection.h" // Needed for enum and typedef
#include <QList> // Needed for typedef

class pqChartValue;
class pqHistogramModel;

typedef QList<pqHistogramSelection> pqHistogramSelectionList;


/// \class pqHistogramSelectionModel
/// \brief
///   The pqHistogramSelectionModel class stores the selection ranges
///   for an associated histogram model.
class QTCHART_EXPORT pqHistogramSelectionModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a histogram selection model object.
  /// \param parent The parent object.
  pqHistogramSelectionModel(QObject *parent=0);
  virtual ~pqHistogramSelectionModel() {}

  /// \brief
  ///   Gets the histogram model associated with the selection model.
  /// \return
  ///   The histogram model associated with the selection model.
  pqHistogramModel *getModel() const {return this->Model;}

  /// \brief
  ///   Sets the histogram model associated with the selection model.
  /// \param model The new histogram model.
  void setModel(pqHistogramModel *model);

  /// \brief
  ///   Gets whether or not the selection model is in an interactive
  ///   change.
  /// \return
  ///   Trure if the selection model is in an interactive change.
  /// \sa pqHistogramSelectionModel::beginInteractiveChange()
  bool isInInteractiveChange() const {return this->InInteractMode;}

  /// \brief
  ///   Called to begin an interactive selection change.
  ///
  /// Interactive selection changes such as a selection box can send
  /// a lot of change signals as the user drags the mouse around. The
  /// chart needs to update the selection based on those signals in
  /// order for the user to see the changes. If an expensive process
  /// is attached to the selection change signal, this can cause a
  /// visible slow-down in the application. This method allows the
  /// selection to keep the chart painter up to date while allowing
  /// the expensive process to delay execution.
  ///
  /// The interactive controller should call this method before
  /// starting a change such as with a selection box. It should call
  /// the \c endInteractiveChange method when the interaction is done.
  /// The expensive process should listen to the \c selectionChanged
  /// and \c interactionFinished signals. The \c interactionFinished
  /// is emitted at the end of the interactive change. In order to
  /// keep track of non-interactive changes, the \c selectionChanged
  /// signal must be monitored. This signal will be emitted for every
  /// selection change. Therefore, the listening code should check
  /// to see if the model is in an interactive change before executing
  /// an expensive process.
  void beginInteractiveChange();

  /// \brief
  ///   Called to end an interactive selection change.
  /// \sa pqHistogramSelectionModel::beginInteractiveChange()
  void endInteractiveChange();

  /// \brief
  ///   Gets whether or not the histogram has selected range(s).
  /// \return
  ///   True if there is a selection.
  bool hasSelection() const;

  /// \brief
  ///   Gets the current selection.
  /// \return
  ///   A reference to the current selection.
  /// \sa pqHistogramSelectionModel::hasSelection()
  const pqHistogramSelectionList &getSelection() const {return this->List;}

  /// \brief
  ///   Gets the selection type for the list.
  /// \return
  ///   The type of selections in the list.
  pqHistogramSelection::SelectionType getType() const {return this->Type;}

  /// \brief
  ///   Gets the range of the entire selection list.
  /// \param min Used to return the minimum of the selection range.
  /// \param max Used to return the maximum of the selection range.
  void getRange(pqChartValue &min, pqChartValue &max) const;

  /// Selects all the bins on the chart.
  void selectAllBins();

  /// Selects all the values on the chart.
  void selectAllValues();

  /// Clears the selection.
  void selectNone();

  /// Inverts the selection.
  void selectInverse();

  /// \brief
  ///   Sets the selection to the specified range(s).
  /// \param list The list of selection range(s).
  void setSelection(const pqHistogramSelectionList &list);

  /// \brief
  ///   Adds the specified range(s) to the selection.
  ///
  /// Each range in the list must be the same type of selection as
  /// the current selection list. The range(s) in the supplied list
  /// are merged with the current selection, which may cause items
  /// to be removed.
  ///
  /// \param list The list of selection range(s).
  void addSelection(const pqHistogramSelectionList &list);

  /// \brief
  ///   Performs an exclusive or between the specified range(s) and
  ///   the selection.
  ///
  /// Each range in the list must be the same type of selection as
  /// the current selection list. The range(s) in the supplied list
  /// are xored with the current selection, which may cause items
  /// to be removed.
  ///
  /// \param list The list of selection range(s).
  void xorSelection(const pqHistogramSelectionList &list);

  /// \brief
  ///   Subtracts the specified range(s) from the selection.
  ///
  /// Each range in the list must be the same type of selection as
  /// the current selection list. The range(s) in the supplied list
  /// are removed from the current selection, which may cause items
  /// to be removed.
  ///
  /// \param list The list of selection range(s).
  void subtractSelection(const pqHistogramSelectionList &list);

  /// \brief
  ///   Sets the selection to the specified range.
  /// \param range The selection range.
  void setSelection(const pqHistogramSelection &range);

  /// \brief
  ///   Adds the specified range to the selection.
  /// \param range The selection range.
  /// \sa pqHistogramSelectionModel::addSelection(
  ///         const pqHistogramSelectionList &)
  void addSelection(const pqHistogramSelection &range);

  /// \brief
  ///   Subtracts the specified range from the selection.
  /// \param range The selection range.
  /// \return
  ///   True if the selection has changed.
  /// \sa pqHistogramSelectionModel::subtractSelection(
  ///         const pqHistogramSelectionList &)
  bool subtractSelection(const pqHistogramSelection &range);

  /// \brief
  ///   Performs an exclusive or between the specified range and
  ///   the selection.
  /// \param range The selection range.
  /// \sa pqHistogramSelectionModel::xorSelection(
  ///         const pqHistogramSelectionList &)
  void xorSelection(const pqHistogramSelection &range);

  /// \brief
  ///   Moves the selection range if it exists.
  ///
  /// After finding the selection range, the offset will be added to
  /// both ends of the range. The new range will be adjusted to fit
  /// within the bounds of the chart. The new, adjusted range will be
  /// united with the remaining selections in the list. In other words,
  /// it is possible for the range to become shorter and/or combined
  /// with overlapping selection ranges.
  ///
  /// \param range The selection range to move.
  /// \param offset The amount to move the selection range.
  void moveSelection(const pqHistogramSelection &range,
      const pqChartValue &offset);

public:
  /// \brief
  ///   Used to sort and merge the selection ranges in the list.
  ///
  /// The selection ranges are sorted according to their first value.
  /// If the selection type has not been set, the first selection
  /// item will be used to determine the type. Items of the wrong
  /// type will be removed from the list. After the items are sorted,
  /// overlapping items are merged together. The merging process can
  /// consolidate items and remove unused ones.
  ///
  /// \param list The list of selection ranges to sort and merge.
  static void sortAndMerge(pqHistogramSelectionList &list);

signals:
  /// \brief
  ///   Emitted when the selection changes.
  /// \param list The list of selected histogram ranges.
  void selectionChanged(const pqHistogramSelectionList &list);

  /// \brief
  ///   Emitted when an interactive selection change is finished.
  ///
  /// This signal can be used to delay expensive processes until
  /// after the selection change is complete.
  ///
  /// \sa pqHistogramSelectionModel::beginInteractiveChange()
  void interactionFinished();

public slots:
  /// \name Model Modification Handlers
  //@{
  void beginModelReset();
  void endModelReset();
  void beginInsertBinValues(int first, int last);
  void endInsertBinValues();
  void beginRemoveBinValues(int first, int last);
  void endRemoveBinValues();
  void beginRangeChange(const pqChartValue &min, const pqChartValue &max);
  void endRangeChange();
  //@}

private:
  void validateRange(pqHistogramSelection &range);
  void clearSelections();

private:
  /// Stores the current selection type.
  pqHistogramSelection::SelectionType Type;
  pqHistogramSelectionList List; ///< Stores the selection list.
  pqHistogramModel *Model;       ///< A pointer to the histogram.
  bool PendingSignal;            ///< Used during model changes.
  bool InInteractMode;           ///< True if in interact mode.
};

#endif
