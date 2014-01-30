/*=========================================================================

   Program: ParaView
   Module:    pqLoadStateReaction.cxx

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
#include "pqLoadStateReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqRecentlyUsedResourcesList.h"
#include "pqServer.h"
#include "pqServerResource.h"
#include "vtkPVXMLParser.h"

#include <QFileInfo>

//-----------------------------------------------------------------------------
pqLoadStateReaction::pqLoadStateReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(serverChanged(pqServer*)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqLoadStateReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->parentAction()->setEnabled(activeObjects->activeServer() != NULL);
}

//-----------------------------------------------------------------------------
void pqLoadStateReaction::loadState(const QString& filename)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  pqServer *server = activeObjects->activeServer();

  // Read in the xml file to restore.
  vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
  xmlParser->SetFileName(filename.toLatin1().data());
  xmlParser->Parse();

  // Get the root element from the parser.
  vtkPVXMLElement *root = xmlParser->GetRootElement();
  if (root)
    {
    pqApplicationCore::instance()->loadState(root, server);

    // Add this to the list of recent server resources ...
    pqServerResource resource;
    resource.setScheme("session");
    resource.setPath(filename);
    resource.setSessionServer(server->getResource());
    pqApplicationCore::instance()->recentlyUsedResources().add(resource);
    pqApplicationCore::instance()->recentlyUsedResources().save(
      *pqApplicationCore::instance()->settings());
    }
  else
    {
    qCritical("Root does not exist. Either state file could not be opened "
      "or it does not contain valid xml");
    }
  xmlParser->Delete();
}

//-----------------------------------------------------------------------------
void pqLoadStateReaction::loadState()
{
  pqFileDialog fileDialog(NULL,
    pqCoreUtilities::mainWidget(),
    tr("Load State File"), QString(),
    "ParaView state file (*.pvsm);;All files (*)");
  fileDialog.setObjectName("FileLoadServerStateDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
    {
    QString selectedFile = fileDialog.getSelectedFiles()[0];
    pqLoadStateReaction::loadState(selectedFile);
    }
}
