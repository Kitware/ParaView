/*=========================================================================

   Program: ParaView
   Module:    pqServerFileDialogModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqServerFileDialogModel.h"

#include <pqServer.h>
#include <QFileIconProvider>
#include <vtkClientServerStream.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkProcessModule.h>
#include <vtkPVFileInformation.h>
#include <vtkSmartPointer.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkStringList.h>

#include "pqSMAdaptor.h"
namespace
{
  
/// The last path accessed by this file dialog model
/// used to remember paths across the session
QString gLastPath;

/////////////////////////////////////////////////////////////////////
// Icons

QFileIconProvider& Icons()
{
  static QFileIconProvider* icons = 0;
  if(!icons)
    icons = new QFileIconProvider();
    
  return *icons;
}

//////////////////////////////////////////////////////////////////////
// FileInfo

class FileInfo
{
public:
  FileInfo()
  {
  }

  FileInfo(const QString& l, const QString& filepath, const bool isdir, const bool isroot) :
    Label(l),
    FilePath(filepath),
    IsDir(isdir),
    IsRoot(isroot)
  {
  }

  const QString& label() const
  {
    return this->Label;
  }

  const QString& filePath() const 
  {
    return this->FilePath;
  }
  
  const bool isDir() const
  {
    return this->IsDir;
  }
  
  const bool isRoot() const
  {
    return this->IsRoot;
  }

private:
  QString Label;
  QString FilePath;
  bool IsDir;
  bool IsRoot;
};

///////////////////////////////////////////////////////////////////////
// CaseInsensitiveSort

bool CaseInsensitiveSort(const QString& A, const QString& B)
{
  // Sort alphabetically (but case-insensitively)
  return A.toLower() < B.toLower();
}

/////////////////////////////////////////////////////////////////////////////////
// pqFileModel

class pqFileModel :
  public QAbstractItemModel
{
public:
  pqFileModel(pqServer* server) : Server(server)
  {
  } 

  void Update(const QString& path, vtkPVFileInformation* dir)
    {
    this->CurrentPath = path;
    this->FileList.clear();

    this->FileList.push_back(FileInfo(".", ".", true, false));
    this->FileList.push_back(FileInfo("..", "..", true, false));
    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(dir->GetContents()->NewIterator());

    QStringList dirs;
    QStringList files;

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkPVFileInformation* info = vtkPVFileInformation::SafeDownCast(
        iter->GetCurrentObject());
      if (!info)
        {
        continue;
        }
      if (info->GetType() == vtkPVFileInformation::DIRECTORY)
        {
        dirs.push_back(info->GetName());
        }
      else if (info->GetType() == vtkPVFileInformation::SINGLE_FILE)
        {
        files.push_back(info->GetName());
        }
      else if (info->GetType() == vtkPVFileInformation::FILE_GROUP)
        {
        // TODO: For now simply expanding the groups and putting them
        // the client may use this grouping information.
        vtkSmartPointer<vtkCollectionIterator> childIter;
        childIter.TakeReference(info->GetContents()->NewIterator());
        for (childIter->InitTraversal(); !childIter->IsDoneWithTraversal();
          childIter->GoToNextItem())
          {
          vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
            childIter->GetCurrentObject());
          files.push_back(child->GetName());
          }
        }
      }

    qSort(dirs.begin(), dirs.end(), CaseInsensitiveSort);
    qSort(files.begin(), files.end(), CaseInsensitiveSort);

    for(int i = 0; i != dirs.size(); ++i)
      {
      const QString directory_name = dirs[i];
      this->FileList.push_back(FileInfo(directory_name, directory_name, true, false));
      }
    for(int i = 0; i != files.size(); ++i)
      {
      const QString file_name = files[i];
      this->FileList.push_back(FileInfo(file_name, file_name, false, false));
      }

    this->reset();
    }

  QStringList getFilePaths(const QModelIndex& Index, const QChar Separator)
    {
    QStringList results;

    if(Index.row() < this->FileList.size())
      { 
      FileInfo& file = this->FileList[Index.row()];
      results.push_back(this->CurrentPath + Separator + file.filePath());
      }

    return results;
    }

  bool isDir(const QModelIndex& Index)
    {
    if(Index.row() >= this->FileList.size())
      return false;

    FileInfo& file = this->FileList[Index.row()];
    return file.isDir();
    }

  int columnCount(const QModelIndex& /*p*/) const
    {
    return 1;
    }

  QVariant data(const QModelIndex & idx, int role) const
    {
    if(!idx.isValid())
      return QVariant();

    if(idx.row() >= this->FileList.size())
      return QVariant();

    const FileInfo& file = this->FileList[idx.row()];

    switch(role)
      {
    case Qt::DisplayRole:
      switch(idx.column())
        {
      case 0:
        return file.filePath();
        }
    case Qt::DecorationRole:
      switch(idx.column())
        {
      case 0:
        return Icons().icon(file.isDir() ? QFileIconProvider::Folder : QFileIconProvider::File);
        }
      }

    return QVariant();
    }

  QModelIndex index(int row, int column, const QModelIndex& /*p*/) const
    {
    return createIndex(row, column);
    }

  QModelIndex parent(const QModelIndex& /*idx*/) const
    {
    return QModelIndex();
    }

  int rowCount(const QModelIndex& /*p*/) const
    {
    return this->FileList.size();
    }

  bool hasChildren(const QModelIndex& p) const
    {
    if(!p.isValid())
      return true;

    return false;
    }

  QVariant headerData(int section, Qt::Orientation /*orientation*/, int role) const
    {
    switch(role)
      {
    case Qt::DisplayRole:
      switch(section)
        {
      case 0:
        return tr("Filename");
        }
      }

    return QVariant();
    }

  /// Connection from which the dir listing is to be fetched.
  pqServer* Server;
  /// Current path being displayed (server's filesystem).
  QString CurrentPath;
  /// Caches information about the set of files within the current path.
  QList<FileInfo> FileList;
};

