/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqLocalFileDialogModel.h"

#include <QDateTime>
#include <QFileIconProvider>

#include <vtkstd/vector>

#ifdef WIN32
#include <shlobj.h>
#endif // WIN32

namespace
{

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
    label(Label),
    QFileInfo(FilePath)
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
  
  FileGroup(const QString& Label) :
    label(Label)
  {
  }
  
  FileGroup(const FileInfo& File) :
    label(File.label)
  {
    files.push_back(File);
  }
  
  QString label;
  QList<FileInfo> files;
};

///////////////////////////////////////////////////////////////////////
// SortFileAlpha

bool SortFileAlpha(const QFileInfo& A, const QFileInfo& B)
{
  return A.absoluteFilePath() < B.absoluteFilePath();
}

///////////////////////////////////////////////////////////////////////
// FileModel

class FileModel :
  public QAbstractItemModel
{
public:
  void setCurrentPath(const QString& Path)
  {
    currentPath.setPath(QDir::cleanPath(Path));
    
    fileGroups.clear();
    
    QFileInfoList files = currentPath.entryInfoList();
    qSort(files.begin(), files.end(), SortFileAlpha);
    
    QRegExp regex("^(.*)\\.(\\d+)$");
    FileGroup numbered_files;
    for(int i = 0; i != files.size(); ++i)
      {
      if(-1 == regex.indexIn(files[i].fileName()))
        {
        if(numbered_files.files.size())
          {
          fileGroups.push_back(numbered_files);
          numbered_files = FileGroup();
          }
          
        fileGroups.push_back(FileGroup(FileInfo(files[i].fileName(), files[i].filePath())));
        }
      else
        {
        const QString name = regex.cap(1);
        const QString index = regex.cap(2);
        
        if(name != numbered_files.label)
          {
          if(numbered_files.files.size())
            fileGroups.push_back(numbered_files);
              
          numbered_files = FileGroup(name);
          }
          
        numbered_files.files.push_back(FileInfo(files[i].fileName(), files[i].filePath()));
        }
      }
      
    if(numbered_files.files.size())
      fileGroups.push_back(numbered_files);

    emit layoutChanged();
    emit dataChanged(QModelIndex(), QModelIndex());
  }

  QStringList getFilePaths(const QModelIndex& Index)
  {
    QStringList results;
    
    if(Index.internalId()) // Selected a member of a file group
      {
      FileGroup& group = fileGroups[Index.internalId()-1];
      FileInfo& file = group.files[Index.row()];
      results.push_back(file.filePath());
      }
    else // Selected a file group
      {
      FileGroup& group = fileGroups[Index.row()];
      for(int i = 0; i != group.files.size(); ++i)
        results.push_back(group.files[i].filePath());
      }
      
    return results;
  }

  bool isDir(const QModelIndex& Index)
  {
    if(Index.internalId()) // This is a member of a file group ...
      {
      FileGroup& group = fileGroups[Index.internalId()-1];
      FileInfo& file = group.files[Index.row()];
      return file.isDir();
      }
    else // This is a file group
      {
      FileGroup& group = fileGroups[Index.row()];
      if(1 == group.files.size())
        return group.files[0].isDir();
      }
      
    return false;
  }

  int columnCount(const QModelIndex& Index) const
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
      group = &fileGroups[Index.internalId()-1];
      file = &group->files[Index.row()];
      }      
    else // This is a file group ...
      {
      group = &fileGroups[Index.row()];
      if(1 == group->files.size())
        file = &group->files[0];
      }
      
    switch(role)
      {
      case Qt::DisplayRole:
        switch(Index.column())
          {
          case 0:
            return file ? file->label : group->label + QString(" (%1 files)").arg(group->files.size());
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
      return fileGroups[Index.row()].files.size(); // This is a file group ...
      }
  
    return fileGroups.size(); // This is the top-level node ...
  }

  bool hasChildren(const QModelIndex& Index) const
  {
    if(Index.isValid())
      {
      if(Index.internalId()) // This is a member of a file group ...
        return false;
      return fileGroups[Index.row()].files.size() > 1; // This is a file group ...
      }
      
    return true; // This is the top-level node ...
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const
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

  QDir currentPath;
  QList<FileGroup> fileGroups;
};

//////////////////////////////////////////////////////////////////
// FavoriteModel

class FavoriteModel :
  public QAbstractItemModel
{
public:
  FavoriteModel()
  {
#ifdef WIN32

    TCHAR szPath[MAX_PATH];

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_HISTORY, NULL, 0, szPath)))
      favoriteList.push_back(FileInfo(tr("History"), szPath));
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szPath)))
      favoriteList.push_back(FileInfo(tr("My Projects"), szPath));
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szPath)))
      favoriteList.push_back(FileInfo(tr("Desktop"), szPath));
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_FAVORITES, NULL, 0, szPath)))
      favoriteList.push_back(FileInfo(tr("Favorites"), szPath));

