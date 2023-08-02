// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFileDialogLocationModel_h
#define pqFileDialogLocationModel_h

#include "pqCoreModule.h"
#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QPointer>

class vtkProcessModule;
class pqFileDialogModel;
class pqServer;
class QModelIndex;

/**
pqFileDialogLocationModel lists "special" locations, either remote from a connected ParaView
server's filesystem, or the local file system.

\sa pqFileDialog, pqFileDialogModel
*/
class PQCORE_EXPORT pqFileDialogLocationModel : public QAbstractListModel
{
  typedef QAbstractListModel Superclass;

  Q_OBJECT

public:
  /**
   * server is the server for which we need the listing.
   * if the server is nullptr, we get file listings from the builtin server
   */
  pqFileDialogLocationModel(pqFileDialogModel* model, pqServer* server, QObject* Parent);
  ~pqFileDialogLocationModel() override = default;

  /**
   * return the path to the item
   */
  QString filePath(const QModelIndex&) const;
  /**
   * return whether this item is a directory
   */
  bool isDirectory(const QModelIndex&) const;

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

  /**
   * Resets to the system default
   */
  virtual void resetToDefault();

  /**
   * Flag to indicate if the ParaView Examples directory should be added to the list.
   */
  static bool AddExamplesInLocations;

protected:
  struct pqFileDialogLocationModelFileInfo
  {
    QString Label;
    QString FilePath;
    int Type;
  };

  void LoadSpecialsFromSystem();

  QPointer<pqFileDialogModel> FileDialogModel;
  pqServer* Server = nullptr;
  QList<pqFileDialogLocationModelFileInfo> LocationList;
};

#endif // !pqFileDialogLocationModel_h
