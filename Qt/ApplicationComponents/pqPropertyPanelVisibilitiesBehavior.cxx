// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPropertyPanelVisibilitiesBehavior.h"

#include "pqProxyWidget.h"
#include "vtkPVLogger.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonValue>
#include <QMainWindow>
#include <QString>

namespace
{
static const QString CONFIG_FILENAME = "PropertyPanelVisibilities.json";
};

pqPropertyPanelVisibilitiesBehavior::~pqPropertyPanelVisibilitiesBehavior() = default;

//-----------------------------------------------------------------------------
pqPropertyPanelVisibilitiesBehavior::pqPropertyPanelVisibilitiesBehavior(QObject* parent)
  : Superclass(parent)
{
  this->loadConfiguration();
  this->updateExistingProxies();

  QObject::connect(pqApplicationCore::instance()->getServerManagerObserver(),
    QOverload<const QString&, const QString&, vtkSMProxy*>::of(
      &pqServerManagerObserver::proxyRegistered),
    [&](const QString&, const QString&, vtkSMProxy* proxy) { this->overrideVisibility(proxy); });
}

//-----------------------------------------------------------------------------
void pqPropertyPanelVisibilitiesBehavior::loadConfiguration()
{
  QString configFilePath = this->getConfigFilePath();
  if (configFilePath.isEmpty())
  {
    return;
  }

  vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "loading visibility settings JSON file '%s'",
    configFilePath.toStdString().c_str());

  if (!this->parseSettingFile(configFilePath))
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "failed to parse file");
    return;
  }
}

//-----------------------------------------------------------------------------
QString pqPropertyPanelVisibilitiesBehavior::getConfigFilePath()
{
  QStringList availables = pqCoreUtilities::findParaviewPaths(::CONFIG_FILENAME, true, true);
  if (availables.empty())
  {
    return "";
  }

  return availables.first();
}

//-----------------------------------------------------------------------------
bool pqPropertyPanelVisibilitiesBehavior::parseSettingFile(const QString& filePath)
{
  QFile visibilityConfig(filePath);
  if (!visibilityConfig.open(QIODevice::ReadOnly))
  {
    qWarning() << "Could not read configuration file";
    return false;
  }

  const QString fileContent = visibilityConfig.readAll();
  this->Config = QJsonDocument::fromJson(fileContent.toUtf8());

  return !this->Config.isNull() && this->Config.isObject();
}

//-----------------------------------------------------------------------------
void pqPropertyPanelVisibilitiesBehavior::updateExistingProxies()
{
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (pxm)
  {
    vtkNew<vtkSMProxyIterator> proxyIterator;
    proxyIterator->SetSessionProxyManager(pxm);
    for (proxyIterator->Begin(); !proxyIterator->IsAtEnd(); proxyIterator->Next())
    {
      vtkSMProxy* proxy = proxyIterator->GetProxy();
      this->overrideVisibility(proxy);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPropertyPanelVisibilitiesBehavior::overrideVisibility(vtkSMProxy* smProxy)
{
  QString proxyName = smProxy->GetXMLName();
  QString proxyGroup = smProxy->GetXMLGroup();

  QJsonObject root = this->Config.object();
  if (!root.contains(proxyGroup))
  {
    return;
  }

  QJsonObject jsonGroup = root[proxyGroup].toObject();
  if (!jsonGroup.contains(proxyName))
  {
    return;
  }

  QJsonObject jsonProxy = jsonGroup[proxyName].toObject();

  QString propertyVisibility;
  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(smProxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    if (prop)
    {
      if (!jsonProxy.contains(prop->GetXMLName()))
      {
        continue;
      }
      propertyVisibility = jsonProxy[prop->GetXMLName()].toString();
      prop->SetPanelVisibility(propertyVisibility.toStdString().c_str());
    }
  }
}
