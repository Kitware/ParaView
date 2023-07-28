// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqOpacityTableModel_h
#define pqOpacityTableModel_h

#include <QAbstractTableModel>

class pqColorOpacityEditorWidget;

// QAbstractTableModel subclass for keeping track of the opacity transfer
// function control points.
// First column is control point scalar value and the second is the opacity.
class pqOpacityTableModel : public QAbstractTableModel
{
  Q_OBJECT
  typedef QAbstractTableModel Superclass;

public:
  pqOpacityTableModel(pqColorOpacityEditorWidget* widget, QObject* parentObject = nullptr);

  ~pqOpacityTableModel() override;

  /**
   * All columns are editable.
   */
  Qt::ItemFlags flags(const QModelIndex& idx) const override;

  bool setData(const QModelIndex& idx, const QVariant& value, int role = Qt::EditRole) override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override;

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

protected Q_SLOTS:

  void controlPointsChanged();

  void updatePoint(const QModelIndex& idx);

private:
  Q_DISABLE_COPY(pqOpacityTableModel)

  pqColorOpacityEditorWidget* Widget;

  double Range[2];

  class pqInternals;
  pqInternals* Internals;
};

#endif
