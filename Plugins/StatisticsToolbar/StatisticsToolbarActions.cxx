#include "StatisticsToolbarActions.h"

#include <pqApplicationCore.h>
#include <pqDisplayPolicy.h>
#include <pqObjectBuilder.h>
#include <pqOutputPort.h>
#include <pqPendingDisplayManager.h>
#include <pqPluginManager.h>
#include <pqPipelineSource.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqServerManagerModelItem.h>
#include <pqServerManagerSelectionModel.h>

#include <vtkPVDataInformation.h>
#include <vtkSMProperty.h>
#include <vtkSMSourceProxy.h>

#include <QAction>
#include <QIcon>
#include <QtDebug>

StatisticsToolbarActions::StatisticsToolbarActions(QObject* p)
  : ToolbarActions(p)
{
  QAction *action = new QAction("Descriptive Statistics", this);
  action->setData("DescriptiveStatistics");
  action->setIcon(QIcon(":StatisticsToolbar/Icons/descriptive_stats_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createFilter()));

  action = new QAction("Order Statistics", this);
  action->setData("OrderStatistics");
  action->setIcon(QIcon(":StatisticsToolbar/Icons/order_stats_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createFilter()));

  action = new QAction("Correlative Statistics", this);
  action->setData("CorrelativeStatistics");
  action->setIcon(QIcon(":StatisticsToolbar/Icons/correlative_stats_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createFilter()));

  action = new QAction("Contingency Statistics", this);
  action->setData("ContingencyStatistics");
  action->setIcon(QIcon(":StatisticsToolbar/Icons/contingency_stats_48.png"));
  this->addAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(createFilter()));

  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  QObject::connect(selection, SIGNAL(currentChanged(pqServerManagerModelItem*)),
      this, SLOT(updateEnableState()), Qt::QueuedConnection);
  QObject::connect(selection,
      SIGNAL(selectionChanged(
          const pqServerManagerSelection&, const pqServerManagerSelection&)),
      this, SLOT(updateEnableState()), Qt::QueuedConnection);
}

StatisticsToolbarActions::~StatisticsToolbarActions()
{
}

//-----------------------------------------------------------------------------
void StatisticsToolbarActions::updateEnableState()
{
  // Setup the default state
  QList<QAction*> actions = this->actions();
  actions[0]->setEnabled(false);
  actions[1]->setEnabled(false);
  actions[2]->setEnabled(false);
  actions[3]->setEnabled(false);

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
      actions[0]->setEnabled(true);
      actions[1]->setEnabled(true);
      actions[2]->setEnabled(true);
      actions[3]->setEnabled(true);
      break;
    case VTK_GRAPH : 
    case VTK_TREE : 
    case VTK_DIRECTED_GRAPH : 
    case VTK_UNDIRECTED_GRAPH : 
    case VTK_DIRECTED_ACYCLIC_GRAPH : 
      actions[0]->setEnabled(false);
      actions[1]->setEnabled(false);
      actions[2]->setEnabled(false);
      actions[3]->setEnabled(false);
      break;
    default:
      break;
    }
}

