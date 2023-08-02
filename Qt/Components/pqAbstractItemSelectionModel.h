// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAbstractItemSelectionModel_h
#define pqAbstractItemSelectionModel_h

#include <QAbstractItemModel>

class QTreeWidgetItem;
class vtkSMProxy;

/**
 * @brief Abstract class implementing a tree model with checkable items.
 * It uses QTreeWidgetItem as its item class. Reimplement the virtual methods
 * to fill it with data.
 */
class pqAbstractItemSelectionModel : public QAbstractItemModel
{
  Q_OBJECT

protected:
  pqAbstractItemSelectionModel(QObject* parent_ = nullptr);
  ~pqAbstractItemSelectionModel() override;

  /**
   * @{
   * QAbstractItemModel implementation
   */
  int rowCount(const QModelIndex& parent_ = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent_ = QModelIndex()) const override;

  QModelIndex index(int row, int column, const QModelIndex& parent_ = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index_) const override;
  QVariant data(const QModelIndex& index_, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index_, const QVariant& value, int role) override;
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index_) const override;
  /**
   * @}
   */

  /**
   * Concrete classes should implement how the model is to be populated.
   */
  virtual void populateModel(void* dataObject) = 0;

  /**
   * Initialize the root item which holds the header tags.
   */
  virtual void initializeRootItem() = 0;

  /**
   * Helper for a more comprehensive validation of indices.
   */
  bool isIndexValid(const QModelIndex& index_) const;

  QTreeWidgetItem* RootItem;
};

#endif
