/*=========================================================================

   Program: ParaView
   Module:  pqUsageLoggingBehavior.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqUsageLoggingBehavior.h"

#include "vtkNew.h"
#include "vtkPVConfig.h"
#include "vtkPVLogger.h"
#include "vtkResourceFileLocator.h"
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
          fname.toLocal8Bit().data(), error.errorString().toLocal8Bit().data());
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
  auto path = locator->Locate(vtk_libs, prefixes, configFName.toLocal8Bit().data());
  if (!path.empty())
  {
    return vtksys::SystemTools::CollapseFullPath(configFName.toLocal8Bit().data(), path).c_str();
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
  vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "query-params: %s",
    query.toString().toLocal8Bit().data());

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
