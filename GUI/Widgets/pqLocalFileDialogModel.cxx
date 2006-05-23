/*=========================================================================

   Program:   ParaQ
   Module:    pqLocalFileDialogModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqLocalFileDialogModel.h"

#include <QDateTime>
#include <QFileIconProvider>

#include <vtkstd/vector>

#ifdef WIN32
#include <shlobj.h>
#endif // WIN32

namespace
{

  /// the last path accessed by this file dialog model
  /// used to remember paths across the session
QString gLastPath;

//////////////////////////////////////////////////////////////////////
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

class FileInfo :
  public QFileInfo
{
public:
  FileInfo()
  {
  }

  FileInfo(const QString& Label, const QString& FilePath) :
    QFileInfo(FilePath),
    label(Label)
  {
  }

  QString label;
};

///////////////////////////////////////////////////////////////////////
// FileGroup

struct FileGroup
{
  FileGroup()
  {
  }
  
  FileGroup(const QString& label) :
    Label(label)
  {
  }
  
  FileGroup(const FileInfo& File) :
    Label(File.label)
  {
    this->Files.push_back(File);
  }
  
  QString Label;
  QList<FileInfo> Files;
};

///////////////////////////////////////////////////////////////////////
// SortFileByTypeThenAlpha

bool SortFileByTypeThenAlpha(const QFileInfo& A, const QFileInfo& B)
{
  // Sort directories before regular files
  if(A.isDir() != B.isDir())
    return A.isDir() > B.isDir();
  
  // Then sort alphabetically
  return A.absoluteFilePath() < B.absoluteFilePath();
}

///////////////////////////////////////////////////////////////////////
// FileModel

class pqFileModel :
  public QAbstractItemModel
{
public:
  void setCurrentPath(const QString& Path)
  {
    this->beginRemoveRows(QModelIndex(), 0, this->FileGroups.size());

    this->CurrentPath.setPath(QDir::cleanPath(Path));
    
    this->FileGroups.clear();

    QFileInfoList files = this->CurrentPath.entryInfoList();
    qSort(files.begin(), files.end(), SortFileByTypeThenAlpha);
    
    QRegExp regex("^(.*)\\.(\\d+)$");
    FileGroup numbered_files;
    for(int i = 0; i != files.size(); ++i)
      {
      if(-1 == regex.indexIn(files[i].fileName()))
        {
        if(numbered_files.Files.size())
          {
          this->FileGroups.push_back(numbered_files);
          numbered_files = FileGroup();
          }
          
        this->FileGroups.push_back(FileGroup(FileInfo(files[i].fileName(), files[i].filePath())));
        }
      else
        {
        const QString name = regex.cap(1);
        const QString idx = regex.cap(2);
        
        if(name != numbered_files.Label)
          {
          if(numbered_files.Files.size())
            this->FileGroups.push_back(numbered_files);
              
          numbered_files = FileGroup(name);
          }
          
        numbered_files.Files.push_back(FileInfo(files[i].fileName(), files[i].filePath()));
        }
      }
      
    if(numbered_files.Files.size())
      this->FileGroups.push_back(numbered_files);

    this->endRemoveRows();
  }

  QStringList GetFilePaths(const QModelIndex& Index)
  {
    QStringList results;
    
    if(Index.internalId()) // Selected a member of a file group
      {
      FileGroup& group = this->FileGroups[Index.internalId()-1];
      FileInfo& file = group.Files[Index.row()];
      results.push_back(QDir::convertSeparators(file.filePath()));
      }
    else // Selected a file group
      {
      FileGroup& group = this->FileGroups[Index.row()];
      for(int i = 0; i != group.Files.size(); ++i)
        results.push_back(QDir::convertSeparators(group.Files[i].filePath()));
      }
      
    return results;
  }

  bool IsDir(const QModelIndex& Index)
  {
    if(Index.internalId()) // This is a member of a file group ...
      {
      FileGroup& group = this->FileGroups[Index.internalId()-1];
      FileInfo& file = group.Files[Index.row()];
      return file.isDir();
      }
    else // This is a file group
      {
      FileGroup& group = this->FileGroups[Index.row()];
      if(1 == group.Files.size())
        return group.Files[0].isDir();
      }
      
    return false;
  }

  int columnCount(const QModelIndex& /*Index*/) const
  {
    return 3;
  }

  QVariant data(const QModelIndex & Index, int role) const
  {
    if(!Index.isValid())
      return QVariant();

    const FileGroup* group = 0;
    const FileInfo* file = 0;
        
    if(Index.internalId()) // This is a member of a file group ...
      {
      group = &this->FileGroups[Index.internalId()-1];
      file = &group->Files[Index.row()];
      }      
    else // This is a file group ...
      {
      group = &this->FileGroups[Index.row()];
      if(1 == group->Files.size())
        file = &group->Files[0];
      }
      
    switch(role)
      {
      case Qt::DisplayRole:
        switch(Index.column())
          {
          case 0:
            return file ? file->label : group->Label + QString(" (%1 files)").arg(group->Files.size());
          case 1:
            return file ? file->size() : QVariant();
          case 2:
            return file ? file->lastModified() : QVariant();
          }
      case Qt::DecorationRole:
        switch(Index.column())
          {
          case 0:
            if(file)
              return Icons().icon(file->isDir() ? QFileIconProvider::Folder : QFileIconProvider::File);
          }
      }
      
    return QVariant();
  }

  QModelIndex index(int row, int column, const QModelIndex& Parent) const
  {
    if(Parent.isValid()) // This is a member of a file group ...
      return createIndex(row, column, Parent.row() + 1);
      
    return createIndex(row, column, 0); // This is a file group ...
  }

  QModelIndex parent(const QModelIndex& Index) const
  {
    if(Index.internalId()) // This is a member of a file group ...
      return createIndex(Index.internalId() - 1, 0, 0);
      
    return QModelIndex(); // This is a file group ...
  }

  int rowCount(const QModelIndex& Index) const
  {
    if(Index.isValid())
      {
      if(Index.internalId()) // This is a member of a file group ...
        return 0;
      return this->FileGroups[Index.row()].Files.size(); // This is a file group ...
      }
  
    return this->FileGroups.size(); // This is the top-level node ...
  }

  bool hasChildren(const QModelIndex& Index) const
  {
    if(Index.isValid())
      {
      if(Index.internalId()) // This is a member of a file group ...
        return false;
      return this->FileGroups[Index.row()].Files.size() > 1; // This is a file group ...
      }
      
    return true; // This is the top-level node ...
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
          case 1:
            return tr("Size");
          case 2:
            return tr("Modified");
          }
      }
      
    return QVariant();
  }

  QDir CurrentPath;
  QList<FileGroup> FileGroups;
};

