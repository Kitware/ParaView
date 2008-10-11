#include "ChartViewFrameActions.h"

#include <pqActiveView.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqDisplayPolicy.h>
#include <pqMultiViewFrame.h>
#include <pqObjectBuilder.h>
#include <pqOutputPort.h>
#include <pqPendingDisplayManager.h>
#include <pqPluginManager.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqSelectionManager.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqServerManagerModelItem.h>
#include <pqServerManagerSelectionModel.h>
#include <pqSMAdaptor.h>
#include <pqView.h>

#include <vtkPVDataInformation.h>
#include <vtkSMClientDeliveryRepresentationProxy.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMSelectionRepresentationProxy.h>
#include <vtkSMSourceProxy.h>

#include <QAction>
#include <QIcon>
#include <QtDebug>
#include <QMap>

//-----------------------------------------------------------------------------
ChartViewFrameActions::ChartViewFrameActions(QObject* p)
  : pqViewFrameActionGroup(p)
{
  this->setExclusive(false);

  // We add checkable actions to groups so they'll take care of the 
  // toggling for us. 
  QActionGroup *zoomingGroup = new QActionGroup(p);
  QAction *action = new QAction("Box Zoom", this);
  action->setData("Box");
  action->setCheckable(true);
  //action->setIcon(QIcon(":ChartViewFrame/Icons/reset_camera_16.png"));
  this->addAction(action);
  zoomingGroup->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onZoomTypeChanged()));

  action = new QAction("Horizontal Zoom", this);
  action->setData("Horizontal");
  action->setCheckable(true);
  //action->setIcon(QIcon(":ChartViewFrame/Icons/reset_camera_16.png"));
  this->addAction(action);
  zoomingGroup->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onZoomTypeChanged()));

  action = new QAction("Vertical Zoom", this);
  action->setData("Vertical");
  action->setCheckable(true);
  //action->setIcon(QIcon(":ChartViewFrame/Icons/reset_camera_16.png"));
  this->addAction(action);
  zoomingGroup->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onZoomTypeChanged()));

  action = new QAction("Normal Zoom", this);
  action->setData("Both");
  action->setCheckable(true);
  action->setChecked(true);
  //action->setIcon(QIcon(":ChartViewFrame/Icons/reset_camera_16.png"));
  this->addAction(action);
  zoomingGroup->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onZoomTypeChanged()));

  action = new QAction("ResetAxes", this);
  action->setData("ResetAxes");
  action->setIcon(QIcon(":ChartViewFrame/Icons/reset_camera_16.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onResetAxes()));

  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  QObject::connect(selection, SIGNAL(currentChanged(pqServerManagerModelItem*)),
      this, SLOT(updateEnableState()), Qt::QueuedConnection);
  QObject::connect(selection,
      SIGNAL(selectionChanged(
          const pqServerManagerSelection&, const pqServerManagerSelection&)),
      this, SLOT(updateEnableState()), Qt::QueuedConnection);
}



//-----------------------------------------------------------------------------
ChartViewFrameActions::~ChartViewFrameActions()
{
}

//-----------------------------------------------------------------------------
void ChartViewFrameActions::updateEnableState()
{
  // Setup the default state
  QList<QAction*> actions = this->actions();
  actions[0]->setEnabled(true);
}

//-----------------------------------------------------------------------------
bool ChartViewFrameActions::connect(pqMultiViewFrame *frame, pqView *view)
{
  if(view->getViewType().contains("ChartView"))
    {
    for(int i=0; i<this->actions().size(); ++i)
      {
      frame->addTitlebarAction(this->actions()[i]);
      }
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
bool ChartViewFrameActions::disconnect(pqMultiViewFrame *frame, pqView *view)
{
  if(view->getViewType() == "ChartView")
    {
    for(int i=0; i<this->actions().size(); ++i)
      {
      frame->removeTitlebarAction(this->actions()[i]);
      }
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
void ChartViewFrameActions::onZoomTypeChanged()
{
  QAction * action = qobject_cast<QAction*>(this->sender());
  if(!action)
    {
    return;
    }

  pqView *view = pqActiveView::instance().current();
  if(!view)
    {
    return;
    }

  pqSMAdaptor::setEnumerationProperty(view->getProxy()->GetProperty("ZoomingBehavior"), action->data());

  view->getProxy()->UpdateVTKObjects();
  view->render();
}


//-----------------------------------------------------------------------------
void ChartViewFrameActions::onResetAxes()
{
  QAction * action = qobject_cast<QAction*>(this->sender());
  if(!action)
    {
    return;
    }

  pqView *view = pqActiveView::instance().current();
  if(!view)
    {
    return;
    }

  pqSMAdaptor::setElementProperty(view->getProxy()->GetProperty("ResetAxes"),1);
  view->getProxy()->UpdateVTKObjects();

  view->render();
}
