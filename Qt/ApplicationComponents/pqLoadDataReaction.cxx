/*=========================================================================

   Program: ParaView
   Module:    pqLoadDataReaction.cxx

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
#include "pqLoadDataReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqSelectReaderDialog.h"
#include "pqServer.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
#include "pqUndoStack.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkStringList.h"

#include <QDebug>

#include <cassert>

//-----------------------------------------------------------------------------
pqLoadDataReaction::pqLoadDataReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqLoadDataReaction::updateEnableState()
{
  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  // TODO: also is there's a pending accept.
  bool enable_state = (activeObjects.activeServer() != NULL);
  this->parentAction()->setEnabled(enable_state);
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> pqLoadDataReaction::loadData()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  vtkSMReaderFactory* readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
  QString filters = readerFactory->GetSupportedFileTypes(server->session());
  // insert "All Files(*)" as the second item after supported files.
  int insertIndex = filters.indexOf(";;");
  if (insertIndex >= 0)
  {
    filters.insert(insertIndex, ";;All Files (*)");
  }
  else
  {
    assert(filters.isEmpty());
    filters = "All Files (*)";
  }

  pqFileDialog fileDialog(
    server, pqCoreUtilities::mainWidget(), tr("Open File:"), QString(), filters);
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFilesAndDirectories);
  QList<pqPipelineSource*> sources;
  if (fileDialog.exec() == QDialog::Accepted)
  {
    QList<QStringList> files = fileDialog.getAllSelectedFiles();
    pqPipelineSource* source = pqLoadDataReaction::loadData(files);
    if (source)
    {
      sources << source;
    }
  }
  return sources;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqLoadDataReaction::loadData(
  const QStringList& files, const QString& smgroup, const QString& smname, pqServer* server)
{
  QList<QStringList> f;
  f.append(files);
  return pqLoadDataReaction::loadData(f, smgroup, smname, server);
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqLoadDataReaction::loadData(
  const QList<QStringList>& files, const QString& smgroup, const QString& smname, pqServer* server)
{
  if (files.empty())
  {
    return NULL;
  }

  server = server != NULL ? server : pqActiveObjects::instance().activeServer();
  if (!server)
  {
    qCritical() << "Cannot create reader without an active server.";
    return NULL;
  }

  vtkSMReaderFactory* readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
  pqPipelineSource* reader = NULL;

  // Extension to ReaderType,ReaderGroup Hash table
  QHash<QString, QPair<QString, QString> > extensionToReaderSelection;
  foreach (const QStringList& filegroup, files)
  {
    QPair<QString, QString> readerInfo; // type,group
    QString filename(filegroup[0]);
    QFileInfo fi(filename);

    if (!pqLoadDataReaction::TestFileReadability(filename, server, readerFactory))
    {
      qWarning() << "File '" << filename << "' cannot be read.";
      continue;
    }

    if (!smgroup.isEmpty() && !smname.isEmpty())
    {
      readerInfo = QPair<QString, QString>(smname, smgroup);
    }
    else
    {
      // Determine reader type based on if we have asked the user for this extension before
      QHash<QString, QPair<QString, QString> >::const_iterator it =
        extensionToReaderSelection.find(fi.suffix());
      if (it != extensionToReaderSelection.end())
      {
        readerInfo = it.value();
      }
      else
      {
        // Determine reader type based on the first filegroup
        bool valid =
          pqLoadDataReaction::DetermineFileReader(filename, server, readerFactory, readerInfo);
        if (valid == 0)
        {
          // no reader selected
          continue;
        }
      }
    }

    // read the filegroup
    BEGIN_UNDO_SET("Create 'Reader'");
    reader = pqLoadDataReaction::LoadFile(filegroup, server, readerInfo);
    END_UNDO_SET();

    // add this extension to the hash set
    extensionToReaderSelection.insert(fi.suffix(), readerInfo);
  }
  return reader;
}

//-----------------------------------------------------------------------------
bool pqLoadDataReaction::TestFileReadability(
  const QString& file, pqServer* server, vtkSMReaderFactory* vtkNotUsed(factory))
{
  return vtkSMReaderFactory::TestFileReadability(file.toUtf8().data(), server->session());
}

//-----------------------------------------------------------------------------
bool pqLoadDataReaction::DetermineFileReader(const QString& filename, pqServer* server,
  vtkSMReaderFactory* factory, QPair<QString, QString>& readerInfo)
{
  QString readerType, readerGroup;
  vtkStringList* list = factory->GetReaders(filename.toUtf8().data(), server->session());
  if (list->GetLength() > 3)
  {
    // If more than one readers found.
    pqSelectReaderDialog prompt(filename, server, list, pqCoreUtilities::mainWidget());
    if (prompt.exec() == QDialog::Accepted)
    {
      readerType = prompt.getReader();
      readerGroup = prompt.getGroup();
    }
    else
    {
      // User didn't choose any reader.
      return false;
    }
  }
  else if (list->GetLength() == 3)
  {
    // reader knows the type
    readerGroup = list->GetString(0);
    readerType = list->GetString(1);
  }
  else
  {
    // The reader factory could not determine the type of reader to create for the
    // file. Ask the user.
    pqSelectReaderDialog prompt(filename, server, factory, pqCoreUtilities::mainWidget());
    if (prompt.exec() == QDialog::Accepted)
    {
      readerType = prompt.getReader();
      readerGroup = prompt.getGroup();
    }
    else
    {
      // User didn't choose any reader.
      return false;
    }
  }
  readerInfo.first = readerType;
  readerInfo.second = readerGroup;
  return true;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqLoadDataReaction::LoadFile(
  const QStringList& files, pqServer* server, const QPair<QString, QString>& readerInfo)
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqPipelineSource* reader =
    builder->createReader(readerInfo.second, readerInfo.first, files, server);

  if (reader)
  {
    pqStandardRecentlyUsedResourceLoaderImplementation::addDataFilesToRecentResources(
      server, files, reader->getProxy()->GetXMLGroup(), reader->getProxy()->GetXMLName());
  }
  return reader;
}
