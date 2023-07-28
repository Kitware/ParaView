// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorTableModel_h
#define pqColorTableModel_h

#include "pqApplicationComponentsModule.h"
#include <QAbstractTableModel>

class pqColorOpacityEditorWidget;

// QAbstractTableModel subclass for viewing and manipulating color transfer
// function control points through a table interface.
// First column is control point scalar value and the second through fourth
// columns are r,g,b colors, respectively.
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorTableModel : public QAbstractTableModel
{
  Q_OBJECT
  typedef QAbstractTableModel Superclass;

public:
  pqColorTableModel(pqColorOpacityEditorWidget* widget, QObject* parentObject = nullptr);

  ~pqColorTableModel() override;

  /**
   * All columns are editable. The first and last value in the first column
   * are not editable as they are set by the range.
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
  Q_DISABLE_COPY(pqColorTableModel)

  pqColorOpacityEditorWidget* Widget;

  double Range[2];

  class pqInternals;
  pqInternals* Internals;
};

#endif
