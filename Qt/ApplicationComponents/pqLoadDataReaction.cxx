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
#include "pqServerResource.h"
#include "pqServerResources.h"
#include "pqUndoStack.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"

#include <QDebug>

#include "vtkStringList.h"

//-----------------------------------------------------------------------------
pqLoadDataReaction::pqLoadDataReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(serverChanged(pqServer*)),
    this, SLOT(updateEnableState()));
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
  vtkSMReaderFactory* readerFactory =
    vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
  QString filters = readerFactory->GetSupportedFileTypes(
    server->session());
  if (!filters.isEmpty())
    {
    filters += ";;";
    }
  filters += "All files (*)";
  pqFileDialog fileDialog(server,
    pqCoreUtilities::mainWidget(),
    tr("Open File:"), QString(), filters);
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFiles);
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
pqPipelineSource* pqLoadDataReaction::loadData(const QStringList& files)
{
  QList<QStringList> f;
  f.append(files);
  return pqLoadDataReaction::loadData(f);
}
//-----------------------------------------------------------------------------
pqPipelineSource* pqLoadDataReaction::loadData(const QList<QStringList>& files)
{
  if (files.empty())
    {
    return NULL;
    }

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
    {
    qCritical() << "Cannot create reader without an active server.";
    return NULL;
    }

  vtkSMReaderFactory* readerFactory =
    vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
  pqPipelineSource* reader = NULL;

  //Extension to ReaderType,ReaderGroup Hash table
  QHash<QString,QPair<QString,QString> > extensionToReaderSelection;
  QStringList file;
  foreach(file,files)
    {
    QPair<QString,QString> readerInfo; //type,group    
    QString filename(file[0]);
    QFileInfo fi(filename);

    if (!pqLoadDataReaction::TestFileReadability(filename,server,readerFactory))
      {
      qWarning() << "File '" << filename << "' cannot be read.";
      continue;
      }
    // Determine reader type based on if we have asked the user for this extension before
    QHash<QString,QPair<QString,QString> >::const_iterator it = 
      extensionToReaderSelection.find(fi.suffix());
    if (it != extensionToReaderSelection.end())
      {
      readerInfo = it.value();
      }
    else
      {
      //Determine reader type based on the first file
      bool valid = pqLoadDataReaction::DetermineFileReader(filename,server,readerFactory,readerInfo);
      if ( valid == 0)
        {
        //no reader selected
        continue;
        }
      }
    //read the file
    BEGIN_UNDO_SET("Create 'Reader'");
    reader = pqLoadDataReaction::LoadFile(file,server,readerInfo);
    END_UNDO_SET();

    //add this extension to the hash set    
    extensionToReaderSelection.insert(fi.suffix(),readerInfo);    
    }
  return reader;
}

//-----------------------------------------------------------------------------
bool pqLoadDataReaction::TestFileReadability(
  const QString& file,
  pqServer *server,
  vtkSMReaderFactory *factory)
{
  return factory->TestFileReadability(file.toAscii().data(), server->session());
}

//-----------------------------------------------------------------------------
bool pqLoadDataReaction::DetermineFileReader(
  const QString& filename, 
  pqServer *server,
  vtkSMReaderFactory *factory,
  QPair<QString,QString>& readerInfo)
{
  QString readerType,readerGroup;
  vtkStringList* list = factory->GetReaders(filename.toAscii().data(),
    server->session());
  if(list->GetLength() > 3)
    {
    // If more than one readers found.
    pqSelectReaderDialog prompt(filename, server,
      list, pqCoreUtilities::mainWidget());
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
  else if (factory->CanReadFile(filename.toAscii().data(),
    server->session()))
    {
    //reader knows the type
    readerType = factory->GetReaderName();
    readerGroup = factory->GetReaderGroup();
    }
  else
    {
    // The reader factory could not determine the type of reader to create for the
    // file. Ask the user.
    pqSelectReaderDialog prompt(filename, server,
      factory, pqCoreUtilities::mainWidget());
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
  const QStringList& files,
  pqServer *server,
  const QPair<QString,QString>& readerInfo)
{  
  pqObjectBuilder* builder =
    pqApplicationCore::instance()->getObjectBuilder();
  pqPipelineSource* reader = builder->createReader(readerInfo.second,
    readerInfo.first, files, server);

  if (reader)
    {
    pqApplicationCore* core = pqApplicationCore::instance();

    // Add this to the list of recent server resources ...
    pqServerResource resource = server->getResource();
    resource.setPath(files[0]);
    resource.addData("readergroup", reader->getProxy()->GetXMLGroup());
    resource.addData("reader", reader->getProxy()->GetXMLName());
    resource.addData("extrafilesCount", QString("%1").arg(files.size()-1));
    for (int cc=1; cc < files.size(); cc++)
      {
      resource.addData(QString("file.%1").arg(cc-1), files[cc]);
      }
    core->serverResources().add(resource);
    core->serverResources().save(*core->settings());
    }
  
  return reader;
}
