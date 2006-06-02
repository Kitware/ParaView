/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineBrowserContextMenu.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqPipelineBrowserContextMenu.h
/// \date 4/20/2006

#include "pqPipelineBrowserContextMenu.h"

#include "pqDisplayProxyEditor.h"
#include "pqFlatTreeView.h"
#include "pqPipelineBrowser.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqServer.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QMenu>

#include "vtkSMProxy.h"
#include "vtkSMDataObjectDisplayProxy.h"


pqPipelineBrowserContextMenu::pqPipelineBrowserContextMenu(
    pqPipelineBrowser *browser)
  : QObject(browser)
{
  this->Browser = browser;

  this->setObjectName("ContextMenu");

  // Listen for the custom context menu signal.
  if(this->Browser)
    {
    QObject::connect(this->Browser->getTreeView(),
        SIGNAL(customContextMenuRequested(const QPoint &)),
        this, SLOT(showContextMenu(const QPoint &)));
    }
}

pqPipelineBrowserContextMenu::~pqPipelineBrowserContextMenu()
{
}

void pqPipelineBrowserContextMenu::showContextMenu(const QPoint &pos)
{
  if(!this->Browser)
    {
    return;
    }

  QMenu menu;
  menu.setObjectName("PipelineObjectMenu");
  bool menuHasItems = false;
  pqFlatTreeView *tree = this->Browser->getTreeView();
  QModelIndex current = tree->selectionModel()->currentIndex();
  pqPipelineModel *model = this->Browser->getListModel();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource*>(
    model->getItem(current));
  pqServer* server = dynamic_cast<pqServer*>(model->getItem(current));
  if(source)
    {
    QAction* action;

    // Add the context menu items for a pipeline object.
    action = menu.addAction("Display Settings...", this, SLOT(showDisplayEditor()));
    action->setObjectName("Display Settings");
    if(source->getDisplayCount() == 0)
      {
      action->setEnabled(false);
      }

    action = menu.addAction("Delete", this->Browser, SLOT(deleteSelected()));
    action->setObjectName("Delete");
    if (source->getNumberOfConsumers() > 0)
      {
      action->setEnabled(false);
      }
    menuHasItems = true;
    }
  else if(server)
    {
    // Show the context menu for a server object.
    }

  if(menuHasItems)
    {
    menu.exec(tree->mapToGlobal(pos));
    }
}

void pqPipelineBrowserContextMenu::showDisplayEditor()
{
  if(!this->Browser)
    {
    return;
    }

  // Show the display proxy editor for the selected object.
  pqFlatTreeView *tree = this->Browser->getTreeView();
  QModelIndex current = tree->selectionModel()->currentIndex();
  pqPipelineModel *model = this->Browser->getListModel();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource*>(
    model->getItem(current));
  if(!source)
    {
    return;
    }

  // Add the dialog to the main window.
  QWidget* topParent = this->Browser;
  while(topParent->parentWidget())
    {
    topParent = topParent->parentWidget();
    }

  // TODO: The display dialog should accept a pqPipelineSource object
  // in order to handle multiple displays.
  QDialog* dialog = new QDialog(topParent);
  dialog->setObjectName("ObjectDisplayProperties");
  dialog->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
  dialog->setWindowTitle("Display Settings");
  QHBoxLayout* l = new QHBoxLayout(dialog);
  pqDisplayProxyEditor* editor = new pqDisplayProxyEditor(dialog);
  editor->setDisplay(source->getDisplay(0));
  l->addWidget(editor);
  QObject::connect(editor, SIGNAL(dismiss()),
    dialog, SLOT(accept()));
  dialog->setModal(true);
  dialog->show();
}

void pqPipelineBrowserContextMenu::showRenderViewEditor()
{
  // TODO: Move this code to the correct location.
  /*QWidget* widget = this->ListModel->getWidgetFor(selection);
  vtkSMRenderModuleProxy* pw = pqPipelineData::instance()->getRenderModule(qobject_cast<QVTKWidget*>(widget));
  if(pw)
    {
    static const char* ViewSettingString = "View Settings...";
    QMenu menu;
    menu.addAction(ViewSettingString);
    QAction* action = menu.exec(this->TreeView->mapToGlobal(position));
    if(action && action->text() == ViewSettingString)
      {
      QDialog* dialog = new QDialog(topParent);
      dialog->setWindowTitle("View Settings");
      QHBoxLayout* l = new QHBoxLayout(dialog);
      pqRenderViewEditor* editor = new pqRenderViewEditor(dialog);
      editor->setRenderView(pw);
      l->addWidget(editor);
      dialog->show();
      }
    }*/
}


