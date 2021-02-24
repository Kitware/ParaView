/*=========================================================================

   Program: ParaView
   Module:    pqFileDialogModel.cxx

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

#include "pqFileDialogModel.h"

#include <algorithm>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QLocale>
#include <QMessageBox>
#include <QStyle>

#include <pqApplicationCore.h>
#include <pqServer.h>
#include <vtkClientServerStream.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkDirectory.h>
#include <vtkPVFileInformation.h>
#include <vtkPVFileInformationHelper.h>
#include <vtkPVSession.h>
#include <vtkSMDirectoryProxy.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSettings.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSmartPointer.h>
#include <vtkStringList.h>

#include "pqObjectBuilder.h"
#include "pqSMAdaptor.h"

//////////////////////////////////////////////////////////////////////
// pqFileDialogModelFileInfo

class pqFileDialogModelFileInfo
{
public:
  pqFileDialogModelFileInfo()
    : Type(vtkPVFileInformation::INVALID)
    , Hidden(false)
  {
  }
  pqFileDialogModelFileInfo(const QString& l, const QString& filepath,
    vtkPVFileInformation::FileTypes t, const bool& h, const QString& extension,
    const long long& sizeVal, const time_t& modificationTimeVal,
    const QList<pqFileDialogModelFileInfo>& g = QList<pqFileDialogModelFileInfo>())
    : Label(l)
    , FilePath(filepath)
    , Type(t)
    , Hidden(h)
    , Group(g)
    , Extension(extension)
    , ModificationTime(modificationTimeVal)
    , Size(sizeVal)
  {
  }

  const QString& label() const { return this->Label; }

  const QString& filePath() const { return this->FilePath; }

  vtkPVFileInformation::FileTypes type() const { return this->Type; }

  bool isGroup() const { return !this->Group.empty(); }

  bool isHidden() const { return this->Hidden; }

  const QList<pqFileDialogModelFileInfo>& group() const { return this->Group; }

  QString extensionTypeString() const
  {
    if (this->Type == vtkPVFileInformation::DIRECTORY)
    {
      return QObject::tr("Folder");
    }
    else
    {
      return QString("%1%2%3")
        .arg(this->Extension)
        .arg(this->Extension.isEmpty() ? "" : " ")
        .arg(QObject::tr("File"));
    }
  }

  QString sizeString() const
  {
    if (this->Type == vtkPVFileInformation::DIRECTORY)
    {
      return QString();
    }
    const long long kb = 1024;
    const long long mb = 1024 * kb;
    const long long gb = 1024 * mb;
    const long long tb = 1024 * gb;

    if (this->Size >= tb)
    {
      return QString("%1 TB").arg(QLocale().toString(qreal(this->Size) / tb, 'f', 3));
    }
    if (this->Size >= gb)
    {
      return QString("%1 GB").arg(QLocale().toString(qreal(this->Size) / gb, 'f', 2));
    }
    if (this->Size >= mb)
    {
      return QString("%1 MB").arg(QLocale().toString(qreal(this->Size) / mb, 'f', 1));
    }
    if (this->Size >= kb)
    {
      return QString("%1 KB").arg(QLocale().toString(qreal(this->Size) / kb, 'f', 1));
    }
    return QString("%1 bytes").arg(QLocale().toString(static_cast<qulonglong>(this->Size)));
  }

  QString modificationTimeString() const
  {
    return QLocale::system().toString(
      QDateTime::fromTime_t(this->ModificationTime), QLocale::ShortFormat);
  }

  qulonglong size() const { return static_cast<qulonglong>(this->Size); }

  QDateTime modificationTime() const { return QDateTime::fromTime_t(this->ModificationTime); }

private:
  QString Label;
  QString FilePath;
  vtkPVFileInformation::FileTypes Type;
  bool Hidden;
  QList<pqFileDialogModelFileInfo> Group;
  QString Extension;
  time_t ModificationTime;
  long long Size;
};

/////////////////////////////////////////////////////////////////////
// Icons

pqFileDialogModelIconProvider::pqFileDialogModelIconProvider()
{
  QStyle* style = QApplication::style();
  this->FolderLinkIcon = style->standardIcon(QStyle::SP_DirLinkIcon);
  this->FileLinkIcon = style->standardIcon(QStyle::SP_FileLinkIcon);
  this->DomainIcon.addPixmap(QPixmap(":/pqCore/Icons/pqDomain16.png"));
  this->NetworkIcon.addPixmap(QPixmap(":/pqCore/Icons/pqNetwork16.png"));
}

QIcon pqFileDialogModelIconProvider::icon(IconType t) const
{
  switch (t)
  {
    case Computer:
      return QFileIconProvider::icon(QFileIconProvider::Computer);
    case Drive:
      return QFileIconProvider::icon(QFileIconProvider::Drive);
    case Folder:
      return QFileIconProvider::icon(QFileIconProvider::Folder);
    case File:
      return QFileIconProvider::icon(QFileIconProvider::File);
    case FolderLink:
      return this->FolderLinkIcon;
    case FileLink:
      return this->FileLinkIcon;
    case NetworkFolder:
      return QFileIconProvider::icon(QFileIconProvider::Network);
    case NetworkRoot:
      return this->NetworkIcon;
    case NetworkDomain:
      return this->DomainIcon;
  }
  return QIcon();
}

QIcon pqFileDialogModelIconProvider::icon(vtkPVFileInformation::FileTypes f) const
{
  if (f == vtkPVFileInformation::DIRECTORY_LINK)
  {
    return icon(pqFileDialogModelIconProvider::FolderLink);
  }
  else if (f == vtkPVFileInformation::SINGLE_FILE_LINK)
  {
    return icon(pqFileDialogModelIconProvider::FileLink);
  }
  else if (f == vtkPVFileInformation::NETWORK_SHARE)
  {
    return icon(pqFileDialogModelIconProvider::NetworkFolder);
  }
  else if (f == vtkPVFileInformation::NETWORK_SERVER)
  {
    return icon(pqFileDialogModelIconProvider::Computer);
  }
  else if (f == vtkPVFileInformation::NETWORK_DOMAIN)
  {
    return icon(pqFileDialogModelIconProvider::NetworkDomain);
  }
  else if (f == vtkPVFileInformation::NETWORK_ROOT)
  {
    return icon(pqFileDialogModelIconProvider::NetworkRoot);
  }
  else if (vtkPVFileInformation::IsDirectory(f))
  {
    return icon(pqFileDialogModelIconProvider::Folder);
  }
  return icon(pqFileDialogModelIconProvider::File);
}
QIcon pqFileDialogModelIconProvider::icon(const QFileInfo& info) const
{
  return QFileIconProvider::icon(info);
}
QIcon pqFileDialogModelIconProvider::icon(QFileIconProvider::IconType ico) const
{
  return QFileIconProvider::icon(ico);
}

Q_GLOBAL_STATIC(pqFileDialogModelIconProvider, Icons);

namespace
{

///////////////////////////////////////////////////////////////////////
// CaseInsensitiveSort

bool CaseInsensitiveSort(const pqFileDialogModelFileInfo& A, const pqFileDialogModelFileInfo& B)
{
  // Sort alphabetically (but case-insensitively)
  return A.label().toLower() < B.label().toLower();
}

class CaseInsensitiveSortGroup
  : public std::binary_function<pqFileDialogModelFileInfo, pqFileDialogModelFileInfo, bool>
{
public:
  CaseInsensitiveSortGroup(const QString& groupName) { this->numPrefix = groupName.length(); }
  bool operator()(const pqFileDialogModelFileInfo& A, const pqFileDialogModelFileInfo& B) const
  {
    QString aLabel = A.label();
    QString bLabel = B.label();
    aLabel = aLabel.right(aLabel.length() - numPrefix);
    bLabel = bLabel.right(bLabel.length() - numPrefix);
    return aLabel < bLabel;
  }

private:
  int numPrefix;
};

bool getShowDetailedInformationSetting()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSMProxy* helperProxy = pxm->NewProxy("misc", "FileInformationHelper");
  settings->GetProxySettings(helperProxy);
  bool result = vtkSMPropertyHelper(helperProxy, "ReadDetailedFileInformation").GetAsInt() == 1;
  helperProxy->Delete();
  return result;
}

void setShowDetailedInformationSetting(pqServer* server, bool show)
{
  if (!server)
  {
    server = pqApplicationCore::instance()->getActiveServer();
  }
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSMProxy* helperProxy = pxm->NewProxy("misc", "FileInformationHelper");
  vtkSMPropertyHelper(helperProxy, "ReadDetailedFileInformation").Set(show ? 1 : 0);
  helperProxy->UpdateVTKObjects();
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->SetProxySettings(helperProxy);
  helperProxy->Delete();
}

} // namespace

/////////////////////////////////////////////////////////////////////////
// pqFileDialogModel::Implementation

class pqFileDialogModel::pqImplementation
{
public:
  pqImplementation(pqServer* server)
    : Separator(0)
    , Server(server)
  {

    // if we are doing remote browsing
    if (server)
    {
      vtkSMSessionProxyManager* pxm = server->proxyManager();

      vtkSMProxy* helper = pxm->NewProxy("misc", "FileInformationHelper");
      this->FileInformationHelperProxy = helper;
      helper->Delete();
      helper->UpdateVTKObjects();
      helper->UpdatePropertyInformation();
      QString separator =
        pqSMAdaptor::getElementProperty(helper->GetProperty("PathSeparator")).toString();
      this->Separator = separator.toLocal8Bit().data()[0];
      // Since this isn't going through the proxy widget we have to manually restore the setting
      vtkSMPropertyHelper(helper, "ReadDetailedFileInformation")
        .Set(getShowDetailedInformationSetting());
    }
    else
    {
      vtkPVFileInformationHelper* helper = vtkPVFileInformationHelper::New();
      this->FileInformationHelper = helper;
      helper->Delete();
      helper->SetReadDetailedFileInformation(getShowDetailedInformationSetting());
      this->Separator = helper->GetPathSeparator()[0];
    }

    this->FileInformation.TakeReference(vtkPVFileInformation::New());

    // current path
    vtkPVFileInformation* info = this->GetData(false, "", ".", false);
    this->CurrentPath = info->GetFullPath();
  }

  ~pqImplementation() = default;

  /// Removes multiple-slashes, ".", and ".." from the given path string,
  /// and points slashes in the correct direction for the server
  QString cleanPath(const QString& path)
  {
    QString result = QDir::cleanPath(QDir::fromNativeSeparators(path));
    return result.trimmed();
  }

  /// query the file system for information
  vtkPVFileInformation* GetData(bool dirListing, const QString& path, bool specialDirs)
  {
    return this->GetData(dirListing, this->CurrentPath, path, specialDirs);
  }

  /// query the file system for information
  vtkPVFileInformation* GetData(
    bool dirListing, const QString& workingDir, const QString& path, bool specialDirs)
  {
    if (this->FileInformationHelperProxy)
    {
      // send data to server
      vtkSMProxy* helper = this->FileInformationHelperProxy;
      pqSMAdaptor::setElementProperty(helper->GetProperty("WorkingDirectory"), workingDir.toUtf8());
      pqSMAdaptor::setElementProperty(helper->GetProperty("DirectoryListing"), dirListing);
      pqSMAdaptor::setElementProperty(helper->GetProperty("Path"), path.toUtf8());
      pqSMAdaptor::setElementProperty(helper->GetProperty("SpecialDirectories"), specialDirs);
      helper->UpdateVTKObjects();

      // get data from server
      this->FileInformation->Initialize();
      this->FileInformationHelperProxy->GatherInformation(this->FileInformation);
    }
    else
    {
      vtkPVFileInformationHelper* helper = this->FileInformationHelper;
      helper->SetDirectoryListing(dirListing);
      helper->SetPath(path.toUtf8().data());
      helper->SetSpecialDirectories(specialDirs);
      helper->SetWorkingDirectory(workingDir.toUtf8().data());
      this->FileInformation->CopyFromObject(helper);
    }
    return this->FileInformation;
  }

  /// put queried information into our model
  void Update(const QString& path, vtkPVFileInformation* dir)
  {
    this->CurrentPath = path;
    this->FileList.clear();

    QList<pqFileDialogModelFileInfo> dirs;
    QList<pqFileDialogModelFileInfo> files;

    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(dir->GetContents()->NewIterator());

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkPVFileInformation* info = vtkPVFileInformation::SafeDownCast(iter->GetCurrentObject());
      if (!info)
      {
        continue;
      }
      if (vtkPVFileInformation::IsDirectory(info->GetType()))
      {
        dirs.push_back(pqFileDialogModelFileInfo(QString::fromUtf8(info->GetName()),
          QString::fromUtf8(info->GetFullPath()),
          static_cast<vtkPVFileInformation::FileTypes>(info->GetType()), info->GetHidden(),
          info->GetExtension(), info->GetSize(), info->GetModificationTime()));
      }
      else if (info->GetType() != vtkPVFileInformation::FILE_GROUP &&
        info->GetType() != vtkPVFileInformation::DIRECTORY_GROUP)
      {
        files.push_back(pqFileDialogModelFileInfo(QString::fromUtf8(info->GetName()),
          QString::fromUtf8(info->GetFullPath()),
          static_cast<vtkPVFileInformation::FileTypes>(info->GetType()), info->GetHidden(),
          info->GetExtension(), info->GetSize(), info->GetModificationTime()));
      }
      else if (info->GetType() == vtkPVFileInformation::FILE_GROUP ||
        info->GetType() == vtkPVFileInformation::DIRECTORY_GROUP)
      {
        QList<pqFileDialogModelFileInfo> groupFiles;
        vtkSmartPointer<vtkCollectionIterator> childIter;
        childIter.TakeReference(info->GetContents()->NewIterator());
        for (childIter->InitTraversal(); !childIter->IsDoneWithTraversal();
             childIter->GoToNextItem())
        {
          vtkPVFileInformation* child =
            vtkPVFileInformation::SafeDownCast(childIter->GetCurrentObject());
          groupFiles.push_back(pqFileDialogModelFileInfo(/*QString::fromUtf8*/ (child->GetName()),
            /*QString::fromUtf8*/ (child->GetFullPath()),
            static_cast<vtkPVFileInformation::FileTypes>(child->GetType()), child->GetHidden(),
            child->GetExtension(), child->GetSize(), child->GetModificationTime()));
        }

        const bool as_files = (info->GetType() == vtkPVFileInformation::FILE_GROUP);
        auto& container = as_files ? files : dirs;
        container.push_back(pqFileDialogModelFileInfo(/*QString::fromUtf8*/ (info->GetName()),
          groupFiles[0].filePath(),
          (as_files ? vtkPVFileInformation::SINGLE_FILE : vtkPVFileInformation::DIRECTORY),
          info->GetHidden(), info->GetExtension(), info->GetSize(), info->GetModificationTime(),
          groupFiles));
      }
    }

    std::sort(dirs.begin(), dirs.end(), CaseInsensitiveSort);
    std::sort(files.begin(), files.end(), CaseInsensitiveSort);

    for (int i = 0; i != dirs.size(); ++i)
    {
      this->FileList.push_back(dirs[i]);
    }
    for (int i = 0; i != files.size(); ++i)
    {
      this->FileList.push_back(files[i]);
    }
  }

  QStringList getFilePaths(const QModelIndex& index)
  {
    QStringList results;

    QModelIndex p = index.parent();
    if (p.isValid())
    {
      if (p.row() < this->FileList.size())
      {
        pqFileDialogModelFileInfo& file = this->FileList[p.row()];
        const QList<pqFileDialogModelFileInfo>& grp = file.group();
        if (index.row() < grp.size())
        {
          results.push_back(grp[index.row()].filePath());
        }
      }
    }
    else if (index.row() < this->FileList.size())
    {
      pqFileDialogModelFileInfo& file = this->FileList[index.row()];
      if (file.isGroup() && file.group().count() > 0)
      {
        for (int i = 0; i < file.group().count(); i++)
        {
          results.push_back(file.group().at(i).filePath());
        }
      }
      else
      {
        results.push_back(file.filePath());
      }
    }

    return results;
  }

  bool isHidden(const QModelIndex& index)
  {
    const pqFileDialogModelFileInfo* info = this->infoForIndex(index);
    return info ? info->isHidden() : false;
  }

  bool isDir(const QModelIndex& index)
  {
    const pqFileDialogModelFileInfo* info = this->infoForIndex(index);
    return info ? vtkPVFileInformation::IsDirectory(info->type()) : false;
  }

  bool isRemote() { return this->Server; }

  pqServer* getServer() { return this->Server; }

  /// Path separator for the connected server's filesystem.
  char Separator;

  /// Current path being displayed (server's filesystem).
  QString CurrentPath;
  /// Caches information about the set of files within the current path.
  QVector<pqFileDialogModelFileInfo> FileList; // adjacent memory occupation for QModelIndex

  const pqFileDialogModelFileInfo* infoForIndex(const QModelIndex& idx) const
  {
    if (idx.isValid() && nullptr == idx.internalPointer() && idx.row() >= 0 &&
      idx.row() < this->FileList.size())
    {
      return &this->FileList[idx.row()];
    }
    else if (idx.isValid() && idx.internalPointer())
    {
      pqFileDialogModelFileInfo* ptr =
        reinterpret_cast<pqFileDialogModelFileInfo*>(idx.internalPointer());
      const QList<pqFileDialogModelFileInfo>& grp = ptr->group();
      if (idx.row() >= 0 && idx.row() < grp.size())
      {
        return &grp[idx.row()];
      }
    }
    return nullptr;
  }

  void setShowDetailedInformation(bool show)
  {
    if (this->FileInformationHelperProxy)
    {
      vtkSMPropertyHelper(this->FileInformationHelperProxy, "ReadDetailedFileInformation")
        .Set(show);
    }
    else if (this->FileInformationHelper)
    {
      this->FileInformationHelper->SetReadDetailedFileInformation(show);
    }
    setShowDetailedInformationSetting(this->Server, show);
  }

  bool isShowingDetailedInformation() const
  {
    bool currentValue = false;
    if (this->FileInformationHelperProxy)
    {
      currentValue =
        vtkSMPropertyHelper(this->FileInformationHelperProxy, "ReadDetailedFileInformation")
          .GetAsInt() == 1;
    }
    else if (this->FileInformationHelper)
    {
      currentValue = this->FileInformationHelper->GetReadDetailedFileInformation();
    }
    return currentValue;
  }

