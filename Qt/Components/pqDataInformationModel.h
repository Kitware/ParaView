// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDataInformationModel_h
#define pqDataInformationModel_h

#include "pqComponentsModule.h"
#include <QAbstractTableModel>

class pqDataInformationModelInternal;
class pqOutputPort;
class pqPipelineSource;
class pqView;

class PQCOMPONENTS_EXPORT pqDataInformationModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  pqDataInformationModel(QObject* _parent = nullptr);
  ~pqDataInformationModel() override;

  /**
   * QAbstractTableModel API.
   * Returns the number of rows under the given parent.
   */
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
   * QAbstractTableModel API.
   * Returns the number of columns for the given parent.
   */
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
   * QAbstractTableModel API.
   * Returns the data stored under the given role for the item referred
   * to by the index.
   */
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
   * QAbstractTableModel API.
   * Returns the data for the given role and section in the header with the
   * specified orientation.
   */
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  /**
   * Given a pqOutputPort, get the index for it, if present in this model,
   * otherwise returns invalid index.
   */
  QModelIndex getIndexFor(pqOutputPort* item) const;

  /**
   * Given a valid index, returns the pqOutputPort item corresponding to it.
   */
  pqOutputPort* getItemFor(const QModelIndex& index) const;

  /**
   * Method needed for copy/past cell editor
   */
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called when a new source/filter is registered.
   */
  void addSource(pqPipelineSource* source);

  /**
   * Called when a new source/filter is unregistered.
   */
  void removeSource(pqPipelineSource* source);

  /**
   * Called when the active view changes. Need to correctly show the geometry
   * size for the source.
   */
  void setActiveView(pqView* view);

private Q_SLOTS:
  /**
   * Called after the associated algorithm executes.
   */
  void dataUpdated(pqPipelineSource* changedSource);

  /**
   * Called at end of every render since geometry sizes may have changed.
   */
  void refreshGeometrySizes();

private: // NOLINT(readability-redundant-access-specifiers)
  pqDataInformationModelInternal* Internal;

  enum ColumnType
  {
    Name = 0,
    DataType,
    CellCount,
    PointCount,
    MemorySize,
    GeometrySize,
    Bounds,
    TimeRange,
    Max_Columns
  };
};

#endif
