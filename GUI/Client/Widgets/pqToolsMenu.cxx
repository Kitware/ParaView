/*=========================================================================

   Program: ParaView
   Module:    pqToolsMenu.cxx

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

========================================================================*/

/// \file pqToolsMenu.cxx
/// \date 6/23/2006

#include "pqToolsMenu.h"

#include "pqApplicationCore.h"
#include "pqCustomFilterDefinitionModel.h"
#include "pqCustomFilterDefinitionWizard.h"
#include "pqCustomFilterManager.h"
#include "pqCustomFilterManagerModel.h"
#include "pqEventTranslator.h"
#include "pqEventPlayer.h"
#include "pqEventPlayerXML.h"
#include "pqFileDialog.h"
#include "pqLocalFileDialogModel.h"
#include "pqObjectNaming.h"
#include "pqRecordEventsDialog.h"
#include "pqRenderModule.h"
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"
#include "pqTestUtility.h"

#ifdef PARAVIEW_EMBED_PYTHON
#include "pqPythonDialog.h"
#endif // PARAVIEW_EMBED_PYTHON

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QtDebug>
#include <QWidget>

#include "QVTKWidget.h"


pqToolsMenu::pqToolsMenu(QObject *parentObject)
  : QObject(parentObject)
{
  this->CustomFilters = new pqCustomFilterManagerModel(this);
  this->Manager = 0;
  this->MenuList = new QAction *[pqToolsMenu::LastAction + 1];

  // Listen for custom filter registration events.
  pqServerManagerObserver *observer =
      pqApplicationCore::instance()->getPipelineData();
  QObject::connect(observer,
      SIGNAL(compoundProxyDefinitionRegistered(QString)),
      this->CustomFilters, SLOT(addCustomFilter(QString)));
  QObject::connect(observer,
      SIGNAL(compoundProxyDefinitionUnRegistered(QString)),
      this->CustomFilters, SLOT(removeCustomFilter(QString)));

  // Initialize the menu actions.
  QAction *action = new QAction(tr("&Create Custom Filter..."), this);
  action->setObjectName("CreateCustomFilter");
  QObject::connect(action, SIGNAL(triggered()),
      this, SLOT(openCustomFilterWizard()));
  this->MenuList[pqToolsMenu::CreateCustomFilter] = action;
  action = new QAction(tr("&Manage Custom Filters..."), this);
  action->setObjectName("ManageCustomFilters");
  QObject::connect(action, SIGNAL(triggered()),
      this, SLOT(openCustomFilterManager()));
  this->MenuList[pqToolsMenu::ManageCustomFilters] = action;
  action = new QAction(tr("&Link Editor..."), this);
  action->setObjectName("LinkEditor");
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(openLinkEditor()));
  action->setEnabled(false); // TEMP
  this->MenuList[pqToolsMenu::LinkEditor] = action;

  action = new QAction(tr("&Dump Widget Names"), this);
  action->setObjectName("DumpWidgets");
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(dumpWidgetNames()));
  this->MenuList[pqToolsMenu::DumpNames] = action;
  action = new QAction(tr("&Record Test"), this);
  action->setObjectName("Record");
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(recordTest()));
  this->MenuList[pqToolsMenu::RecordTest] = action;
  action = new QAction(tr("Record &Test Screenshot"), this);
  action->setObjectName("RecordTestScreenshot");
  QObject::connect(action, SIGNAL(triggered()),
      this, SLOT(recordTestScreenshot()));
  this->MenuList[pqToolsMenu::TestScreenshot] = action;
  action = new QAction(tr("&Play Test"), this);
  action->setObjectName("Play");
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(playTest()));
  this->MenuList[pqToolsMenu::PlayTest] = action;

  action = new QAction(tr("Python &Shell"), this);
  action->setObjectName("PythonShell");
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(openPythonShell()));
  this->MenuList[pqToolsMenu::PythonShell] = action;

  action = new QAction(tr("&Options"), this);
  action->setObjectName("Options");
  QObject::connect(action, SIGNAL(triggered()),
      this, SLOT(openOptionsDialog()));
  this->MenuList[pqToolsMenu::Options] = action;
}

