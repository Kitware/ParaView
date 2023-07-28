// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqUsageLoggingBehavior.h"

#include "vtkNew.h"
#include "vtkPVLogger.h"
#include "vtkPVVersion.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"
#include "vtksys/SystemTools.hxx"

#include <QApplication>
#include <QDate>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QSysInfo>
#include <QUrl>
#include <QUrlQuery>

QString pqUsageLoggingBehavior::ConfigFileName{ "usage_logger.json" };

//-----------------------------------------------------------------------------
pqUsageLoggingBehavior::pqUsageLoggingBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  auto fname = this->configurationFile();
  if (!fname.isEmpty())
  {
    QFile file(fname);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QJsonParseError error;
      auto doc = QJsonDocument::fromJson(file.readAll(), &error);
      if (doc.isNull())
      {
        vtkLogF(ERROR, "Invalid usage-logging config '%s'. Not be a valid json string:\n %s",
          fname.toUtf8().data(), error.errorString().toUtf8().data());
      }
      else
      {
        this->logUsage(doc.object());
      }
    }
  }
}

//-----------------------------------------------------------------------------
pqUsageLoggingBehavior::~pqUsageLoggingBehavior() = default;

//-----------------------------------------------------------------------------
void pqUsageLoggingBehavior::setConfigFileName(const QString& fname)
{
  pqUsageLoggingBehavior::ConfigFileName = fname;
}

//-----------------------------------------------------------------------------
QString pqUsageLoggingBehavior::configurationFile() const
{
  auto vtk_libs = vtkGetLibraryPathForSymbol(GetVTKVersion);
#if defined(_WIN32)
  const std::vector<std::string> prefixes = { ".", "share" };
#elif defined(APPLE)
  const std::vector<std::string> prefixes = { ".", "Resources" };
#else
  const std::vector<std::string> prefixes = { ".", "lib", "lib64" };
#endif

  vtkNew<vtkResourceFileLocator> locator;
  locator->SetLogVerbosity(vtkPVLogger::GetApplicationVerbosity());

  auto configFName = pqUsageLoggingBehavior::configFileName();
  auto path = locator->Locate(vtk_libs, prefixes, configFName.toUtf8().toStdString());
  if (!path.empty())
  {
    return vtksys::SystemTools::CollapseFullPath(configFName.toUtf8().toStdString(), path).c_str();
  }
  return QString();
}

//-----------------------------------------------------------------------------
void pqUsageLoggingBehavior::logUsage(const QJsonObject& config)
{
  vtkVLogScopeFunction(PARAVIEW_LOG_APPLICATION_VERBOSITY());
  auto url = config["url"];
  if (!url.isString())
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "Missing (or invalid) 'url'.");
    return;
  }

  QUrl serviceUrl = QUrl(url.toString());
  QUrlQuery query;
  if (!config["params"].isObject())
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "Invalid 'params'. Must be a json-object.");
  }
  else
  {
    const auto params = config["params"].toObject();
    for (auto iter = params.begin(); iter != params.end(); ++iter)
    {
      query.addQueryItem(iter.key(), pqUsageLoggingBehavior::substitute(iter.value().toString()));
    }
  }
  vtkVLogF(
    PARAVIEW_LOG_APPLICATION_VERBOSITY(), "query-params: %s", query.toString().toUtf8().data());

  auto networkManager = new QNetworkAccessManager(this);
  QNetworkRequest networkRequest(serviceUrl);
  networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  networkManager->post(networkRequest, query.toString(QUrl::FullyEncoded).toUtf8());
}

//-----------------------------------------------------------------------------
QString pqUsageLoggingBehavior::substitute(const QString& value)
{
  if (value == "$username$")
  {
    QString name = qgetenv("USER");
    if (name.isEmpty())
    {
      name = qgetenv("USERNAME");
    }
    return name;
  }
  else if (value == "$platform$")
  {
    return QSysInfo::prettyProductName();
  }
  else if (value == "$date$")
  {
    return QDate::currentDate().toString("MM/dd/yy");
  }
  else if (value == "$appname$")
  {
    return QApplication::applicationName();
  }
  else if (value == "$appversion$")
  {
    return PARAVIEW_VERSION_FULL;
  }
  return value;
}
