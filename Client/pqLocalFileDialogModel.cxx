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

class FileInfo
{
public:
  FileInfo()
  {
  }

  FileInfo(const QString& label, const QString& filePath, const bool isDir, const bool isRoot) :
    Label(label),
    FilePath(filePath),
    IsDir(isDir),
    IsRoot(isRoot)
  {
  }

  const QString& getLabel() const
  {
    return Label;
  }

  const QString& filePath() const 
  {
    return FilePath;
  }
  
  const bool isDir() const
  {
    return IsDir;
  }
  
  const bool isRoot() const
  {
    return IsRoot;
  }

private:
  QString Label;
  QString FilePath;
  bool IsDir;
  bool IsRoot;
};

///////////////////////////////////////////////////////////////////////
// FileModel

class FileModel :
  public QAbstractItemModel
{
public:
  void setCurrentPath(const QString& Path)
  {
    currentPath.setPath(QDir::cleanPath(Path));
    fileList = currentPath.entryInfoList();
    
    emit layoutChanged();
    emit dataChanged(QModelIndex(), QModelIndex());
  }

  QString getFilePath(const QModelIndex& Index)
  {
    if(Index.row() >= fileList.size())
      return QString();
      
    QFileInfo& file = fileList[Index.row()];
    return QDir::convertSeparators(currentPath.path() + "/" + file.fileName());
  }

  bool isDir(const QModelIndex& Index)
  {
    if(Index.row() >= fileList.size())
      return false;
      
    QFileInfo& file = fileList[Index.row()];
    return file.isDir();
  }

  int columnCount(const QModelIndex& parent) const
  {
    return 3;
  }

  QVariant data(const QModelIndex & index, int role) const
  {
    if(!index.isValid())
      return QVariant();

    if(index.row() >= fileList.size())
      return QVariant();

    const QFileInfo& file = fileList[index.row()];

    switch(role)
      {
      case Qt::DisplayRole:
        switch(index.column())
          {
          case 0:
            return file.fileName();
          case 1:
            return file.size();
          case 2:
            return file.lastModified();
          }
      case Qt::DecorationRole:
        switch(index.column())
          {
          case 0:
            return Icons().icon(file.isDir() ? QFileIconProvider::Folder : QFileIconProvider::File);
          }
      }
      
    return QVariant();
  }

  QModelIndex index(int row, int column, const QModelIndex& parent) const
  {
    return createIndex(row, column);
  }

  QModelIndex parent(const QModelIndex& index) const
  {
    return QModelIndex();
  }

  int rowCount(const QModelIndex& parent) const
  {
    return fileList.size();
  }

  bool hasChildren(const QModelIndex& parent) const
  {
    if(!parent.isValid())
      return true;
      
    return false;
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
  QFileInfoList fileList;
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
      favoriteList.push_back(FileInfo(tr("History"), szPath, true, false));
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szPath)))
      favoriteList.push_back(FileInfo(tr("My Projects"), szPath, true, false));
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szPath)))
      favoriteList.push_back(FileInfo(tr("Desktop"), szPath, true, false));
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_FAVORITES, NULL, 0, szPath)))
      favoriteList.push_back(FileInfo(tr("Favorites"), szPath, true, false));

#else // WIN32

    favoriteList.push_back(FileInfo(tr("Home"), QDir::home().absolutePath(), true, false));

#endif // !WIN32
  
    const QFileInfoList drives = QDir::drives();
    for(int i = 0; i != drives.size(); ++i)
      {
      QFileInfo drive = drives[i];
      favoriteList.push_back(FileInfo(drive.absoluteFilePath(), drive.absoluteFilePath(), true, true));
      }
  }

  QString getFilePath(const QModelIndex& Index)
  {
    if(Index.row() >= favoriteList.size())
      return QString();
      
    FileInfo& file = favoriteList[Index.row()];
    return QDir::convertSeparators(file.filePath());
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
            return file.getLabel();
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
  
  vtkstd::vector<FileInfo> favoriteList;
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

QString pqLocalFileDialogModel::getFilePath(const QModelIndex& Index)
{
  if(Index.model() == implementation->fileModel)
    return implementation->fileModel->getFilePath(Index);
  
  if(Index.model() == implementation->favoriteModel)
    return implementation->favoriteModel->getFilePath(Index);  

  return QString();    
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

