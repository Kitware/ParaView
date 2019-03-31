/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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

//-----------------------------------------------------------------------------
class pqPluginDocumentationBehavior::pqInternals
{
public:
  QSet<QString> RegisteredPlugins;
  QTimer Timer;

  pqInternals()
  {
    this->Timer.setInterval(100);
    this->Timer.setSingleShot(true);
  }
};

//-----------------------------------------------------------------------------
pqPluginDocumentationBehavior::pqPluginDocumentationBehavior(QHelpEngine* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals())
{
  assert(parentObject != NULL);

  QObject::connect(&this->Internals->Timer, SIGNAL(timeout()), this, SLOT(refreshHelpEngine()));

  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  pqCoreUtilities::connect(tracker, vtkCommand::RegisterEvent, this, SLOT(updatePlugins()));
  this->updatePlugins();
}

//-----------------------------------------------------------------------------
pqPluginDocumentationBehavior::~pqPluginDocumentationBehavior()
{
  delete this->Internals;
  this->Internals = NULL;
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
      qCritical() << "Failed to create temporary files." << endl;
      delete[] decoded_stream;
      decoded_stream = NULL;
      continue;
    }
    qint64 written =
      file->write(reinterpret_cast<char*>(decoded_stream), static_cast<qint64>(length));
    assert(written == static_cast<qint64>(length));
    (void)written;
    engine->registerDocumentation(file->fileName());

    delete[] decoded_stream;
    decoded_stream = NULL;

    this->Internals->Timer.start();
  }
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
