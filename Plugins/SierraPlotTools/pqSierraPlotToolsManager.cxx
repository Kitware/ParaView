// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSierraPlotToolsManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "warningState.h"

#include "pqSierraPlotToolsManager.h"

#include "pqElementPlotter.h"
#include "pqGlobalPlotter.h"
#include "pqNodePlotter.h"
#include "pqPlotVariablesDialog.h"
#include "pqSierraPlotToolsDataLoadManager.h"
#include "pqVariableVariablePlotter.h"

#include "ui_pqVariablePlot.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkExodusFileSeriesReader.h"
#include "vtkExodusIIReader.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSelectionNode.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "pqXYChartView.h"
#include <pqSetName.h>

#include <QDockWidget>
#include <QGridLayout>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMetaType>
#include <QPointer>
#include <QPushButton>
#include <QStringList>
#include <QToolButton>
#include <QVariant>
#include <QtAlgorithms>
#include <QtDebug>

#include "ui_pqSierraPlotToolsActionHolder.h"

// used to show line number in #pragma messages
#define STRING2(x) #x
#define STRING(x) STRING2(x)

//=============================================================================
class pqSierraPlotToolsManager::pqInternal
{
public:
  enum plotVariableType
  {
    eGlobal,
    eNode,
    eElement
  };

  class PlotterMetaData : QObject
  {
  public:
    PlotterMetaData(plotVariableType theType, pqPlotter::plotDomain theDomain, QString theHeader,
      pqPlotter* thePlotter, bool theEnabledFlag = true)
    {
      type = theType;
      domain = theDomain;
      header = theHeader;
      plotter = thePlotter;
      plotter->setDomain(theDomain);
      enabledFlag = theEnabledFlag;
    }
    ~PlotterMetaData() { delete this->plotter; }

    plotVariableType type;
    pqPlotter::plotDomain domain;
    QString header;
    pqPlotter* plotter;
    bool enabledFlag;
  };

  pqInternal()
    : plotGUI(NULL)
    , currentMetaPlotter(NULL)
  {

    whoAmI = QString("Sierra Plot Tools Data Manager");

    PlotterMetaData* plotMeta;
    QString heading;

    heading = QString("Global var. vs time...");
    plotMenuItemsList.push_back(heading);
    plotMeta = new PlotterMetaData(eGlobal, pqPlotter::eTime, heading, new pqGlobalPlotter);
    plotterMap[heading] = plotMeta;

    heading = QString("Node var. vs time...");
    plotMenuItemsList.push_back(heading);
    plotMeta = new PlotterMetaData(eNode, pqPlotter::eTime, heading, new pqNodePlotter);
    plotterMap[heading] = plotMeta;

    heading = QString("Element var. vs time...");
    plotMenuItemsList.push_back(heading);
    plotMeta = new PlotterMetaData(eElement, pqPlotter::eTime, heading, new pqElementPlotter);
    plotterMap[heading] = plotMeta;

    // add a separator
    plotMenuItemsList.push_back("<dash>");

    heading = QString("Node var. along path...");
    plotMenuItemsList.push_back(heading);
    plotMeta = new PlotterMetaData(eNode, pqPlotter::ePath, heading, new pqNodePlotter, false);
    plotterMap[heading] = plotMeta;

    heading = QString("Element var. along path...");
    plotMenuItemsList.push_back(heading);
    plotMeta =
      new PlotterMetaData(eElement, pqPlotter::ePath, heading, new pqElementPlotter, false);
    plotterMap[heading] = plotMeta;

    // add a separator
    plotMenuItemsList.push_back("<dash>");

    heading = QString("Variable vs. variable...");
    plotMenuItemsList.push_back(heading);
    plotMeta = new PlotterMetaData(
      eElement, pqPlotter::eVariable, heading, new pqVariableVariablePlotter, false);
    plotterMap[heading] = plotMeta;
  }

  virtual ~pqInternal()
  {
    foreach (PlotterMetaData* md, this->plotterMap)
    {
      delete md;
    }
    this->plotterMap.clear();
  }

  // This method checks to see if the selection range (e.g. via the line edit box)
  // is within range of what is in the mesh
  bool withinSelectionRange(pqPipelineSource* meshReader, QList<int>& selectedItems)
  {
    //
    // Use the nodes specified in the node plot variables dialog
    //
    bool errFlag;
    QString numberItemsString = this->plotGUI->getNumberItemsLineEdit();
    if (numberItemsString.size() > 0)
    {
      selectedItems = this->plotGUI->determineSelectedItemsList(errFlag);
      if (errFlag)
      {
        qWarning() << "pqSierraPlotToolsManager::pqInternal:withinSelectionRange: ERROR - some "
                      "problem with the node selection: "
                   << numberItemsString;
        return false;
      }

      if (!this->currentMetaPlotter->plotter->selectionWithinRange(selectedItems, meshReader))
      {
        qWarning() << "pqSierraPlotToolsManager::pqInternal:withinSelectionRange: ERROR - out of "
                      "range id with: "
                   << numberItemsString;
        return false;
      }
    }

    return true;
  }

  QString StripDotDotDot(QString str);

  bool withinRange(QList<int>& selectedItems, pqPipelineSource* meshReader,
    vtkSelectionNode::SelectionField selectType);

  QVector<int> getGlobalIds(vtkSMSourceProxy* sourceProxy);

  QVector<int> getGlobalIdsClientSide(vtkObjectBase* clientSideObject);

  QVector<int> getGlobalIdsServerSide(vtkSMSourceProxy* sourceProxy);

  QVector<int> getGlobalIdsFromComposite(vtkCompositeDataSet* compositeDataSet);