private:
  // server vs. local implementation private
  pqServer* Server;

  vtkSmartPointer<vtkPVFileInformationHelper> FileInformationHelper;
  vtkSmartPointer<vtkSMProxy> FileInformationHelperProxy;
  vtkSmartPointer<vtkPVFileInformation> FileInformation;
};

//////////////////////////////////////////////////////////////////////////
// pqFileDialogModel
pqFileDialogModel::pqFileDialogModel(pqServer* _server, QObject* Parent)
  : base(Parent)
  , Implementation(new pqImplementation(_server))
{
}

pqFileDialogModel::~pqFileDialogModel()
{
  delete this->Implementation;
}

bool pqFileDialogModel::isShowingDetailedInfo()
{
  return this->Implementation->isShowingDetailedInformation();
}

void pqFileDialogModel::setShowDetailedInfo(bool show)
{
  this->Implementation->setShowDetailedInformation(show);
  this->setCurrentPath(this->getCurrentPath());
}

pqServer* pqFileDialogModel::server() const
{
  return this->Implementation->getServer();
}

void pqFileDialogModel::setCurrentPath(const QString& path)
{
  this->beginResetModel();
  QString cPath = this->Implementation->cleanPath(path);
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(true, cPath, false);
  this->Implementation->Update(cPath, info);
  this->endResetModel();
}