#else // WIN32

    favoriteList.push_back(FileInfo(tr("Home"), QDir::home().absolutePath()));

#endif // !WIN32
  
    const QFileInfoList drives = QDir::drives();
    for(int i = 0; i != drives.size(); ++i)
      {
      QFileInfo drive = drives[i];
      favoriteList.push_back(FileInfo(QDir::convertSeparators(drive.absoluteFilePath()), QDir::convertSeparators(drive.absoluteFilePath())));
      }
  }

  QStringList getFilePaths(const QModelIndex& Index)
  {
    QStringList results;
    
    if(Index.row() < favoriteList.size())
      {
      FileInfo& file = favoriteList[Index.row()];
      results.push_back(QDir::convertSeparators(file.filePath()));
      }
    
    return results;
  }

  bool isDir(const QModelIndex& Index)
  {
    if(Index.row() >= favoriteList.size())
      return false;
      
    FileInfo& file = favoriteList[Index.row()];
    return file.isDir();
  }

  virtual int columnCount(const QModelIndex& parent) const
  {
    return 1;
  }
  
  virtual QVariant data(const QModelIndex & index, int role) const
  {
    if(!index.isValid())
      return QVariant();

    if(index.row() >= favoriteList.size())
      return QVariant();

    const FileInfo& file = favoriteList[index.row()];
    switch(role)
      {
      case Qt::DisplayRole:
        switch(index.column())
          {
          case 0:
            return file.label;
          }
      case Qt::DecorationRole:
        switch(index.column())
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
  
  virtual QModelIndex index(int row, int column, const QModelIndex& parent) const
  {
    return createIndex(row, column);
  }
  
  virtual QModelIndex parent(const QModelIndex& index) const
  {
    return QModelIndex();
  }
  
  virtual int rowCount(const QModelIndex& parent) const
  {
    return favoriteList.size();
  }
  
  virtual bool hasChildren(const QModelIndex& parent) const
  {
    if(!parent.isValid())
      return true;
      
    return false;
  }
  
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
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
  
  QList<FileInfo> favoriteList;
};

} // namespace

///////////////////////////////////////////////////////////////////////////
// pqLocalFileDialogModel

class pqLocalFileDialogModel::Implementation
{
public:
  Implementation() :
    fileModel(new FileModel()),
    favoriteModel(new FavoriteModel())
  {
  }
  
  ~Implementation()
  {
    delete fileModel;
    delete favoriteModel;
  }

  FileModel* const fileModel;
  FavoriteModel* const favoriteModel;
};

pqLocalFileDialogModel::pqLocalFileDialogModel(QObject* Parent) :
  pqFileDialogModel(Parent),
  implementation(new Implementation())
{
}

pqLocalFileDialogModel::~pqLocalFileDialogModel()
{
  delete implementation;
}

QString pqLocalFileDialogModel::getStartPath()
{
  return QDir::currentPath();
}

void pqLocalFileDialogModel::setCurrentPath(const QString& Path)
{
  implementation->fileModel->setCurrentPath(Path);
}

QString pqLocalFileDialogModel::getCurrentPath()
{
  return QDir::convertSeparators(implementation->fileModel->currentPath.path());
}

QStringList pqLocalFileDialogModel::getFilePaths(const QModelIndex& Index)
{
  if(Index.model() == implementation->fileModel)
    return implementation->fileModel->getFilePaths(Index);
  
  if(Index.model() == implementation->favoriteModel)
    return implementation->favoriteModel->getFilePaths(Index);  

  return QStringList();
}

QString pqLocalFileDialogModel::getFilePath(const QString& Path)
{
  if(QDir::isAbsolutePath(Path))
    return Path;
    
  return QDir::convertSeparators(implementation->fileModel->currentPath.path() + "/" + Path);
}

QString pqLocalFileDialogModel::getParentPath(const QString& Path)
{
  QDir temp(Path);
  temp.cdUp();
  return temp.path();
}

bool pqLocalFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.model() == implementation->fileModel)
    return implementation->fileModel->isDir(Index);
  
  if(Index.model() == implementation->favoriteModel)
    return implementation->favoriteModel->isDir(Index);  

  return false;    
}

QStringList pqLocalFileDialogModel::splitPath(const QString& Path)
{
  QStringList results;
  
  for(int i = Path.indexOf(QDir::separator()); i != -1; i = Path.indexOf(QDir::separator(), i+1))
    results.push_back(Path.left(i) + QDir::separator());
    
  results.push_back(Path);
  
  return results;
}

QAbstractItemModel* pqLocalFileDialogModel::fileModel()
{
  return implementation->fileModel;
}

QAbstractItemModel* pqLocalFileDialogModel::favoriteModel()
{
  return implementation->favoriteModel;
}

