/*=========================================================================

   Program: ParaView
   Module:    pqFileDialogFavoriteModel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqFileDialogFavoriteModel.h"

#include <QApplication>
#include <QBrush>
#include <QDir>
#include <QFont>
#include <QStyle>
#include <pqApplicationCore.h>
#include <pqCoreConfiguration.h>
#include <pqFileDialogModel.h>
#include <pqServer.h>
#include <pqServerConfiguration.h>
#include <pqServerResource.h>
#include <pqSettings.h>
#include <vtkClientServerStream.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkPVFileInformation.h>
#include <vtkPVFileInformationHelper.h>
#include <vtkProcessModule.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSmartPointer.h>
#include <vtkStringList.h>

#include "pqSMAdaptor.h"

bool pqFileDialogFavoriteModel::AddExamplesInFavorites = true;

/////////////////////////////////////////////////////////////////////
// Icons
// NOLINTNEXTLINE(readability-redundant-member-init)
Q_GLOBAL_STATIC(pqFileDialogModelIconProvider, Icons);

//-----------------------------------------------------------------------------
pqFileDialogFavoriteModel::pqFileDialogFavoriteModel(
  pqFileDialogModel* fileDialogModel, pqServer* server, QObject* p)
  : Superclass(p)
  , FileDialogModel(fileDialogModel)
{
  this->Server = server;
  // We need to determine the URI for this server to get the list of favorites directories
  // from the pqSettings. If server==nullptr, we use the "builtin:" resource.
  pqServerResource resource = server ? server->getResource() : pqServerResource("builtin:");

  QString uri = resource.configuration().URI();
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();

  QString key = QString("UserFavorites/%1").arg(uri);
  if (settings->contains(key))
  {
    QVariantList const fileInfos = settings->value(key).toList();
    // ensure that the directories exist.
    for (QVariant const& fileInfo : fileInfos)
    {
      auto fileInfoList = fileInfo.toList();
      QString path = fileInfoList[1].toString();

      // If it is the Examples dir placeholder, replace it with the real path to the examples.
      if (path == "_examples_path_")
      {
        // this will not be stored in the favorites
        path = QString::fromStdString(vtkPVFileInformation::GetParaViewExampleFilesDirectory());
      }

      // Check type and existence on creation
      int type = this->FileDialogModel->fileType(path);

      this->FavoriteList.push_back(pqFileDialogFavoriteModelFileInfo{
        fileInfoList[0].toString(), fileInfoList[1].toString(), type });
    }
  }
  else
  {
    this->LoadFavoritesFromSystem();
  }
  this->SettingsKey = key;
}

//-----------------------------------------------------------------------------
pqFileDialogFavoriteModel::~pqFileDialogFavoriteModel()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings)
  {
    QVariantList favoriteListVariant;
    favoriteListVariant.reserve(this->FavoriteList.size());
    for (pqFileDialogFavoriteModelFileInfo const& fileInfo : this->FavoriteList)
    {
      favoriteListVariant.push_back(
        QVariant{ { fileInfo.Label, fileInfo.FilePath, fileInfo.Type } });
    }
    settings->setValue(this->SettingsKey, favoriteListVariant);
  }
}

//-----------------------------------------------------------------------------
QString pqFileDialogFavoriteModel::filePath(const QModelIndex& index) const
{
  return this->data(index, Qt::UserRole).toString();
}

//-----------------------------------------------------------------------------
bool pqFileDialogFavoriteModel::isDir(const QModelIndex& index) const
{
  if (index.row() >= this->FavoriteList.size())
  {
    return false;
  }

  const pqFileDialogFavoriteModelFileInfo& file = this->FavoriteList[index.row()];
  return vtkPVFileInformation::IsDirectory(file.Type);
}

//-----------------------------------------------------------------------------
QVariant pqFileDialogFavoriteModel::data(const QModelIndex& idx, int role) const
{
  if (!idx.isValid() || idx.row() >= this->FavoriteList.size() || idx.column() != 0)
  {
    return QVariant();
  }

  const pqFileDialogFavoriteModelFileInfo& file = this->FavoriteList[idx.row()];
  auto dir = file.FilePath;
  // If it is the Examples dir placeholder, replace it with the real path to the examples.
  if (dir == "_examples_path_")
  {
    // FIXME when the shared resources dir is not found, this is equal to `/examples`. This might be
    // confusing for people without the `Examples` directory (mostly ParaView devs). This directory
    // can be hidden by setting the `AddExamplesInFavoritesBehavior` to `false`.
    dir = QString::fromStdString(vtkPVFileInformation::GetParaViewExampleFilesDirectory());
  }

  QString temp;
  switch (role)
  {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return file.Label;
    case Qt::UserRole:
      return dir;
    case Qt::ItemDataRole::ToolTipRole:
      if (file.Type != vtkPVFileInformation::INVALID)
      {
        return dir;
      }
      else
      {
        return dir + " (Warning: invalid)";
      }
    case Qt::ItemDataRole::FontRole:
      if (file.Type != vtkPVFileInformation::INVALID)
      {
        return {};
      }
      else
      {
        QFont font;
        font.setItalic(true);
        return font;
      }

    case Qt::ItemDataRole::ForegroundRole:
      if (file.Type != vtkPVFileInformation::INVALID)
      {
        return {};
      }
      else
      {
        QBrush brush;
        brush.setColor(QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
        return brush;
      }

    case Qt::DecorationRole:
      return Icons()->icon(static_cast<vtkPVFileInformation::FileTypes>(file.Type));
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
int pqFileDialogFavoriteModel::rowCount(const QModelIndex&) const
{
  return this->FavoriteList.size();
}

//-----------------------------------------------------------------------------
QVariant pqFileDialogFavoriteModel::headerData(int section, Qt::Orientation, int role) const
{
  switch (role)
  {
    case Qt::DisplayRole:
      switch (section)
      {
        case 0:
          return tr("Favorites");
      }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
void pqFileDialogFavoriteModel::addToFavorites(QString const& dirPath)
{
  QString temp;
  if (!this->FileDialogModel->dirExists(dirPath, temp))
  {
    return;
  }

  QString const cleanDirPath = QDir::cleanPath(this->FileDialogModel->absoluteFilePath(dirPath));

  QList<pqFileDialogFavoriteModelFileInfo>& favoriteList = this->FavoriteList;
  auto foundIter = std::find_if(favoriteList.begin(), favoriteList.end(),
    [&cleanDirPath](pqFileDialogFavoriteModelFileInfo const& favInfo) {
      return favInfo.FilePath == cleanDirPath;
    });

  if (foundIter != favoriteList.end())
  {
    // No need to add to favorites because it is already present
    return;
  }

  int type = this->FileDialogModel->fileType(dirPath);
  this->beginInsertRows(QModelIndex(), favoriteList.size(), favoriteList.size());
  favoriteList.push_back(
    pqFileDialogFavoriteModelFileInfo{ QFileInfo(dirPath).baseName(), cleanDirPath, type });
  this->endInsertRows();
}

//-----------------------------------------------------------------------------
void pqFileDialogFavoriteModel::removeFromFavorites(QString const& dirPath)
{
  QString cleanDirPath = QDir::cleanPath(this->FileDialogModel->absoluteFilePath(dirPath));
  // Check if this is the Examples directory, because it is not stored like the other directories
  if (cleanDirPath ==
    QString::fromStdString(vtkPVFileInformation::GetParaViewExampleFilesDirectory()))
  {
    cleanDirPath = "_examples_path_";
  }

  auto foundIter = std::find_if(this->FavoriteList.begin(), this->FavoriteList.end(),
    [&](pqFileDialogFavoriteModelFileInfo const& fileInfo) {
      return fileInfo.FilePath == cleanDirPath;
    });

  if (foundIter == this->FavoriteList.end())
  {
    return;
  }

  int const row = std::distance(this->FavoriteList.begin(), foundIter);
  this->beginRemoveRows(QModelIndex(), row, row);
  this->FavoriteList.erase(foundIter);
  this->endRemoveRows();
}

//-----------------------------------------------------------------------------
void pqFileDialogFavoriteModel::resetFavoritesToDefault()
{
  this->beginResetModel();
  this->LoadFavoritesFromSystem();
  this->endResetModel();
}

//-----------------------------------------------------------------------------
bool pqFileDialogFavoriteModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (index.isValid() && role == Qt::EditRole)
  {
    this->FavoriteList[index.row()].Label = value.toString();

    emit this->dataChanged(index, index);

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqFileDialogFavoriteModel::flags(const QModelIndex& index) const
{
  if (index.isValid())
  {
    return Qt::ItemFlag::ItemIsEditable | Superclass::flags(index);
  }

  return Superclass::flags(index);
}

//-----------------------------------------------------------------------------
void pqFileDialogFavoriteModel::LoadFavoritesFromSystem()
{
  vtkPVFileInformation* information = vtkPVFileInformation::New();

  if (this->Server)
  {
    vtkSMSessionProxyManager* pxm = this->Server->proxyManager();

    vtkSMProxy* helper = pxm->NewProxy("misc", "FileInformationHelper");
    pqSMAdaptor::setElementProperty(helper->GetProperty("SpecialDirectories"), true);
    helper->UpdateVTKObjects();
    helper->GatherInformation(information);
    helper->Delete();
  }
  else
  {
    vtkPVFileInformationHelper* helper = vtkPVFileInformationHelper::New();
    helper->SetSpecialDirectories(1);
    information->CopyFromObject(helper);
    helper->Delete();
  }

  this->FavoriteList.clear();
  if (pqFileDialogFavoriteModel::AddExamplesInFavorites)
  {
    // Adds the Examples entry in the favorites
    // This is using `_examples_path_` as a placeholder because storing the absolute path to the
    // `Examples` directory causes issues when switching to another ParaView version, because this
    // path would still point to the previous version's examples. This entry is added even if the
    // `Examples` directory is not present.
    this->FavoriteList.push_back(pqFileDialogFavoriteModelFileInfo{
      "Examples", "_examples_path_", vtkPVFileInformation::DIRECTORY });
  }
  vtkCollectionIterator* iter = information->GetContents()->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkPVFileInformation* cur_info = vtkPVFileInformation::SafeDownCast(iter->GetCurrentObject());
    if (!cur_info)
    {
      continue;
    }
    this->FavoriteList.push_back(pqFileDialogFavoriteModelFileInfo{
      cur_info->GetName(), QDir::cleanPath(cur_info->GetFullPath()), cur_info->GetType() });
  }

  iter->Delete();
  information->Delete();
}