QString pqFileDialogModel::getCurrentPath()
{
  return this->Implementation->CurrentPath;
}

QString pqFileDialogModel::absoluteFilePath(const QString& path)
{
  if (path.isEmpty())
  {
    return QString();
  }

  vtkPVFileInformation* info;
  info = this->Implementation->GetData(false, path, false);
  return this->Implementation->cleanPath(QString::fromUtf8(info->GetFullPath()));
}

QStringList pqFileDialogModel::getFilePaths(const QModelIndex& localIndex)
{
  if (localIndex.model() == this)
  {
    return this->Implementation->getFilePaths(localIndex);
  }
  return QStringList();
}

bool pqFileDialogModel::isHidden(const QModelIndex& localIndex)
{
  if (localIndex.model() == this)
  {
    return this->Implementation->isHidden(localIndex);
  }
  return false;
}

bool pqFileDialogModel::isDir(const QModelIndex& localIndex)
{
  if (localIndex.model() == this)
  {
    return this->Implementation->isDir(localIndex);
  }

  return false;
}

bool pqFileDialogModel::fileExists(const QString& file, QString& fullpath)
{
  QString filePath = this->Implementation->cleanPath(file);
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(false, filePath, false);

  // try again for shortcut
  if (info->GetType() != vtkPVFileInformation::SINGLE_FILE)
  {
    info = this->Implementation->GetData(false, filePath + ".lnk", false);
  }

  if (info->GetType() == vtkPVFileInformation::SINGLE_FILE)
  {
    fullpath = info->GetFullPath();
    return true;
  }
  return false;
}