//////////////////////////////////////////////////////////////////
// FavoriteModel

class pqFavoriteModel :
  public QAbstractItemModel
{
public:
  pqFavoriteModel(pqServer* server):
    Server(server)
  {
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();  
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  vtkSMProxy* helper = pxm->NewProxy("misc","FileInformationHelper");
  helper->SetConnectionID(server->GetConnectionID());
  helper->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
  pqSMAdaptor::setElementProperty(helper->GetProperty("SpecialDirectories"), true);
  helper->UpdateVTKObjects();

  vtkPVFileInformation* information = vtkPVFileInformation::New();
  pm->GatherInformation(server->GetConnectionID(),
    vtkProcessModule::DATA_SERVER, information, helper->GetID(0));

  vtkCollectionIterator* iter = information->GetContents()->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVFileInformation* cur_info = vtkPVFileInformation::SafeDownCast(
      iter->GetCurrentObject());
    if (!cur_info)
      {
      continue;
      }
    int type = cur_info->GetType();
    this->FavoriteList.push_back(FileInfo(
        cur_info->GetName(), cur_info->GetFullPath(),
        (type == vtkPVFileInformation::DIRECTORY || type == vtkPVFileInformation::DRIVE),
        (type == vtkPVFileInformation::DRIVE)));
    }
  iter->Delete();
  helper->Delete();
  information->Delete();
  }

  QStringList getFilePaths(const QModelIndex& Index)
  {
    QStringList results;
    
    if(Index.row() < this->FavoriteList.size())
      {
      FileInfo& file = this->FavoriteList[Index.row()];
      results.push_back(file.filePath());
      }
    
    return results;
  }

  bool isDir(const QModelIndex& Index)
  {
    if(Index.row() >= this->FavoriteList.size())
      return false;
      
    FileInfo& file = this->FavoriteList[Index.row()];
    return file.isDir();
  }

  virtual int columnCount(const QModelIndex& /*parent*/) const
  {
    return 1;
  }
  
  virtual QVariant data(const QModelIndex& idx, int role) const
  {
    if(!idx.isValid())
      return QVariant();

    if(idx.row() >= this->FavoriteList.size())
      return QVariant();

    const FileInfo& file = this->FavoriteList[idx.row()];
    switch(role)
      {
      case Qt::DisplayRole:
        switch(idx.column())
          {
          case 0:
            return file.label();
          }
      case Qt::DecorationRole:
        switch(idx.column())
          {
          case 0:
            if(file.isRoot())
              {
              return Icons().icon(QFileIconProvider::Drive);
              }
            if(file.isDir())
              {
              return Icons().icon(QFileIconProvider::Folder);
              }
            return Icons().icon(QFileIconProvider::File);
          }
      }
      
    return QVariant();
  }
  
  virtual QModelIndex index(int row, int column, const QModelIndex& /*parent*/) const
  {
    return createIndex(row, column);
  }
  
  virtual QModelIndex parent(const QModelIndex& /*index*/) const
  {
    return QModelIndex();
  }
  
  virtual int rowCount(const QModelIndex& /*parent*/) const
  {
    return this->FavoriteList.size();
  }
  
  virtual bool hasChildren(const QModelIndex& p) const
  {
    if(!p.isValid())
      return true;
      
    return false;
  }
  
  virtual QVariant headerData(int section, Qt::Orientation /*orientation*/, int role) const
  {
    switch(role)
      {
      case Qt::DisplayRole:
        switch(section)
          {
          case 0:
            return tr("Favorites");
          }
      }
      
    return QVariant();
  }
 
  pqServer* Server;
  QList<FileInfo> FavoriteList;
};

