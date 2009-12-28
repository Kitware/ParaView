/*=========================================================================

   Program: ParaView
   Module:    streamingMainWindow.cxx

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
#include "streamingMainWindow.h"
#include "ui_streamingMainWindow.h"

#include "pqHelpReaction.h"
#include "pqObjectInspectorWidget.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"

#include "pqActiveView.h"
#include "vtkSMViewProxy.h"
#include "pqView.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMPropertyHelper.h"

#include "pqApplicationCore.h"
#include "pqCustomDisplayPolicy.h"

#include <QShortCut>
#include <QStatusBar>

#include <iostream>
using namespace std;

class streamingMainWindow::pqInternals : public Ui::pqClientMainWindow
{
};

//-----------------------------------------------------------------------------
streamingMainWindow::streamingMainWindow()
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  // Setup default GUI layout.

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  this->Internals->animationViewDock->hide();
  this->Internals->statisticsDock->hide();
  this->Internals->selectionInspectorDock->hide();
  this->Internals->comparativePanelDock->hide();
  this->tabifyDockWidget(this->Internals->animationViewDock,
    this->Internals->statisticsDock);

  // Enable automatic creation of representation on accept.
  this->Internals->proxyTabWidget->setShowOnAccept(true);

  // Enable help for from the object inspector.
  QObject::connect(this->Internals->proxyTabWidget->getObjectInspector(),
    SIGNAL(helpRequested(QString)),
    this, SLOT(showHelpForProxy(const QString&)));

  // Populate application menus with actions.
  pqParaViewMenuBuilders::buildFileMenu(*this->Internals->menu_File);
  pqParaViewMenuBuilders::buildEditMenu(*this->Internals->menu_Edit);

  // Populate sources menu.
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);

  // Populate filters menu.
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Populate Tools menu.
  pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools);

  // setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(
    *this->Internals->pipelineBrowser);

  pqParaViewMenuBuilders::buildToolbars(*this);

  // Setup the View menu. This must be setup after all toolbars and dockwidgets
  // have been created.
  pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menu_View, *this);

  // Setup the menu to show macros.
  pqParaViewMenuBuilders::buildMacrosMenu(*this->Internals->menu_Macros);

  // Setup the help menu.
  pqParaViewMenuBuilders::buildHelpMenu(*this->Internals->menu_Help);

  // Final step, define application behaviors. Since we want all ParaView
  // behaviors, we use this convenience method.
  new pqParaViewBehaviors(this, this);

//STUFF TO CUSTOMIZE THIS FOR STREAMING
  //load the streaming options
  vtkSMProxyManager * pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm)
    {
    vtkSMProxy* prototype =
      pxm->GetPrototypeProxy("helpers", "StreamingOptions");
    if (!prototype)
      {
      //vtkWarningMacro("Tried and failed to create a streaming module. "
      //                << "Make sure the streaming plugin can be found by ParaView.");
      }
    }  

/*
  //remove surface selection buttons
  pqClientMainWindow ->disableSelections();

  //remove the view types that do not have streaming support
  pqPluginManager* plugin_manager =
    pqApplicationCore::instance()->getPluginManager();
  QObjectList ifcs = plugin_manager->interfaces();
  for (int i = 0; i < ifcs.size(); i++)
    {
    QObject *nxt = ifcs.at(i);
    if (nxt->inherits("pqStandardViewModules"))
      {
      plugin_manager->removeInterface(ifcs.at(i));
      break;
      }
    }
  //add back the ones that do
  plugin_manager->addInterface(
    new pqCustomViewModules(plugin_manager));

  //link up pass number message message alerts
  w->connect(myCore,
    SIGNAL(setMessage(const QString&)),
    SLOT(setMessage(const QString&)));
*/

  // Watch view to set up multipass renders
  QObject::connect(&pqActiveView::instance(),
    SIGNAL(changed(pqView*)),
    this, SLOT(onActiveViewChanged(pqView*)));

  // Make the accept button stop multipass rendering
  QObject::connect(this->Internals->proxyTabWidget->getObjectInspector(),
                   SIGNAL(canAccept()),
                   this, SLOT(stopStreaming()));

  //add way to for user to manually stop streaming
  QShortcut *pauseKey = new QShortcut(Qt::Key_Space, this);
  QObject::connect(pauseKey, SIGNAL(activated()), this, SLOT(stopStreaming()));

}

//-----------------------------------------------------------------------------
streamingMainWindow::~streamingMainWindow()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void streamingMainWindow::showHelpForProxy(const QString& proxyname)
{
  pqHelpReaction::showHelp(
    QString("qthelp://paraview.org/paraview/%1.html").arg(proxyname));
}

