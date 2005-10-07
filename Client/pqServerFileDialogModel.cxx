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

#include <vtkstd/vector>

namespace
{

QFileIconProvider& Icons()
{
  static QFileIconProvider* icons = 0;
  if(!icons)
    icons = new QFileIconProvider();
    
  return *icons;
}

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

class FileModel :
  public QAbstractItemModel
{
public:
  FileModel(vtkProcessModule* processModule) :
    ProcessModule(processModule)
  {
  } 

  void setViewDirectory(const QString& Path)
  {
    ViewDirectory.setPath(QDir::cleanPath(Path));
    ViewList.clear();

    ViewList.push_back(FileInfo(".", true));
    ViewList.push_back(FileInfo("..", true));

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
          ViewList.push_back(FileInfo(directory_name, true));
          }
        }

      // The second message lists files.
      for(int i = 0; i < result.GetNumberOfArguments(1); ++i)
        {
        const char* file_name = 0;
        if(result.GetArgument(1, i, &file_name))
          {
          ViewList.push_back(FileInfo(file_name, false));
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

    if(index.row() >= ViewList.size())
      return QVariant();

    const FileInfo& file = ViewList[index.row()];

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
    return ViewList.size();
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
  QDir ViewDirectory;
  vtkstd::vector<FileInfo> ViewList;
};

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

pqServerFileDialogModel::pqServerFileDialogModel(vtkProcessModule* ProcessModule, QObject* Parent) :
  base(Parent),
  implementation(new Implementation(ProcessModule))
{
}

pqServerFileDialogModel::~pqServerFileDialogModel()
{
  delete implementation;
}

QString pqServerFileDialogModel::getStartDirectory()
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

void pqServerFileDialogModel::setViewDirectory(const QString& Path)
{
  implementation->fileModel->setViewDirectory(Path);
}

QString pqServerFileDialogModel::getViewDirectory()
{
  return QDir::convertSeparators(implementation->fileModel->ViewDirectory.path());
}

QString pqServerFileDialogModel::getFilePath(const QModelIndex& Index)
{
  if(Index.row() >= implementation->fileModel->ViewList.size())
    return QString();
    
  FileInfo& file = implementation->fileModel->ViewList[Index.row()];
  return QDir::convertSeparators(implementation->fileModel->ViewDirectory.path() + "/" + file.fileName());
}

bool pqServerFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.row() >= implementation->fileModel->ViewList.size())
    return false;
    
  FileInfo& file = implementation->fileModel->ViewList[Index.row()];
  return file.isDir();
}

QAbstractItemModel* pqServerFileDialogModel::fileModel()
{
  return implementation->fileModel;
}

QAbstractItemModel* pqServerFileDialogModel::favoriteModel()
{
  return implementation->favoriteModel;
}

void pqServerFileDialogModel::navigateUp()
{
  QDir temp = implementation->fileModel->ViewDirectory;
  temp.cdUp();
  
  setViewDirectory(temp.path());
}

void pqServerFileDialogModel::navigateDown(const QModelIndex& Index)
{
  if(Index.row() >= implementation->fileModel->ViewList.size())
    return;
    
  FileInfo& file = implementation->fileModel->ViewList[Index.row()];
  
  if(file.isDir())
    {
    setViewDirectory(implementation->fileModel->ViewDirectory.path() + "/" + file.fileName());
    return;
    }
}

