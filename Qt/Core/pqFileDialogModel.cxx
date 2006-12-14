/*=========================================================================

   Program: ParaView
   Module:    pqFileDialogModel.cxx

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

#include "pqFileDialogModel.h"

#include <pqServer.h>
#include <QFileIconProvider>
#include <vtkClientServerStream.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkProcessModule.h>
#include <vtkPVFileInformation.h>
#include <vtkPVFileInformationHelper.h>
#include <vtkSmartPointer.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkStringList.h>

#include "pqSMAdaptor.h"
namespace
{
  

/////////////////////////////////////////////////////////////////////
// Icons

Q_GLOBAL_STATIC(QFileIconProvider, Icons);

//////////////////////////////////////////////////////////////////////
// FileInfo

class FileInfo
{
public:
  FileInfo()
  {
  }

  FileInfo(const QString& l, const QString& filepath, 
           const bool isdir, const bool isroot) :
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

//////////////////////////////////////////////////////////////////
// FavoriteModel

class pqFavoriteModel :
  public QAbstractItemModel
{
public:
  pqFavoriteModel(pqServer* server)
  {
  vtkPVFileInformation* information = vtkPVFileInformation::New();

  if(server)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();  
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

    vtkSMProxy* helper = pxm->NewProxy("misc","FileInformationHelper");
    helper->SetConnectionID(server->GetConnectionID());
    helper->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
    pqSMAdaptor::setElementProperty(helper->GetProperty("SpecialDirectories"),
                                    true);
    helper->UpdateVTKObjects();

    pm->GatherInformation(server->GetConnectionID(),
      vtkProcessModule::DATA_SERVER, information, helper->GetID(0));

    helper->Delete();
    }
  else
    {
    vtkPVFileInformationHelper* helper = vtkPVFileInformationHelper::New();
    helper->SetSpecialDirectories(1);
    information->CopyFromObject(helper);
    helper->Delete();
    }
  
  vtkCollectionIterator* iter = information->GetContents()->NewIterator();
  for (iter->InitTraversal(); 
       !iter->IsDoneWithTraversal(); iter->GoToNextItem())
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
        (type == vtkPVFileInformation::DIRECTORY || 
          type == vtkPVFileInformation::DRIVE),
        (type == vtkPVFileInformation::DRIVE)));
    }

  iter->Delete();
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
              return Icons()->icon(QFileIconProvider::Drive);
              }
            if(file.isDir())
              {
              return Icons()->icon(QFileIconProvider::Folder);
              }
            return Icons()->icon(QFileIconProvider::File);
          }
      }
      
    return QVariant();
  }
  
  virtual QModelIndex index(int row, int column, 
                            const QModelIndex& /*parent*/) const
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
  
  virtual QVariant headerData(int section, 
                              Qt::Orientation /*orientation*/, int role) const
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
 
  QList<FileInfo> FavoriteList;
};


} // namespace

/////////////////////////////////////////////////////////////////////////
// pqFileDialogModel::Implementation

class pqFileDialogModel::pqImplementation
{
public:
  pqImplementation(pqServer* server) :
    Separator(0),
    Server(server),
    FavoriteModel(new pqFavoriteModel(server))
  {
  
    // if we are doing remote browsing
    if(server)
      {
      vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

      vtkSMProxy* helper = pxm->NewProxy("misc", "FileInformationHelper");
      this->FileInformationHelperProxy = helper;
      helper->SetConnectionID(server->GetConnectionID());
      helper->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
      helper->Delete();
      helper->UpdateVTKObjects();
      helper->UpdatePropertyInformation();
      QString separator = pqSMAdaptor::getElementProperty(
        helper->GetProperty("PathSeparator")).toString();
      this->Separator = separator.toAscii().data()[0];
      }
    else
      {
      vtkPVFileInformationHelper* helper = vtkPVFileInformationHelper::New();
      this->FileInformationHelper = helper;
      helper->Delete();
      this->Separator = helper->GetPathSeparator()[0];
      }

    this->FileInformation.TakeReference(vtkPVFileInformation::New());
  }
  
  ~pqImplementation()
  {
    delete this->FavoriteModel;
  }

  /// Removes multiple-slashes, ".", and ".." from the given path string,
  /// and points slashes in the correct direction for the server
  const QString cleanPath(const QString& Path)
  {
    QString result = QDir::cleanPath(Path);
    result.replace('/', this->Separator);
    return result;
  }

