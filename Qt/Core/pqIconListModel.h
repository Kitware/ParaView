// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqIconListModel_h
#define pqIconListModel_h

#include "pqCoreModule.h"

#include <QAbstractListModel>

#include <memory>

class QFileInfo;
using QFileInfoList = QList<QFileInfo>;

/**
 * pqIconListModel is a model for list of icons.
 * Icons may have different origins like application resources or
 * user directory.
 * Supported file format are defined based on the file extension.
 */
class PQCORE_EXPORT pqIconListModel : public QAbstractListModel
{
  Q_OBJECT;
  typedef QAbstractListModel Superclass;

public:
  pqIconListModel(QObject* parent);
  ~pqIconListModel() override;

  enum IconRoles
  {
    PathRole = Qt::UserRole + 1, // icon absolute path
    OriginRole,                  // icon file location, like application resources or user dir
  };

  /**
   * Returns the number of icons.
   */
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
   * Reimplements parent class to return data handled by pqIconListModel.
   */
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
   * Insert new rows
   */
  bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

  /**
   * Add a list of icons
   */
  void addIcons(const QFileInfoList& iconInfoList);

  /**
   * Add a single icon in the model
   */
  void addIcon(const QFileInfo& iconInfo);

  /**
   * Clear the model: remove all icons items.
   */
  void clearAll();

  /**
   * Returns the list of file extensions that can be used
   * for icons.
   */
  static QStringList getSupportedIconFormats()
  {
    return QStringList() << ".svg"
                         << ".png";
  }

  /**
   * Returns available extensions as a map of {name: display name}.
   */
  static QMap<QString, QString> getAvailableOrigins()
  {
    QMap<QString, QString> origins;
    origins["Resources"] = tr("Application resources");
    origins["UserDefined"] = tr("User icons");
    return origins;
  }

  /**
   * Returns true if index matches a user defined icon.
   */
  static bool isUserDefined(const QModelIndex& index);

private:
  struct pqInternal;
  std::unique_ptr<pqInternal> Internal;
};

#endif
