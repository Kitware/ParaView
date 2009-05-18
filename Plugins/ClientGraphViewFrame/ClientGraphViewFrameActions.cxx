#include "ClientGraphViewFrameActions.h"

#include <pqActiveView.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqDisplayPolicy.h>
#include <pqMultiViewFrame.h>
#include <pqNameCount.h>
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
#include <vtkPVXMLElement.h>
#include <vtkSelection.h>
#include <vtkSMClientDeliveryRepresentationProxy.h>
#include <vtkSMInputProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMSelectionRepresentationProxy.h>
#include <vtkSMSourceProxy.h>

#include <vtkConvertSelection.h>
#include <vtkConvertSelectionDomain.h>
#include <vtkGraph.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include <QAction>
#include <QIcon>
#include <QtDebug>
#include <QMap>

inline QString pqObjectBuilderGetName(vtkSMProxy* proxy,
  pqNameCount *nameGenerator)
{
  QString label = 
    proxy->GetXMLLabel()? proxy->GetXMLLabel() : proxy->GetXMLName();
  label.remove(' ');
  return QString("%1%2").arg(label).arg(
    nameGenerator->GetCountAndIncrement(label));
}

//-----------------------------------------------------------------------------
ClientGraphViewFrameActions::ClientGraphViewFrameActions(QObject* p)
  : pqViewFrameActionGroup(p)
{
  this->setExclusive(false);
  this->NameGenerator = new pqNameCount;
}

//-----------------------------------------------------------------------------
ClientGraphViewFrameActions::~ClientGraphViewFrameActions()
{
  delete this->NameGenerator;
}

//-----------------------------------------------------------------------------
bool ClientGraphViewFrameActions::connect(pqMultiViewFrame *frame, pqView *view)
{
  if(!view->getViewType().contains("ClientGraphView"))
    return false;

  QAction* action = new QAction("ResetCamera", this);
  action->setData("ResetCamera");
  action->setIcon(QIcon(":ClientGraphViewFrame/Icons/reset_camera_16.png"));
  frame->addTitlebarAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onResetCamera()));

  action = new QAction("ZoomToSelection", this);
  action->setData("ZoomToSelection");
  action->setIcon(QIcon(":ClientGraphViewFrame/Icons/zoom_to_selection_16.png"));
  frame->addTitlebarAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onZoomToSelection()));

  action = new QAction("ExpandSelection", this);
  action->setData("ExpandSelection");
  action->setIcon(QIcon(":ClientGraphViewFrame/Icons/expand_selection_16.png"));
  frame->addTitlebarAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onExpandSelection()));

  action = new QAction("ExtractSubgraph", this);
  action->setData("ExtractSubgraph");
  action->setIcon(QIcon(":ClientGraphViewFrame/Icons/cut_16.png"));
  frame->addTitlebarAction(action);
  QObject::connect(action, SIGNAL(triggered(bool)),
    this, SLOT(onExtractSubgraph()));

  return true;
}

//-----------------------------------------------------------------------------
bool ClientGraphViewFrameActions::disconnect(pqMultiViewFrame *frame, pqView *view)
{
  if(!view->getViewType().contains("ClientGraphView"))
    return false;

  QAction *action = frame->getAction("ResetCamera");
  if(action)
    {
    frame->removeTitlebarAction(action);
    delete action;
    }

  action = frame->getAction("ZoomToSelection");
  if(action)
    {
    frame->removeTitlebarAction(action);
    delete action;
    }

  action = frame->getAction("ExpandSelection");
  if(action)
    {
    frame->removeTitlebarAction(action);
    delete action;
    }

  action = frame->getAction("ExtractSubgraph");
  if(action)
    {
    frame->removeTitlebarAction(action);
    delete action;
    }

  return true;
}


//-----------------------------------------------------------------------------
void ClientGraphViewFrameActions::onResetCamera()
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

  vtkSMPropertyHelper(view->getProxy(), "ResetCamera").Set(1);
  view->getProxy()->UpdateVTKObjects();

  view->render();
}

//-----------------------------------------------------------------------------
void ClientGraphViewFrameActions::onZoomToSelection()
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

  vtkSMPropertyHelper(view->getProxy(), "ZoomToSelection").Set(1);
  view->getProxy()->UpdateVTKObjects();

  view->render();
}

