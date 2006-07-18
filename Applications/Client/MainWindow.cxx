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

#include <vtkPQConfig.h>

#include "AboutDialog.h"
#include "MainWindow.h"

#include "ui_MainWindow.h"

#include <pqApplicationCore.h>
#include <pqMainWindowCore.h>
#include <pqMultiView.h>
#include <pqObjectInspectorWidget.h>
#include <pqPipelineMenu.h>
#include <pqUndoStack.h>
#include <pqPipelineBrowser.h>
#include <pqSelectionManager.h>
#include <pqSetName.h>
#include <pqVCRController.h>
#include <pqViewMenu.h>

#include <QAssistantClient>
#include <QDir>
#include <QMessageBox>

//////////////////////////////////////////////////////////////////////////////
// MainWindow::pqImplementation

class MainWindow::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    AssistantClient(0),
    Core(parent),
    ViewMenu(0),
    ToolbarsMenu(0)
  {
  }
  
  ~pqImplementation()
  {
    delete this->ViewMenu;
  }

  QAssistantClient* AssistantClient;
  Ui::MainWindow UI;
  pqMainWindowCore Core;
  pqViewMenu* ViewMenu;
  pqViewMenu* ToolbarsMenu;
};

//////////////////////////////////////////////////////////////////////////////
// MainWindow