/// Removes multiple-slashes, ".", and ".." from the given path string,
/// and points slashes in the correct direction for the server
const QString cleanPath(const QString& Path, const char Separator)
{
  QString result = QDir::cleanPath(Path);
  result.replace('/', Separator);
  return result;
}

} // namespace

/////////////////////////////////////////////////////////////////////////
// pqServerFileDialogModel::Implementation

class pqServerFileDialogModel::pqImplementation
{
public:
  pqImplementation(pqServer* server) :
    Separator(0),
    FileModel(new pqFileModel(server)),
    FavoriteModel(new pqFavoriteModel(server))
  {
  }
  
  ~pqImplementation()
  {
    delete this->FileModel;
    delete this->FavoriteModel;
  }

  void UpdateInformation()
    {
    this->FileInformation->Initialize();
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(this->FileInformationHelperProxy->GetConnectionID(),
      vtkProcessModule::DATA_SERVER, 
      this->FileInformation, 
      this->FileInformationHelperProxy->GetID(0));
    }

  char Separator; // Path separator for the connected server's filesystem.
  pqFileModel* const FileModel;
  pqFavoriteModel* const FavoriteModel;

  vtkSmartPointer<vtkSMProxy> FileInformationHelperProxy;
  vtkSmartPointer<vtkPVFileInformation> FileInformation;
};

//////////////////////////////////////////////////////////////////////////
// pqServerFileDialogModel
pqServerFileDialogModel::pqServerFileDialogModel(QObject* Parent, pqServer* server) :
  base(Parent),
  Implementation(new pqImplementation(server))
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  vtkSMProxy* helper = pxm->NewProxy("misc", "FileInformationHelper");
  this->Implementation->FileInformationHelperProxy = helper;
  helper->SetConnectionID(server->GetConnectionID());
  helper->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
  helper->Delete();

  helper->UpdateVTKObjects();
  helper->UpdatePropertyInformation();
  QString separator = pqSMAdaptor::getElementProperty(
    helper->GetProperty("PathSeparator")).toString();

  this->Implementation->FileInformation.TakeReference(
    vtkPVFileInformation::New());
  this->Implementation->Separator = separator.toAscii().data()[0];
}

pqServerFileDialogModel::~pqServerFileDialogModel()
{
  delete this->Implementation;
}

QString pqServerFileDialogModel::getStartPath()
{
  if (gLastPath.isEmpty())
    {
    vtkSMProxy* helper = this->Implementation->FileInformationHelperProxy;
    pqSMAdaptor::setElementProperty(
      helper->GetProperty("DirectoryListing"), 0);
    pqSMAdaptor::setElementProperty(helper->GetProperty("Path"), ".");
    pqSMAdaptor::setElementProperty(
      helper->GetProperty("SpecialDirectories"), 0);
    helper->UpdateVTKObjects();
    this->Implementation->UpdateInformation();
    gLastPath = this->Implementation->FileInformation->GetFullPath();
    }

  return gLastPath;
}

void pqServerFileDialogModel::setCurrentPath(const QString& Path)
{
  gLastPath = Path;
  vtkSMProxy* helper = this->Implementation->FileInformationHelperProxy;
  pqSMAdaptor::setElementProperty(helper->GetProperty("DirectoryListing"), 1);
  pqSMAdaptor::setElementProperty(helper->GetProperty("Path"), Path);
  pqSMAdaptor::setElementProperty(helper->GetProperty("SpecialDirectories"), 0);
  helper->UpdateVTKObjects();
  this->Implementation->UpdateInformation();

  this->Implementation->FileModel->Update(
    cleanPath(Path, this->Implementation->Separator),
    this->Implementation->FileInformation);

//  this->Implementation->FileModel->SetCurrentPath(
//    cleanPath(Path, this->Implementation->Separator));
}

