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
#include <vtkProcessModuleConnectionManager.h>

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

/////////////////////////////////////////////////////////////////////////////////
// pqFileModel

class pqFileModel :
  public QAbstractItemModel
{
public:
  pqFileModel(vtkProcessModule* processModule) :
    ProcessModule(processModule)
  {
  } 

  void SetCurrentPath(const QString& Path)
  {
    this->CurrentPath.setPath(QDir::cleanPath(Path));
    this->FileList.clear();

    this->FileList.push_back(FileInfo(".", ".", true, false));
    this->FileList.push_back(FileInfo("..", "..", true, false));

    vtkClientServerStream stream;
    const vtkClientServerID ID = this->ProcessModule->NewStreamObject("vtkPVServerFileListing", stream);
    stream
      << vtkClientServerStream::Invoke
      << ID
      << "GetFileListing"
      << Path.toAscii().data()
      << 0
      << vtkClientServerStream::End;
    this->ProcessModule->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    vtkClientServerStream result;
    this->ProcessModule->GetLastResult(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result);

    if(result.GetNumberOfMessages() == 2)
      {
      // The first message lists directories.
      for(int i = 0; i < result.GetNumberOfArguments(0); ++i)
        {
        const char* directory_name = 0;
        if(result.GetArgument(0, i, &directory_name))
          {
          this->FileList.push_back(FileInfo(directory_name, directory_name, true, false));
          }
        }

      // The second message lists files.
      for(int i = 0; i < result.GetNumberOfArguments(1); ++i)
        {
        const char* file_name = 0;
        if(result.GetArgument(1, i, &file_name))
          {
          this->FileList.push_back(FileInfo(file_name, file_name, false, false));
          }
        }
      }
    this->ProcessModule->DeleteStreamObject(ID, stream);

    emit dataChanged(QModelIndex(), QModelIndex());
    emit layoutChanged();
  }

  QStringList getFilePaths(const QModelIndex& Index)
  {
    QStringList results;
    
    if(Index.row() < this->FileList.size())
      { 
      FileInfo& file = this->FileList[Index.row()];
      results.push_back(QDir::convertSeparators(this->CurrentPath.path() + "/" + file.filePath()));
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

  vtkProcessModule* const ProcessModule;
  QDir CurrentPath;
  QList<FileInfo> FileList;
};

//////////////////////////////////////////////////////////////////
// FavoriteModel

class pqFavoriteModel :
  public QAbstractItemModel
{
public:
  pqFavoriteModel(vtkProcessModule* processModule) :
    ProcessModule(processModule)
  {
    vtkClientServerStream stream;
    const vtkClientServerID ID = this->ProcessModule->NewStreamObject("vtkPVServerFileListing", stream);
    stream
      << vtkClientServerStream::Invoke
      << ID
      << "GetSpecial"
      << vtkClientServerStream::End;
    this->ProcessModule->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    vtkClientServerStream result;
    this->ProcessModule->GetLastResult(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
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
      
    this->ProcessModule->DeleteStreamObject(ID, stream);
  }

  QStringList getFilePaths(const QModelIndex& Index)
  {
    QStringList results;
    
    if(Index.row() < this->FavoriteList.size())
      {
      FileInfo& file = this->FavoriteList[Index.row()];
      results.push_back(QDir::convertSeparators(file.filePath()));
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
  
  vtkProcessModule* const ProcessModule;
  QList<FileInfo> FavoriteList;
};

} // namespace

/////////////////////////////////////////////////////////////////////////
// pqServerFileDialogModel::Implementation

class pqServerFileDialogModel::pqImplementation
{
public:
  pqImplementation(vtkProcessModule* processModule) :
    FileModel(new pqFileModel(processModule)),
    FavoriteModel(new pqFavoriteModel(processModule))
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

//////////////////////////////////////////////////////////////////////////
// pqServerFileDialogModel

pqServerFileDialogModel::pqServerFileDialogModel(vtkProcessModule* ProcessModule, QObject* Parent) :
  base(Parent),
  Implementation(new pqImplementation(ProcessModule))
{
}

pqServerFileDialogModel::~pqServerFileDialogModel()
{
  delete this->Implementation;
}

QString pqServerFileDialogModel::getStartPath()
{
  vtkClientServerStream stream;
  const vtkClientServerID id = this->Implementation->FileModel->ProcessModule->NewStreamObject("vtkPVServerFileListing", stream);
  stream
    << vtkClientServerStream::Invoke
    << id
    << "GetCurrentWorkingDirectory"
    << vtkClientServerStream::End;
    
  this->Implementation->FileModel->ProcessModule->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
    vtkProcessModule::DATA_SERVER_ROOT, stream);
  const char* cwd = "";
  this->Implementation->FileModel->ProcessModule->GetLastResult(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
    vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &cwd);
  QString result = cwd;
  this->Implementation->FileModel->ProcessModule->DeleteStreamObject(id, stream);
  
  return result;
}

void pqServerFileDialogModel::setCurrentPath(const QString& Path)
{
  this->Implementation->FileModel->SetCurrentPath(Path);
}

QString pqServerFileDialogModel::getCurrentPath()
{
  return QDir::convertSeparators(this->Implementation->FileModel->CurrentPath.path());
}

QStringList pqServerFileDialogModel::getFilePaths(const QModelIndex& Index)
{
  if(Index.model() == this->Implementation->FileModel)
    return this->Implementation->FileModel->getFilePaths(Index);
  
  if(Index.model() == this->Implementation->FavoriteModel)
    return this->Implementation->FavoriteModel->getFilePaths(Index);  

  return QStringList();
}

QString pqServerFileDialogModel::getFilePath(const QString& Path)
{
  if(QDir::isAbsolutePath(Path))
    return Path;
    
  return QDir::convertSeparators(this->Implementation->FileModel->CurrentPath.path() + "/" + Path);
}

QString pqServerFileDialogModel::getParentPath(const QString& Path)
{
  QDir temp(Path);
  temp.cdUp();
  return temp.path();
}

bool pqServerFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.model() == this->Implementation->FileModel)
    return this->Implementation->FileModel->isDir(Index);
  
  if(Index.model() == this->Implementation->FavoriteModel)
    return this->Implementation->FavoriteModel->isDir(Index);  

  return false;    
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
  return this->Implementation->FileModel;
}

QAbstractItemModel* pqServerFileDialogModel::favoriteModel()
{
  return this->Implementation->FavoriteModel;
}