MainWindow::MainWindow() :
  Implementation(new pqImplementation(this))
{
  this->Implementation->UI.setupUi(this);
  this->Implementation->ViewMenu = new pqViewMenu(*this->Implementation->UI.menuView);
  this->Implementation->ToolbarsMenu = new pqViewMenu(*this->Implementation->UI.menuToolbars);

  this->setWindowTitle(
    QString("ParaView %1 (alpha)").arg(PARAVIEW_VERSION_FULL));

  // Setup menus and toolbars ...
  connect(this->Implementation->UI.actionFileNew,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileNew()));

  connect(this->Implementation->UI.actionFileOpen,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileOpen()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileOpen(bool)),
    this->Implementation->UI.actionFileOpen,
    SLOT(setEnabled(bool)));

  this->Implementation->Core.setRecentFilesMenu(
    this->Implementation->UI.menuRecentFiles);

  connect(this->Implementation->UI.actionFileLoadServerState,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileLoadServerState()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileLoadServerState(bool)),
    this->Implementation->UI.actionFileLoadServerState,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveServerState,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileSaveServerState()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileSaveServerState(bool)),
    this->Implementation->UI.actionFileSaveServerState,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveData,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileSaveData()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileSaveData(bool)),
    this->Implementation->UI.actionFileSaveData,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveScreenshot,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileSaveScreenshot()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionFileSaveScreenshot,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveAnimation,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileSaveAnimation()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileSaveAnimation(bool)),
    this->Implementation->UI.actionFileSaveAnimation,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileExit,
    SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));  

  connect(this->Implementation->UI.actionEditUndo,
    SIGNAL(triggered()), pqApplicationCore::instance()->getUndoStack(), SLOT(Undo()));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(CanUndoChanged(bool)), this->Implementation->UI.actionEditUndo, SLOT(setEnabled(bool)));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(UndoLabelChanged(const QString&)), this, SLOT(onUndoLabel(const QString&)));
    
  connect(this->Implementation->UI.actionEditRedo,
    SIGNAL(triggered()), pqApplicationCore::instance()->getUndoStack(), SLOT(Redo()));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(CanRedoChanged(bool)), this->Implementation->UI.actionEditRedo, SLOT(setEnabled(bool)));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(RedoLabelChanged(const QString&)), this, SLOT(onRedoLabel(const QString&)));

  connect(this->Implementation->UI.actionEditCameraUndo,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onEditCameraUndo()));
  connect(&this->Implementation->Core,
    SIGNAL(enableCameraUndo(bool)),
    this->Implementation->UI.actionEditCameraUndo,
    SLOT(setEnabled(bool)));
  connect(&this->Implementation->Core,
    SIGNAL(cameraUndoLabel(const QString&)),
    this,
    SLOT(onCameraUndoLabel(const QString&)));
    
  connect(this->Implementation->UI.actionEditCameraRedo,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onEditCameraRedo()));
  connect(&this->Implementation->Core,
    SIGNAL(enableCameraRedo(bool)),
    this->Implementation->UI.actionEditCameraRedo,
    SLOT(setEnabled(bool)));
  connect(&this->Implementation->Core,
    SIGNAL(cameraRedoLabel(const QString&)),
    this,
    SLOT(onCameraRedoLabel(const QString&)));

  connect(this->Implementation->UI.actionServerConnect,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onServerConnect()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableServerConnect(bool)),
    this->Implementation->UI.actionServerConnect,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionServerDisconnect,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onServerDisconnect()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableServerDisconnect(bool)),
    this->Implementation->UI.actionServerDisconnect,
    SLOT(setEnabled(bool)));

  this->Implementation->Core.setSourceMenu(
    this->Implementation->UI.menuSources);
  connect(
    &this->Implementation->Core,
    SIGNAL(enableSourceCreate(bool)),
    this->Implementation->UI.menuSources,
    SLOT(setEnabled(bool)));

  this->Implementation->Core.setFilterMenu(
    this->Implementation->UI.menuFilters);
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFilterCreate(bool)),
    this->Implementation->UI.menuFilters,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionToolsCreateCustomFilter,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsCreateCustomFilter()));

  connect(this->Implementation->UI.actionToolsManageCustomFilters,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsManageCustomFilters()));

  connect(this->Implementation->UI.actionToolsDumpWidgetNames,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsDumpWidgetNames()));

  connect(this->Implementation->UI.actionToolsRecordTest,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsRecordTest()));

  connect(this->Implementation->UI.actionToolsRecordTestScreenshot,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsRecordTestScreenshot()));

  connect(this->Implementation->UI.actionToolsPlayTest,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsPlayTest()));

  connect(this->Implementation->UI.actionToolsPythonShell,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsPythonShell()));

  this->Implementation->Core.pipelineMenu().addActionsToMenu(
    this->Implementation->UI.menuPipeline);

  connect(this->Implementation->UI.actionHelpAbout,
    SIGNAL(triggered()), this, SLOT(onHelpAbout()));

  connect(this->Implementation->UI.actionHelpHelp,
    SIGNAL(triggered()), this, SLOT(onHelpHelp()));

  connect(this->Implementation->UI.actionHelpEnableTooltips,
    SIGNAL(triggered(bool)), &this->Implementation->Core, SLOT(onHelpEnableTooltips(bool)));
  this->Implementation->Core.onHelpEnableTooltips(
    this->Implementation->UI.actionHelpEnableTooltips->isChecked());

  connect(this->Implementation->UI.actionVCRFirstFrame,
    SIGNAL(triggered()), &this->Implementation->Core.VCRController(), SLOT(onFirstFrame()));
    
  connect(this->Implementation->UI.actionVCRPreviousFrame,
    SIGNAL(triggered()), &this->Implementation->Core.VCRController(), SLOT(onPreviousFrame()));
    
  connect(this->Implementation->UI.actionVCRNextFrame,
    SIGNAL(triggered()), &this->Implementation->Core.VCRController(), SLOT(onNextFrame()));
    
  connect(this->Implementation->UI.actionVCRLastFrame,
    SIGNAL(triggered()), &this->Implementation->Core.VCRController(), SLOT(onLastFrame()));

  connect(this->Implementation->UI.actionMoveMode, 
    SIGNAL(triggered()), &this->Implementation->Core.selectionManager(), SLOT(switchToInteraction()));
    
  connect(this->Implementation->UI.actionSelectionMode, 
    SIGNAL(triggered()), &this->Implementation->Core.selectionManager(), SLOT(switchToSelection()));

  this->Implementation->Core.setupVariableToolbar(
    this->Implementation->UI.variableToolbar);
  connect(
    &this->Implementation->Core,
    SIGNAL(enableVariableToolbar(bool)),
    this->Implementation->UI.variableToolbar,
    SLOT(setEnabled(bool)));

  connect(
    &this->Implementation->Core,
    SIGNAL(enableSelectionToolbar(bool)),
    this->Implementation->UI.selectionToolbar,
    SLOT(setEnabled(bool)));

  this->Implementation->Core.setupCustomFilterToolbar(
    this->Implementation->UI.customFilterToolbar);

  // Setup dockable windows ...
  this->Implementation->Core.setupPipelineBrowser(
    this->Implementation->UI.pipelineBrowserDock);

  pqObjectInspectorWidget* const object_inspector =
    this->Implementation->Core.setupObjectInspector(
      this->Implementation->UI.objectInspectorDock);
      
  QObject::connect(
    object_inspector,
    SIGNAL(preaccept()), 
    this,
    SLOT(onPreAccept()));

  QObject::connect(
    object_inspector,
    SIGNAL(postaccept()), 
    this,
    SLOT(onPostAccept()));
      
    
  this->Implementation->Core.setupStatisticsView(
    this->Implementation->UI.statisticsViewDock);
    
  this->Implementation->Core.setupElementInspector(
    this->Implementation->UI.elementInspectorDock);
  
  // Setup the view menu ...
  this->Implementation->ToolbarsMenu->addWidget(this->Implementation->UI.mainToolBar,
    this->Implementation->UI.mainToolBar->windowTitle());
  
  this->Implementation->ToolbarsMenu->addWidget(this->Implementation->UI.selectionToolbar,
    this->Implementation->UI.selectionToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(this->Implementation->UI.variableToolbar,
    this->Implementation->UI.variableToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(this->Implementation->UI.customFilterToolbar,
    this->Implementation->UI.customFilterToolbar->windowTitle());
    
  this->Implementation->ToolbarsMenu->addWidget(this->Implementation->UI.undoRedoToolbar,
    this->Implementation->UI.undoRedoToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(this->Implementation->UI.VCRToolbar,
    this->Implementation->UI.VCRToolbar->windowTitle());

  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.pipelineBrowserDock,
    this->Implementation->UI.pipelineBrowserDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.objectInspectorDock,
    this->Implementation->UI.objectInspectorDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.statisticsViewDock,
    this->Implementation->UI.statisticsViewDock->windowTitle());
    
  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.elementInspectorDock,
    this->Implementation->UI.elementInspectorDock->windowTitle());
  
  // Setup the multiview render window ...
  this->setCentralWidget(&this->Implementation->Core.multiViewManager());

  // Setup the statusbar ...
  this->Implementation->Core.setupProgressBar(this->statusBar());

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Setup the default dock configuration ...
  this->Implementation->UI.elementInspectorDock->hide();

  // Now that we're ready, initialize everything ...
  this->Implementation->Core.initializeStates();
  
  this->Implementation->UI.actionEditUndo->setEnabled(
    pqApplicationCore::instance()->getUndoStack()->CanUndo());
  this->Implementation->UI.actionEditRedo->setEnabled(
    pqApplicationCore::instance()->getUndoStack()->CanRedo());
  this->onUndoLabel(pqApplicationCore::instance()->getUndoStack()->UndoLabel());
  this->onRedoLabel(pqApplicationCore::instance()->getUndoStack()->RedoLabel());
}