  void UpdateInformation()
    {
    if(!this->FileInformationHelperProxy)
      {
      return;
      }

    this->FileInformation->Initialize();
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(this->FileInformationHelperProxy->GetConnectionID(),
      vtkProcessModule::DATA_SERVER, 
      this->FileInformation, 
      this->FileInformationHelperProxy->GetID(0));
    }

  void Update(const QString& path, vtkPVFileInformation* dir)
    {
    this->CurrentPath = path;
    this->FileList.clear();

    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(dir->GetContents()->NewIterator());

    QStringList dirs;
    QStringList files;

    for (iter->InitTraversal(); 
         !iter->IsDoneWithTraversal(); 
         iter->GoToNextItem())
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
      this->FileList.push_back(FileInfo(directory_name, directory_name, 
                               true, false));
      }
    for(int i = 0; i != files.size(); ++i)
      {
      const QString file_name = files[i];
      this->FileList.push_back(FileInfo(file_name, file_name, false, false));
      }
    }

  QStringList getFilePaths(const QModelIndex& Index)
    {
    QStringList results;

    if(Index.row() < this->FileList.size())
      { 
      FileInfo& file = this->FileList[Index.row()];
      results.push_back(this->CurrentPath + this->Separator + file.filePath());
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

  /// Path separator for the connected server's filesystem. 
  char Separator;

  /// Connection from which the dir listing is to be fetched.
  pqServer* Server;
  /// Current path being displayed (server's filesystem).
  QString CurrentPath;
  /// Caches information about the set of files within the current path.
  QList<FileInfo> FileList;

  pqFavoriteModel* const FavoriteModel;
  
  /// The last path accessed by this file dialog model
  /// used to remember paths across the session
  /// TODO:  this will not work if going between multiple servers
  static QString gLastLocalPath;
  static QString gLastServerPath;

  vtkSmartPointer<vtkPVFileInformationHelper> FileInformationHelper;
  vtkSmartPointer<vtkSMProxy> FileInformationHelperProxy;
  vtkSmartPointer<vtkPVFileInformation> FileInformation;
};
  
QString pqFileDialogModel::pqImplementation::gLastLocalPath;
QString pqFileDialogModel::pqImplementation::gLastServerPath;

//////////////////////////////////////////////////////////////////////////
// pqFileDialogModel
pqFileDialogModel::pqFileDialogModel(pqServer* server, QObject* Parent) :
  base(Parent),
  Implementation(new pqImplementation(server))
{
}

pqFileDialogModel::~pqFileDialogModel()
{
  delete this->Implementation;
}

QString pqFileDialogModel::getStartPath()
{
  QString ret;
  if(this->Implementation->FileInformationHelperProxy)
    {
    if (this->Implementation->gLastServerPath.isEmpty())
      {
      vtkSMProxy* helper = this->Implementation->FileInformationHelperProxy;
      pqSMAdaptor::setElementProperty(
        helper->GetProperty("DirectoryListing"), 0);
      pqSMAdaptor::setElementProperty(helper->GetProperty("Path"), ".");
      pqSMAdaptor::setElementProperty(
        helper->GetProperty("SpecialDirectories"), 0);
      helper->UpdateVTKObjects();
      this->Implementation->UpdateInformation();
      this->Implementation->gLastServerPath = 
            this->Implementation->FileInformation->GetFullPath();
      }
    ret = this->Implementation->gLastServerPath;
    }
  else
    {
    if (this->Implementation->gLastLocalPath.isEmpty())
      {
      vtkPVFileInformationHelper* helper = 
                     this->Implementation->FileInformationHelper;
      helper->SetDirectoryListing(0);
      helper->SetPath(".");
      helper->SetSpecialDirectories(0);
      this->Implementation->FileInformation->CopyFromObject(helper);
      this->Implementation->gLastLocalPath = 
             this->Implementation->FileInformation->GetFullPath();
      }
    ret = this->Implementation->gLastLocalPath;
    }
  return ret;
}

void pqFileDialogModel::setCurrentPath(const QString& Path)
{
  QString cPath = this->Implementation->cleanPath(Path);
  if(this->Implementation->FileInformationHelperProxy)
    {
    this->Implementation->gLastServerPath = Path;
    vtkSMProxy* helper = this->Implementation->FileInformationHelperProxy;
    pqSMAdaptor::setElementProperty(helper->GetProperty("DirectoryListing"),
                                    1);
    pqSMAdaptor::setElementProperty(helper->GetProperty("Path"), Path);
    pqSMAdaptor::setElementProperty(helper->GetProperty("SpecialDirectories"), 
                                    0);
    helper->UpdateVTKObjects();
    this->Implementation->UpdateInformation();
    this->Implementation->Update(cPath, this->Implementation->FileInformation);
    }
  else
    {
    this->Implementation->gLastLocalPath = Path;
    vtkPVFileInformationHelper* helper = 
               this->Implementation->FileInformationHelper;
    helper->SetDirectoryListing(1);
    helper->SetPath(Path.toAscii().data());
    helper->SetSpecialDirectories(0);
    this->Implementation->FileInformation->CopyFromObject(helper);
    this->Implementation->Update(cPath, this->Implementation->FileInformation);
  }

  this->reset();
}

void pqFileDialogModel::setParentPath()
{
  QFileInfo info(this->Implementation->CurrentPath);
  this->setCurrentPath(info.path());
}

QString pqFileDialogModel::getCurrentPath()
{
  return this->Implementation->CurrentPath;
}

QStringList pqFileDialogModel::getFilePaths(const QModelIndex& Index)
{
  if(Index.model() == this)
    {
    return this->getFilePaths(Index);
    }
  
  if(Index.model() == this->Implementation->FavoriteModel)
    {
    return this->Implementation->FavoriteModel->getFilePaths(Index);  
    }

  return QStringList();
}

QString pqFileDialogModel::getFilePath(const QString& Path)
{
  /** \todo Use the server's definition of absolute */
  if(QDir::isAbsolutePath(Path))
    return Path;

  return this->Implementation->CurrentPath + 
         this->Implementation->Separator + Path;
}

bool pqFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.model() == this)
    return this->Implementation->isDir(Index);
  
  if(Index.model() == this->Implementation->FavoriteModel)
    return this->Implementation->FavoriteModel->isDir(Index);  

  return false;    
}