//////////////////////////////////////////////////////////////////
// FavoriteModel

class pqFavoriteModel :
  public QAbstractItemModel
{
public:
  pqFavoriteModel()
  {
#ifdef WIN32

    TCHAR szPath[MAX_PATH];

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szPath)))
      this->FavoriteList.push_back(FileInfo(tr("My Projects"), szPath));
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szPath)))
      this->FavoriteList.push_back(FileInfo(tr("Desktop"), szPath));
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_FAVORITES, NULL, 0, szPath)))
      this->FavoriteList.push_back(FileInfo(tr("Favorites"), szPath));

#else // WIN32

    this->FavoriteList.push_back(FileInfo(tr("Home"), QDir::home().absolutePath()));

#endif // !WIN32

    const QFileInfoList drives = QDir::drives();
    for(int i = 0; i != drives.size(); ++i)
      {
      QFileInfo drive = drives[i];
      this->FavoriteList.push_back(FileInfo(QDir::convertSeparators(drive.absoluteFilePath()), QDir::convertSeparators(drive.absoluteFilePath())));
      }
  }

  QStringList GetFilePaths(const QModelIndex& Index)
  {
    QStringList results;
    
    if(Index.row() < this->FavoriteList.size())
      {
      FileInfo& file = this->FavoriteList[Index.row()];
      results.push_back(QDir::convertSeparators(file.filePath()));
      }
    
    return results;
  }

  bool IsDir(const QModelIndex& Index)
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
            return file.label;
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
  
  QList<FileInfo> FavoriteList;
};

} // namespace

///////////////////////////////////////////////////////////////////////////
// pqLocalFileDialogModel

class pqLocalFileDialogModel::pqImplementation
{
public:
  pqImplementation() :
    FileModel(new pqFileModel()),
    FavoriteModel(new pqFavoriteModel())
  {
  }
  
  ~pqImplementation()
  {
    delete this->FileModel;
    delete this->FavoriteModel;
  }

  pqFileModel* const FileModel;
  pqFavoriteModel* const FavoriteModel;
};

pqLocalFileDialogModel::pqLocalFileDialogModel(QObject* Parent) :
  pqFileDialogModel(Parent),
  Implementation(new pqImplementation())
{
}

pqLocalFileDialogModel::~pqLocalFileDialogModel()
{
  delete this->Implementation;
}

QString pqLocalFileDialogModel::getStartPath()
{
  if(!gLastPath.isNull())
    {
    return gLastPath;
    }
  gLastPath = QDir::currentPath();
  return gLastPath;
}

void pqLocalFileDialogModel::setCurrentPath(const QString& Path)
{
  gLastPath = Path;
  this->Implementation->FileModel->setCurrentPath(Path);
}

QString pqLocalFileDialogModel::getCurrentPath()
{
  return QDir::convertSeparators(this->Implementation->FileModel->CurrentPath.path());
}

QStringList pqLocalFileDialogModel::getFilePaths(const QModelIndex& Index)
{
  if(Index.model() == this->Implementation->FileModel)
    return this->Implementation->FileModel->GetFilePaths(Index);
  
  if(Index.model() == this->Implementation->FavoriteModel)
    return this->Implementation->FavoriteModel->GetFilePaths(Index);  

  return QStringList();
}

QString pqLocalFileDialogModel::getFilePath(const QString& Path)
{
  if(QDir::isAbsolutePath(Path))
    return Path;
    
  return QDir::convertSeparators(this->Implementation->FileModel->CurrentPath.path() + "/" + Path);
}

QString pqLocalFileDialogModel::getParentPath(const QString& Path)
{
  QDir temp(Path);
  temp.cdUp();
  return temp.path();
}

bool pqLocalFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.model() == this->Implementation->FileModel)
    return this->Implementation->FileModel->IsDir(Index);
  
  if(Index.model() == this->Implementation->FavoriteModel)
    return this->Implementation->FavoriteModel->IsDir(Index);  

  return false;    
}

QStringList pqLocalFileDialogModel::splitPath(const QString& Path)
{
  return Path.split(QDir::separator());
}

QAbstractItemModel* pqLocalFileDialogModel::fileModel()
{
  return this->Implementation->FileModel;
}

QAbstractItemModel* pqLocalFileDialogModel::favoriteModel()
{
  return this->Implementation->FavoriteModel;
}
