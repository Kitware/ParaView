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

    this->FileList.push_back(FileInfo(".", true));
    this->FileList.push_back(FileInfo("..", true));

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
          this->FileList.push_back(FileInfo(directory_name, true));
          }
        }

      // The second message lists files.
      for(int i = 0; i < result.GetNumberOfArguments(1); ++i)
        {
        const char* file_name = 0;
        if(result.GetArgument(1, i, &file_name))
          {
          this->FileList.push_back(FileInfo(file_name, false));
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

    if(index.row() >= this->FileList.size())
      return QVariant();

    const FileInfo& file = this->FileList[index.row()];

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
    return this->FileList.size();
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
  QDir CurrentPath;
  QList<FileInfo> FileList;
};

//////////////////////////////////////////////////////////////////////////////////////////
// pqFavoriteModel

class pqFavoriteModel :
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

class pqServerFileDialogModel::pqImplementation
{
public:
  pqImplementation(vtkProcessModule* processModule) :
    FileModel(new pqFileModel(processModule)),
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

QString pqServerFileDialogModel::GetStartPath()
{
  vtkClientServerStream stream;
  const vtkClientServerID id = this->Implementation->FileModel->ProcessModule->NewStreamObject("vtkPVServerFileListing", stream);
  stream
    << vtkClientServerStream::Invoke
    << id
    << "GetCurrentWorkingDirectory"
    << vtkClientServerStream::End;
    
  this->Implementation->FileModel->ProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  const char* cwd = "";
  this->Implementation->FileModel->ProcessModule->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &cwd);
  QString result = cwd;
  this->Implementation->FileModel->ProcessModule->DeleteStreamObject(id, stream);
  
  return result;
}

void pqServerFileDialogModel::SetCurrentPath(const QString& Path)
{
  this->Implementation->FileModel->SetCurrentPath(Path);
}

QString pqServerFileDialogModel::GetCurrentPath()
{
  return QDir::convertSeparators(this->Implementation->FileModel->CurrentPath.path());
}

QStringList pqServerFileDialogModel::GetFilePaths(const QModelIndex& Index)
{
  QStringList results;
  
  if(Index.row() < this->Implementation->FileModel->FileList.size())
    { 
    FileInfo& file = this->Implementation->FileModel->FileList[Index.row()];
    results.push_back(QDir::convertSeparators(this->Implementation->FileModel->CurrentPath.path() + "/" + file.fileName()));
    }
    
  return results;
}

QString pqServerFileDialogModel::GetFilePath(const QString& Path)
{
  if(QDir::isAbsolutePath(Path))
    return Path;
    
  return QDir::convertSeparators(this->Implementation->FileModel->CurrentPath.path() + "/" + Path);
}

QString pqServerFileDialogModel::GetParentPath(const QString& Path)
{
  QDir temp(Path);
  temp.cdUp();
  return temp.path();
}

bool pqServerFileDialogModel::IsDir(const QModelIndex& Index)
{
  if(Index.row() >= this->Implementation->FileModel->FileList.size())
    return false;
    
  FileInfo& file = this->Implementation->FileModel->FileList[Index.row()];
  return file.isDir();
}

QStringList pqServerFileDialogModel::SplitPath(const QString& Path)
{
  QStringList results;
  
  for(int i = Path.indexOf(QDir::separator()); i != -1; i = Path.indexOf(QDir::separator(), i+1))
    results.push_back(Path.left(i) + QDir::separator());
    
  results.push_back(Path);
  
  return results;
}

QAbstractItemModel* pqServerFileDialogModel::FileModel()
{
  return this->Implementation->FileModel;
}

QAbstractItemModel* pqServerFileDialogModel::FavoriteModel()
{
  return this->Implementation->FavoriteModel;
}

