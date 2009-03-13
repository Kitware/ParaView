#include "CommonToolbarActions.h"

#include <pqActiveView.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqDisplayPolicy.h>
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
CommonToolbarActions::CommonToolbarActions(QObject* p)
  : ToolbarActions(p)
{
  QAction* action = new QAction("Database To Table", this);
  action->setData("SQLDatabaseTableSource");
  action->setIcon(QIcon(":CommonToolbar/Icons/database2table_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createSource()));

  action = new QAction("Database To Graph", this);
  action->setData("SQLDatabaseGraphSource");
  action->setIcon(QIcon(":CommonToolbar/Icons/database2graph_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createSource()));

  action = new QAction("Geospatial View", this);
  action->setData("ClientGeoView");
  action->setIcon(QIcon(":CommonToolbar/Icons/internet_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Bar Chart", this);
  action->setData("ClientBarChartView");
  action->setIcon(QIcon(":CommonToolbar/Icons/column_chart_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Line Chart", this);
  action->setData("ClientLineChartView");
  action->setIcon(QIcon(":CommonToolbar/Icons/line_chart_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Stacked Chart", this);
  action->setData("ClientStackedChartView");
  action->setIcon(QIcon(":CommonToolbar/Icons/area_chart_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Table View", this);
  action->setData("ClientTableView");
  action->setIcon(QIcon(":CommonToolbar/Icons/table_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));
  
  action = new QAction("Attribute View", this);
  action->setData("ClientAttributeView");
  action->setIcon(QIcon(":CommonToolbar/Icons/column_zoom_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Record View", this);
  action->setData("ClientRecordView");
  action->setIcon(QIcon(":CommonToolbar/Icons/row_zoom_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Extract Selected Graph", this);
  action->setData("FreezeGraph");
  action->setIcon(QIcon(":CommonToolbar/Icons/extract_graph_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)), 
    this, SLOT(createGraphSourceFromGraphSelection()));
    
  action = new QAction("Nearest Neighbors Selection", this);
  action->setData("ExpandSelection");
  action->setIcon(QIcon(":CommonToolbar/Icons/nearest_neighbor_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)), this, SLOT(createSelectionFilter()));
    
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
CommonToolbarActions::~CommonToolbarActions()
{
}



//-----------------------------------------------------------------------------
void CommonToolbarActions::updateEnableState()
{
  // Setup the default state
  QList<QAction*> actions = this->actions();
  actions[0]->setEnabled(true);
  actions[1]->setEnabled(true);
  actions[2]->setEnabled(false);
  actions[3]->setEnabled(false);
  actions[4]->setEnabled(false);
  actions[5]->setEnabled(false);
  actions[6]->setEnabled(true);
  actions[7]->setEnabled(true);
  actions[8]->setEnabled(true);
  actions[9]->setEnabled(false);
  actions[10]->setEnabled(false);

  pqPipelineSource *src = this->getActiveSource();
  if(!src)
    {
    return;
    }

  pqOutputPort* opPort = src->getOutputPort(0);
  if(!opPort)
    {
    return;
    }

  switch( opPort->getDataInformation(false)->GetDataSetType() )
    {
    case VTK_TABLE : 
      actions[3]->setEnabled(true);
      actions[4]->setEnabled(true);
      //actions[5]->setEnabled(true);
      break;
    case VTK_GRAPH : 
    case VTK_TREE : 
    case VTK_DIRECTED_GRAPH : 
    case VTK_UNDIRECTED_GRAPH : 
    case VTK_DIRECTED_ACYCLIC_GRAPH : 
      actions[2]->setEnabled(true);
      actions[9]->setEnabled(true);
      actions[10]->setEnabled(true);
      break;
    default:
      break;
    }
}

//-----------------------------------------------------------------------------
void CommonToolbarActions::createGraphSourceFromGraphSelection()
{
  QAction * action = qobject_cast<QAction*>(this->sender());
  if(!action)
    {
    return;
    }

  QString type=action->data().toString();
  if(type != "FreezeGraph")
    {
    return;
    }

  pqServer * server = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if(!server)
    {
    qDebug() << "No server present, cannot convert source.";
    }

  // Get the currently selected source
  pqPipelineSource * src = this->getActiveSource();
  if(!src)
    {
    return;
    }

  pqPendingDisplayManager * pdm = qobject_cast<pqPendingDisplayManager*>(pqApplicationCore::instance()->manager("PENDING_DISPLAY_MANAGER"));

  // Create ExtractSelectedGraph filter
  vtkSMSourceProxy *selProxy = vtkSMSourceProxy::SafeDownCast(src->getProxy())->GetSelectionInput(0);
  vtkSMSourceProxy* filterProxy = vtkSMSourceProxy::SafeDownCast(vtkSMProxyManager::GetProxyManager()->NewProxy("filters", "ExtractSelectedGraph"));
  filterProxy->SetConnectionID(server->GetConnectionID());
  vtkSMProxyManager::GetProxyManager()->RegisterProxy("filters", "Extract Selected Graph", filterProxy);
  vtkSMInputProperty *selInput = vtkSMInputProperty::SafeDownCast(filterProxy->GetProperty("Input"));
  selInput->AddInputConnection(src->getProxy(),0);
  vtkSMInputProperty *graphInput = vtkSMInputProperty::SafeDownCast(filterProxy->GetProperty("Selection"));
  graphInput->AddInputConnection(selProxy,0);
  filterProxy->UpdatePipeline();

  // Create the FreezeGraph filter
  vtkSMSourceProxy* freezeGraphProxy = vtkSMSourceProxy::SafeDownCast(vtkSMProxyManager::GetProxyManager()->NewProxy("filters", "FreezeGraph"));
  freezeGraphProxy->SetConnectionID(server->GetConnectionID());
  vtkSMProxyManager::GetProxyManager()->RegisterProxy("sources", "Graph Copy", freezeGraphProxy);
  vtkSMInputProperty *fgInput = vtkSMInputProperty::SafeDownCast(freezeGraphProxy->GetProperty("Input"));
  fgInput->AddInputConnection(filterProxy,0);
  freezeGraphProxy->UpdatePipeline();

  pdm->setAddSourceIgnored(true);
  pqPipelineSource* pqFreezeGraphProxy = pqApplicationCore::instance()->
      getServerManagerModel()->findItem<pqPipelineSource*>(freezeGraphProxy);
  pdm->setAddSourceIgnored(false);
 
  // create a view...
  pqOutputPort * opPort = pqFreezeGraphProxy->getOutputPort(0);
  QString preferredViewType = pqApplicationCore::instance()->getDisplayPolicy()->getPreferredViewType(opPort,0);
  if(preferredViewType.isNull())
    {
    return;
    }

  // Add it to the view.
  pqObjectBuilder * builder = pqApplicationCore::instance()->getObjectBuilder();
  pqView * view = builder->createView(preferredViewType, server);
  builder->createDataRepresentation(pqFreezeGraphProxy->getOutputPort(0), view);
  view->render();

  // Cleanup. Disconnect filter inputs.
  fgInput->RemoveAllProxies();
  selInput->RemoveAllProxies();
  graphInput->RemoveAllProxies();
  filterProxy->Delete();
  freezeGraphProxy->Delete();
}

