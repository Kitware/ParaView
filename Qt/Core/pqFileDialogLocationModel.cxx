// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFileDialogLocationModel.h"

#include <pqFileDialogModel.h>
#include <pqSMAdaptor.h>
#include <pqServer.h>
#include <pqServerResource.h>
#include <vtkClientServerStream.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkPVEnvironmentInformation.h>
#include <vtkPVEnvironmentInformationHelper.h>
#include <vtkPVFileInformation.h>
#include <vtkPVFileInformationHelper.h>
#include <vtkProcessModule.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSettings.h>
#include <vtkSMSettingsProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSmartPointer.h>
#include <vtkStringList.h>

#include <QApplication>
#include <QBrush>
#include <QDir>
#include <QFont>
#include <QStyle>

bool pqFileDialogLocationModel::AddExamplesInLocations = true;

/////////////////////////////////////////////////////////////////////
// Icons
// NOLINTNEXTLINE(readability-redundant-member-init)
Q_GLOBAL_STATIC(pqFileDialogModelIconProvider, Icons);

//-----------------------------------------------------------------------------
pqFileDialogLocationModel::pqFileDialogLocationModel(
  pqFileDialogModel* fileDialogModel, pqServer* server, QObject* p)
  : Superclass(p)
  , FileDialogModel(fileDialogModel)
{
  this->Server = server;
  this->LoadSpecialsFromSystem();
}

//-----------------------------------------------------------------------------
QString pqFileDialogLocationModel::filePath(const QModelIndex& index) const
{
  return this->data(index, Qt::UserRole).toString();
}

//-----------------------------------------------------------------------------
bool pqFileDialogLocationModel::isDirectory(const QModelIndex& index) const
{
  if (index.row() >= this->LocationList.size())
  {
    return false;
  }

  const pqFileDialogLocationModelFileInfo& file = this->LocationList[index.row()];
  return vtkPVFileInformation::IsDirectory(file.Type);
}