MainWindow::~MainWindow()
{
  delete this->Implementation;
}

bool MainWindow::compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory)
{
  return this->Implementation->Core.compareView(ReferenceImage, Threshold, Output, TempDirectory);
}

void MainWindow::onUndoLabel(const QString& label)
{
  this->Implementation->UI.actionEditUndo->setText(
    label.isEmpty() ? tr("Can't Undo") : QString(tr("&Undo %1")).arg(label));
}

void MainWindow::onRedoLabel(const QString& label)
{
  this->Implementation->UI.actionEditRedo->setText(
    label.isEmpty() ? tr("Can't Redo") : QString(tr("&Redo %1")).arg(label));
}

void MainWindow::onCameraUndoLabel(const QString& label)
{
  this->Implementation->UI.actionEditCameraUndo->setText(
    label.isEmpty() ? tr("Can't Undo Camera") : QString(tr("U&ndo %1")).arg(label));
}

void MainWindow::onCameraRedoLabel(const QString& label)
{
  this->Implementation->UI.actionEditCameraRedo->setText(
    label.isEmpty() ? tr("Can't Redo Camera") : QString(tr("R&edo %1")).arg(label));
}

void MainWindow::onPreAccept()
{
  this->setEnabled(false);
  this->statusBar()->showMessage(tr("Updating..."));
}

void MainWindow::onPostAccept()
{
  this->setEnabled(true);
  this->statusBar()->showMessage(tr("Ready"), 2000);
}

void MainWindow::onHelpAbout()
{
  AboutDialog* const dialog = new AboutDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
}

void MainWindow::onHelpHelp()
{
  if(this->Implementation->AssistantClient)
    {
    if(this->Implementation->AssistantClient->isOpen())
      {
      return;
      }
    else
      {
      this->Implementation->AssistantClient->openAssistant();
      }
    }

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
    this->Implementation->AssistantClient = new QAssistantClient(assistant, this);
    }
  else
    {
    // not bundled, see if it can can be found in PATH
    this->Implementation->AssistantClient = new QAssistantClient(QString(), this);
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
    
    delete this->Implementation->AssistantClient;
    this->Implementation->AssistantClient = NULL;
    return;
    }
  
  this->Implementation->AssistantClient->setArguments(args);
  this->Implementation->AssistantClient->openAssistant();
}