QStringList pqFileDialogModel::getParentPaths(const QString& Path)
{
  QStringList paths = Path.split(this->Implementation->Separator);
  for(int i = 1; i < paths.size(); ++i)
    {
    paths[i] = paths[i-1] + this->Implementation->Separator + paths[i];
    }

  return paths;
}

bool pqFileDialogModel::fileExists(const QString& FilePath)
{
  if(this->Implementation->FileInformationHelperProxy)
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
    }
  else
    {
    vtkPVFileInformationHelper* helper = 
             this->Implementation->FileInformationHelper;
    helper->SetPath(FilePath.toAscii().data());
    helper->SetSpecialDirectories(0);
    helper->SetDirectoryListing(0);
    this->Implementation->FileInformation->CopyFromObject(helper);

    if (this->Implementation->FileInformation->GetType() ==
      vtkPVFileInformation::SINGLE_FILE)
      {
      return true;
      }
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

bool pqFileDialogModel::dirExists(const QString& dir)
{
  if(this->Implementation->FileInformationHelperProxy)
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
    }
  else
    {
    vtkPVFileInformationHelper* helper = 
         this->Implementation->FileInformationHelper;
    helper->SetPath(dir.toAscii().data());
    helper->SetSpecialDirectories(0);
    helper->SetDirectoryListing(0);
    this->Implementation->FileInformation->CopyFromObject(helper);

    if (this->Implementation->FileInformation->GetType() ==
      vtkPVFileInformation::DIRECTORY || 
      this->Implementation->FileInformation->GetType() == 
      vtkPVFileInformation::DRIVE)
      {
      return true;
      }
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

QAbstractItemModel* pqFileDialogModel::favoriteModel()
{
  return this->Implementation->FavoriteModel;
}

int pqFileDialogModel::columnCount(const QModelIndex& /*idx*/) const
{
  return 1;
}

QVariant pqFileDialogModel::data(const QModelIndex & idx, int role) const
{
  if(!idx.isValid())
    return QVariant();

  if(idx.row() >= this->Implementation->FileList.size())
    return QVariant();

  const FileInfo& file = this->Implementation->FileList[idx.row()];

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
      return Icons()->icon(file.isDir() ? 
                 QFileIconProvider::Folder : QFileIconProvider::File);
      }
    }

  return QVariant();
}

QModelIndex pqFileDialogModel::index(int row, int column, 
                const QModelIndex& /*p*/) const
{
  return this->createIndex(row, column);
}

QModelIndex pqFileDialogModel::parent(const QModelIndex& /*idx*/) const
{
  return QModelIndex();
}

int pqFileDialogModel::rowCount(const QModelIndex& /*idx*/) const
{
  return this->Implementation->FileList.size();
}

bool pqFileDialogModel::hasChildren(const QModelIndex& idx) const
{
  if(!idx.isValid())
    return true;

  return false;
}

QVariant pqFileDialogModel::headerData(int section, 
                                       Qt::Orientation, int role) const
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

