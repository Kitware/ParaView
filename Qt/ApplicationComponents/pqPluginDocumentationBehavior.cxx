// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPluginDocumentationBehavior.h"

#include "pqCoreUtilities.h"
#include "vtkCommand.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginTracker.h"
#include <vtksys/Base64.h>

#include <QHelpContentWidget>
#include <QHelpEngine>
#include <QHelpSearchEngine>
#include <QSet>
#include <QTemporaryFile>
#include <QTimer>
#include <QtDebug>

#include <cassert>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#define QT_ENDL endl
#else
#define QT_ENDL Qt::endl
#endif

constexpr int INDEX_TIMEOUT_MSEC = 100;

//-----------------------------------------------------------------------------
class pqPluginDocumentationBehavior::pqInternals
{
public:
  QSet<QString> RegisteredPlugins;
};

//-----------------------------------------------------------------------------
pqPluginDocumentationBehavior::pqPluginDocumentationBehavior(QHelpEngine* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals())
{
  assert(parentObject != nullptr);

  QTimer::singleShot(INDEX_TIMEOUT_MSEC, this, SLOT(refreshHelpEngine()));

  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  pqCoreUtilities::connect(tracker, vtkCommand::RegisterEvent, this, SLOT(updatePlugins()));
  this->updatePlugins();
}

//-----------------------------------------------------------------------------
pqPluginDocumentationBehavior::~pqPluginDocumentationBehavior()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqPluginDocumentationBehavior::updatePlugins()
{
  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  for (unsigned int cc = 0; cc < tracker->GetNumberOfPlugins(); cc++)
  {
    if (tracker->GetPluginLoaded(cc))
    {
      this->updatePlugin(tracker->GetPlugin(cc));
    }
  }
}

//-----------------------------------------------------------------------------
void pqPluginDocumentationBehavior::updatePlugin(vtkPVPlugin* plugin)
{
  if (!plugin || this->Internals->RegisteredPlugins.contains(plugin->GetPluginName()))
  {
    return;
  }

  std::vector<std::string> resources;
  plugin->GetBinaryResources(resources);

  QHelpEngine* engine = qobject_cast<QHelpEngine*>(this->parent());
  assert(engine);

  for (size_t cc = 0; cc < resources.size(); cc++)
  {
    const std::string& str = resources[cc];
    unsigned char* decoded_stream = new unsigned char[str.size()];
    size_t length = vtksysBase64_Decode(reinterpret_cast<const unsigned char*>(str.c_str()),
      static_cast<unsigned long>(str.size()), decoded_stream, 0);

    // the file gets deleted with the pqPluginDocumentationBehavior is deleted.
    QTemporaryFile* file = new QTemporaryFile(this);
    if (!file->open())
    {
      qCritical() << "Failed to create temporary files." << QT_ENDL;
      delete[] decoded_stream;
      decoded_stream = nullptr;
      continue;
    }
    qint64 written =
      file->write(reinterpret_cast<char*>(decoded_stream), static_cast<qint64>(length));
    assert(written == static_cast<qint64>(length));
    (void)written;
    engine->registerDocumentation(file->fileName());

    delete[] decoded_stream;
    decoded_stream = nullptr;
  }

  QTimer::singleShot(INDEX_TIMEOUT_MSEC, this, SLOT(refreshHelpEngine()));
}

//-----------------------------------------------------------------------------
void pqPluginDocumentationBehavior::refreshHelpEngine()
{
  QHelpEngine* engine = qobject_cast<QHelpEngine*>(this->parent());
  assert(engine);

  engine->setupData();
  engine->contentWidget()->reset();
  engine->searchEngine()->reindexDocumentation();
}