  QVector<int> getGlobalIdsFromCompositeOrMultiBlock(vtkCompositeDataSet* compositeDataSet);

  QVector<int> getGlobalIdsFromMultiBlock(vtkMultiBlockDataSet* block);

  QVector<int> getGlobalIdsFromDataSet(vtkDataSet* dataSet);

  QStringList getTheVars(vtkSMProxy* meshReaderProxy);

  void adjustPlotterForPickedVariables(pqPipelineSource* meshReader);

  Ui::pqSierraPlotToolsActionHolder Actions;
  QWidget ActionPlaceholder;
  pqPlotVariablesDialog* plotGUI;
  // pqPlotter * plotter;
  QString whoAmI;
  QList<QPair<int, QString> > plotHeaders;
  QMap<int, QString> typeToHeaderMap;

  QVector<QString> plotMenuItemsList;
  QMap<QString, PlotterMetaData*> plotterMap;
  PlotterMetaData* currentMetaPlotter;
};

//=============================================================================
QVector<int> pqSierraPlotToolsManager::pqInternal::getGlobalIdsServerSide(
  vtkSMSourceProxy* /*sourceProxy*/)
{
  QVector<int> idVector;
  idVector.clear();

  qWarning() << "pqSierraPlotToolsManager::pqInternal::getGlobalIdsServerSide: * WARNING *  unable "
                "to get server side IDs yet";

  return idVector;
}

//=============================================================================
QVector<int> pqSierraPlotToolsManager::pqInternal::getGlobalIdsFromDataSet(vtkDataSet* dataSet)
{
  QVector<int> idVector;
  idVector.clear();

  vtkDataSetAttributes* dataSetAttrib = dataSet->GetAttributes(vtkDataObject::POINT);
  vtkIdTypeArray* globalIds =
    dynamic_cast<vtkIdTypeArray*>(dataSetAttrib->GetAttribute(vtkDataSetAttributes::GLOBALIDS));

  int i;
  for (i = 0; i < globalIds->GetNumberOfTuples(); i++)
  {
    vtkIdType id = globalIds->GetValue(i);
    idVector.push_back(int(id));
  }

  return idVector;
}

//=============================================================================
QString pqSierraPlotToolsManager::pqInternal::StripDotDotDot(QString str)
{
  if (str.endsWith("..."))
  {
    str.replace(str.size() - 3, 3, QString(""));
  }
  return str;
}

//=============================================================================
QVector<int> pqSierraPlotToolsManager::pqInternal::getGlobalIdsFromMultiBlock(
  vtkMultiBlockDataSet* multiBlockDataSet)
{
  QVector<int> idVector;
  idVector.clear();

  unsigned int numBlocks = multiBlockDataSet->GetNumberOfBlocks();

  if (numBlocks == 0)
  {
    idVector += this->getGlobalIdsFromComposite(multiBlockDataSet);
  }
  else
  {
    unsigned int k;
    for (k = 0; k < numBlocks; k++)
    {
      vtkDataObject* block = multiBlockDataSet->GetBlock(k);

      vtkCompositeDataSet* subCompositeDataSet = dynamic_cast<vtkCompositeDataSet*>(block);
      if (subCompositeDataSet)
      {
        // recursive call
        idVector += this->getGlobalIdsFromCompositeOrMultiBlock(subCompositeDataSet);
      }
      else
      {
        vtkDataSet* dataSet = dynamic_cast<vtkDataSet*>(block);
        if (dataSet != NULL)
        {
          idVector += this->getGlobalIdsFromDataSet(dataSet);
        }
      }
    }
  }

  return idVector;
}

//=============================================================================
QVector<int> pqSierraPlotToolsManager::pqInternal::getGlobalIdsFromComposite(
  vtkCompositeDataSet* compositeDataSet)
{
  QVector<int> idVector;
  idVector.clear();

  vtkCompositeDataIterator* iterator = compositeDataSet->NewIterator();
  iterator->InitTraversal();
  while (!iterator->IsDoneWithTraversal())
  {
    vtkDataObject* dataObj = iterator->GetCurrentDataObject();
    vtkDataSet* dataSet = dynamic_cast<vtkDataSet*>(dataObj);
    if (dataSet != NULL)
    {
      vtkCompositeDataSet* subCompositeDataSet = dynamic_cast<vtkCompositeDataSet*>(dataSet);
      if (subCompositeDataSet)
      {
        idVector += getGlobalIdsFromComposite(subCompositeDataSet);
      }
      else
      {
        idVector += this->getGlobalIdsFromDataSet(dataSet);
      }
    }

    iterator->GoToNextItem();
  }

  return idVector;
}

//=============================================================================
QVector<int> pqSierraPlotToolsManager::pqInternal::getGlobalIdsFromCompositeOrMultiBlock(
  vtkCompositeDataSet* compositeDataSet)
{
  QVector<int> idVector;
  idVector.clear();

  vtkMultiBlockDataSet* multiBlockDataSet = dynamic_cast<vtkMultiBlockDataSet*>(compositeDataSet);
  if (multiBlockDataSet)
  {
    idVector += this->getGlobalIdsFromMultiBlock(multiBlockDataSet);
  }
  else
  {
    idVector += this->getGlobalIdsFromComposite(compositeDataSet);
  }

  return idVector;
}