void pqServerFileDialogModel::setParentPath()
{
  QFileInfo info(this->Implementation->FileModel->CurrentPath);
  this->setCurrentPath(info.path());
}

QString pqServerFileDialogModel::getCurrentPath()
{
  return this->Implementation->FileModel->CurrentPath;
}

QStringList pqServerFileDialogModel::getFilePaths(const QModelIndex& Index)
{
  if(Index.model() == this->Implementation->FileModel)
    {
    return this->Implementation->FileModel->getFilePaths(Index, 
      this->Implementation->Separator);
    }
  
  if(Index.model() == this->Implementation->FavoriteModel)
    {
    return this->Implementation->FavoriteModel->getFilePaths(Index);  
    }

  return QStringList();
}

QString pqServerFileDialogModel::getFilePath(const QString& Path)
{
  /** \todo Use the server's definition of absolute */
  if(QDir::isAbsolutePath(Path))
    return Path;

  return this->Implementation->FileModel->CurrentPath + this->Implementation->Separator + Path;
}

bool pqServerFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.model() == this->Implementation->FileModel)
    return this->Implementation->FileModel->isDir(Index);
  
  if(Index.model() == this->Implementation->FavoriteModel)
    return this->Implementation->FavoriteModel->isDir(Index);  

  return false;    
}

QStringList pqServerFileDialogModel::getParentPaths(const QString& Path)
{
  QStringList paths = Path.split(this->Implementation->Separator);
  for(int i = 1; i < paths.size(); ++i)
    {
    paths[i] = paths[i-1] + this->Implementation->Separator + paths[i];
    }

  return paths;
}

bool pqServerFileDialogModel::fileExists(const QString& FilePath)
{
  vtkSMProxy* helper = this->Implementation->FileInformationHelperProxy;
  pqSMAdaptor::setElementProperty(helper->GetProperty("Path"), FilePath);
  pqSMAdaptor::setElementProperty(
    helper->GetProperty("SpecialDirectories"), 0);
  pqSMAdaptor::setElementProperty(
    helper->GetProperty("DirectoryListing"), 0);
  helper->UpdateVTKObjects();
  this->Implementation->UpdateInformation();

  if (this->Implementation->FileInformation->GetType() ==
    vtkPVFileInformation::SINGLE_FILE)
    {
    return true;
    }

  /*
  vtkSMProxy* proxy = this->Implementation->ServerFileListingProxy;
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("ActiveFileName"));
  svp->SetElement(0, FilePath.toAscii().data());
  proxy->UpdateVTKObjects();
  proxy->UpdatePropertyInformation();
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      proxy->GetProperty("ActiveFileIsReadable"));
  if (ivp->GetElement(0))
    {
      return true;
    }
    */
  return false;
}

bool pqServerFileDialogModel::dirExists(const QString& dir)
{
  vtkSMProxy* helper = this->Implementation->FileInformationHelperProxy;
  pqSMAdaptor::setElementProperty(helper->GetProperty("Path"), dir);
  pqSMAdaptor::setElementProperty(
    helper->GetProperty("SpecialDirectories"), 0);
  pqSMAdaptor::setElementProperty(
    helper->GetProperty("DirectoryListing"), 0);
  helper->UpdateVTKObjects();
  this->Implementation->UpdateInformation();

  if (this->Implementation->FileInformation->GetType() ==
    vtkPVFileInformation::DIRECTORY || 
    this->Implementation->FileInformation->GetType() == 
    vtkPVFileInformation::DRIVE)
    {
    return true;
    }

  /*
  vtkSMProxy* proxy = this->Implementation->ServerFileListingProxy;
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("ActiveFileName"));
  svp->SetElement(0, Dir.toAscii().data());
  proxy->UpdateVTKObjects();
  proxy->UpdatePropertyInformation();
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      proxy->GetProperty("ActiveFileIsDirectory"));
  if (ivp->GetElement(0))
    {
      return true;
    }
    */
  return false;
}

QAbstractItemModel* pqServerFileDialogModel::fileModel()
{
  return this->Implementation->FileModel;
}

QAbstractItemModel* pqServerFileDialogModel::favoriteModel()
{
  return this->Implementation->FavoriteModel;
}

