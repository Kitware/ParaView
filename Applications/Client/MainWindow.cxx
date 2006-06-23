/*=========================================================================

   Program: ParaView
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "AboutDialog.h"
#include "MainWindow.h"

#include <pqConnect.h>
#include <pqSetName.h>

#include <QMenu>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QAssistantClient>
#include <QMessageBox>

#include <vtkPQConfig.h>
#include <vtkSMProxyManager.h>
#include <pqSetData.h>
#include <pqSetName.h>


MainWindow::MainWindow()
  : HelpClient(NULL)
{
  this->setWindowTitle(QByteArray("ParaView") + 
                       QByteArray(" ") + 
                       QByteArray(PARAVIEW_VERSION_FULL) + 
                       QByteArray(" (alpha)"));
  this->setWindowIcon(QIcon(":pqWidgets/pqAppIcon64.png"));

  this->createStandardStatusBar();
  this->createStandardFileMenu();
  this->createStandardEditMenu();
  this->createStandardViewMenu();
  this->createStandardServerMenu();
  this->createStandardSourcesMenu();
  this->createStandardFiltersMenu();
  this->createStandardPipelineMenu();
  this->createStandardToolsMenu();
  this->createStandardHelpMenu();
  
  QMenu* const help_menu = this->helpMenu();
  help_menu->addAction(tr("&ParaView Help"))
    << pqSetName("Help")
    << pqConnect(SIGNAL(triggered()), this, SLOT(showHelp()));
  help_menu->addAction(tr("&About ParaView"))
    << pqSetName("About")
    << pqConnect(SIGNAL(triggered()), this, SLOT(showHelpAbout()));
  
  this->createStandardPipelineBrowser();
  this->createStandardObjectInspector();
  this->createStandardElementInspector(false);
  this->createStandardDataInformationWidget(false);
  
  this->createStandardVCRToolBar();
  this->createUndoRedoToolBar();
  this->createSelectionToolBar();
  this->createStandardVariableToolBar();
  this->createStandardCompoundProxyToolBar();
}

void MainWindow::showHelpAbout()
{
  AboutDialog* const dialog = new AboutDialog(this);
  dialog->show();
}


/*
//-----------------------------------------------------------------------------
void MainWindow::buildFiltersMenu()
{

QStringList allowedFilters;
allowedFilters<<"Clip";
allowedFilters<<"Cut";
allowedFilters<<"Threshold";

this->filtersMenu()->clear();
QMenu *alphabetical =this->filtersMenu();

vtkSMProxyManager* manager = vtkSMObject::GetProxyManager();
manager->InstantiateGroupPrototypes("filters");
int numFilters = manager->GetNumberOfProxies("filters_prototypes");
for(int i=0; i<numFilters; i++)
{
  QStringList categoryList;
  QString proxyName = manager->GetProxyName("filters_prototypes",i);

  if(allowedFilters.contains(proxyName))
    {
    QAction* action = alphabetical->addAction(proxyName) << pqSetName(proxyName)
                                                         << pqSetData(proxyName);
    action->setEnabled(false);
    }
}
}

//-----------------------------------------------------------------------------
void MainWindow::buildSourcesMenu()
{
  this->sourcesMenu()->clear();
  this->sourcesMenu()->addAction("Cone") 
    << pqSetName("Cone") << pqSetData("ConeSource");
}
*/


void MainWindow::showHelp()
{
  if(!this->HelpClient)
    {
    // find the assistant
#if defined(Q_WS_WIN)
    const char* assistantName = "assistant.exe";
#elif defined(Q_WS_MAC)
    const char* assistantName = "assistant.app/Contents/MacOS/assistant";
#else
    const char* assistantName = "assistant";
#endif
    
    // first look if assistant is bundled with application
    QString assistant = QCoreApplication::applicationDirPath();
    assistant += QDir::separator();
    assistant += assistantName;
    if(QFile::exists(assistant))
      {
      this->HelpClient = new QAssistantClient(assistant, this);
      }
    else
      {
      // not bundled, see if it can can be found in PATH
      this->HelpClient = new QAssistantClient(QString(), this);
      }

    QStringList args;
    args.append(QString("-profile"));

    // see if help is bundled up with the application
    QString profile = QCoreApplication::applicationDirPath() + QDir::separator()
      + QString("pqClient.adp");

    if(QFile::exists(profile))
      {
      args.append(profile);
      }
    else if(getenv("PARAVIEW_HELP"))
      {
      // not bundled, ask for help
      args.append(getenv("PARAVIEW_HELP"));
      }
    else
      {
      // no help, error out
      QMessageBox::critical(this, "Help error", "Couldn't find"
                            " pqClient.adp.\nTry setting the PARAVIEW_HELP environment variable which"
                            " points to that file");
      
      delete this->HelpClient;
      this->HelpClient = NULL;
      return;
      }
    
    this->HelpClient->setArguments(args);
    this->HelpClient->openAssistant();
    }
  else if(this->HelpClient && !this->HelpClient->isOpen())
    {
    this->HelpClient->openAssistant();
    }

}

void MainWindow::helpClosed()
{
  delete this->HelpClient;
  this->HelpClient = NULL;
}


