// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFileDialogFavoriteModel_h
#define pqFileDialogFavoriteModel_h

#include "pqCoreModule.h"
#include "vtkParaViewDeprecation.h" // for deprecation

#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QPointer>

class vtkProcessModule;
class pqFileDialogModel;
class pqServer;
class QModelIndex;

/**
pqFileDialogFavoriteModel allows remote browsing of a connected ParaView server's
filesystem, as well as browsing of the local file system.

\sa pqFileDialog, pqFileDialogModel
*/
class PQCORE_EXPORT pqFileDialogFavoriteModel : public QAbstractListModel
{
  typedef QAbstractListModel Superclass;

  Q_OBJECT

public:
  /**
   * server is the server for which we need the listing.
   * if the server is nullptr, we get file listings from the builtin server
   */
  pqFileDialogFavoriteModel(pqFileDialogModel* model, pqServer* server, QObject* Parent);
  ~pqFileDialogFavoriteModel() override;

  /**
   * return the path to the favorites item
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
   * used by the view to edit the label of the favorite
   */
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

  /**
   * return the flags for a specific item
   */
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  /**
   * return header data
   */
  QVariant headerData(int section, Qt::Orientation, int role) const override;

  /**
   * Adds a directory to the favorites
   */
  virtual void addToFavorites(QString const& dirPath);

  /**
   * Removes a directory from the favorites
   */
  virtual void removeFromFavorites(QString const& dirPath);

  /**
   * Resets the favorites to the system default
   */
  virtual void resetFavoritesToDefault();

  /**
   * Flag to indicate if the ParaView Examples directory must be added when creating the settings
   * for the first time, or when reseting it to the default value. Deprecated.
   */
  PARAVIEW_DEPRECATED_IN_5_12_0("Use pqFileDialogLocationModel::AddExamplesInLocations instead")
  static bool AddExamplesInFavorites;

protected:
  struct pqFileDialogFavoriteModelFileInfo
  {
    QString Label;
    QString FilePath;
    int Type;
  };

  QPointer<pqFileDialogModel> FileDialogModel;
  pqServer* Server = nullptr;
  QList<pqFileDialogFavoriteModelFileInfo> FavoriteList;
  QString SettingsKey;
};

#endif // !pqFileDialogFavoriteModel_h