pqToolsMenu::~pqToolsMenu()
{
  if(this->Manager)
    {
    delete this->Manager;
    }

  // The actions on the list will get cleaned up by Qt.
  delete [] this->MenuList;
  delete this->CustomFilters;
}

void pqToolsMenu::addActionsToMenuBar(QMenuBar *menubar) const
{
  if(menubar)
    {
    QMenu *menu = menubar->addMenu(tr("&Tools"));
    menu->setObjectName("ToolsMenu");
    this->addActionsToMenu(menu);
    }
}

void pqToolsMenu::addActionsToMenu(QMenu *menu) const
{
  if(!menu)
    {
    return;
    }

  menu->addAction(this->MenuList[pqToolsMenu::CreateCustomFilter]);
  menu->addAction(this->MenuList[pqToolsMenu::ManageCustomFilters]);
  menu->addAction(this->MenuList[pqToolsMenu::LinkEditor]);

#ifdef PARAVIEW_EMBED_PYTHON
  menu->addAction(this->MenuList[pqToolsMenu::PythonShell]);
#endif // PARAVIEW_EMBED_PYTHON

  menu->addSeparator();
  menu->addAction(this->MenuList[pqToolsMenu::DumpNames]);
  menu->addAction(this->MenuList[pqToolsMenu::RecordTest]);
  menu->addAction(this->MenuList[pqToolsMenu::TestScreenshot]);
  menu->addAction(this->MenuList[pqToolsMenu::PlayTest]);

  //menu->addSeparator();
  //menu->addAction(this->MenuList[pqToolsMenu::Options]);
}

QAction *pqToolsMenu::getMenuAction(pqToolsMenu::ActionName name) const
{
  if(name != pqToolsMenu::InvalidAction)
    {
    return this->MenuList[name];
    }

  return 0;
}