bool pqFileDialogModel::mkdir(const QString& dirName)
{
  QString path;
  QString dirPath = this->absoluteFilePath(dirName);
  if (this->dirExists(dirPath, path))
  {
    return false;
  }

  bool ret = false;

  if (this->Implementation->isRemote())
  {
    vtkSMDirectoryProxy* dirProxy = vtkSMDirectoryProxy::SafeDownCast(
      this->Implementation->getServer()->proxyManager()->NewProxy("misc", "Directory"));
    dirProxy->SetLocation(vtkPVSession::SERVERS);
    ret = dirProxy->MakeDirectory(dirPath.toUtf8().data());
    dirProxy->Delete();
  }
  else
  {
    // File system is local.
    ret = (vtkDirectory::MakeDirectory(dirPath.toLocal8Bit().data()) != 0);
  }

  this->beginResetModel();
  QString cPath = this->Implementation->cleanPath(this->getCurrentPath());
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(true, cPath, false);
  this->Implementation->Update(cPath, info);
  this->endResetModel();

  return ret;
}

bool pqFileDialogModel::rmdir(const QString& dirName)
{
  QString path;
  QString dirPath = this->absoluteFilePath(dirName);
  if (!this->dirExists(dirPath, path))
  {
    return false;
  }

  bool ret = false;

  if (this->Implementation->isRemote())
  {
    vtkSMDirectoryProxy* dirProxy = vtkSMDirectoryProxy::SafeDownCast(
      this->Implementation->getServer()->proxyManager()->NewProxy("misc", "Directory"));
    ret = dirProxy->DeleteDirectory(dirPath.toUtf8().data());
    dirProxy->Delete();
  }
  else
  {
    // File system is local.
    ret = (vtkDirectory::DeleteDirectory(dirPath.toLocal8Bit().data()) != 0);
  }

  this->beginResetModel();
  QString cPath = this->Implementation->cleanPath(this->getCurrentPath());
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(true, cPath, false);
  this->Implementation->Update(cPath, info);
  this->endResetModel();

  return ret;
}

