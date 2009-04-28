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

  action = new QAction("Table To Graph", this);
  action->setData("TableToGraph");
  action->setIcon(QIcon(":CommonToolbar/Icons/table_to_graph_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createFilter()));

  action = new QAction("Geospatial View", this);
  action->setData("ClientGeoView");
  action->setIcon(QIcon(":CommonToolbar/Icons/internet_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Bar Chart", this);
  action->setData("BarChartView");
  action->setIcon(QIcon(":CommonToolbar/Icons/column_chart_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Line Chart", this);
  action->setData("LineChartView");
  action->setIcon(QIcon(":CommonToolbar/Icons/line_chart_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));

  action = new QAction("Stacked Chart", this);
  action->setData("StackedChartView");
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
/*  
  action = new QAction("Attribute View", this);
  action->setData("ClientAttributeView");
  action->setIcon(QIcon(":CommonToolbar/Icons/column_zoom_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));
*/
  action = new QAction("Record View", this);
  action->setData("ClientRecordView");
  action->setIcon(QIcon(":CommonToolbar/Icons/row_zoom_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createView()));
  
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
  actions[6]->setEnabled(false);
  actions[7]->setEnabled(true);
  actions[8]->setEnabled(true);
  //actions[9]->setEnabled(true);

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
      actions[2]->setEnabled(true);
      actions[4]->setEnabled(true);
      actions[5]->setEnabled(true);
      actions[6]->setEnabled(true);
      break;
    case VTK_GRAPH : 
    case VTK_TREE : 
    case VTK_DIRECTED_GRAPH : 
    case VTK_UNDIRECTED_GRAPH : 
    case VTK_DIRECTED_ACYCLIC_GRAPH : 
      //actions[3]->setEnabled(true);
      break;
    default:
      break;
    }
}
