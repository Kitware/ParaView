/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqServerFileDialogModel.h"

#include <vtkClientServerStream.h>
#include <vtkProcessModule.h>

#include <QFileIconProvider>

namespace
{

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

  FileInfo(const QString& fileName, const bool isDir) :
    FileName(fileName),
    IsDir(isDir)
  {
  }

  const QString& fileName() const 
  {
    return FileName;
  }
  
  const bool isDir() const
  {
    return IsDir;
  }

private:
  QString FileName;
  bool IsDir;
};

/////////////////////////////////////////////////////////////////////////////////
// FileModel

class FileModel :
  public QAbstractItemModel
{
public:
  FileModel(vtkProcessModule* processModule) :
    ProcessModule(processModule)
  {
  } 

  void setCurrentPath(const QString& Path)
  {
    currentPath.setPath(QDir::cleanPath(Path));
    fileList.clear();

    fileList.push_back(FileInfo(".", true));
    fileList.push_back(FileInfo("..", true));

    vtkClientServerStream stream;
    const vtkClientServerID ID = ProcessModule->NewStreamObject("vtkPVServerFileListing", stream);
    stream
      << vtkClientServerStream::Invoke
      << ID
      << "GetFileListing"
      << Path.toAscii().data()
      << 0
      << vtkClientServerStream::End;
    ProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
    vtkClientServerStream result;
    ProcessModule->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result);

    if(result.GetNumberOfMessages() == 2)
      {
      // The first message lists directories.
      for(int i = 0; i < result.GetNumberOfArguments(0); ++i)
        {
        const char* directory_name = 0;
        if(result.GetArgument(0, i, &directory_name))
          {
          fileList.push_back(FileInfo(directory_name, true));
          }
        }

      // The second message lists files.
      for(int i = 0; i < result.GetNumberOfArguments(1); ++i)
        {
        const char* file_name = 0;
        if(result.GetArgument(1, i, &file_name))
          {
          fileList.push_back(FileInfo(file_name, false));
          }
        }
      }
    ProcessModule->DeleteStreamObject(ID, stream);

    emit dataChanged(QModelIndex(), QModelIndex());
    emit layoutChanged();
  }

  int columnCount(const QModelIndex& parent) const
  {
    return 1;
  }

  QVariant data(const QModelIndex & index, int role) const
  {
    if(!index.isValid())
      return QVariant();

    if(index.row() >= fileList.size())
      return QVariant();

    const FileInfo& file = fileList[index.row()];

    switch(role)
      {
      case Qt::DisplayRole:
        switch(index.column())
          {
          case 0:
            return file.fileName();
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
          }
      }
      
    return QVariant();
  }

  vtkProcessModule* const ProcessModule;
  QDir currentPath;
  QList<FileInfo> fileList;
};

//////////////////////////////////////////////////////////////////////////////////////////
// FavoriteModel

class FavoriteModel :
  public QAbstractItemModel
{
public:
  virtual int columnCount(const QModelIndex& parent) const
  {
    return 1;
  }
  
  virtual QVariant data(const QModelIndex & index, int role) const
  {
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
    return 0;
  }
  
  virtual bool hasChildren(const QModelIndex& parent) const
  {
    if(!parent.isValid())
      return true;
      
    return false;
  }
  
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
  {
    return QVariant();
  }
};

} // namespace

/////////////////////////////////////////////////////////////////////////
// pqServerFileDialogModel::Implementation

class pqServerFileDialogModel::Implementation
{
public:
  Implementation(vtkProcessModule* processModule) :
    fileModel(new FileModel(processModule)),
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

//////////////////////////////////////////////////////////////////////////
// pqServerFileDialogModel

pqServerFileDialogModel::pqServerFileDialogModel(vtkProcessModule* ProcessModule, QObject* Parent) :
  base(Parent),
  implementation(new Implementation(ProcessModule))
{
}

pqServerFileDialogModel::~pqServerFileDialogModel()
{
  delete implementation;
}

QString pqServerFileDialogModel::getStartPath()
{
  vtkClientServerStream stream;
  const vtkClientServerID id = implementation->fileModel->ProcessModule->NewStreamObject("vtkPVServerFileListing", stream);
  stream
    << vtkClientServerStream::Invoke
    << id
    << "GetCurrentWorkingDirectory"
    << vtkClientServerStream::End;
    
  implementation->fileModel->ProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  const char* cwd = "";
  implementation->fileModel->ProcessModule->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &cwd);
  QString result = cwd;
  implementation->fileModel->ProcessModule->DeleteStreamObject(id, stream);
  
  return result;
}

void pqServerFileDialogModel::setCurrentPath(const QString& Path)
{
  implementation->fileModel->setCurrentPath(Path);
}

QString pqServerFileDialogModel::getCurrentPath()
{
  return QDir::convertSeparators(implementation->fileModel->currentPath.path());
}

QStringList pqServerFileDialogModel::getFilePaths(const QModelIndex& Index)
{
  QStringList results;
  
  if(Index.row() < implementation->fileModel->fileList.size())
    { 
    FileInfo& file = implementation->fileModel->fileList[Index.row()];
    results.push_back(QDir::convertSeparators(implementation->fileModel->currentPath.path() + "/" + file.fileName()));
    }
    
  return results;
}

QString pqServerFileDialogModel::getFilePath(const QString& Path)
{
  if(QDir::isAbsolutePath(Path))
    return Path;
    
  return QDir::convertSeparators(implementation->fileModel->currentPath.path() + "/" + Path);
}

QString pqServerFileDialogModel::getParentPath(const QString& Path)
{
  QDir temp(Path);
  temp.cdUp();
  return temp.path();
}

bool pqServerFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.row() >= implementation->fileModel->fileList.size())
    return false;
    
  FileInfo& file = implementation->fileModel->fileList[Index.row()];
  return file.isDir();
}

QStringList pqServerFileDialogModel::splitPath(const QString& Path)
{
  QStringList results;
  
  for(int i = Path.indexOf(QDir::separator()); i != -1; i = Path.indexOf(QDir::separator(), i+1))
    results.push_back(Path.left(i) + QDir::separator());
    
  results.push_back(Path);
  
  return results;
}

QAbstractItemModel* pqServerFileDialogModel::fileModel()
{
  return implementation->fileModel;
}

QAbstractItemModel* pqServerFileDialogModel::favoriteModel()
{
  return implementation->favoriteModel;
}