//=============================================================================
QVector<int> pqSierraPlotToolsManager::pqInternal::getGlobalIdsClientSide(
  vtkObjectBase* clientSideObject)
{
  QVector<int> idVector;
  idVector.clear();

  vtkObject* object = dynamic_cast<vtkObject*>(clientSideObject);
  if (object == NULL)
    return idVector;

  vtkExodusFileSeriesReader* exoFileSeriesReader = dynamic_cast<vtkExodusFileSeriesReader*>(object);
  if (exoFileSeriesReader == NULL)
    return idVector;

  vtkDataObject* dataObj = exoFileSeriesReader->GetOutput();

  vtkCompositeDataSet* compositeDataSet = dynamic_cast<vtkCompositeDataSet*>(dataObj);
  if (compositeDataSet)
  {
    // idVector = this->getGlobalIdsFromComposite(compositeDataSet);
    idVector += this->getGlobalIdsFromCompositeOrMultiBlock(compositeDataSet);
  }
  else
  {
    vtkDataSet* dataSet = dynamic_cast<vtkDataSet*>(compositeDataSet);
    if (dataSet)
    {
      idVector += this->getGlobalIdsFromDataSet(dataSet);
    }
  }

  return idVector;
}

//=============================================================================
QVector<int> pqSierraPlotToolsManager::pqInternal::getGlobalIds(vtkSMSourceProxy* sourceProxy)
{
  QVector<int> idVector;
  idVector.clear();

  vtkObjectBase* clientSideObject = sourceProxy->GetClientSideObject();

  if (clientSideObject != NULL)
  {
    idVector = this->getGlobalIdsClientSide(clientSideObject);
  }
  else
  {
    idVector = this->getGlobalIdsServerSide(sourceProxy);
  }

  return idVector;
}

//=============================================================================
QStringList pqSierraPlotToolsManager::pqInternal::getTheVars(vtkSMProxy* meshReaderProxy)
{
  return this->currentMetaPlotter->plotter->getTheVars(meshReaderProxy);
}

//=============================================================================
bool pqSierraPlotToolsManager::pqInternal::withinRange(
  QList<int>& nodeList, pqPipelineSource* meshReader, vtkSelectionNode::SelectionField selectType)
{
  vtkSMProxy* meshReaderProxy = meshReader->getProxy();

  vtkSMSourceProxy* sourceProxy = dynamic_cast<vtkSMSourceProxy*>(meshReaderProxy);

  if (sourceProxy != NULL)
  {

    //
    // get a vector of globalIds
    //
    QVector<int> globalIds = this->getGlobalIds(sourceProxy);
    if (globalIds.size() <= 0)
    {
      // this can happen if the server can not or does not return a proper set of globalIds
      return false;
    }

    vtkSMOutputPort* smOutputPort = sourceProxy->GetOutputPort((unsigned int)(0));
    vtkPVDataInformation* pvDataInfo = smOutputPort->GetDataInformation();
    if (pvDataInfo != NULL)
    {
      vtkPVDataSetAttributesInformation* pvDataSetAttributesInformation = NULL;

      pvDataSetAttributesInformation = pvDataInfo->GetPointDataInformation();

      vtkPVArrayInformation* arrayInfo = NULL;
      arrayInfo = pvDataSetAttributesInformation->GetArrayInformation("GlobalNodeId");
      if (arrayInfo != NULL)
      {
        int numComponents = arrayInfo->GetNumberOfComponents();

        if (numComponents > 1)
        {
          // some sort of crazy error!
          qWarning() << "pqSierraPlotToolsManager::pqInternal::withinRange: ERROR - GlobalNodeId "
                        "array returning more than one component!";
          return false;
        }

        double range[2];
        arrayInfo->GetComponentRange(0, range);
        int rangeLow = int(range[0]);
        int rangeHigh = int(range[1]);

        int i;
        long int max = -1;
        long int min = LONG_MAX;
        for (i = 0; i < nodeList.size(); i++)
        {
          int val = nodeList[i];

          if (val < min)
          {
            min = val;
          }

          if (val > max)
          {
            max = val;
          }
        }

        if (min >= rangeLow && max <= rangeHigh)
        {
          return true;
        }
      }
    }
  }

  if (selectType == vtkSelectionNode::POINT)
  {
  }
  else if (selectType == vtkSelectionNode::CELL)
  {
  }

  return false;
}

//=============================================================================

//
// Singleton instance of pqSierraPlotToolsManager *
//
pqSierraPlotToolsManager* pqSierraPlotToolsManager::instance()
{

  static pqSierraPlotToolsManager theManager(NULL);
  return &theManager;
}

//-----------------------------------------------------------------------------
pqSierraPlotToolsManager::pqSierraPlotToolsManager(QObject* p)
  : QObject(p)
{
  this->Internal = new pqSierraPlotToolsManager::pqInternal;

  // This widget serves no real purpose other than initializing the Actions
  // structure created with designer that holds the actions.
  this->Internal->Actions.setupUi(&this->Internal->ActionPlaceholder);

  QObject::connect(
    this->actionDataLoadManager(), SIGNAL(triggered(bool)), this, SLOT(showDataLoadManager()));

  // The slots and signals below are borrowed from Ken's SLAC toolbar plugin...
  //  ... some are useful and perhaps we will keep them, one or more may
  //      eventually be removed or changed for the Sierra Tools plugin
  QObject::connect(this->actionSolidMesh(), SIGNAL(triggered(bool)), this, SLOT(showSolidMesh()));
  QObject::connect(this->actionWireframeSolidMesh(), SIGNAL(triggered(bool)), this,
    SLOT(showWireframeSolidMesh()));
  QObject::connect(this->actionWireframeAndBackMesh(), SIGNAL(triggered(bool)), this,
    SLOT(showWireframeAndBackMesh()));
  QObject::connect(
    this->actionToggleBackgroundBW(), SIGNAL(triggered(bool)), this, SLOT(toggleBackgroundBW()));

  this->checkActionEnabled();
}

