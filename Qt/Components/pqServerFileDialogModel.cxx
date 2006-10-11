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
#include <vtkClientServerStream.h>
#include <vtkProcessModule.h>
#include <vtkSmartPointer.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkStringList.h>
#include <QFileIconProvider>

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
  pqFileModel(pqServer* server) :
    Server(server)
  {
  } 

  void SetCurrentPath(const QString& Path)
  {
    this->CurrentPath = Path;
    this->FileList.clear();

    this->FileList.push_back(FileInfo(".", ".", true, false));
    this->FileList.push_back(FileInfo("..", "..", true, false));
    
    vtkSmartPointer<vtkStringList> vtk_dirs = vtkSmartPointer<vtkStringList>::New();
    vtkSmartPointer<vtkStringList> vtk_files = vtkSmartPointer<vtkStringList>::New();
    
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    if (!pm->GetDirectoryListing(this->Server->GetConnectionID(),
      Path.toAscii().data(), vtk_dirs.GetPointer(), vtk_files.GetPointer(), 0))
      {
      // error failed to obtain directory listing.
      return;
      }

    QStringList dirs;
    for(int i = 0; i != vtk_dirs->GetNumberOfStrings(); ++i)
      {
      dirs.push_back(vtk_dirs->GetString(i));
      }
    QStringList files;
    for(int i = 0; i != vtk_files->GetNumberOfStrings(); ++i)
      {
      files.push_back(vtk_files->GetString(i));
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
    
    vtkClientServerStream stream;
    const vtkClientServerID ID = pm->NewStreamObject("vtkPVServerFileListing", stream);
    stream << vtkClientServerStream::Invoke << ID
      << "GetSpecial" << vtkClientServerStream::End;
    pm->SendStream(this->Server->GetConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    
    vtkClientServerStream result;
    pm->GetLastResult(this->Server->GetConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result);

    for(int i = 0; i < result.GetNumberOfMessages(); ++i)
      {
      if(result.GetNumberOfArguments(i) == 3)
        {
        const char* label = 0;
        result.GetArgument(i, 0, &label);
        
        const char* path = 0;
        result.GetArgument(i, 1, &path);
        
        int type = 0; // 0 == directory, 1 == drive letter, 2 == file
        result.GetArgument(i, 2, &type);
        
        this->FavoriteList.push_back(FileInfo(label, path, type != 2, type == 1));
        }
      }
      
    pm->DeleteStreamObject(ID, stream);
    pm->SendStream(this->Server->GetConnectionID(), 
      vtkProcessModule::DATA_SERVER_ROOT, stream);
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
              return Icons().icon(QFileIconProvider::Drive);
            if(file.isDir())
              return Icons().icon(QFileIconProvider::Folder);
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

  char Separator; // Path separator for the connected server's filesystem.
  pqFileModel* const FileModel;
  pqFavoriteModel* const FavoriteModel;
  vtkSmartPointer<vtkSMProxy> ServerFileListingProxy;
};

//////////////////////////////////////////////////////////////////////////
// pqServerFileDialogModel

pqServerFileDialogModel::pqServerFileDialogModel(QObject* Parent, pqServer* server) :
  base(Parent),
  Implementation(new pqImplementation(server))
{
  // Setup the connection to the server ...
  vtkSMProxy* proxy = vtkSMObject::GetProxyManager()->NewProxy(
    "file_listing", "ServerFileListing");
  proxy->SetConnectionID(server->GetConnectionID());
  proxy->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
  proxy->UpdateVTKObjects();
  
  // Use an ugly heuristic to figure-out what the server uses as a path separator
  proxy->UpdatePropertyInformation();
  if(vtkSMStringVectorProperty* const svp =
    vtkSMStringVectorProperty::SafeDownCast(
      proxy->GetProperty("CurrentWorkingDirectory")))
    {
    QString cwd = svp->GetElement(0);
    if(cwd[0] == '/')
      this->Implementation->Separator = '/';
    else
      this->Implementation->Separator = '\\';
    }
  
  this->Implementation->ServerFileListingProxy = proxy;
  proxy->Delete();
}

pqServerFileDialogModel::~pqServerFileDialogModel()
{
  delete this->Implementation;
}

QString pqServerFileDialogModel::getStartPath()
{
  if(gLastPath.isEmpty())
    {
    if(vtkSMProxy* const proxy = this->Implementation->ServerFileListingProxy)
      {
      proxy->UpdatePropertyInformation();
      if(vtkSMStringVectorProperty* const svp =
        vtkSMStringVectorProperty::SafeDownCast(
          proxy->GetProperty("CurrentWorkingDirectory")))
        {
        gLastPath = svp->GetElement(0);
        }
      }
    }

  return gLastPath;
}

void pqServerFileDialogModel::setCurrentPath(const QString& Path)
{
  gLastPath = Path;
  this->Implementation->FileModel->SetCurrentPath(cleanPath(Path, this->Implementation->Separator));
}

void pqServerFileDialogModel::setParentPath()
{
  this->setCurrentPath(this->Implementation->FileModel->CurrentPath + this->Implementation->Separator + "..");
}

QString pqServerFileDialogModel::getCurrentPath()
{
  return this->Implementation->FileModel->CurrentPath;
}

QStringList pqServerFileDialogModel::getFilePaths(const QModelIndex& Index)
{
  if(Index.model() == this->Implementation->FileModel)
    return this->Implementation->FileModel->getFilePaths(Index, this->Implementation->Separator);
  
  if(Index.model() == this->Implementation->FavoriteModel)
    return this->Implementation->FavoriteModel->getFilePaths(Index);  

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
  return false;
}

bool pqServerFileDialogModel::dirExists(const QString& Dir)
{
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