bool pqFileDialogModel::rename(const QString& oldname, const QString& newname)
{
  QString oldPath = this->absoluteFilePath(oldname);
  QString newPath = this->absoluteFilePath(newname);

  if (oldPath == newPath)
  {
    return true;
  }

  vtkPVFileInformation* info;
  info = this->Implementation->GetData(false, oldPath, false);

  int oldType = info->GetType();

  if (oldType != vtkPVFileInformation::SINGLE_FILE && !vtkPVFileInformation::IsDirectory(oldType))
  {
    return false;
  }

  // don't replace file/dir
  info = this->Implementation->GetData(false, newPath, false);
  if (info->GetType() == oldType)
  {
    QString message("Cannot rename to %1, which already exists");
    message = message.arg(newname);
    QMessageBox::warning(nullptr, "Error renaming", message);
    return false;
  }

  bool ret = false;

  if (this->Implementation->isRemote())
  {
    vtkSMDirectoryProxy* dirProxy = vtkSMDirectoryProxy::SafeDownCast(
      this->Implementation->getServer()->proxyManager()->NewProxy("misc", "Directory"));
    ret = dirProxy->Rename(oldPath.toUtf8().data(), newPath.toUtf8().data());
    dirProxy->Delete();
  }
  else
  {
    ret = (vtkDirectory::Rename(oldPath.toLocal8Bit().data(), newPath.toLocal8Bit().data()) != 0);
  }

  this->beginResetModel();
  QString cPath = this->Implementation->cleanPath(this->getCurrentPath());
  info = this->Implementation->GetData(true, cPath, false);
  this->Implementation->Update(cPath, info);
  this->endResetModel();

  return ret;
}

