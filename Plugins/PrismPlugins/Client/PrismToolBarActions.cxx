

#include "PrismToolBarActions.h"
#include "pqApplicationCore.h"
#include "pqServerManagerSelectionModel.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelection.h"
#include "pqView.h"
#include "pqOutputPort.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "pqServerManagerModel.h"
#include "pqSelectionManager.h"
#include "pqDataRepresentation.h"
#include "pqSMAdaptor.h"
#include <QApplication>
#include <QStyle>
#include <QMessageBox>

PrismToolBarActions::PrismToolBarActions(QObject* p)
  : QActionGroup(p)
{
  this->ProcessingEvent=false;
  this->VTKConnections = vtkEventQtSlotConnect::New();

  this->SesameViewAction = new QAction("SESAME View",this);
  this->SesameViewAction->setToolTip("Open SESAME View");
  this->SesameViewAction->setIcon(QIcon(":/Prism/Icons/CreateSESAME.png"));
  this->addAction(this->SesameViewAction);

  QObject::connect(this->SesameViewAction, SIGNAL(triggered(bool)), this, SLOT(onSESAMEFileOpen()));

  pqServerManagerModel* model=pqApplicationCore::instance()->getServerManagerModel();

  this->connect(model, SIGNAL(connectionAdded(pqPipelineSource*,pqPipelineSource*, int)),
      this, SLOT(onConnectionAdded(pqPipelineSource*,pqPipelineSource*)));

  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  this->connect(selection, SIGNAL(currentChanged(pqServerManagerModelItem*)),
      this, SLOT(onSelectionChanged()));
  this->connect(selection,
      SIGNAL(selectionChanged(const pqServerManagerSelection&, const pqServerManagerSelection&)),
      this, SLOT(onSelectionChanged()));

  this->onSelectionChanged();
}

PrismToolBarActions::~PrismToolBarActions()
{
  this->VTKConnections->Delete();

}

void PrismToolBarActions::onSESAMEFileOpen()
{
  this->createFilterForActiveSource("PrismFilter");
}

//-----------------------------------------------------------------------------
pqPipelineSource* PrismToolBarActions::createFilterForActiveSource(
  const QString& xmlname)
{
  // Get the list of selected sources.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqServerManagerSelection sels = *core->getSelectionModel()->selectedItems();
  pqPipelineSource* source = 0;
  pqPipelineSource* filter = 0;
  pqServerManagerModelItem* item = 0;
  pqServerManagerSelection::ConstIterator iter = sels.begin();

  pqServer* server = 0;
  QList<pqOutputPort*> inputs;


  item = *iter;
  source = dynamic_cast<pqPipelineSource*>(item);   
  inputs.push_back(source->getOutputPort(0));
  if (!server)
    {
    server = source->getServer();
    }

  QMap<QString, QList<pqOutputPort*> > namedInputs;
  namedInputs["Input"] = inputs;
  filter = builder->createFilter("filters", xmlname, namedInputs, server);

  return filter;
}

void PrismToolBarActions::onConnectionAdded(pqPipelineSource* source, 
    pqPipelineSource* consumer)
{
  if (consumer)
    {
    QString name=consumer->getProxy()->GetXMLName();
    if(name=="PrismFilter")
      {
      vtkSMSourceProxy* prismP = vtkSMSourceProxy::SafeDownCast(consumer->getProxy());
      vtkSMSourceProxy* sourceP = vtkSMSourceProxy::SafeDownCast(source->getProxy());

      this->VTKConnections->Connect(sourceP, vtkCommand::SelectionChangedEvent,
        this, 
        SLOT(onGeometrySelection(vtkObject*, unsigned long, void*, void*)),prismP);

      this->VTKConnections->Connect(prismP, vtkCommand::SelectionChangedEvent,
        this, 
        SLOT(onPrismSelection(vtkObject*, unsigned long, void*, void*)),sourceP);


      this->connect(consumer, SIGNAL(representationAdded(pqPipelineSource*, 
        pqDataRepresentation*, int)),
        this, SLOT(onPrismRepresentationAdded(pqPipelineSource*,
        pqDataRepresentation*,
        int)));
      }
    }
}