//-----------------------------------------------------------------------------
pqSierraPlotToolsManager::~pqSierraPlotToolsManager()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::slotUseParaViewGUIToSelectNodesCheck()
{
  bool checkState = this->Internal->plotGUI->getUseParaViewGUIToSelectNodesCheckBoxState();

  if (checkState)
  {
    // disable the numberItemsLabel and numberItemsLineEdit
    this->Internal->plotGUI->setEnableNumberItems(false);
  }
  else
  {
    // enable the numberItemsLabel and numberItemsLineEdit
    this->Internal->plotGUI->setEnableNumberItems(true);
  }
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::slotVariableSelectionByName(QString varStr)
{
  if (!this->Internal->plotGUI->addRangeToUI(varStr))
  {
    // some sort of error...
    return;
  }
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::slotVariableDeselectionByName(QString varStr)
{
  if (!this->Internal->plotGUI->removeRangeFromUI(varStr))
  {
    // some sort of error...
    return;
  }
}

//-----------------------------------------------------------------------------
QAction* pqSierraPlotToolsManager::actionDataLoadManager()
{
  return this->Internal->Actions.actionDataLoadManager;
}

QAction* pqSierraPlotToolsManager::actionPlotVars()
{
  return this->Internal->Actions.actionPlotVars;
}

QAction* pqSierraPlotToolsManager::actionSolidMesh()
{
  return this->Internal->Actions.actionSolidMesh;
}

QAction* pqSierraPlotToolsManager::actionWireframeSolidMesh()
{
  return this->Internal->Actions.actionWireframeSolidMesh;
}

QAction* pqSierraPlotToolsManager::actionWireframeAndBackMesh()
{
  return this->Internal->Actions.actionWireframeAndBackMesh;
}

QAction* pqSierraPlotToolsManager::actionToggleBackgroundBW()
{
  return this->Internal->Actions.actionToggleBackgroundBW;
}

QAction* pqSierraPlotToolsManager::actionPlotDEBUG()
{
  return this->Internal->Actions.actionPlotDEBUG;
}

//-----------------------------------------------------------------------------
pqServer* pqSierraPlotToolsManager::getActiveServer()
{
  pqApplicationCore* app = pqApplicationCore::instance();
  pqServerManagerModel* smModel = app->getServerManagerModel();
  pqServer* server = smModel->getItemAtIndex<pqServer*>(0);
  return server;
}

//-----------------------------------------------------------------------------
QWidget* pqSierraPlotToolsManager::getMainWindow()
{
  foreach (QWidget* topWidget, QApplication::topLevelWidgets())
  {
    if (qobject_cast<QMainWindow*>(topWidget))
      return topWidget;
  }
  return NULL;
}

//-----------------------------------------------------------------------------
pqView* pqSierraPlotToolsManager::findView(
  pqPipelineSource* source, int port, const QString& viewType)
{
  // Step 1, try to find a view in which the source is already shown.
  if (source)
  {
    foreach (pqView* view, source->getViews())
    {
      pqDataRepresentation* repr = source->getRepresentation(port, view);
      if (repr && repr->isVisible())
        return view;
    }
  }

  // Step 2, check to see if the active view is the right type.
  pqView* view = pqActiveObjects::instance().activeView();

  if (view == NULL)
  {
    qWarning() << "You have the wrong view type... a new view type needs to be created";
    return NULL;
  }

  if (view->getViewType() == viewType)
    return view;

  // Step 3, check all the views and see if one is the right type and not
  // showing anything.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();
  foreach (view, smModel->findItems<pqView*>())
  {
    if (view && (view->getViewType() == viewType) &&
      (view->getNumberOfVisibleRepresentations() < 1))
    {
      return view;
    }
  }

  // Give up.  A new view needs to be created.
  return NULL;
}

//-----------------------------------------------------------------------------
pqView* pqSierraPlotToolsManager::getMeshView()
{
  return this->findView(this->getMeshReader(), 0, pqRenderView::renderViewType());
}

//-----------------------------------------------------------------------------
pqView* pqSierraPlotToolsManager::getPlotView()
{
  return this->findView(this->getPlotFilter(), 0, pqXYChartView::XYChartViewType());
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::showSolidMesh()
{
  pqPipelineSource* reader = this->getMeshReader();
  if (!reader)
    return;

  pqView* view = this->getMeshView();
  if (!view)
    return;

  pqDataRepresentation* repr = reader->getRepresentation(0, view);
  if (!repr)
    return;
  vtkSMProxy* reprProxy = repr->getProxy();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqUndoStack* stack = core->getUndoStack();

  if (stack)
    stack->beginUndoSet("Show Solid Mesh");

  pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty("Representation"), "Surface");
  pqSMAdaptor::setEnumerationProperty(
    reprProxy->GetProperty("BackfaceRepresentation"), "Follow Frontface");

  reprProxy->UpdateVTKObjects();

  if (stack)
    stack->endUndoSet();

  view->render();
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSierraPlotToolsManager::findPipelineSource(const char* SMName)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();

  QList<pqPipelineSource*> sources = smModel->findItems<pqPipelineSource*>(this->getActiveServer());
  foreach (pqPipelineSource* s, sources)
  {
    char* xmlName = s->getProxy()->GetXMLName();
    if (strcmp(xmlName, SMName) == 0)
      return s;
  }

  return NULL;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSierraPlotToolsManager::getMeshReader()
{
  return this->findPipelineSource("ExodusIIReader");
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSierraPlotToolsManager::getPlotFilter()
{
  return this->findPipelineSource("ProbeLine");
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSierraPlotToolsManager::getGlobalVariablesPlotOverTimeFilter()
{
  return this->findPipelineSource("ExtractFieldDataOverTime");
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSierraPlotToolsManager::getSelectionPlotOverTimeFilter()
{
  return this->findPipelineSource("ExtractSelectionOverTime");
}

//-----------------------------------------------------------------------------
static void destroyPortConsumers(pqOutputPort* port)
{
  foreach (pqPipelineSource* consumer, port->getConsumers())
  {
    pqSierraPlotToolsManager::destroyPipelineSourceAndConsumers(consumer);
  }
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::destroyPipelineSourceAndConsumers(pqPipelineSource* source)
{
  if (!source)
    return;

  foreach (pqOutputPort* port, source->getOutputPorts())
  {
    destroyPortConsumers(port);
  }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  builder->destroy(source);
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::showDataLoadManager()
{
  pqSierraPlotToolsDataLoadManager* dialog =
    new pqSierraPlotToolsDataLoadManager(this->getMainWindow());
  dialog->setAttribute(Qt::WA_DeleteOnClose, true);
  QObject::connect(dialog, SIGNAL(createdPipeline()), this, SLOT(checkActionEnabled()));

  dialog->show();
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::setupPlotMenu()
{
  //
  // This method builds the plot menu
  //

  QList<QWidget*> associatedWidgets = this->Internal->Actions.actionPlotVars->associatedWidgets();

  QToolButton* button = NULL;
  for (QList<QWidget*>::iterator iter = associatedWidgets.begin(); iter != associatedWidgets.end();
       iter++)
  {
    QWidget* theWidget = *iter;
    button = dynamic_cast<QToolButton*>(theWidget);
    if (button != NULL)
    {
      break;
    }
  }

  if (button == NULL)
  {
    qWarning() << "Could not find toolbar button";
    return;
  }

  QMenu* plottingMenu = new QMenu();

  QVector<QString>::iterator iterMenu = this->Internal->plotMenuItemsList.begin();
  for (; iterMenu != this->Internal->plotMenuItemsList.end(); iterMenu++)
  {
    QString heading = *iterMenu;

    if (heading == "<dash>")
    {
      plottingMenu->addSeparator();
    }
    else
    {
      QAction* action = plottingMenu->addAction(heading);
      action->setObjectName(heading);

      pqInternal::PlotterMetaData* plot = this->Internal->plotterMap[heading];
      if (plot != NULL)
      {
        action->setEnabled(plot->enabledFlag);
        //
        // Set up a signal/slot for when the user selects this plot choice
        // from the pull-down menu that is shown off of the plot icon/button on
        // the (SierraPlotTools) toolbar.
        //
        QObject::connect(action, SIGNAL(triggered(bool)), this, SLOT(actOnPlotSelection()));
      }
      else
      {
        qWarning() << "* ERROR * Invalid plot action" << heading;
      }
    }
  }

  // This sets the menu for PLOT button on the toolbar
  button->setMenu(plottingMenu);
  button->setPopupMode(QToolButton::InstantPopup);
}

//-----------------------------------------------------------------------------
//
// This method acts on the selection of a plot type
// as a result of the user clicking on the plot button/icon
// (on the SierraPlotTools toolbar), and selecting
// one of the plot types from the pull-down menu.
// So for example, if the user selects "Global var. vs time...",
// "Node var. vs time...", etc, then this method is called.
//
void pqSierraPlotToolsManager::actOnPlotSelection()
{
  QObject* aSender = this->sender();
  QAction* theAction = dynamic_cast<QAction*>(aSender);

  if (theAction != NULL)
  {
    QString heading = dynamic_cast<QAction*>(aSender)->objectName();
    pqInternal::PlotterMetaData* metaData = this->Internal->plotterMap[heading];

    if (this->Internal->plotGUI)
    {
      delete this->Internal->plotGUI;
    }

    //
    // create a dialog that will show various variable and other plotting parameters
    //
    this->Internal->plotGUI = new pqPlotVariablesDialog(this->getMainWindow(), Qt::Dialog);
    this->Internal->plotGUI->setPlotter(metaData->plotter);

    // store the current (meta) plotter info
    this->Internal->currentMetaPlotter = metaData;

    // activate all the variables for this plotter
    // so that they will all show up in the GUI
    pqPipelineSource* meshReader = this->getMeshReader();
    vtkSMProxy* meshReaderProxy = meshReader->getProxy();
    this->Internal->currentMetaPlotter->plotter->setVarsStatus(meshReaderProxy, true);
    meshReaderProxy->UpdateVTKObjects();
    meshReader->updatePipeline();
  }
  else
  {
    qWarning() << "* ERROR * can not translate pull-down menu item into an identifiable action";
    return;
  }

  bool setupResult = this->setupGUIForVars();
  if (!setupResult)
  {
    qCritical()
      << "pqSierraPlotToolsManager::actOnPlotSelection: setup of GUI to show variables failed";
    return;
  }

  //
  // Bring up the dialog used for plotting, for selecting variables and
  //   parameters that affect plots of such
  //
  this->showPlotGUI(this->Internal->plotGUI);
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::showPlotGUI(pqPlotVariablesDialog* plotGUI)
{
  plotGUI->show();

  // connect accepted() signal to a slot here so that we can act on the dialog being accepted
  QObject::connect(
    this->Internal->plotGUI, SIGNAL(accepted()), this, SLOT(slotPlotDialogAccepted()));

  // connect slot, for when variable(s) are deselected -- works for single click as well as swipe
  QObject::connect(this->Internal->plotGUI, SIGNAL(variableDeselectionByName(QString)), this,
    SLOT(slotVariableDeselectionByName(QString)));

  // connect slot, for when variable(s) are selected  -- works for single click as well as swipe
  QObject::connect(this->Internal->plotGUI, SIGNAL(variableSelectionByName(QString)), this,
    SLOT(slotVariableSelectionByName(QString)));

  // connect slot, for when the "use ParaView GUI to Select Nodes" check box is checked
  QObject::connect(this->Internal->plotGUI, SIGNAL(useParaViewGUIToSelectNodesCheck()), this,
    SLOT(slotUseParaViewGUIToSelectNodesCheck()));
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::slotPlotDialogAccepted()
{
  //
  // The dialog was accepted by user hitting the OK button.
  // Now we need to figure out what to... plot

  // First see if any variables were actually selected
  if (this->Internal->plotGUI->areVariablesSelected())
  {
    QList<QListWidgetItem*> selectedItems = this->Internal->plotGUI->getSelectedItems();

    if (!this->createPlotOverTime())
    {
      // some sort of problem
      return;
    }
  }
}

//-----------------------------------------------------------------------------
//
// pqSierraPlotToolsManager::setupGUIForVars:
//
//   This method queries the reader through its proxy to determine various
//   Variable values, attributes, etc, so that the GUI can be filled
//   in with appropriate information.
//
bool pqSierraPlotToolsManager::setupGUIForVars()
{
  pqPipelineSource* meshReader = this->getMeshReader();

  QString smName = meshReader->getSMName();
  // qWarning() << "- INFO - variable processing, working on data named: " << qPrintable(smName);
  vtkSMProxy* meshReaderProxy = meshReader->getProxy();
  vtkSMProperty* prop = NULL;
  prop = this->Internal->currentMetaPlotter->plotter->getSMVariableProperty(meshReaderProxy);

  if (prop != NULL)
  {
    // add variable names for all the variables
    vtkSMStringVectorProperty* stringVecProp = dynamic_cast<vtkSMStringVectorProperty*>(prop);
    if (stringVecProp != NULL)
    {
      unsigned int uNumElems = stringVecProp->GetNumberOfElements();
      unsigned int i;
      for (i = 0; i < uNumElems; i += 2)
      {
        const char* elemPtr = stringVecProp->GetElement(i);
        const char* elemPtr_status = stringVecProp->GetElement(i + 1);
        if (*elemPtr_status == '1')
        {
          this->Internal->plotGUI->addVariable(QString(elemPtr));
        }
      }
    }
  }
  else
  {
    return false;
  }

  vtkSMStringVectorProperty* stringVecProp = NULL;
  unsigned int uNumElems = 0;
  vtkSMSourceProxy* sourceProxy = dynamic_cast<vtkSMSourceProxy*>(meshReaderProxy);
  if (sourceProxy != NULL)
  {
    unsigned int numOutputPorts = sourceProxy->GetNumberOfOutputPorts();

    if (numOutputPorts > 0)
    {

      // NOTE: this line of code assumes that our interest is only on
      // output port 0. Is this always correct?
      vtkSMOutputPort* smOutputPort = sourceProxy->GetOutputPort((unsigned int)(0));

      vtkPVDataInformation* pvDataInfo = smOutputPort->GetDataInformation();
      if (pvDataInfo != NULL)
      {
        double timeMin, timeMax;
        pvDataInfo->GetTimeSpan(timeMin, timeMax);

        this->Internal->plotGUI->setTimeRange(timeMin, timeMax);

        if (prop != NULL)
        {
          // get variable names and their component ranges
          stringVecProp = dynamic_cast<vtkSMStringVectorProperty*>(prop);
          if (stringVecProp != NULL)
          {
            uNumElems = stringVecProp->GetNumberOfElements();
            unsigned int i;
            for (i = 0; i < uNumElems; i += 2)
            {
              const char* elemPtr = stringVecProp->GetElement(i);
              const char* elemPtr_status = stringVecProp->GetElement(i + 1);
              if (*elemPtr_status == '1')
              {
                QString arrayName = QString(elemPtr);

                vtkPVDataSetAttributesInformation* pvDataSetAttributesInformation = NULL;
                vtkPVArrayInformation* arrayInfo = NULL;

                // this gets the data set attributes information for the current plotter
                // i.e. pqNodePlotter, pqGlobalPlotter, etc.
                pvDataSetAttributesInformation =
                  this->Internal->currentMetaPlotter->plotter->getDataSetAttributesInformation(
                    pvDataInfo);

                if (pvDataSetAttributesInformation)
                {
                  arrayInfo =
                    pvDataSetAttributesInformation->GetArrayInformation(qPrintable(arrayName));
                }

                if (arrayInfo != NULL)
                {
                  int numComponents = arrayInfo->GetNumberOfComponents();
                  if (numComponents > 0)
                  {
                    double range[2];
                    // double ranges[3][2];
                    double** ranges;

                    int numberElements = 2;

                    ranges = new double*[numComponents];
                    for (int j = 0; j < numComponents; j++)
                    {
                      ranges[j] = new double[numberElements];
                    }

                    for (int k = 0; k < numComponents; k++)
                    {
                      arrayInfo->GetComponentRange(k, range);
                      ranges[k][0] = range[0];
                      ranges[k][1] = range[1];
                    }

                    // allocate and set the ranges for all the elements
                    // (usually there will be only 2 elements per component -- i.e. min,max)
                    this->Internal->plotGUI->allocSetRange(arrayName, numComponents, 2, ranges);

                    // free up ranges memory
                    for (int k = 0; k < numComponents; k++)
                    {
                      delete[] ranges[k];
                    }
                    delete[] ranges;
                  }
                  else
                  {
                    qWarning() << "* ERROR * " << this->Internal->whoAmI << ": "
                               << "has 0 components " << elemPtr;
                    return false;
                  }

                } // if (arrayInfo != NULL)
                else
                {
                  qWarning() << "* WARNING * " << this->Internal->whoAmI << ": "
                             << "That's odd! pqSierraPlotToolsManager::setupGUIForVars Expected "
                                "arrayInfo for array named "
                             << elemPtr;
                  // return;
                  return false;
                }
              } // if (*elemPtr_status == '1')
            }   // for (i = 0; i < uNumElems; i += 2)
          }     // if (stringVecProp != NULL)
        }       // if (prop != NULL)
      }         // if  (pvDataInfo != NULL)
      else
      {
        qWarning() << "* WARNING * " << this->Internal->whoAmI << ": "
                   << "That's odd! pqSierraPlotToolsManager::setupGUIForVars Expected a valid "
                      "ParaView information object on the mesh reader output port";
        // return;
        return false;
      }
    }
    else
    {
      qWarning() << "* WARNING * " << this->Internal->whoAmI << ": "
                 << "That's odd! pqSierraPlotToolsManager::setupGUIForVars Expected at least one "
                    "output port on the mesh reader";
      // return;
      return false;
    }
  }

  QStringList theVars;
  if (stringVecProp != NULL)
  {
    // get the list of variables names (with component info) to show in the
    // pick frame list widget.
    // For example, a 3-component vector DISPL[3] will get these variables
    // to show in the listbox:
    //    DISPL_x
    //    DISPL_y
    //    DISPL_z
    // And a 6-component vector SIG[6] will get these variables
    // to show in the listbox:
    //    SIG_xx
    //    SIG_xy
    //    SIG_xz
    //    SIG_yy
    //    SIG_yz
    //    SIG_zz

    theVars = this->Internal->plotGUI->getVarsWithComponentSuffixes(stringVecProp);
  }

  //
  // Fill the list box with variable names
  //
  this->Internal->plotGUI->setupVariablesList(theVars);

  // possibly activated the selection by number widgets
  this->Internal->plotGUI->activateSelectionByNumberFrame();

  // possibly set the selection by numbers label
  QString numberItemsLabel = this->Internal->currentMetaPlotter->plotter->getNumberItemsLabel();
  this->Internal->plotGUI->setNumberItemsLabel(numberItemsLabel);

  // set the text heading for this gui
  this->Internal->plotGUI->setHeading(
    this->Internal->StripDotDotDot(this->Internal->currentMetaPlotter->header));

  // if we get here, assume success
  return true;
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::checkActionEnabled()
{
  pqPipelineSource* meshReader = this->getMeshReader();

  if (!meshReader)
  {
    // If we get to this point than the mesh reader has not been instatiated, i.e. the
    // data has not been read in yet
    this->actionPlotVars()->setEnabled(false);
    this->actionSolidMesh()->setEnabled(false);
    this->actionWireframeSolidMesh()->setEnabled(false);
    this->actionWireframeAndBackMesh()->setEnabled(false);
    // this->actionPlotOverTime()->setEnabled(false);

    // turn DEBUG button on
    this->actionPlotDEBUG()->setEnabled(true);
  }
  else
  {
    //
    // in theory, the data has been read in by this point, and is valid
    //

    //
    // activate some GUI elements
    //

    this->actionPlotVars()->setEnabled(true);

    // Set up the pull-down menu for plot types
    this->setupPlotMenu();

    this->actionSolidMesh()->setEnabled(true);
    this->actionWireframeSolidMesh()->setEnabled(true);
    this->actionWireframeAndBackMesh()->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::showWireframeSolidMesh()
{
  pqPipelineSource* reader = this->getMeshReader();
  if (!reader)
    return;

  pqView* view = this->getMeshView();
  if (!view)
    return;

  pqDataRepresentation* repr = reader->getRepresentation(0, view);
  if (!repr)
    return;
  vtkSMProxy* reprProxy = repr->getProxy();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqUndoStack* stack = core->getUndoStack();

  if (stack)
    stack->beginUndoSet("Show Wireframe Mesh");

  pqSMAdaptor::setEnumerationProperty(
    reprProxy->GetProperty("Representation"), "Surface With Edges");
  pqSMAdaptor::setEnumerationProperty(
    reprProxy->GetProperty("BackfaceRepresentation"), "Follow Frontface");

  reprProxy->UpdateVTKObjects();

  if (stack)
    stack->endUndoSet();

  view->render();
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::showWireframeAndBackMesh()
{
  pqPipelineSource* reader = this->getMeshReader();
  if (!reader)
    return;

  pqView* view = this->getMeshView();
  if (!view)
    return;

  pqDataRepresentation* repr = reader->getRepresentation(0, view);
  if (!repr)
    return;
  vtkSMProxy* reprProxy = repr->getProxy();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqUndoStack* stack = core->getUndoStack();

  if (stack)
    stack->beginUndoSet("Show Wireframe Front and Solid Back");

  pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty("Representation"), "Wireframe");
  pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty("BackfaceRepresentation"), "Surface");

  reprProxy->UpdateVTKObjects();

  if (stack)
    stack->endUndoSet();

  view->render();
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::pqInternal::adjustPlotterForPickedVariables(
  pqPipelineSource* meshReader)
{
  QListWidget* listWidget = this->plotGUI->getVariableList();

  QList<QListWidgetItem*> selecteditems = listWidget->selectedItems();
  QList<QListWidgetItem*>::const_iterator iter = selecteditems.begin();

  QMap<QString, QString> vars;
  while (iter != selecteditems.end())
  {
    QString variableAsString = (*iter)->text();
    vars[variableAsString] = this->plotGUI->stripComponentSuffix(variableAsString);
    iter++;
  }

  this->currentMetaPlotter->plotter->setDisplayOfVariables(meshReader, vars);
}

//-----------------------------------------------------------------------------
bool pqSierraPlotToolsManager::createPlotOverTime()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqUndoStack* stack = core->getUndoStack();
  pqDisplayPolicy* displayPolicy = core->getDisplayPolicy();

  pqPipelineSource* meshReader = this->getMeshReader();
  if (!meshReader)
    return false;

  if (stack)
    stack->beginUndoSet("Plot Over time");

  // Determine view.  Do this before deleting existing pipeline objects.
  pqView* plotView = this->getPlotView();

  pqPipelineSource* plotterSource = this->Internal->currentMetaPlotter->plotter->getPlotFilter();

  if (plotterSource == NULL)
  {
    // qWarning() << "pqSierraPlotToolsManager::createPlotOverTime: INFO - NULL plot filter... maybe
    // no view created yet";
  }

  this->destroyPipelineSourceAndConsumers(plotterSource);

  // 9-22-2009: JG: not sure if I need this updatePipeline() method call... but
  // I think leaving it in will not hurt anything
  meshReader->updatePipeline();

  //
  // this section of code toggles the appropriate (global, nodal, etc) variables
  //

  pqPipelineSource* plotFilter = NULL;
  QStringList::const_iterator constIter;
  // bool checkState = this->Internal->plotGUI->getUseParaViewGUIToSelectNodesCheckBoxState();
  //#pragma message (__FILE__ "[" STRING(__LINE__) "]: pqSierraPlotToolsManager::createPlotOverTime:
  // NOTE: Not currently handing plotGUI->getUseParaViewGUIToSelectNodesCheckBoxState()")

  //
  // This section of code first toggles all the variables off for this plotter, then...
  //
  vtkSMProxy* meshReaderProxy = meshReader->getProxy();
  this->Internal->currentMetaPlotter->plotter->setVarsStatus(meshReaderProxy, false);
  QStringList selectedItemsList = this->Internal->plotGUI->getSelectedItemsStringList();
  constIter = selectedItemsList.constBegin();
  // ...this section of code toggles back on the ones the user selected
  while (constIter != selectedItemsList.constEnd())
  {
    QString selIt = *constIter;
    selIt = this->Internal->plotGUI->stripComponentSuffix(selIt);
    this->Internal->currentMetaPlotter->plotter->setVarsActive(meshReaderProxy, selIt, true);
    constIter++;
  }
  meshReaderProxy->UpdateVTKObjects();

  // check selection range
  // NOTE: for plots (e.g. global var vs. time) that don't require a selection
  //   this should automagically return true...
  QList<int> selectedItems;
  if (!this->Internal->withinSelectionRange(meshReader, selectedItems))
  {
    return false;
  }

  // build the inputs
  bool success;
  QMap<QString, QList<pqOutputPort*> > namedInputs =
    this->Internal->currentMetaPlotter->plotter->buildNamedInputs(
      meshReader, selectedItems, success);
  if (!success)
  {
    return false;
  }

  plotFilter =
    builder->createFilter("filters", this->Internal->currentMetaPlotter->plotter->getFilterName(),
      namedInputs, this->getActiveServer());

  if (!plotFilter)
  {
    // for some reason the plotFilter has not been initialized... better bail
    return false;
  }

  //
  // Make representation
  //
  pqDataRepresentation* repr;
  repr = displayPolicy->setRepresentationVisibility(plotFilter->getOutputPort(0), plotView, true);
  (void)repr;

  // UpdateSelfAndAllInputs--
  // Calls UpdateVTKObjects() on self and all proxies that depend on this proxy
  // (through vtkSMProxyProperty properties). It will traverse the dependence
  // tree and update starting from the source. This allows instantiating a whole
  // pipeline (including connectivity) without having to worry about the order
  plotFilter->getProxy()->UpdateSelfAndAllInputs();

  //
  // re-render the view to force the series variables to show and be avail to API
  //
  plotView = this->Internal->currentMetaPlotter->plotter->getPlotView(plotFilter);
  if (!plotView)
    return false;
  plotView->getProxy()->UpdateVTKObjects();
  plotView->forceRender();

  this->Internal->adjustPlotterForPickedVariables(meshReader);

  // re-render
  plotView->render();

  //
  // We have already made the representations and pushed everything to the
  // server manager.  Thus, there is no state left to be modified.
  //
  meshReader->setModifiedState(pqProxy::UNMODIFIED);
  plotFilter->setModifiedState(pqProxy::UNMODIFIED);

  if (stack)
    stack->endUndoSet();

  // presume success if we get to this point
  return true;
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsManager::toggleBackgroundBW()
{
  pqView* view = this->getMeshView();
  if (!view)
    return;
  vtkSMProxy* viewProxy = view->getProxy();

  QList<QVariant> oldBackground;
  QList<QVariant> newBackground;

  oldBackground = pqSMAdaptor::getMultipleElementProperty(viewProxy->GetProperty("Background"));
  if ((oldBackground[0].toDouble() == 0.0) && (oldBackground[1].toDouble() == 0.0) &&
    (oldBackground[2].toDouble() == 0.0))
  {
    newBackground << 1.0 << 1.0 << 1.0;
  }
  else
  {
    newBackground << 0.0 << 0.0 << 0.0;
  }

  pqSMAdaptor::setMultipleElementProperty(viewProxy->GetProperty("Background"), newBackground);

  viewProxy->UpdateVTKObjects();
  view->render();
}
