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

/////////////////////////////////////////////////////////////////////
// Icons
// NOLINTNEXTLINE(readability-redundant-member-init)
Q_GLOBAL_STATIC(pqFileDialogModelIconProvider, Icons);

//////////////////////////////////////////////////////////////////////
// FileInfo

struct pqFileDialogFavoriteModelFileInfo
{
  pqFileDialogFavoriteModelFileInfo() = default;

  pqFileDialogFavoriteModelFileInfo(const QString& l, const QString& filepath, int t)
    : Label(l)
    , FilePath(filepath)
    , Type(t)
  {
  }

  QString Label;
  QString FilePath;
  int Type;
};

//////////////////////////////////////////////////////////////////
// FavoriteModel

class pqFileDialogFavoriteModel::pqImplementation
{
public:
  pqServer* Server;
  QList<pqFileDialogFavoriteModelFileInfo> FavoriteList;
  QString SettingsKey;

  //-----------------------------------------------------------------------------
  pqImplementation(pqServer* server)
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
        this->FavoriteList.push_back(pqFileDialogFavoriteModelFileInfo{
          fileInfoList[0].toString(), fileInfoList[1].toString(), fileInfoList[2].toInt() });
      }
    }
    else
    {
      this->LoadFavoritesFromSystem();
    }
    this->SettingsKey = key;
  }

  //-----------------------------------------------------------------------------
  void LoadFavoritesFromSystem()
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
    vtkCollectionIterator* iter = information->GetContents()->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkPVFileInformation* cur_info = vtkPVFileInformation::SafeDownCast(iter->GetCurrentObject());
      if (!cur_info)
      {
        continue;
      }
      this->FavoriteList.push_back(pqFileDialogFavoriteModelFileInfo(
        cur_info->GetName(), QDir::cleanPath(cur_info->GetFullPath()), cur_info->GetType()));
    }

    iter->Delete();
    information->Delete();
  }

  //-----------------------------------------------------------------------------
  ~pqImplementation()
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
};

//-----------------------------------------------------------------------------
pqFileDialogFavoriteModel::pqFileDialogFavoriteModel(pqServer* server, QObject* p)
  : base(p)
  , Implementation(new pqImplementation(server))
{
}

//-----------------------------------------------------------------------------
pqFileDialogFavoriteModel::~pqFileDialogFavoriteModel()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
QString pqFileDialogFavoriteModel::filePath(const QModelIndex& Index) const
{
  if (Index.row() < this->Implementation->FavoriteList.size())
  {
    pqFileDialogFavoriteModelFileInfo& file = this->Implementation->FavoriteList[Index.row()];
    return file.FilePath;
  }
  return QString();
}

//-----------------------------------------------------------------------------
bool pqFileDialogFavoriteModel::isDir(const QModelIndex& Index) const
{
  if (Index.row() >= this->Implementation->FavoriteList.size())
    return false;

  pqFileDialogFavoriteModelFileInfo& file = this->Implementation->FavoriteList[Index.row()];
  return vtkPVFileInformation::IsDirectory(file.Type);
}

//-----------------------------------------------------------------------------
QVariant pqFileDialogFavoriteModel::data(const QModelIndex& idx, int role) const
{
  if (!idx.isValid())
    return QVariant();

  if (idx.row() >= this->Implementation->FavoriteList.size())
    return QVariant();

  if (idx.column() != 0)
  {
    return QVariant();
  }

  const pqFileDialogFavoriteModelFileInfo& file = this->Implementation->FavoriteList[idx.row()];
  switch (role)
  {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return file.Label;
    case Qt::ItemDataRole::ToolTipRole:
      if (QDir(file.FilePath).exists())
      {
        return file.FilePath;
      }
      else
      {
        return file.FilePath + " (Warning: does not exist)";
      }
    case Qt::ItemDataRole::FontRole:
      if (QDir(file.FilePath).exists())
      {
        return QFont{};
      }
      else
      {
        QFont font;
        font.setItalic(true);
        return font;
      }

    case Qt::ItemDataRole::ForegroundRole:
      if (QDir(file.FilePath).exists())
      {
        return QBrush{};
      }
      else
      {
        QBrush brush;
        brush.setColor(QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
        return brush;
      }

    case Qt::DecorationRole:
      if (QDir(file.FilePath).exists())
      {
        return Icons()->icon(static_cast<vtkPVFileInformation::FileTypes>(file.Type));
      }
      else
      {
        return QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
      }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
int pqFileDialogFavoriteModel::rowCount(const QModelIndex&) const
{
  return this->Implementation->FavoriteList.size();
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
  QFileInfo fileInfo(dirPath);
  if (!fileInfo.isDir())
  {
    return;
  }

  QString const cleanDirPath = QDir::cleanPath(fileInfo.absoluteFilePath());

  QList<pqFileDialogFavoriteModelFileInfo>& favoriteList = this->Implementation->FavoriteList;
  auto foundIter = std::find_if(favoriteList.begin(), favoriteList.end(),
    [&cleanDirPath](pqFileDialogFavoriteModelFileInfo const& fileInfo) {
      return fileInfo.FilePath == cleanDirPath;
    });

  if (foundIter != favoriteList.end())
  {
    // No need to add to favorites because it is already present
    return;
  }

  int type = fileInfo.isSymLink() ? vtkPVFileInformation::FileTypes::DIRECTORY_LINK
                                  : vtkPVFileInformation::FileTypes::DIRECTORY;

  this->beginInsertRows(QModelIndex(), favoriteList.size(), favoriteList.size());
  favoriteList.push_back(
    pqFileDialogFavoriteModelFileInfo{ fileInfo.baseName(), cleanDirPath, type });
  this->endInsertRows();
}

//-----------------------------------------------------------------------------
void pqFileDialogFavoriteModel::removeFromFavorites(QString const& dirPath)
{
  QString const cleanDirPath = QDir::cleanPath(QFileInfo(dirPath).absoluteFilePath());

  QList<pqFileDialogFavoriteModelFileInfo>& favoriteList = this->Implementation->FavoriteList;
  auto foundIter = std::find_if(favoriteList.begin(), favoriteList.end(),
    [&](pqFileDialogFavoriteModelFileInfo const& fileInfo) {
      return fileInfo.FilePath == cleanDirPath;
    });

  if (foundIter == favoriteList.end())
  {
    return;
  }

  int const row = std::distance(favoriteList.begin(), foundIter);
  this->beginRemoveRows(QModelIndex(), row, row);
  favoriteList.erase(foundIter);
  this->endRemoveRows();
}

//-----------------------------------------------------------------------------
void pqFileDialogFavoriteModel::resetFavoritesToDefault()
{
  this->beginResetModel();
  this->Implementation->LoadFavoritesFromSystem();
  this->endResetModel();
}

//-----------------------------------------------------------------------------
bool pqFileDialogFavoriteModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (index.isValid() && role == Qt::EditRole)
  {
    this->Implementation->FavoriteList[index.row()].Label = value.toString();

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
    return Qt::ItemFlag::ItemIsEditable | base::flags(index);
  }

  return base::flags(index);
}