void PrismToolBarActions::onPrismRepresentationAdded(pqPipelineSource* ,
                                                      pqDataRepresentation* repr,
                                                      int srcOutputPort)
{
  if(srcOutputPort==0)
    {
    pqSMAdaptor::setElementProperty(repr->getProxy()->GetProperty("Pickable"),0);
    }
  
}
void PrismToolBarActions::onPrismSelection(vtkObject* caller,
                                              unsigned long,
                                              void* client_data, void* call_data)
{



  if(!this->ProcessingEvent)
    {
    this->ProcessingEvent=true;

    unsigned int portIndex = *(unsigned int*)call_data;
    vtkSMSourceProxy* prismP=static_cast<vtkSMSourceProxy*>(caller);
    vtkSMSourceProxy* sourceP=static_cast<vtkSMSourceProxy*>(client_data);

    pqServerManagerModel* model= pqApplicationCore::instance()->getServerManagerModel();
    pqPipelineSource* pqPrismSourceP=model->findItem<pqPipelineSource*>(prismP);

    pqOutputPort* opport = pqPrismSourceP->getOutputPort(portIndex);



    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMSourceProxy* selSource =vtkSMSourceProxy::SafeDownCast(
      pxm->NewProxy("sources", "SelectionSource"));

    vtkSMSourceProxy* selPrism = prismP->GetSelectionInput(portIndex);
    

    pqApplicationCore* const core = pqApplicationCore::instance();
    pqSelectionManager* slmanager = qobject_cast<pqSelectionManager*>(
      core->manager("SELECTION_MANAGER"));
     QList<vtkIdType> globalIds= slmanager->getGlobalIDs(selPrism,opport);


   QList<QVariant> ids;
   foreach (vtkIdType gid, globalIds)
      {
      ids.push_back(-1);
      ids.push_back(gid);
      }
    

  
    pqSMAdaptor::setMultipleElementProperty(
       selSource->GetProperty("IDs"), ids);
    pqSMAdaptor::setEnumerationProperty(
       selSource->GetProperty("ContentType"),"GLOBALIDs");
     selSource->GetProperty("FieldType")->Copy(selPrism->GetProperty("FieldType"));
   
     selSource->UpdateVTKObjects();
    sourceP->SetSelectionInput(0,selSource,0);
    selSource->UnRegister(NULL);

    pqPipelineSource* pqSourceP=model->findItem<pqPipelineSource*>(sourceP);
    QList<pqView*> Views=pqSourceP->getViews();

    foreach(pqView* v,Views)
      {
      v->render();
      }


    this->ProcessingEvent=false;
    }


}

void PrismToolBarActions::onGeometrySelection(vtkObject* caller,
                                              unsigned long,
                                              void* client_data, void* call_data)
{

  if(!this->ProcessingEvent)
    {
    this->ProcessingEvent=true;

    unsigned int portIndex = *(unsigned int*)call_data;
    vtkSMSourceProxy* sourceP=static_cast<vtkSMSourceProxy*>(caller);
    vtkSMSourceProxy* prismP=static_cast<vtkSMSourceProxy*>(client_data);

    pqServerManagerModel* model= pqApplicationCore::instance()->getServerManagerModel();
    pqPipelineSource* pqSourceP=model->findItem<pqPipelineSource*>(sourceP);

    pqOutputPort* opport = pqSourceP->getOutputPort(portIndex);



    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMSourceProxy* selPrism =vtkSMSourceProxy::SafeDownCast(
      pxm->NewProxy("sources", "SelectionSource"));

    vtkSMSourceProxy* selSource = sourceP->GetSelectionInput(portIndex);
    

    pqApplicationCore* const core = pqApplicationCore::instance();
    pqSelectionManager* slmanager = qobject_cast<pqSelectionManager*>(
      core->manager("SELECTION_MANAGER"));
     QList<vtkIdType> globalIds= slmanager->getGlobalIDs(selSource,opport);


   QList<QVariant> ids;
   foreach (vtkIdType gid, globalIds)
      {
      ids.push_back(-1);
      ids.push_back(gid);
      }
    

  
    pqSMAdaptor::setMultipleElementProperty(
       selPrism->GetProperty("IDs"), ids);
    pqSMAdaptor::setEnumerationProperty(
       selPrism->GetProperty("ContentType"),"GLOBALIDs");
     selPrism->GetProperty("FieldType")->Copy(selSource->GetProperty("FieldType"));
   
     selPrism->UpdateVTKObjects();
    prismP->SetSelectionInput(1,selPrism,0);
    selPrism->UnRegister(NULL);

    pqPipelineSource* pqPrismSourceP=model->findItem<pqPipelineSource*>(prismP);
    QList<pqView*> Views=pqPrismSourceP->getViews();

    foreach(pqView* v,Views)
      {
      v->render();
      }


    this->ProcessingEvent=false;
    }

}
void PrismToolBarActions::onSelectionChanged()
{
  pqServerManagerModelItem *item = this->getActiveObject();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* prismFilter = proxyManager->GetProxy("filters_prototypes", "PrismFilter");
  
  if (source && prismFilter)
    {
    vtkSMProperty *input = prismFilter->GetProperty("Input");
    if(input)
      {
      pqSMAdaptor::setUncheckedProxyProperty(input, source->getProxy());
      if(input->IsInDomains())
        {
        this->SesameViewAction->setEnabled(true);
        return;
        }
      }
    }

  this->SesameViewAction->setEnabled(false);
}

pqServerManagerModelItem *PrismToolBarActions::getActiveObject() const
{
  pqServerManagerModelItem *item = 0;
  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection *sels = selection->selectedItems();
  if(sels->size() == 1)
    {
    item = sels->first();
    }
  else if(sels->size() > 1)
    {
    item = selection->currentItem();
    if(item && !selection->isSelected(item))
      {
      item = 0;
      }
    }

  return item;
}


