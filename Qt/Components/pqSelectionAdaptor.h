// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSelectionAdaptor_h
#define pqSelectionAdaptor_h

#include "pqComponentsModule.h"
#include <QItemSelectionModel> //need for qtSelectionFlags
#include <QObject>

class pqServerManagerModelItem;
class QAbstractItemModel;
class QModelIndex;

/**
 * pqSelectionAdaptor is the abstract base class for an adaptor that connects a
 * QItemSelectionModel to pqActiveObjects making it possible to update the
 * pqActiveObjects source selection when the QItemSelectionModel changes and
 * vice-versa.
 * Subclass typically only need to implement mapToItem() and mapFromItem().
 */
class PQCOMPONENTS_EXPORT pqSelectionAdaptor : public QObject
{
  Q_OBJECT
public:
  ~pqSelectionAdaptor() override;

  /**
   * Returns a pointer to the QItemSelectionModel.
   */
  QItemSelectionModel* getQSelectionModel() const { return this->QSelectionModel; }

protected:
  pqSelectionAdaptor(QItemSelectionModel* pipelineSelectionModel);

  /**
   * Maps a pqServerManagerModelItem to an index in the QAbstractItemModel.
   */
  virtual QModelIndex mapFromItem(pqServerManagerModelItem* item) const = 0;

  /**
   * Maps a QModelIndex to a pqServerManagerModelItem.
   */
  virtual pqServerManagerModelItem* mapToItem(const QModelIndex& index) const = 0;

  /**
   * Returns the QAbstractItemModel used by the QSelectionModel.
   * If QSelectionModel uses a QAbstractProxyModel, this method skips
   * over all such proxy models and returns the first non-proxy model
   * encountered.
   */
  const QAbstractItemModel* getQModel() const;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * called when the selection in the Qt-model changes.
   */
  virtual void selectionChanged();

  /**
   * called when the ServerManager level selection (or current) changes.
   */
  virtual void currentProxyChanged();
  virtual void proxySelectionChanged();

  /**
   * subclasses can override this method to provide model specific selection
   * overrides such as QItemSelection::Rows or QItemSelection::Columns etc.
   */
  virtual QItemSelectionModel::SelectionFlag qtSelectionFlags() const
  {
    return QItemSelectionModel::NoUpdate;
  }

private:
  /**
   * Given a QModelIndex for the QAbstractItemModel under the QItemSelectionModel,
   * this returns the QModelIndex for the inner most non-proxy
   * QAbstractItemModel.
   */
  QModelIndex mapToSource(const QModelIndex& inIndex) const;

  /**
   * Given a QModelIndex for the innermost non-proxy QAbstractItemModel,
   * this returns the QModelIndex for the QAbstractItemModel under the
   * QItemSelectionModel.
   */
  QModelIndex mapFromSource(const QModelIndex& inIndex, const QAbstractItemModel* model) const;

  QItemSelectionModel* QSelectionModel;
  bool IgnoreSignals;
};

#endif
