/*=========================================================================

   Program: ParaView
   Module:    pqChartLegendModel.h

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

/// \file pqChartLegendModel.h
/// \date 11/14/2006

#ifndef _pqChartLegendModel_h
#define _pqChartLegendModel_h


#include "QtChartExport.h"
#include <QObject>
#include <QPixmap> // Needed for return type
#include <QString> // Needed for return type

class pqChartLegendModelInternal;
class pqPointMarker;
class QPen;


/// \class pqChartLegendModel
/// \brief
///   The pqChartLegendModel class stores the data for a chart legend.
class QTCHART_EXPORT pqChartLegendModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart legend model.
  /// \param parent The parent object.
  pqChartLegendModel(QObject *parent=0);
  virtual ~pqChartLegendModel();

  /// \brief
  ///   Adds an entry to the chart legend.
  /// \param icon The series identifying image.
  /// \param text The series label.
  /// \return
  ///   The id for the inserted entry or zero for failure.
  int addEntry(const QPixmap &icon, const QString &text);

  /// \brief
  ///   Inserts an entry into the chart legend.
  /// \param index Where to place the new entry.
  /// \param icon The series identifying image.
  /// \param text The series label.
  /// \return
  ///   The id for the inserted entry or zero for failure.
  int insertEntry(int index, const QPixmap &icon, const QString &text);

  /// \brief
  ///   Removes an entry from the chart legend.
  /// \param index The index of the entry to remove.
  void removeEntry(int index);

  /// Removes all the entries from the legend.
  void removeAllEntries();

  /// \brief
  ///   Blocks the model modification signals.
  ///
  /// This method should be called before making multiple changes to
  /// the model. It will prevent the view from updating before the
  /// changes are complete. Once all the changes are made, the
  /// \c finishModifyingData method should be called to notify the
  /// view of the changes.
  ///
  /// \sa pqChartLegendModel::finishModifyingData()
  void startModifyingData();

  /// \brief
  ///   Unblocks the model modification signals.
  ///
  /// The \c entriesReset signal is emitted to synchronize the view.
  ///
  /// \sa pqChartLegendModel::startModifyingData()
  void finishModifyingData();

  /// \brief
  ///   Gets the number of entries in the legend.
  /// \return
  ///   The number of entries in the legend.
  int getNumberOfEntries() const;

  /// \brief
  ///   Gets the index for the given id.
  /// \param id The entry identifier.
  /// \return
  ///   The index for the entry that matches the id or -1 if there is
  ///   no matching entry.
  int getIndexForId(unsigned int id) const;

  /// \brief
  ///   Gets the icon for the given index.
  /// \param index The index of the entry.
  /// \return
  ///   The icon for the given index or a null pixmap if the index is
  ///   out of bounds.
  QPixmap getIcon(int index) const;

  /// \brief
  ///   Sets the icon for the given index.
  /// \param index The index of the entry.
  /// \param icon The new series icon.
  void setIcon(int index, const QPixmap &icon);

  /// \brief
  ///   Gets the text for the given index.
  /// \param index The index of the entry.
  /// \return
  ///   The text for the given index or a null string if the index is
  ///   out of bounds.
  QString getText(int index) const;

  /// \brief
  ///   Sets the text for the given index.
  /// \param index The index of the entry.
  /// \param text The new series label.
  void setText(int index, const QString &text);

  /// \brief
  ///   Generates an icon for a line chart series.
  /// \param pen The pen to draw the line with.
  /// \param marker The point marker to use.
  /// \param pointPen The pen to draw the point with.
  /// \return
  ///   An icon for a line chart series.
  static QPixmap generateLineIcon(const QPen &pen, pqPointMarker *marker=0,
      const QPen *pointPen=0);

  /// \brief
  ///   Generates an icon for a solid color.
  /// \param color The color to use.
  /// \return
  ///   An icon for a solid color.
  static QPixmap generateColorIcon(const QColor &color);

signals:
  /// \brief
  ///   Emitted when a new entry is added.
  /// \param index Where the entry was added.
  void entryInserted(int index);

  /// \brief
  ///   Emitted before an entry is removed.
  /// \param index The index being removed.
  void removingEntry(int index);

  /// \brief
  ///   Emitted after an entry is removed.
  /// \param index The index being removed.
  void entryRemoved(int index);

  /// Emitted when the legend entries are reset.
  void entriesReset();

  /// \brief
  ///   Emitted when the icon for an entry has changed.
  /// \param index The index of the entry that changed.
  void iconChanged(int index);

  /// \brief
  ///   Emitted when the text for an entry has changed.
  /// \param index The index of the entry that changed.
  void textChanged(int index);

private:
  pqChartLegendModelInternal *Internal; ///< Stores the legend items.
  bool InModify;                        ///< True when blocking signals.
};

#endif
