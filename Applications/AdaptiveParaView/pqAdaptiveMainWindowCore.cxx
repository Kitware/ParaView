#include "pqAdaptiveMainWindowCore.h"

#include "pqServer.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSMPropertyHelper.h"

#include "pqView.h"
#include "pqActiveView.h"
#include "pqProxyTabWidget.h"
#include "pqObjectInspectorWidget.h"
#include "pqApplicationCore.h"
#include "pqServerManagerSelectionModel.h"
#include "pqPipelineFilter.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqDataRepresentation.h"
#include "pqProxyMenuManager.h"
#include "pqDisplayPolicy.h"
#include "pqFiltersMenuManager.h"

//#include "Plugin/vtkSMAdaptiveViewProxy.h"

#include <QList>
#include <QTimer>
#include <QDockWidget>
#include <QDebug>

#include <vtksys/ios/sstream>

pqAdaptiveMainWindowCore::pqAdaptiveMainWindowCore() :
  pqMainWindowCore()
{
  this->Pass = 0;
}

//-----------------------------------------------------------------------------
pqAdaptiveMainWindowCore::~pqAdaptiveMainWindowCore()
{

}

//-----------------------------------------------------------------------------
void pqAdaptiveMainWindowCore::onActiveViewChanged(pqView* view)
{
  Superclass::onActiveViewChanged(view);
  if (view)
    {
    QObject::connect(view, SIGNAL(endRender()), this, SLOT(scheduleNextPass()));
    }
}

//-----------------------------------------------------------------------------
void pqAdaptiveMainWindowCore::scheduleNextPass()
{
  vtksys_ios::stringstream message;
  message.str("");
  message << "pass " << this->Pass;
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
  
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy * helper = vtkSMProxy::SafeDownCast(
    pxm->GetProxy("helpers", "AdaptiveOptionsInstance"));
  if (!helper)
    {
    emit this->setMessage("WARNING: Adaptive plugin not found.");
    }

  //for streaming schedule another render pass if required to render the next
  //piece.
  if (!vp->GetDisplayDone())
    {
    emit this->setMessage(QString::fromStdString(message.str()));
    //schedule next render pass
    QTimer *t = new QTimer(this);
    t->setSingleShot(true);
    QObject::connect(t, SIGNAL(timeout()), view, SLOT(render()), Qt::QueuedConnection);
    t->start();    
    this->Pass++;
    }
  else
    {
    message << "\tDONE ";
    this->Pass = 0;
    emit this->setMessage(QString::fromStdString(message.str()));
    }
}

//-----------------------------------------------------------------------------
pqProxyTabWidget* pqAdaptiveMainWindowCore::setupProxyTabWidget(QDockWidget* dock_widget)
{
  //any changes that light up accept button should interrupt streaming

  pqProxyTabWidget* proxyPanel = Superclass::setupProxyTabWidget(dock_widget);
  pqObjectInspectorWidget* object_inspector = proxyPanel->getObjectInspector();
  QObject::connect(object_inspector, SIGNAL(canAccept()), this, SLOT(stopAdaptive()));

  return proxyPanel;
}

//-----------------------------------------------------------------------------
void pqAdaptiveMainWindowCore::onPostAccept()
{
  //any changes that light up accept button should interrupt streaming
  this->stopAdaptive();

  if (this->filtersMenuManager())
    {
    qobject_cast<pqFiltersMenuManager*>(this->filtersMenuManager())->updateEnableState();
    }

  Superclass::onPostAccept();
}

//-----------------------------------------------------------------------------
void pqAdaptiveMainWindowCore::stopAdaptive()
{
  //space and accept button can interrupt streaming
  emit this->setMessage(tr("Stopping..."));
  vtkSMViewProxy *vp = pqActiveView::instance().current()->getViewProxy();
  vp->Interrupt(); 
}

//-----------------------------------------------------------------------------
void pqAdaptiveMainWindowCore::onRemovingSource(pqPipelineSource *source)
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
    // this triggers an eventually render call to kick off streaming
    view->render();
    }
}