bool pqFileDialogModel::dirExists(const QString& path, QString& fullpath)
{
  QString dir = this->Implementation->cleanPath(path);
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(false, dir, false);

  // try again for shortcuts
  if (!vtkPVFileInformation::IsDirectory(info->GetType()))
  {
    info = this->Implementation->GetData(false, dir + ".lnk", false);
  }

  if (vtkPVFileInformation::IsDirectory(info->GetType()))
  {
    fullpath = info->GetFullPath();
    return true;
  }
  return false;
}

QChar pqFileDialogModel::separator() const
{
  return this->Implementation->Separator;
}

int pqFileDialogModel::columnCount(const QModelIndex& idx) const
{
  const pqFileDialogModelFileInfo* file = this->Implementation->infoForIndex(idx);
  if (!file)
  {
    // should never get here anyway
    return 4;
  }
  return file->group().size() + 4;
}

QVariant pqFileDialogModel::data(const QModelIndex& idx, int role) const
{

  const pqFileDialogModelFileInfo* file;

  if (idx.column() <= 3)
  {
    file = this->Implementation->infoForIndex(idx);
  }
  else
  {
    file = this->Implementation->infoForIndex(idx.parent());
  }

  if (!file)
  {
    // should never get here anyway
    return QVariant();
  }

  int grSize = file->group().size();
  if (role == Qt::DisplayRole || role == Qt::EditRole)
  {
    if (grSize == 0)
    {
      switch (idx.column())
      {
        case 0:
          return file->label();
          break;
        case 1:
          return file->extensionTypeString();
          break;
        case 2:
          return file->sizeString();
          break;
        case 3:
          return file->modificationTimeString();
          break;
        default:
          return QVariant();
          break;
      }
    }
    else if (grSize > 0)
    {
      switch (idx.column())
      {
        case 0:
          return file->label();
          break;
        case 1:
          return tr("Group");
          break;
        case 2:
          return QVariant();
          break;
        case 3:
          return QVariant();
          break;
        default:
          return file->group().at(idx.column() - 4).label();
          break;
      }
    }
  }
  else if (role == Qt::UserRole)
  {
    if (grSize == 0)
    {
      switch (idx.column())
      {
        case 0:
          return file->filePath();
          break;
        case 1:
          return static_cast<int>(file->type());
          break;
        case 2:
          return static_cast<qulonglong>(file->size());
          break;
        case 3:
          return file->modificationTime();
          break;
        default:
          return QVariant();
          break;
      }
    }
    else if (grSize > 0)
    {
      switch (idx.column())
      {
        case 0:
          return file->filePath();
          break;
        case 1:
          return static_cast<int>(file->type());
          break;
        case 2:
          return QVariant();
          break;
        case 3:
          return QVariant();
          break;
        default:
          return file->group().at(idx.column() - 4).filePath();
          break;
      }
    }
  }
  else if (role == Qt::DecorationRole && idx.column() == 0)
  {
    return Icons()->icon(file->type());
  }

  return QVariant();
}

