/*=========================================================================

   Program: ParaView
   Module:    pqRecentFilesMenu.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqRecentFilesMenu.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialogModel.h"
#include "pqObjectBuilder.h"
#include "pqRecentlyUsedResourcesList.h"
#include "pqServerConfiguration.h"
#include "pqServerConnectDialog.h"
#include "pqServerLauncher.h"
#include "pqServerManagerModel.h"
#include "pqServerResource.h"
#include "pqTimer.h"
#include "pqUndoStack.h"

#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"

#include <QMenu>
#include <QtDebug>
#include <QMessageBox>

#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// pqRecentFilesMenu::pqImplementation

class pqRecentFilesMenu::pqImplementation
{
public:
  pqImplementation(QMenu& menu) :
    Menu(menu)
  {
  }
  
  ~pqImplementation()
  {
  }

  QMenu& Menu;
  pqServerResource RecentResource;
  
  /// Functor that returns true if two resources have the same URI scheme and host(s)
  class SameSchemeAndHost
  {
  public:
    SameSchemeAndHost(const pqServerResource& lhs) :
      LHS(lhs)
    {
    }
    
    bool operator()(const pqServerResource& rhs) const
    {
      return this->LHS.schemeHosts() == rhs.schemeHosts();
    }
    
  private:
    void operator=(const SameSchemeAndHost&);
    const pqServerResource& LHS;
  };
private:
  void operator=(const pqImplementation&);
};

/////////////////////////////////////////////////////////////////////////////
// pqRecentFilesMenu

pqRecentFilesMenu::pqRecentFilesMenu(QMenu& menu, QObject* p) :
  QObject(p), Implementation(new pqImplementation(menu))
{
  connect(
    &pqApplicationCore::instance()->recentlyUsedResources(),
    SIGNAL(changed()),
    this,
    SLOT(onResourcesChanged()));
  
  connect(
    &this->Implementation->Menu,
    SIGNAL(triggered(QAction*)),
    this,
    SLOT(onOpenResource(QAction*)));
    
  this->onResourcesChanged();
}

//-----------------------------------------------------------------------------
pqRecentFilesMenu::~pqRecentFilesMenu()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqRecentFilesMenu::onResourcesChanged()
{
  this->Implementation->Menu.clear();
  
  // Get the set of all resources in most-recently-used order ...
  const pqRecentlyUsedResourcesList::ListT &resources = 
    pqApplicationCore::instance()->recentlyUsedResources().list();

  // Get the set of servers with unique scheme/host in most-recently-used order ...
  pqRecentlyUsedResourcesList::ListT servers;
  for(int i = 0; i != resources.size(); ++i)
    {
    pqServerResource resource = resources[i];
    pqServerResource server = resource.scheme() == "session" ?
      resource.sessionServer().schemeHostsPorts() :
      resource.schemeHostsPorts();
      
    // If this host isn't already in the list, add it ...
    if(!std::count_if(servers.begin(), servers.end(), pqImplementation::SameSchemeAndHost(server)))
      {
      servers.push_back(server);
      }
    }

  // Display the servers ...
  for(int i = 0; i != servers.size(); ++i)
    {
    const pqServerResource& server = servers[i];
    
    const QString label = server.schemeHosts().toURI();
    
    QAction* const action = new QAction(label, &this->Implementation->Menu);
    action->setData(server.serializeString());
    
    action->setIcon(QIcon(":/pqWidgets/Icons/pqConnect16.png"));
    
    QFont font = action->font();
    font.setBold(true);
    action->setFont(font);
    
    this->Implementation->Menu.addAction(action);
    
    // Display sessions associated with the server first ...
    for(int j = 0; j != resources.size(); ++j)
      {
      const pqServerResource& resource = resources[j];

      if(
        resource.scheme() != "session"
        || resource.path().isEmpty()
        || resource.sessionServer().schemeHosts() != server.schemeHosts())
        {
        continue;
        }
        
      QAction* const act = new QAction(resource.path(), &this->Implementation->Menu);
      act->setData(resource.serializeString());
      act->setIcon(QIcon(":/pqWidgets/Icons/pvIcon32.png"));
      
      this->Implementation->Menu.addAction(act);
      }
    
    // Display files associated with the server next ...
    for(int j = 0; j != resources.size(); ++j)
      {
      const pqServerResource& resource = resources[j];

      if(
        resource.scheme() == "session"
        || resource.path().isEmpty()
        || resource.schemeHosts() != server.schemeHosts())
        {
        continue;
        }
        
      QAction* const act = new QAction(resource.path(), &this->Implementation->Menu);
      act->setData(resource.serializeString());

      this->Implementation->Menu.addAction(act);
      }
    }
}

//-----------------------------------------------------------------------------
void pqRecentFilesMenu::onOpenResource(QAction* action)
{
  // Note: we can't update the resources here because it would destroy the
  // action that's calling this slot.  So, schedule an update for the
  // next time the UI is idle.
  this->Implementation->RecentResource =
    pqServerResource(action->data().toString());
  pqTimer::singleShot(0, this, SLOT(onOpenResource()));
}

//-----------------------------------------------------------------------------
void pqRecentFilesMenu::onOpenResource()
{
  const pqServerResource resource = this->Implementation->RecentResource;
  
  const pqServerResource server =
    resource.scheme() == "session"
      ? resource.sessionServer().schemeHostsPorts()
      : resource.schemeHostsPorts();

  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqServer *pq_server = smModel->findServer(server);
  if (!pq_server)
    {
    int ret = QMessageBox::warning(
      pqCoreUtilities::mainWidget(),
      tr("Disconnect from current server?"),
      tr("The file you opened requires connecting to a new server. \n"
        "The current connection will be closed.\n\n"
        "Are you sure you want to continue?"),
      QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
      {
      return;
      }
    pqServerConfiguration config_to_connect;
    if (pqServerConnectDialog::selectServer(config_to_connect,
        pqCoreUtilities::mainWidget(), server))
      {
      pqServerLauncher launcher(config_to_connect);
      if (launcher.connectToServer())
        {
        pq_server = launcher.connectedServer();
        }
      }
    }
  if (pq_server)
    {
    this->onServerStarted(pq_server);
    }
}

//-----------------------------------------------------------------------------
void pqRecentFilesMenu::onServerStarted(pqServer* server)
{
  if (this->open(server, this->Implementation->RecentResource))
    {
    pqRecentlyUsedResourcesList& mruList =
      pqApplicationCore::instance()->recentlyUsedResources();
    mruList.add(this->Implementation->RecentResource);
    mruList.save(*pqApplicationCore::instance()->settings());
    }
}

//-----------------------------------------------------------------------------
bool pqRecentFilesMenu::open(
  pqServer* server, const pqServerResource& resource) const
{
  if(!server)
    {
    qCritical() << "Cannot open a resource with NULL server";
    return false;
    }
  pqFileDialogModel fileModel(server, NULL);
  if(resource.scheme() == "session")
    {
    if(!resource.path().isEmpty())
      {
      // make sure the file exists
      QString sessionFile = resource.path();
      if (sessionFile.isEmpty() || !fileModel.fileExists(sessionFile, sessionFile))
        {
        qCritical() << "File does not exist: " << sessionFile << "\n";
        return false;
        }

      // Read in the xml file to restore.
      vtkSmartPointer<vtkPVXMLParser> xmlParser = vtkSmartPointer<vtkPVXMLParser>::New();
      xmlParser->SetFileName(sessionFile.toLatin1().data());
      xmlParser->Parse();

      // Get the root element from the parser.
      if(vtkPVXMLElement* const root = xmlParser->GetRootElement())
        {
        pqApplicationCore::instance()->loadState(root, server);
        return true;
        }
      else
        {
        qCritical() << "Root does not exist. Either state file could not be opened "
                  "or it does not contain valid xml";
        }
      }
    }
  else if (!resource.path().isEmpty())
    {
    QString readerGroup = resource.data("readergroup");
    QString readerName = resource.data("reader");
    if (readerName.isEmpty() || readerGroup.isEmpty())
      {
      qDebug() << "Recent changes to the settings code have "
        << "made these old entries unusable.";
      return false;
      }

    QStringList files;
    // make sure the file exists
    QString filename = resource.path();
    if (filename.isEmpty() || !fileModel.fileExists(filename, filename))
      {
      qCritical() << "File does not exist: " << filename << "\n";
      return false;
      }
    files.push_back(filename);
    QString extrafilesCount = resource.data("extrafilesCount");
    if (!extrafilesCount.isEmpty() && extrafilesCount.toInt() > 0)
      {
      for (int cc=0; cc < extrafilesCount.toInt(); cc++)
        {
        QString extrafile = resource.data(QString("file.%1").arg(cc));
        if (!extrafile.isEmpty() &&
          fileModel.fileExists(extrafile, extrafile))
          {
          files.push_back(extrafile);
          }
        }
      }
    if (this->createReader(readerGroup, readerName, files, server) != NULL)
      {
      return true;
      }
    qCritical() << "Error opening file " << resource.path() << "\n";
    }

  return false;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqRecentFilesMenu::createReader(
  const QString& readerGroup,
  const QString& readerName,
  const QStringList& files,
  pqServer* server) const
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  BEGIN_UNDO_SET("Create Reader");
  pqPipelineSource* reader = builder->createReader(
    readerGroup, readerName, files, server);
  END_UNDO_SET();
  return reader;
}
