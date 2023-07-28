// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFileDialogRecentDirsModel_h
#define pqFileDialogRecentDirsModel_h

#include "pqCoreModule.h"
#include <QAbstractListModel>
#include <QPointer>
#include <QStringList>

class pqServer;
class pqFileDialogModel;

/**
 * pqFileDialogRecentDirsModel is a model which used by file dialog
 * (pqFileDialog) to populate the list showing the recent directory locations.
 * This is per server based, since the locations are connection dependent.
 */
class PQCORE_EXPORT pqFileDialogRecentDirsModel : public QAbstractListModel
{
  Q_OBJECT
  typedef QAbstractListModel Superclass;

public:
  /**
   * server is the server for which we need the listing.
   * if the server is nullptr, we get file listings locally (i.e. builtin server).
   * pqFileDialogModel is used to test the validity of directories.
   */
  pqFileDialogRecentDirsModel(pqFileDialogModel* model, pqServer* server, QObject* parent);
  ~pqFileDialogRecentDirsModel() override;

  /**
   * Set the directory chosen by the user so that it gets added to the recent
   * list.
   */
  void setChosenDir(const QString& dir);

  /**
   * returns the path.
   */
  QString filePath(const QModelIndex&) const;

  /**
   * returns the data for an item
   */
  QVariant data(const QModelIndex& idx, int role) const override;

  /**
   * return the number of rows in the model
   */
  int rowCount(const QModelIndex& idx) const override;

  /**
   * return header data
   */
  QVariant headerData(int section, Qt::Orientation, int role) const override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setChosenFiles(const QList<QStringList>& files);

protected:
  QStringList Directories;
  QString SettingsKey;
  QPointer<pqFileDialogModel> FileDialogModel;

private:
  Q_DISABLE_COPY(pqFileDialogRecentDirsModel)
};

#endif