QModelIndex pqFileDialogModel::index(int row, int column, const QModelIndex& p) const
{
  if (!p.isValid())
  {
    return this->createIndex(row, column);
  }
  if (p.row() >= 0 && p.row() < this->Implementation->FileList.size() &&
    nullptr == p.internalPointer())
  {
    pqFileDialogModelFileInfo* fi = &this->Implementation->FileList[p.row()];
    return this->createIndex(row, column, fi);
  }

  return QModelIndex();
}

QModelIndex pqFileDialogModel::parent(const QModelIndex& idx) const
{
  if (!idx.isValid() || !idx.internalPointer())
  {
    return QModelIndex();
  }

  const pqFileDialogModelFileInfo* ptr =
    reinterpret_cast<pqFileDialogModelFileInfo*>(idx.internalPointer());
  int row = ptr - &this->Implementation->FileList.first();
  return this->createIndex(row, idx.column());
}

int pqFileDialogModel::rowCount(const QModelIndex& idx) const
{
  if (!idx.isValid())
  {
    return this->Implementation->FileList.size();
  }

  if (nullptr == idx.internalPointer() && idx.row() >= 0 &&
    idx.row() < this->Implementation->FileList.size())
  {
    return this->Implementation->FileList[idx.row()].group().size();
  }

  return 0;
}

