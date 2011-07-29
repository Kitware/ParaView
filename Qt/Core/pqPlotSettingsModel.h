/*=========================================================================

   Program: ParaView
   Module:    pqPlotSettingsModel.h

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
#ifndef __pqPlotSettingsModel_h
#define __pqPlotSettingsModel_h

#include "pqCheckableHeaderModel.h"
#include "pqCoreExport.h"
#include <QColor>

class vtkTable;
class vtkChartXY;
class pqDataRepresentation;

/// pqPlotSettingsModel is used by display panels for plots to show the
/// properties of all the series in a table.
class PQCORE_EXPORT pqPlotSettingsModel : public pqCheckableHeaderModel
{
  typedef pqCheckableHeaderModel Superclass;

  Q_OBJECT

public:
  pqPlotSettingsModel(QObject* parent = 0);
  ~pqPlotSettingsModel();

  void setRepresentation(pqDataRepresentation* rep);
  pqDataRepresentation* representation() const;

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const;

  /// \brief Gets the data for a given model index.
  /// \param index The model index.
  /// \param role The role to get data for.
  /// \return The data for the given model index.
  virtual QVariant data(const QModelIndex &index,
                        int role=Qt::DisplayRole) const;

  /// \brief Sets the data for the given model index.
  /// \param index The model index.
  /// \param value The new data for the given role.
  /// \param role The role to set data for.
  /// \return True if the data was changed successfully.
  virtual bool setData(const QModelIndex &index, const QVariant &value,
                       int role=Qt::EditRole);

  /// \brief Gets the flags for a given model index.
  ///
  /// The flags for an item indicate if it is enabled, editable, etc.
  ///
  /// \param index The model index.
  /// \return The flags for the given model index.
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  /// \brief Gets a model index for a given location.
  /// \param row The row number.
  /// \param column The column number.
  /// \param parent The parent index.
  /// \return A model index for the given location.
  virtual QModelIndex index(int row, int column,
                            const QModelIndex &parent=QModelIndex()) const;

  /// \brief Gets the parent for a given index.
  /// \param index The model index.
  /// \return A model index for the parent of the given index.
  virtual QModelIndex parent(const QModelIndex &index) const;

public slots:
  /// Reloads the model i.e. refreshes all data from the display and resets the
  /// model.
  void reload();

  // Description:
  // API to set series properties.
  void setSeriesEnabled(int row, bool enabled);
  void setSeriesLabel(int row, const QString& label);
  void setSeriesColor(int row, const QColor &color);
  void setSeriesThickness(int row, int thickness);
  void setSeriesStyle(int row, int style);
  void setSeriesAxisCorner(int row, int axiscorner);
  void setSeriesMarkerStyle(int row, int style);

  // Description:
  // API to get series properties.
  const char* getSeriesName(int row) const;
  bool getSeriesEnabled(int row) const;
  QString getSeriesLabel(int row) const;
  QColor getSeriesColor(int row) const;
  int getSeriesThickness(int row) const;
  int getSeriesStyle(int row) const;
  int getSeriesAxisCorner(int row) const;
  int getSeriesMarkerStyle(int row) const;

protected slots:
  /// emits data-changed event whenever the properties are modified.
  void emitDataChanged();

signals:
  void redrawChart();
  void rescaleChart();

private:
  class pqImplementation;
  pqImplementation* Implementation;
};

#endif
