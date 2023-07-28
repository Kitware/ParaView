// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqScalarSetModel_h
#define pqScalarSetModel_h

#include "pqComponentsModule.h"

#include <QAbstractListModel>

/**
 * Qt model that stores a sorted collection of unique floating-point numbers
 */
class PQCOMPONENTS_EXPORT pqScalarSetModel : public QAbstractListModel
{
  typedef QAbstractListModel base;

  Q_OBJECT

public:
  pqScalarSetModel();
  ~pqScalarSetModel() override;

  /**
   * Clears the model contents
   */
  void clear();
  /**
   * Inserts a floating-point number into the model
   */
  QModelIndex insert(double value);
  /**
   * Erases a floating-point number from the model
   */
  void erase(double value);
  /**
   * Erases a zero-based row from the model
   */
  void erase(int row);
  /**
   * Returns the sorted collection of numbers stored in the model
   */
  QList<double> values();

  /** Controls formatting of displayed data, supports the
  'e', 'E', 'f', 'F', 'g', and 'G' formats provided by printf() */
  void setFormat(char f, int precision = 3);

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

  /**
   * Get/Set if the order in which the values are inserted must be preserved.
   * Off by default i.e. values will be sorted. If set after inserting a few values,
   * the order of values inserted until the flag was set is lost.
   */
  void setPreserveOrder(bool);
  bool preserveOrder() const;

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !pqScalarSetModel_h
