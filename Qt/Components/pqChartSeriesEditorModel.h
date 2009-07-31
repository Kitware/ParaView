/*=========================================================================

   Program: ParaView
   Module:    pqChartSeriesEditorModel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef __pqChartSeriesEditorModel_h 
#define __pqChartSeriesEditorModel_h

#include "pqCheckableHeaderModel.h"
#include "pqComponentsExport.h"

#include <QColor>
#include <QPointer>
#include "vtkWeakPointer.h"

class pqDataRepresentation;
class vtkQtChartSeriesOptions;
class vtkSMChartRepresentationProxy;

/// pqChartSeriesEditorModel is a model that can be used to connect to a
/// Tree or Table view that shows the series that be plotted. It allows the user
/// to edit certain properties of the series as well such as color/legend name.
/// It provide API to simplify changing/querying almost all the series options
/// (although not all of them are shown in the [Tree|Table] View.

class PQCOMPONENTS_EXPORT pqChartSeriesEditorModel :
  public pqCheckableHeaderModel
{
  Q_OBJECT
  typedef pqCheckableHeaderModel Superclass;
public:
  pqChartSeriesEditorModel(QObject* parent=0);
  virtual ~pqChartSeriesEditorModel();

  /// Set the representation for the series editor model.
  /// The model must have a pqDataRepresentation for one of the charting
  /// representations otherwise it will remain empty.
  void setRepresentation(pqDataRepresentation* display);
  pqDataRepresentation* representation() const;

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the number of columns for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of columns for the given index.
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets whether or not the given index has child items.
  /// \param parent The parent index.
  /// \return
  ///   True if the given index has child items.
  virtual bool hasChildren(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets a model index for a given location.
  /// \param row The row number.
  /// \param column The column number.
  /// \param parent The parent index.
  /// \return
  ///   A model index for the given location.
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the parent for a given index.
  /// \param index The model index.
  /// \return
  ///   A model index for the parent of the given index.
  virtual QModelIndex parent(const QModelIndex &index) const;

  /// \brief
  ///   Gets the data for a given model index.
  /// \param index The model index.
  /// \param role The role to get data for.
  /// \return
  ///   The data for the given model index.
  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;

  /// \brief
  ///   Gets the flags for a given model index.
  ///
  /// The flags for an item indicate if it is enabled, editable, etc.
  ///
  /// \param index The model index.
  /// \return
  ///   The flags for the given model index.
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  /// \brief
  ///   Sets the data for the given model index.
  /// \param index The model index.
  /// \param value The new data for the given role.
  /// \param role The role to set data for.
  /// \return
  ///   True if the data was changed successfully.
  virtual bool setData(const QModelIndex &index, const QVariant &value, 
      int role=Qt::EditRole);

  virtual QVariant headerData(int section, Qt::Orientation orient,
      int role=Qt::DisplayRole) const;
  //@}

public slots:
  /// Reloads the model i.e. refreshes all data from the display and resets the
  /// model.
  void reload();

  // Description:
  // API to set series properties.
  void setSeriesEnabled(int row, bool enabled);
  void setSeriesColor(int row, const QColor &color);
  void setSeriesThickness(int row, int thickness);
  void setSeriesStyle(int row, int style);
  void setSeriesAxisCorner(int row, int axiscorner);
  void setSeriesMarkerStyle(int row, int style);
  void setSeriesLabel(int row, const QString& label);

  // Description:
  // API to get series properties.
  const char* getSeriesName(int row) const;
  bool getSeriesEnabled(int row) const;
  QColor getSeriesColor(int row) const;
  int getSeriesThickness(int row) const;
  int getSeriesStyle(int row) const;
  int getSeriesAxisCorner(int row) const;
  int getSeriesMarkerStyle(int row) const;
  QString getSeriesLabel(int row) const;

private slots:
  /// When ever an option changes, we want to fire data-changed signal.
  void optionsChanged(vtkQtChartSeriesOptions* options);

private:
  QPointer<pqDataRepresentation> Representation;
  vtkWeakPointer<vtkSMChartRepresentationProxy> RepresentationProxy;

private:
  pqChartSeriesEditorModel(const pqChartSeriesEditorModel&); // Not implemented.
  void operator=(const pqChartSeriesEditorModel&); // Not implemented.
};

#endif


