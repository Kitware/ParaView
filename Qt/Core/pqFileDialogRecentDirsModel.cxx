// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqFileDialogRecentDirsModel.h"
// Server Manager Includes.

// Qt Includes.

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqFileDialogModel.h"
#include "pqServer.h"
#include "pqServerConfiguration.h"
#include "pqServerResource.h"
#include "pqSettings.h"

#include <string>
#include <vtksys/SystemTools.hxx>

/////////////////////////////////////////////////////////////////////
// Icons
// NOLINTNEXTLINE(readability-redundant-member-init)
Q_GLOBAL_STATIC(pqFileDialogModelIconProvider, Icons);

//-----------------------------------------------------------------------------
pqFileDialogRecentDirsModel::pqFileDialogRecentDirsModel(
  pqFileDialogModel* fileDialogModel, pqServer* server, QObject* _parent)
  : Superclass(_parent)
{
  this->FileDialogModel = fileDialogModel;

  // We need to determine the URI for this server to get the list of recent dirs
  // from the pqSettings. If server==nullptr, we use the "builtin:" resource.
  pqServerResource resource = server ? server->getResource() : pqServerResource("builtin:");

  QString uri = resource.toURI();
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();

  QString key = QString("RecentDirs/%1").arg(uri);
  if (settings->contains(key))
  {
    QStringList dirs = settings->value(key).toStringList();
    // ensure that the directories exist.
    Q_FOREACH (QString dir, dirs)
    {
      QString temp;
      if (!this->FileDialogModel || this->FileDialogModel->dirExists(dir, temp))
      {
        this->Directories.push_back(dir);
      }
    }
  }
  this->SettingsKey = key;
}

//-----------------------------------------------------------------------------
pqFileDialogRecentDirsModel::~pqFileDialogRecentDirsModel()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings)
  {
    settings->setValue(this->SettingsKey, QVariant(this->Directories));
  }
}

//-----------------------------------------------------------------------------
/// returns the path.
QString pqFileDialogRecentDirsModel::filePath(const QModelIndex& idx) const
{
  if (idx.row() < this->Directories.size())
  {
    return this->Directories[idx.row()];
  }
  return QString();
}

//-----------------------------------------------------------------------------
/// returns the data for an item
QVariant pqFileDialogRecentDirsModel::data(const QModelIndex& idx, int role) const

{
  if (idx.isValid() && idx.row() < this->Directories.size())
  {
    switch (role)
    {
      case Qt::DisplayRole:
      {
        // We only return the directory name, not the full path.
        QString path = this->Directories[idx.row()];
        // We don't use QFileInfo here since it messes the paths up if the client and
        // the server are heterogeneous systems.
        std::string unix_path = path.toUtf8().toStdString();
        vtksys::SystemTools::ConvertToUnixSlashes(unix_path);
        std::string filename;
        std::string::size_type slashPos = unix_path.rfind('/');
        if (slashPos != std::string::npos)
        {
          filename = unix_path.substr(slashPos + 1);
        }
        else
        {
          filename = unix_path;
        }
        return filename.c_str();
      }

      case Qt::ToolTipRole:
      case Qt::StatusTipRole:
        return this->Directories[idx.row()];

      case Qt::DecorationRole:
        return Icons()->icon(pqFileDialogModelIconProvider::Folder);
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
/// return the number of rows in the model
int pqFileDialogRecentDirsModel::rowCount(const QModelIndex&) const
{
  return this->Directories.size();
}

//-----------------------------------------------------------------------------
/// return header data
QVariant pqFileDialogRecentDirsModel::headerData(int section, Qt::Orientation, int role) const
{
  if (role == Qt::DisplayRole && section == 0)
  {
    return tr("Recent Directories");
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
void pqFileDialogRecentDirsModel::setChosenDir(const QString& dir)
{
  QString temp;
  if (!dir.isEmpty() && (!this->FileDialogModel || this->FileDialogModel->dirExists(dir, temp)))
  {
    this->Directories.removeAll(dir);
    this->Directories.push_front(dir);
    // For now only 5 paths are saved in the most recent list.
    this->Directories = this->Directories.mid(0, 5);
  }
}

//-----------------------------------------------------------------------------
void pqFileDialogRecentDirsModel::setChosenFiles(const QList<QStringList>& files)
{
  if (files.empty())
  {
    return;
  }
  QString filename = files[0][0];

  // We don't use QFileInfo here since it messes the paths up if the client and
  // the server are heterogeneous systems.
  std::string unix_path = filename.toUtf8().toStdString();
  vtksys::SystemTools::ConvertToUnixSlashes(unix_path);

  std::string dirname;
  std::string::size_type slashPos = unix_path.rfind('/');
  if (slashPos == std::string::npos)
  {
    return;
  }
  dirname = unix_path.substr(0, slashPos);
  this->setChosenDir(dirname.c_str());
}