void pqToolsMenu::openCustomFilterWizard()
{
  // Get the selected sources from the application core. Notify the user
  // if the selection is empty.
  QWidget *activeWindow = QApplication::activeWindow();
  const pqServerManagerSelection *selections =
      pqApplicationCore::instance()->getSelectionModel()->selectedItems();
  if(selections->size() == 0)
    {
    QMessageBox::warning(activeWindow, "Create Custom Filter Error",
        "No pipeline objects are selected.\n"
        "To create a new custom filter, select the sources and "
        "filters you want.\nThen, launch the creation wizard.",
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
    }

  // Create a custom filter definition model with the pipeline
  // selection. The model only accepts pipeline sources. Notify the
  // user if the model is empty.
  pqCustomFilterDefinitionModel custom(this);
  custom.setContents(selections);
  if(!custom.hasChildren(QModelIndex()))
    {
    QMessageBox::warning(activeWindow, "Create Custom Filter Error",
        "The selected objects cannot be used to make a custom filter.\n"
        "To create a new custom filter, select the sources and "
        "filters you want.\nThen, launch the creation wizard.",
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
    }

  pqCustomFilterDefinitionWizard wizard(&custom, activeWindow);
  wizard.setCustomFilterList(this->CustomFilters);
  if(wizard.exec() == QDialog::Accepted)
    {
    // Create a new compound proxy from the custom filter definition.
    wizard.createCustomFilter();
    QString customName = wizard.getCustomFilterName();

    // Launch the custom filter manager in case the user wants to save
    // the compound proxy definition. Select the new custom filter for
    // the user.
    this->openCustomFilterManager();
    this->Manager->selectCustomFilter(customName);
    }
}

void pqToolsMenu::openCustomFilterManager()
{
  if(!this->Manager)
    {
    this->Manager = new pqCustomFilterManager(this->CustomFilters,
        QApplication::activeWindow());
    }

  this->Manager->show();
}

void pqToolsMenu::openLinkEditor()
{
  // TODO
}

void pqToolsMenu::dumpWidgetNames()
{
  const QString output = pqObjectNaming::DumpHierarchy();
  qDebug() << output;
}

void pqToolsMenu::recordTest()
{
  QString filters;
  filters += "XML Files (*.xml)";
  filters += ";;All Files (*)";
  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      QApplication::activeWindow(), tr("Record Test"), QString(), filters);
  fileDialog->setObjectName("ToolsRecordTestDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), 
      this, SLOT(recordTest(const QStringList &)));
  fileDialog->show();
}

void pqToolsMenu::recordTest(const QStringList &fileNames)
{
  QStringList::ConstIterator iter = fileNames.begin();
  for( ; iter != fileNames.end(); ++iter)
    {
    pqEventTranslator* const translator = new pqEventTranslator();
    pqTestUtility::Setup(*translator);

    pqRecordEventsDialog* const dialog = new pqRecordEventsDialog(
        translator, *iter, QApplication::activeWindow());
    dialog->show();
    }
}

void pqToolsMenu::recordTestScreenshot()
{
  if(!pqApplicationCore::instance()->getActiveRenderModule())
    {
    qDebug() << "Cannnot save image. No active render module.";
    return;
    }

  QString filters;
  filters += "PNG Image (*.png)";
  filters += ";;BMP Image (*.bmp)";
  filters += ";;TIFF Image (*.tif)";
  filters += ";;PPM Image (*.ppm)";
  filters += ";;JPG Image (*.jpg)";
  filters += ";;All Files (*)";
  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      QApplication::activeWindow(), tr("Save Test Screenshot"), QString(),
      filters);
  fileDialog->setObjectName("RecordTestScreenshotDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), 
      this, SLOT(recordTestScreenshot(const QStringList &)));
  fileDialog->show();
}

void pqToolsMenu::recordTestScreenshot(const QStringList &fileNames)
{
  pqRenderModule* render_module =
      pqApplicationCore::instance()->getActiveRenderModule();
  if(!render_module)
    {
    qDebug() << "Cannnot save image. No active render module.";
    return;
    }

  QSize old_size = render_module->getWidget()->size();
  render_module->getWidget()->resize(300,300);

  QStringList::ConstIterator iter = fileNames.begin();
  for( ; iter != fileNames.end(); ++iter)
    {
    if(!pqTestUtility::SaveScreenshot(
        render_module->getWidget()->GetRenderWindow(), *iter))
      {
      qCritical() << "Save Image failed.";
      }
    }

  render_module->getWidget()->resize(old_size);
  render_module->render();
}

void pqToolsMenu::playTest()
{
  QString filters;
  filters += "XML Files (*.xml)";
  filters += ";;All Files (*)";
  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      QApplication::activeWindow(), tr("Play Test"), QString(), filters);
  fileDialog->setObjectName("ToolsPlayTestDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList&)), 
      this, SLOT(playTest(const QStringList&)));
  fileDialog->show();
}

void pqToolsMenu::playTest(const QStringList &fileNames)
{
  QApplication::processEvents();

  pqEventPlayer player;
  pqTestUtility::Setup(player);

  QStringList::ConstIterator iter = fileNames.begin();
  for( ; iter != fileNames.end(); ++iter)
    {
    pqEventPlayerXML xml_player;
    xml_player.playXML(player, *iter);
    }
}

void pqToolsMenu::openPythonShell()
{
#ifdef PARAVIEW_EMBED_PYTHON
  pqPythonDialog* const dialog = new pqPythonDialog(this);
  dialog->show();
#endif // PARAVIEW_EMBED_PYTHON
}

void pqToolsMenu::openOptionsDialog()
{
  // TODO
}