//-----------------------------------------------------------------------------
QVariant pqFileDialogLocationModel::data(const QModelIndex& idx, int role) const
{
  if (!idx.isValid() || idx.row() >= this->LocationList.size() || idx.column() != 0)
  {
    return QVariant();
  }

  const pqFileDialogLocationModelFileInfo& file = this->LocationList[idx.row()];
  auto dir = file.FilePath;
  // If it is the Examples dir placeholder, replace it with the real path to the examples.
  if (dir == "_examples_path_")
  {
    // FIXME when the shared resources dir is not found, this is equal to `/examples`. This might be
    // confusing for people without the `Examples` directory (mostly ParaView devs). This directory
    // can be hidden by setting the `AddExamplesInFileDialogBehavior` to `false`.
    dir = QString::fromStdString(vtkPVFileInformation::GetParaViewExampleFilesDirectory());
  }

  QString temp;
  switch (role)
  {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return file.Label;
    case Qt::UserRole:
      return dir;
    case Qt::ItemDataRole::ToolTipRole:
      if (file.Type != vtkPVFileInformation::INVALID)
      {
        return dir;
      }
      else
      {
        return dir + " (Warning: invalid)";
      }
    case Qt::ItemDataRole::FontRole:
      if (file.Type != vtkPVFileInformation::INVALID)
      {
        return {};
      }
      else
      {
        QFont font;
        font.setItalic(true);
        return font;
      }

    case Qt::ItemDataRole::ForegroundRole:
      if (file.Type != vtkPVFileInformation::INVALID)
      {
        return {};
      }
      else
      {
        QBrush brush;
        brush.setColor(QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
        return brush;
      }

    case Qt::DecorationRole:
      return Icons()->icon(static_cast<vtkPVFileInformation::FileTypes>(file.Type));
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
int pqFileDialogLocationModel::rowCount(const QModelIndex&) const
{
  return this->LocationList.size();
}

//-----------------------------------------------------------------------------
QVariant pqFileDialogLocationModel::headerData(int section, Qt::Orientation, int role) const
{
  switch (role)
  {
    case Qt::DisplayRole:
      switch (section)
      {
        case 0:
          return tr("Locations");
      }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
void pqFileDialogLocationModel::resetToDefault()
{
  this->beginResetModel();
  this->LoadSpecialsFromSystem();
  this->endResetModel();
}

//-----------------------------------------------------------------------------
void pqFileDialogLocationModel::LoadSpecialsFromSystem()
{
  vtkNew<vtkPVFileInformation> information;
  vtkNew<vtkPVEnvironmentInformation> envInfo;
  auto* settings = vtkSMSettings::GetInstance();
  // this setting is a list of labels and env-var pairs, the env-vars should store directory paths
  char const* settingName = ".settings.IOSettings.LocationEnvironmentVariables";
  unsigned int const numberOfEntries = settings->GetSettingNumberOfElements(settingName) / 2;
  std::vector<std::pair<QString, QString>> envVarList;

  if (this->Server)
  {
    vtkSMSessionProxyManager* pxm = this->Server->proxyManager();

    vtkSmartPointer<vtkSMProxy> helper;
    helper.TakeReference(pxm->NewProxy("misc", "FileInformationHelper"));
    pqSMAdaptor::setElementProperty(helper->GetProperty("SpecialDirectories"), true);
    pqSMAdaptor::setElementProperty(helper->GetProperty("ExamplesInSpecialDirectories"),
      pqFileDialogLocationModel::AddExamplesInLocations);
    helper->UpdateVTKObjects();
    helper->GatherInformation(information);

    helper.TakeReference(pxm->NewProxy("misc", "EnvironmentInformationHelper"));
    for (unsigned int ii = 0; ii < numberOfEntries; ++ii)
    {
      auto const label = settings->GetSettingAsString(settingName, ii * 2, "");
      auto const var = settings->GetSettingAsString(settingName, ii * 2 + 1, "");
      pqSMAdaptor::setElementProperty(helper->GetProperty("Variable"), var.c_str());
      helper->UpdateVTKObjects();
      helper->GatherInformation(envInfo);
      // only add a Location entry if the env-var has a non-empty value.
      if (envInfo->GetVariable() && !std::string(envInfo->GetVariable()).empty())
      {
        envVarList.push_back({ label.c_str(), envInfo->GetVariable() });
      }
    }
  }
  else
  {
    vtkNew<vtkPVFileInformationHelper> helper;
    helper->SetSpecialDirectories(1);
    helper->SetExamplesInSpecialDirectories(pqFileDialogLocationModel::AddExamplesInLocations);
    information->CopyFromObject(helper);

    vtkNew<vtkPVEnvironmentInformationHelper> envHelper;
    for (unsigned int ii = 0; ii < numberOfEntries; ++ii)
    {
      auto const label = settings->GetSettingAsString(settingName, ii * 2, "");
      auto const var = settings->GetSettingAsString(settingName, ii * 2 + 1, "");
      envHelper->SetVariable(var.c_str());
      envInfo->CopyFromObject(envHelper);
      if (envInfo->GetVariable() && !std::string(envInfo->GetVariable()).empty())
      {
        envVarList.push_back({ label.c_str(), envInfo->GetVariable() });
      }
    }
  }

  this->LocationList.clear();
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(information->GetContents()->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkPVFileInformation* cur_info = vtkPVFileInformation::SafeDownCast(iter->GetCurrentObject());
    if (!cur_info)
    {
      continue;
    }
    this->LocationList.push_back(pqFileDialogLocationModelFileInfo{
      cur_info->GetName(), QDir::cleanPath(cur_info->GetFullPath()), cur_info->GetType() });
  }
  // add directories from env-vars at the end
  for (auto& it : envVarList)
  {
    QString label = it.first;
    QString path = it.second;
    int type = this->FileDialogModel->fileType(path);

    this->LocationList.push_back(pqFileDialogLocationModelFileInfo{ label, path, type });
  }
}