void ClientGraphViewFrameActions::onExtractSubgraph()
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

  pqServer * server = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if(!server)
    {
    qDebug() << "No server present, cannot convert source.";
    }

  // currently graph views only support one input source
  pqDataRepresentation* pqRepr = 0;
  for (int i=0; i<view->getRepresentations().size(); i++)
    {
    pqRepresentation *repr = view->getRepresentations()[i];
    if (repr && repr->isVisible())
      {
      pqRepr = qobject_cast<pqDataRepresentation*>(repr);
      break;
      }
    }
  if(!pqRepr)
    return;

  pqOutputPort* repOpPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* src = vtkSMSourceProxy::SafeDownCast(
    repOpPort->getSource()->getProxy());
  if(!src)
    {
    return;
    }

  vtkSMSourceProxy* selProxy = 0;
  vtkSMSourceProxy* domainMap = 0;
  int useDomainMap = vtkSMPropertyHelper(pqRepr->getProxy(), "UseDomainMap").GetAsInt();
  if(useDomainMap)
    {
    if (vtkSMPropertyHelper(pqRepr->getProxy(), "DomainMap").GetNumberOfElements() > 0)
      {
      domainMap = vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(pqRepr->getProxy(), "DomainMap").GetAsProxy());
      }
    if (domainMap)
      {
      vtkSMProxyManager* pm = vtkSMProxyManager::GetProxyManager();
      vtkSMClientDeliveryRepresentationProxy* delivery =
        vtkSMClientDeliveryRepresentationProxy::SafeDownCast(
          pm->NewProxy("representations", "ClientDeliveryRepresentation"));
      delivery->SetConnectionID(domainMap->GetConnectionID());
      delivery->AddInput(domainMap, "AddInput");
      delivery->Update();
      vtkTable* output = vtkTable::SafeDownCast(delivery->GetOutput());
      if (output)
        {
        vtkSMSelectionDeliveryRepresentationProxy* const proxy = 
          vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(pqRepr->getProxy());
        proxy->GetSelectionRepresentation()->Update();
        proxy->Update();
        vtkSelection* sel = vtkSelection::SafeDownCast(
          proxy->GetSelectionRepresentation()->GetOutput());
        vtkGraph* data = vtkGraph::SafeDownCast(proxy->GetOutput());
        vtkSmartPointer<vtkConvertSelectionDomain> csd = vtkSmartPointer<vtkConvertSelectionDomain>::New();
        csd->SetInput(0, sel);
        csd->SetInput(1, output);
        csd->SetInputConnection(2, proxy->GetOutputPort());
        csd->Update();

        vtkSmartPointer<vtkSelection> converted;
        converted.TakeReference(vtkConvertSelection::ToIndexSelection(
          vtkSelection::SafeDownCast(csd->GetOutput()), proxy->GetOutput()));

        vtkSmartPointer<vtkIdTypeArray> edgeList = vtkSmartPointer<vtkIdTypeArray>::New();
        bool hasEdges = false;
        vtkSmartPointer<vtkIdTypeArray> vertexList = vtkSmartPointer<vtkIdTypeArray>::New();
        bool hasVertices = false;
        for (unsigned int i = 0; i < converted->GetNumberOfNodes(); ++i)
          {
          vtkSelectionNode* node = converted->GetNode(i);
          vtkIdTypeArray* list = 0;
          if (node->GetFieldType() == vtkSelectionNode::VERTEX)
            {
            list = vertexList;
            hasVertices = true;
            }
          else if (node->GetFieldType() == vtkSelectionNode::EDGE)
            {
            list = edgeList;
            hasEdges = true;
            }

          if (list)
            {
            // Append the selection list to the selection
            vtkIdTypeArray* curList = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
            if (curList)
              {
              int inverse = node->GetProperties()->Get(vtkSelectionNode::INVERSE());
              if (inverse)
                {
                vtkIdType num =
                  (node->GetFieldType() == vtkSelectionNode::VERTEX) ?
                  data->GetNumberOfVertices() : data->GetNumberOfEdges();
                for (vtkIdType j = 0; j < num; ++j)
                  {
                  if (curList->LookupValue(j) < 0 && list->LookupValue(j) < 0)
                    {
                    list->InsertNextValue(j);
                    }
                  }
                }
              else
                {
                vtkIdType numTuples = curList->GetNumberOfTuples();
                for (vtkIdType j = 0; j < numTuples; ++j)
                  {
                  vtkIdType curValue = curList->GetValue(j);
                  if (list->LookupValue(curValue) < 0)
                    {
                    list->InsertNextValue(curValue);
                    }
                  }
                }
              } // end if (curList)
            } // end if (list)
          } // end for each child

        if(hasEdges)
          {
          vtkIdType numSelectedEdges = edgeList->GetNumberOfTuples();
          for (vtkIdType i = 0; i < numSelectedEdges; ++i)
            {
            vtkIdType eid = edgeList->GetValue(i);
            vertexList->InsertNextValue(data->GetSourceVertex(eid));
            vertexList->InsertNextValue(data->GetTargetVertex(eid));
            }
          }
        
        vtkSmartPointer<vtkSelection> vertexIndexSelection = vtkSmartPointer<vtkSelection>::New();
        vtkSmartPointer<vtkSelectionNode> vertexIndexSelectionNode = vtkSmartPointer<vtkSelectionNode>::New();
        vertexIndexSelection->AddNode(vertexIndexSelectionNode);
        vertexIndexSelectionNode->SetContentType(vtkSelectionNode::INDICES);
        vertexIndexSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
        vertexIndexSelectionNode->SetSelectionList(vertexList);

        vtkSmartPointer<vtkSelection> vertexPedigreeIdSelection;
        vertexPedigreeIdSelection.TakeReference(vtkConvertSelection::ToPedigreeIdSelection(vertexIndexSelection, proxy->GetOutput()));

        selProxy = pqSelectionManager::createSelectionSource(
          vertexPedigreeIdSelection, src->GetConnectionID());
        }
      delivery->Delete();
      }
    }
  else
    {
    selProxy = src->GetSelectionInput(0);
    }

  pqPendingDisplayManager * pdm = qobject_cast<pqPendingDisplayManager*>(pqApplicationCore::instance()->manager("PENDING_DISPLAY_MANAGER"));

  // Create ExtractSelectedGraph filter
  vtkSMSourceProxy* filterProxy = vtkSMSourceProxy::SafeDownCast(vtkSMProxyManager::GetProxyManager()->NewProxy("filters", "ExtractSelectedGraph"));
  filterProxy->SetConnectionID(server->GetConnectionID());
  QString name = ::pqObjectBuilderGetName(filterProxy,this->NameGenerator);
  vtkSMProxyManager::GetProxyManager()->RegisterProxy("filters", name.toAscii().data(), filterProxy);
  vtkSMInputProperty *selInput = vtkSMInputProperty::SafeDownCast(filterProxy->GetProperty("Input"));
  selInput->AddInputConnection(src,0);
  vtkSMInputProperty *graphInput = vtkSMInputProperty::SafeDownCast(filterProxy->GetProperty("Selection"));
  graphInput->AddInputConnection(selProxy,0);
  filterProxy->UpdatePipeline();

  // Create the FreezeGraph filter
  vtkSMSourceProxy* freezeGraphProxy = vtkSMSourceProxy::SafeDownCast(vtkSMProxyManager::GetProxyManager()->NewProxy("filters", "PassThroughFilter"));
  freezeGraphProxy->SetConnectionID(server->GetConnectionID());
  name = ::pqObjectBuilderGetName(freezeGraphProxy,this->NameGenerator);
  vtkSMProxyManager::GetProxyManager()->RegisterProxy("sources", name.toAscii().data(), freezeGraphProxy);
  vtkSMInputProperty *fgInput = vtkSMInputProperty::SafeDownCast(freezeGraphProxy->GetProperty("Input"));
  fgInput->AddInputConnection(filterProxy,0);
  vtkSMPropertyHelper(freezeGraphProxy,"DeepCopyInput").Set(1);
  freezeGraphProxy->UpdateVTKObjects();
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
  pqView * newView = builder->createView(preferredViewType, server);
  pqDataRepresentation *newRep = builder->createDataRepresentation(pqFreezeGraphProxy->getOutputPort(0), newView);

  // Inherit display properties from original graph
  vtkPVXMLElement* hints = newRep->getProxy()->GetHints();
  hints = hints? hints->FindNestedElementByName("InheritRepresentationProperties") : 0;
  if (hints == 0)
    {
    return;
    }
  pqDataRepresentation* input_repr = pqRepr;
  vtkSMProxy* reprProxy = newRep->getProxy();
  vtkSMProxy* inputReprProxy = input_repr->getProxy();
  unsigned int num_children = hints->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < num_children; cc++)
    {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Property") == 0)
      {
      const char* propname = child->GetAttribute("name");
      if (propname && reprProxy->GetProperty(propname) &&
        inputReprProxy->GetProperty(propname))
        {
        reprProxy->GetProperty(propname)->Copy(
          inputReprProxy->GetProperty(propname));
        }
      }
    }

  if (domainMap)
    {
    vtkSMPropertyHelper(newRep->getProxy(), "DomainMap").Set(domainMap);
    }
  vtkSMPropertyHelper(newRep->getProxy(), "UseDomainMap").Set(useDomainMap);

  newView->render();

  // Cleanup. Disconnect filter inputs.
  fgInput->RemoveAllProxies();
  selInput->RemoveAllProxies();
  graphInput->RemoveAllProxies();
  filterProxy->Delete();
  freezeGraphProxy->Delete();
}

