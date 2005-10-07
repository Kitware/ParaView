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

  const QString& fileName()
  {
    return FileName;
  }
  
  const bool isDir()
  {
    return IsDir;
  }

private:
  QString FileName;
  bool IsDir;
};

} // namespace

class pqServerFileDialogModel::Implementation
{
public:
  Implementation(vtkProcessModule* processModule) :
    ProcessModule(processModule)
  {
  }

  vtkProcessModule* const ProcessModule;
  QDir ViewDirectory;
  vtkstd::vector<FileInfo> ViewList;
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
  const vtkClientServerID id = implementation->ProcessModule->NewStreamObject("vtkPVServerFileListing", stream);
  stream
    << vtkClientServerStream::Invoke
    << id
    << "GetCurrentWorkingDirectory"
    << vtkClientServerStream::End;
    
  implementation->ProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  const char* cwd = "";
  implementation->ProcessModule->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &cwd);
  QString result = cwd;
  implementation->ProcessModule->DeleteStreamObject(id, stream);
  
  return result;
}

void pqServerFileDialogModel::setViewDirectory(const QString& Path)
{
  implementation->ViewDirectory.setPath(QDir::cleanPath(Path));
  implementation->ViewList.clear();

  implementation->ViewList.push_back(FileInfo(".", true));
  implementation->ViewList.push_back(FileInfo("..", true));

  vtkClientServerStream stream;
  const vtkClientServerID ID = implementation->ProcessModule->NewStreamObject("vtkPVServerFileListing", stream);
  stream
    << vtkClientServerStream::Invoke
    << ID
    << "GetFileListing"
    << Path.toAscii().data()
    << 0
    << vtkClientServerStream::End;
  implementation->ProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  vtkClientServerStream result;
  implementation->ProcessModule->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result);

  if(result.GetNumberOfMessages() == 2)
    {
    // The first message lists directories.
    for(int i = 0; i < result.GetNumberOfArguments(0); ++i)
      {
      const char* directory_name = 0;
      if(result.GetArgument(0, i, &directory_name))
        {
        implementation->ViewList.push_back(FileInfo(directory_name, true));
        }
      }

    // The second message lists files.
    for(int i = 0; i < result.GetNumberOfArguments(1); ++i)
      {
      const char* file_name = 0;
      if(result.GetArgument(1, i, &file_name))
        {
        implementation->ViewList.push_back(FileInfo(file_name, false));
        }
      }
    }
  implementation->ProcessModule->DeleteStreamObject(ID, stream);

  emit dataChanged(QModelIndex(), QModelIndex());
  emit layoutChanged();
}

QString pqServerFileDialogModel::getViewDirectory()
{
  return QDir::convertSeparators(implementation->ViewDirectory.path());
}

QString pqServerFileDialogModel::getFilePath(const QModelIndex& Index)
{
  if(Index.row() >= implementation->ViewList.size())
    return QString();
    
  FileInfo& file = implementation->ViewList[Index.row()];
  return QDir::convertSeparators(implementation->ViewDirectory.path() + "/" + file.fileName());
}

bool pqServerFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.row() >= implementation->ViewList.size())
    return false;
    
  FileInfo& file = implementation->ViewList[Index.row()];
  return file.isDir();
}

void pqServerFileDialogModel::navigateUp()
{
  QDir temp = implementation->ViewDirectory;
  temp.cdUp();
  
  setViewDirectory(temp.path());
}

void pqServerFileDialogModel::navigateDown(const QModelIndex& Index)
{
  if(Index.row() >= implementation->ViewList.size())
    return;
    
  FileInfo& file = implementation->ViewList[Index.row()];
  
  if(file.isDir())
    {
    setViewDirectory(implementation->ViewDirectory.path() + "/" + file.fileName());
    return;
    }
}

int pqServerFileDialogModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant pqServerFileDialogModel::data(const QModelIndex & index, int role) const
{
  if(!index.isValid())
    return QVariant();

  if(index.row() >= implementation->ViewList.size())
    return QVariant();

  FileInfo& file = implementation->ViewList[index.row()];

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

QModelIndex pqServerFileDialogModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column);
}

QModelIndex pqServerFileDialogModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int pqServerFileDialogModel::rowCount(const QModelIndex& parent) const
{
  return implementation->ViewList.size();
}

bool pqServerFileDialogModel::hasChildren(const QModelIndex& parent) const
{
  if(!parent.isValid())
    return true;
    
  return false;
}

QVariant pqServerFileDialogModel::headerData(int section, Qt::Orientation orientation, int role) const
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