bool pqFileDialogModel::hasChildren(const QModelIndex& idx) const
{
  if (!idx.isValid())
    return true;

  if (nullptr == idx.internalPointer() && idx.row() >= 0 &&
    idx.row() < this->Implementation->FileList.size())
  {
    return this->Implementation->FileList[idx.row()].isGroup();
  }

  return false;
}

QVariant pqFileDialogModel::headerData(int section, Qt::Orientation, int role) const
{
  switch (role)
  {
    case Qt::DisplayRole:
      switch (section)
      {
        case 0:
          return tr("Filename");
          break;
        case 1:
          return tr("Type");
          break;
        case 2:
          return tr("Size");
          break;
        case 3:
          return tr("Date Modified");
          break;
      }
  }

  return QVariant();
}

bool pqFileDialogModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  if (role != Qt::DisplayRole && role != Qt::EditRole)
  {
    return false;
  }

  const pqFileDialogModelFileInfo* file = this->Implementation->infoForIndex(idx);

  if (!file)
  {
    return false;
  }

  QString name = value.toString();
  return this->rename(file->filePath(), name);
}

Qt::ItemFlags pqFileDialogModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags ret = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  const pqFileDialogModelFileInfo* file = this->Implementation->infoForIndex(idx);
  if (file && !file->isGroup())
  {
    ret |= Qt::ItemIsEditable;
  }
  return ret;
}