void ClientGraphViewFrameActions::onExpandSelection()
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

  pqServer * server = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if(!server)
    {
    qDebug() << "No server present, cannot convert source.";
    }

  // currently graph views only support one input source
  pqDataRepresentation* pqRepr = 0;
  for (int i=0; i<view->getRepresentations().size(); i++)
    {
    pqRepresentation *repr = view->getRepresentations()[i];
    if (repr && repr->isVisible())
      {
      pqRepr = qobject_cast<pqDataRepresentation*>(repr);
      break;
      }
    }
  if(!pqRepr)
    return;

  pqOutputPort* repOpPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* src = vtkSMSourceProxy::SafeDownCast(
    repOpPort->getSource()->getProxy());
  if(!src)
    {
    return;
    }

  vtkSMSourceProxy* selProxy = 0;
  int useDomainMap = vtkSMPropertyHelper(pqRepr->getProxy(), "UseDomainMap").GetAsInt();
  if(useDomainMap)
    {
    vtkSMSourceProxy* domainMap = 0;
    if (vtkSMPropertyHelper(pqRepr->getProxy(), "DomainMap").GetNumberOfElements() > 0)
      {
      domainMap = vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(pqRepr->getProxy(), "DomainMap").GetAsProxy());
      }
    if (domainMap)
      {
      vtkSMProxyManager* pm = vtkSMProxyManager::GetProxyManager();
      vtkSMClientDeliveryRepresentationProxy* delivery =
        vtkSMClientDeliveryRepresentationProxy::SafeDownCast(
          pm->NewProxy("representations", "ClientDeliveryRepresentation"));
      delivery->SetConnectionID(domainMap->GetConnectionID());
      delivery->AddInput(domainMap, "AddInput");
      delivery->Update();
      vtkTable* output = vtkTable::SafeDownCast(delivery->GetOutput());
      if (output)
        {
        vtkSMSelectionDeliveryRepresentationProxy* const proxy = 
          vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(pqRepr->getProxy());
        proxy->GetSelectionRepresentation()->Update();
        proxy->Update();
        vtkSelection* sel = vtkSelection::SafeDownCast(
          proxy->GetSelectionRepresentation()->GetOutput());
        vtkGraph* data = vtkGraph::SafeDownCast(proxy->GetOutput());
        vtkSmartPointer<vtkConvertSelectionDomain> csd = vtkSmartPointer<vtkConvertSelectionDomain>::New();
        csd->SetInput(0, sel);
        csd->SetInput(1, output);
        csd->SetInputConnection(2, proxy->GetOutputPort());
        csd->Update();

        vtkSmartPointer<vtkSelection> converted;
        converted.TakeReference(vtkConvertSelection::ToIndexSelection(
          vtkSelection::SafeDownCast(csd->GetOutput()), proxy->GetOutput()));

        vtkSmartPointer<vtkIdTypeArray> edgeList = vtkSmartPointer<vtkIdTypeArray>::New();
        bool hasEdges = false;
        vtkSmartPointer<vtkIdTypeArray> vertexList = vtkSmartPointer<vtkIdTypeArray>::New();
        bool hasVertices = false;
        for (unsigned int i = 0; i < converted->GetNumberOfNodes(); ++i)
          {
          vtkSelectionNode* node = converted->GetNode(i);
          vtkIdTypeArray* list = 0;
          if (node->GetFieldType() == vtkSelectionNode::VERTEX)
            {
            list = vertexList;
            hasVertices = true;
            }
          else if (node->GetFieldType() == vtkSelectionNode::EDGE)
            {
            list = edgeList;
            hasEdges = true;
            }

          if (list)
            {
            // Append the selection list to the selection
            vtkIdTypeArray* curList = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
            if (curList)
              {
              int inverse = node->GetProperties()->Get(vtkSelectionNode::INVERSE());
              if (inverse)
                {
                vtkIdType num =
                  (node->GetFieldType() == vtkSelectionNode::VERTEX) ?
                  data->GetNumberOfVertices() : data->GetNumberOfEdges();
                for (vtkIdType j = 0; j < num; ++j)
                  {
                  if (curList->LookupValue(j) < 0 && list->LookupValue(j) < 0)
                    {
                    list->InsertNextValue(j);
                    }
                  }
                }
              else
                {
                vtkIdType numTuples = curList->GetNumberOfTuples();
                for (vtkIdType j = 0; j < numTuples; ++j)
                  {
                  vtkIdType curValue = curList->GetValue(j);
                  if (list->LookupValue(curValue) < 0)
                    {
                    list->InsertNextValue(curValue);
                    }
                  }
                }
              } // end if (curList)
            } // end if (list)
          } // end for each child

        if(hasEdges)
          {
          vtkIdType numSelectedEdges = edgeList->GetNumberOfTuples();
          for (vtkIdType i = 0; i < numSelectedEdges; ++i)
            {
            vtkIdType eid = edgeList->GetValue(i);
            vertexList->InsertNextValue(data->GetSourceVertex(eid));
            vertexList->InsertNextValue(data->GetTargetVertex(eid));
            }
          }
        
        vtkSmartPointer<vtkSelection> vertexIndexSelection = vtkSmartPointer<vtkSelection>::New();
        vtkSmartPointer<vtkSelectionNode> vertexIndexSelectionNode = vtkSmartPointer<vtkSelectionNode>::New();
        vertexIndexSelection->AddNode(vertexIndexSelectionNode);
        vertexIndexSelectionNode->SetContentType(vtkSelectionNode::INDICES);
        vertexIndexSelectionNode->SetFieldType(vtkSelectionNode::VERTEX);
        vertexIndexSelectionNode->SetSelectionList(vertexList);

        vtkSmartPointer<vtkSelection> vertexPedigreeIdSelection;
        vertexPedigreeIdSelection.TakeReference(vtkConvertSelection::ToPedigreeIdSelection(vertexIndexSelection, proxy->GetOutput()));

        selProxy = pqSelectionManager::createSelectionSource(
          vertexPedigreeIdSelection, src->GetConnectionID());
        }
      delivery->Delete();
      }
    }
  else
    {
    selProxy = src->GetSelectionInput(0);
    }

  pqPendingDisplayManager * pdm = qobject_cast<pqPendingDisplayManager*>(pqApplicationCore::instance()->manager("PENDING_DISPLAY_MANAGER"));

  // Create ExtractSelectedGraph filter
  vtkSMSourceProxy* filterProxy = vtkSMSourceProxy::SafeDownCast(vtkSMProxyManager::GetProxyManager()->NewProxy("filters", "ExpandSelectedGraph"));
  filterProxy->SetConnectionID(server->GetConnectionID());
  QString name = ::pqObjectBuilderGetName(filterProxy,this->NameGenerator);
  vtkSMProxyManager::GetProxyManager()->RegisterProxy("filters", name.toAscii().data(), filterProxy);
  vtkSMInputProperty *selInput = vtkSMInputProperty::SafeDownCast(filterProxy->GetProperty("Input"));
  selInput->AddInputConnection(selProxy,0);
  vtkSMInputProperty *graphInput = vtkSMInputProperty::SafeDownCast(filterProxy->GetProperty("Graph"));
  graphInput->AddInputConnection(src,0);
  filterProxy->UpdatePipeline();

  src->SetSelectionInput(0, filterProxy, 0);

  view->render();

  // Cleanup. Disconnect filter inputs.
  selInput->RemoveAllProxies();
  graphInput->RemoveAllProxies();
  filterProxy->Delete();
}