//-----------------------------------------------------------------------------
void streamingMainWindow::onActiveViewChanged(pqView* view)
{
  if (view)
    {
    QObject::connect(view, SIGNAL(endRender()), this, SLOT(scheduleNextPass()));
    }
}

//-----------------------------------------------------------------------------
void streamingMainWindow::scheduleNextPass()
{
  //TODO: make this emit pass number messages like it used to
  //emit this->setMessage(tr(""));
  pqView* view = pqActiveView::instance().current();
  if(!view)
    {
    return;
    }
  vtkSMViewProxy *vp = view->getViewProxy();
  if (!vp)
    {
    return;
    }
  
  int doPrint = 0;
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy * helper = vtkSMProxy::SafeDownCast(
    pxm->GetProxy("helpers", "StreamingOptionsInstance"));
  if (helper)
    {
    doPrint = vtkSMPropertyHelper(helper, "EnableStreamMessages").GetAsInt();
    }

  //for streaming schedule another render pass if required to render the next
  //piece.
  if (!vp->GetDisplayDone() && !this->StopStreaming)
    {
    if (doPrint)
      {
      cerr << "MWC Schedule next render" << endl;
      }    
    //this->statusBar->setMessage("Next Render", 10000);
    //schedule next render pass
    QTimer *t = new QTimer(this);
    t->setSingleShot(true);
    QObject::connect(t, SIGNAL(timeout()), view, SLOT(render()), Qt::QueuedConnection);
    t->start();    
    }
  else
    {
    if (doPrint)
      {
      cerr << "MWC Render Finished" << endl;
      }
    //this->statusBar->setMessage("Render Finished", 10000);
    }
  this->StopStreaming = false;
}

//-----------------------------------------------------------------------------
void streamingMainWindow::stopStreaming()
{
  //This will stop multipass streaming rendering from continuing.
  this->StopStreaming = true;
}

/*
//-----------------------------------------------------------------------------
TODO: NOT SURE WHY I NEEDED THIS EXACTLY

void streamingMainWindow::onPostAccept()
{
  this->stopStreaming();

  if (this->filtersMenuManager())
    {
    qobject_cast<pqFiltersMenuManager*>(this->filtersMenuManager())->updateEnableState();
    }

  Superclass::onPostAccept();
}

//-----------------------------------------------------------------------------
TODO: THIS CHANGE TO PARENT PREVENTS THE AUTOMATIC SHOWING OF THE INPUT TO THE
CURRENT FILTER

// This method is called only when the gui intiates the removal of the source.
void streamingMainWindow::onRemovingSource(pqPipelineSource *source)
{
  // FIXME: updating of selection must happen even is the source is removed
  // from python script or undo redo.
  // If the source is selected, remove it from the selection.
  pqApplicationCore *core = pqApplicationCore::instance();
  pqServerManagerSelectionModel *selection = core->getSelectionModel();
  if(selection->isSelected(source))
    {
    if(selection->selectedItems()->size() > 1)
      {
      // Deselect the source.
      selection->select(source, pqServerManagerSelectionModel::Deselect);

      // If the source is the current item, change the current item.
      if(selection->currentItem() == source)
        {
        selection->setCurrentItem(selection->selectedItems()->last(),
            pqServerManagerSelectionModel::NoUpdate);
        }
      }
    else
      {
      // If the item is a filter and has only one input, set the
      // input as the current item. Otherwise, select the server.
      pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter *>(source);
      if(filter && filter->getInputCount() == 1)
        {
        selection->setCurrentItem(filter->getInput(0),
            pqServerManagerSelectionModel::ClearAndSelect);
        }
      else
        {
        selection->setCurrentItem(source->getServer(),
            pqServerManagerSelectionModel::ClearAndSelect);
        }
      }
    }

  QList<pqView*> views = source->getViews();

  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
  
  if (!pqApplicationCore::instance()->getDisplayPolicy()->getHideByDefault() &&
      filter)
    {
    // Make all inputs visible in views that the removed source
    // is currently visible in.
    QList<pqOutputPort*> inputs = filter->getInputs();
    foreach(pqView* view, views)
      {
      pqDataRepresentation* src_disp = source->getRepresentation(view);
      if (!src_disp || !src_disp->isVisible())
        {
        continue;
        }
      // For each input, if it is not visibile in any of the views
      // that the delete filter is visible, we make the input visible.
      for(int cc=0; cc < inputs.size(); ++cc)
        {
        pqPipelineSource* input = inputs[cc]->getSource();
        pqDataRepresentation* input_disp = input->getRepresentation(view);
        if (input_disp && !input_disp->isVisible())
          {
          input_disp->setVisible(true);
          }
        }
      }
    }

  foreach (pqView* view, views)
    {
    // this triggers an eventually render call.
    view->render();
    }
}
*/
