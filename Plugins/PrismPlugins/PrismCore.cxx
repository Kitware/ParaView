

#include "PrismCore.h"
#include "PrismView.h"
#include "PrismScaleViewDialog.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqActiveView.h"
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
#include "vtkSMSessionProxyManager.h"
#include "pqServerManagerModel.h"
#include "pqSelectionManager.h"
#include "pqDataRepresentation.h"
#include "pqSMAdaptor.h"
#include "pqObjectBuilder.h"
#include "pqCoreUtilities.h"
#include <QApplication>
#include <QActionGroup>
#include <QStyle>
#include <QMessageBox>
#include <QtDebug>
#include <pqFileDialog.h>
#include <QAction>
#include <QMainWindow>
#include "pqUndoStack.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "pqRenderView.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewProxy.h"
#include "pqRenderView.h"
#include <QHeaderView>
#include <QLayout>
#include "pqTreeWidget.h"
#include <QComboBox>
#include <QList>
#include "vtkSMOutputPort.h"

//-----------------------------------------------------------------------------
static PrismCore* Instance = 0;


//-----------------------------------------------------------------------------
PrismTableWidget::PrismTableWidget(QWidget* p)
  : QTableWidget(p)
{
  QObject::connect(this->model(), SIGNAL(rowsInserted(QModelIndex, int, int)),
                   this, SLOT(invalidateLayout()));
  QObject::connect(this->model(), SIGNAL(rowsRemoved(QModelIndex, int, int)),
                   this, SLOT(invalidateLayout()));
  QObject::connect(this->model(), SIGNAL(modelReset()),
                   this, SLOT(invalidateLayout()));

}

//-----------------------------------------------------------------------------
PrismTableWidget::~PrismTableWidget()
{
}



//-----------------------------------------------------------------------------
QSize PrismTableWidget::sizeHint() const
{
  // lets show X items before we get a scrollbar
  // probably want to make this a member variable
  // that a caller has access to
  int maxItemHint = 10;
  // for no items, let's give a space of X pixels
  int minItemHeight = 20;

  int num = this->rowCount() + 1; /* extra room for scroll bar */
  num = qMin(num, maxItemHint);

  int pix = minItemHeight;

  if (num)
    {
    pix = qMax(pix, this->sizeHintForRow(0) * num);
    }

  int margin[4];
  this->getContentsMargins(margin, margin+1, margin+2, margin+3);
  int h = pix + margin[1] + margin[3] + this->horizontalHeader()->frameSize().height();
  return QSize(156, h);
}

//-----------------------------------------------------------------------------
QSize PrismTableWidget::minimumSizeHint() const
{
  return this->sizeHint();
}

//-----------------------------------------------------------------------------
void PrismTableWidget::invalidateLayout()
{
  // sizeHint is dynamic, so we need to invalidate parent layouts
  // when items are added or removed
  for(QWidget* w = this->parentWidget();
      w && w->layout();
      w = w->parentWidget())
    {
    w->layout()->invalidate();
    }
  // invalidate() is not enough, we need to reset the cache of the
  // QWidgetItemV2, so sizeHint() could be recomputed
  this->updateGeometry();
}


SESAMEComboBoxDelegate::SESAMEComboBoxDelegate(QObject *par): QItemDelegate(par)
{

this->SPanel=NULL;
this->PPanel=NULL;
}
void SESAMEComboBoxDelegate::setPanel(PrismSurfacePanel* panel)
{
this->SPanel=panel;
this->PPanel=NULL;
}
void SESAMEComboBoxDelegate::setPanel(PrismPanel* panel)
{
this->PPanel=panel;
this->SPanel=NULL;
}
 void SESAMEComboBoxDelegate::setVariableList(QStringList &variables)
{
  this->Variables=variables;
}

QWidget *SESAMEComboBoxDelegate::createEditor(QWidget *par,
     const QStyleOptionViewItem &/* option */,
     const QModelIndex &/* index */) const
 {
   QComboBox *editor = new QComboBox(par);
   editor->addItems(this->Variables);


   if(this->SPanel)
   {
    QObject::connect(editor, SIGNAL(currentIndexChanged(int)),
        this->SPanel, SLOT(onConversionVariableChanged(int)));
   }
   else if(this->PPanel)
   {
    QObject::connect(editor, SIGNAL(currentIndexChanged(int)),
        this->PPanel, SLOT(onConversionVariableChanged(int)));

   }


   return editor;
 }

 void SESAMEComboBoxDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
 {
   QString value = index.model()->data(index, Qt::DisplayRole).toString();

   QComboBox *comboBox = static_cast<QComboBox*>(editor);
   comboBox->blockSignals(true);
   int cbIndex=comboBox->findText(value);
   comboBox->setCurrentIndex(cbIndex);
   comboBox->blockSignals(false);
 }

 void SESAMEComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
   const QModelIndex &index) const
 {

   QComboBox *comboBox = static_cast<QComboBox*>(editor);
   QString value=comboBox->currentText();
   model->setData(index, value, Qt::EditRole);


 }

void SESAMEComboBoxDelegate::updateEditorGeometry(QWidget *editor,
     const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
 {
     editor->setGeometry(option.rect);
 }


//-----------------------------------------------------------------------------

PrismCore::PrismCore(QObject* p)
:QObject(p)
    {
    
    this->ProcessingEvent=false;
    this->VTKConnections = NULL;

    pqServerManagerModel* model=pqApplicationCore::instance()->getServerManagerModel();

    this->connect(model, SIGNAL(connectionAdded(pqPipelineSource*,pqPipelineSource*, int)),
        this, SLOT(onConnectionAdded(pqPipelineSource*,pqPipelineSource*)));

    this->setParent(model);

    //used to update the selection when the user is selecting something in the pipeline browser
    this->connect(&pqActiveObjects::instance(),
      SIGNAL(sourceChanged(pqPipelineSource*)),
      this, SLOT(onSelectionChanged()));

   //used to determine if the current object is valid after it is created on the server
   pqObjectBuilder *builder=pqApplicationCore::instance()->getObjectBuilder();
    this->connect(builder,SIGNAL(proxyCreated(pqProxy*)),
       this, SLOT(onSelectionChanged()));

    this->onSelectionChanged();
    }

//-----------------------------------------------------------------------------
PrismCore::~PrismCore()
{
  Instance=NULL;  
}

//-----------------------------------------------------------------------------
PrismCore* PrismCore::instance()
{        
  if(!Instance)
    {
    Instance=new PrismCore(NULL);
    }
  return Instance;
}

//-----------------------------------------------------------------------------
void PrismCore::registerActions(QAction* prismView, QAction* sesameSurface,
  QAction* scaleView)
{

  prismView->setText("Prism View");
  prismView->setToolTip("Create Prism View");
  prismView->setIcon(QIcon(":/Prism/Icons/PrismSmall.png"));
  prismView->setEnabled(false);
  QObject::connect(prismView, SIGNAL(triggered(bool)), this, SLOT(onCreatePrismView()));
  QObject::connect(this,SIGNAL(prismViewCreatable(bool)),prismView, SLOT(setEnabled(bool)));

  sesameSurface->setText("SESAME Surface");
  sesameSurface->setToolTip("Open SESAME Surface");
  sesameSurface->setIcon(QIcon(":/Prism/Icons/CreateSESAME.png"));
  QObject::connect(sesameSurface, SIGNAL(triggered(bool)), this, SLOT(onSESAMEFileOpen()));

  scaleView->setText("Change Prism View Scale");
  scaleView->setToolTip("Change Prism View Scale");
  scaleView->setIcon(QIcon(":/Prism/Icons/PrismViewScale.png"));
  QObject::connect(scaleView, SIGNAL(triggered(bool)), this, SLOT(onChangePrismViewScale()));
}

//-----------------------------------------------------------------------------
pqPipelineSource* PrismCore::getActiveSource() const
{
  return pqActiveObjects::instance().activeSource();
}

//-----------------------------------------------------------------------------
pqServer* PrismCore::getActiveServer() const
{
  pqApplicationCore* core = pqApplicationCore::instance();

  pqServer* server=core->getActiveServer();
  return server;
}

//-----------------------------------------------------------------------------
void PrismCore::onSESAMEFileOpen()
    {
    // Get the list of selected sources.

    pqServer* server = this->getActiveServer();

    if(!server)
        {
        qDebug() << "No active server selected.";
        }

    QString filters = "All files (*)";
    pqFileDialog dialog(server, pqCoreUtilities::mainWidget(), tr("Open File"), QString(), filters);
    dialog.setModal(true);
    dialog.setObjectName("FileOpenDialog");
    dialog.setFileMode(pqFileDialog::ExistingFile);
    if (QDialog::Accepted == dialog.exec())
      {
      this->onSESAMEFileOpen(dialog.getAllSelectedFiles()[0]);
      }

    }

//-----------------------------------------------------------------------------
void PrismCore::onSESAMEFileOpen(const QStringList& files)
    {
    if (files.empty())
        {
        return ;
        }

    pqApplicationCore* core = pqApplicationCore::instance();
    pqObjectBuilder* builder = core->getObjectBuilder();

    pqServer* server = this->getActiveServer();
    if(!server)
        {
        qCritical() << "Cannot create reader without an active server.";
        return ;
        }

    builder->createView("PrismView",server);


    pqUndoStack *stack=core->getUndoStack();
    if(stack)
        {
        stack->beginUndoSet("Open Prism Surface");
        }

    builder->createReader("sources", "PrismSurfaceReader", files, server);
    if(stack)
        {
        stack->endUndoSet();
        }  
    }

//-----------------------------------------------------------------------------
void PrismCore::onCreatePrismView()
{
    // Get the list of selected sources.

    pqPipelineSource* source = 0;

    source = this->getActiveSource();   

    if(!source)
    {
        QMessageBox::warning(NULL, tr("No Object Selected"),
            tr("No pipeline object is selected.\n"
            "Please select a pipeline object from the list on the left."),
            QMessageBox::Ok );

        return;
    }   
       

    pqServer* server = source->getServer();

    if(!server)
        {
        qDebug() << "No active server selected.";
        return;
        }

    QString filters = "All files (*)";
    pqFileDialog dialog(server, pqCoreUtilities::mainWidget(), tr("Open File"), QString(), filters);
    dialog.setModal(true);
    dialog.setObjectName("FileOpenDialog");
    dialog.setFileMode(pqFileDialog::ExistingFile);
    if (QDialog::Accepted == dialog.exec())
      {
      this->onCreatePrismView(dialog.getAllSelectedFiles()[0]);
      }

    }

//-----------------------------------------------------------------------------
void PrismCore::onCreatePrismView(const QStringList& files)
    {
    // Get the list of selected sources.
    pqApplicationCore* core = pqApplicationCore::instance();
    pqObjectBuilder* builder = core->getObjectBuilder();
    pqPipelineSource* source = 0;
    pqPipelineSource* filter = 0;
    pqServer* server = 0;
    QList<pqOutputPort*> inputs;


    source = this->getActiveSource();

    if(!source)
    {
        QMessageBox::warning(NULL, tr("No Object Selected"),
            tr("No pipeline object is selected.\n"
            "Please select a pipeline object from the list on the left."),
            QMessageBox::Ok );

        return;
    }
    server = source->getServer();
    if(!server)
        {
        qDebug() << "No active server selected.";
        }
    builder->createView("PrismView",server);

    inputs.push_back(source->getOutputPort(0));

    QMap<QString, QList<pqOutputPort*> > namedInputs;
    namedInputs["Input"] = inputs;

    pqUndoStack *stack=core->getUndoStack();
    if(stack)
        {
        stack->beginUndoSet("Create Prism Filter");
        }

    QMap<QString,QVariant> defaultProperties;
    defaultProperties["FileName"]=files;

    filter = builder->createFilter("filters", "PrismFilter", namedInputs, server,defaultProperties);

    //I believe that this is needs to be called twice because there are properties that depend on other properties.
    //Calling it once doesn't set all the properties right.
    filter->setDefaultPropertyValues();
    filter->setDefaultPropertyValues();
    if(stack)
        {
        stack->endUndoSet();
        }  
    }

//-----------------------------------------------------------------------------
void PrismCore::onConnectionAdded(pqPipelineSource* source, 
                                  pqPipelineSource* consumer)
{
  if (consumer)
  {
    QString name=consumer->getProxy()->GetXMLName();
    if(name=="PrismFilter")
    {
      vtkSMSourceProxy* prismP = vtkSMSourceProxy::SafeDownCast(consumer->getProxy());
      vtkSMSourceProxy* sourceP = vtkSMSourceProxy::SafeDownCast(source->getProxy());

      if(this->VTKConnections==NULL)
      {
        this->VTKConnections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
      }

      this->VTKConnections->Connect(sourceP, vtkCommand::SelectionChangedEvent,
        this,
        SLOT(onGeometrySelection(vtkObject*, unsigned long, void*, void*)),prismP);

      this->VTKConnections->Connect(prismP, vtkCommand::SelectionChangedEvent,
        this,
        SLOT(onPrismSelection(vtkObject*, unsigned long, void*, void*)),sourceP);
    }
  }
}

//-----------------------------------------------------------------------------
void PrismCore::onPrismSelection(vtkObject* caller,
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
    pqPipelineSource* pqPrismP=model->findItem<pqPipelineSource*>(prismP);


    vtkSMSourceProxy* selPrism = prismP->GetSelectionInput(portIndex);
    if(!selPrism)
    {
      sourceP->CleanSelectionInputs(0);
      this->ProcessingEvent=false;
      pqPipelineSource* pqSourceP=model->findItem<pqPipelineSource*>(sourceP);
      if(pqSourceP)
      {
        QList<pqView*> Views=pqSourceP->getViews();

        foreach(pqView* v,Views)
        {
          v->render();
        }
      }
      return;
    }

    pqApplicationCore* const core = pqApplicationCore::instance();
    pqSelectionManager* slmanager = qobject_cast<pqSelectionManager*>(
      core->manager("SelectionManager"));
    pqOutputPort* opport = pqPrismP->getOutputPort(portIndex);

    slmanager->select(opport);




    vtkSMSourceProxy* newSource=NULL;
    if (strcmp(selPrism->GetXMLName(), "GlobalIDSelectionSource") != 0)
    {
      newSource = vtkSMSourceProxy::SafeDownCast(
        vtkSMSelectionHelper::ConvertSelection(vtkSelectionNode::GLOBALIDS,
        selPrism,
        prismP,
        portIndex));

      if(!newSource)
      {
        return;
      }
      newSource->UpdateVTKObjects();
   //   prismP->SetSelectionInput(portIndex,newSource, 0);
      selPrism=newSource;
    }



    vtkSMSessionProxyManager* pxm = prismP->GetSessionProxyManager();

    vtkSMSourceProxy* selSource=vtkSMSourceProxy::SafeDownCast(
      pxm->NewProxy("sources", "GlobalIDSelectionSource"));





    pxm->UnRegisterLink(prismP->GetGlobalIDAsString());//TODO we need a unique id that represents the connection.
    //Otherwise we geometry in multiple SESAME views and vise versa.




    vtkSMPropertyLink* link = vtkSMPropertyLink::New();

    // bi-directional link

    link->AddLinkedProperty(selPrism,
      "IDs",
      vtkSMLink::INPUT);
    link->AddLinkedProperty(selSource,
      "IDs",
      vtkSMLink::OUTPUT);
    link->AddLinkedProperty(selSource,
      "IDs",
      vtkSMLink::INPUT);
    link->AddLinkedProperty(selPrism,
      "IDs",
      vtkSMLink::OUTPUT);
    pxm->RegisterLink(prismP->GetGlobalIDAsString(), link);
    link->Delete();




    selSource->UpdateVTKObjects();
    sourceP->SetSelectionInput(0,selSource,0);

    selSource->Delete();

    if(newSource)
    {
      newSource->Delete();
    }


    pqPipelineSource* pqPrismSourceP=model->findItem<pqPipelineSource*>(sourceP);
    QList<pqView*> Views=pqPrismSourceP->getViews();

    foreach(pqView* v,Views)
    {
      v->render();
    }

    this->ProcessingEvent=false;
  }
}

//-----------------------------------------------------------------------------
void PrismCore::onGeometrySelection(vtkObject* caller,
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

    vtkSMSessionProxyManager* pxm = sourceP->GetSessionProxyManager();

    vtkSMSourceProxy* selSource = sourceP->GetSelectionInput(portIndex);
    if(!selSource)
    {
      prismP->CleanSelectionInputs(3);
      this->ProcessingEvent=false;
      pqPipelineSource* pqPrismP=model->findItem<pqPipelineSource*>(prismP);
      if(pqPrismP)
      {
        QList<pqView*> Views=pqPrismP->getViews();

        foreach(pqView* v,Views)
        {
          v->render();
        }
      }
      return;
    }

    pqApplicationCore* const core = pqApplicationCore::instance();
    pqSelectionManager* slmanager = qobject_cast<pqSelectionManager*>(
      core->manager("SelectionManager"));
    pqOutputPort* opport = pqSourceP->getOutputPort(portIndex);

    slmanager->select(opport);



    vtkSMSourceProxy* newSource=NULL;
    if (strcmp(selSource->GetXMLName(), "GlobalIDSelectionSource") != 0)
    {
      newSource = vtkSMSourceProxy::SafeDownCast(
        vtkSMSelectionHelper::ConvertSelection(vtkSelectionNode::GLOBALIDS,
        selSource,
        sourceP,
        portIndex));

      if(!newSource)
      {
        return;
      }
      newSource->UpdateVTKObjects();
   //   sourceP->SetSelectionInput(portIndex,newSource, 0);
      selSource=newSource;
    }


    vtkSMSourceProxy* selPrism=vtkSMSourceProxy::SafeDownCast(
      pxm->NewProxy("sources", "GlobalIDSelectionSource"));




    pxm->UnRegisterLink(prismP->GetGlobalIDAsString());//TODO we need a unique id that represents the connection.
    //Otherwise we geometry in multiple SESAME views and vise versa.



    vtkSMPropertyLink* link = vtkSMPropertyLink::New();

    // bi-directional link

    link->AddLinkedProperty(selSource,
      "IDs",
      vtkSMLink::INPUT);
    link->AddLinkedProperty(selPrism,
      "IDs",
      vtkSMLink::OUTPUT);
    link->AddLinkedProperty(selPrism,
      "IDs",
      vtkSMLink::INPUT);
    link->AddLinkedProperty(selSource,
      "IDs",
      vtkSMLink::OUTPUT);
    pxm->RegisterLink(prismP->GetGlobalIDAsString(), link);
    link->Delete();


    selPrism->UpdateVTKObjects();
    prismP->SetSelectionInput(3,selPrism,0);
    selPrism->Delete();
    if(newSource)
    {
      newSource->Delete();
    }




    pqPipelineSource* pqPrismSourceP=model->findItem<pqPipelineSource*>(prismP);
    QList<pqView*> Views=pqPrismSourceP->getViews();

    foreach(pqView* v,Views)
    {
      v->render();
    }

    this->ProcessingEvent=false;
  }
}

//-----------------------------------------------------------------------------
void PrismCore::onSelectionChanged()
{
  pqPipelineSource *source = pqActiveObjects::instance().activeSource();
  if (source)
    {
    int portNumber = pqActiveObjects::instance().activePort()->getPortNumber();
    vtkSMSessionProxyManager *proxyManager = source->proxyManager();
    proxyManager->InstantiateGroupPrototypes("filters");
    vtkSMProxy* prismFilter = proxyManager->GetProxy("filters_prototypes", "PrismFilter");

    if (prismFilter)
      {
      vtkSMProperty *inputP = prismFilter->GetProperty("Input");
      vtkSMInputProperty* input = vtkSMInputProperty::SafeDownCast(inputP);

      if (input)
        {
        if (input->GetNumberOfProxies() == 1)
          {
          input->SetUncheckedInputConnection(0, source->getProxy(), portNumber);
          }
        else
          {
          input->RemoveAllUncheckedProxies();
          input->AddUncheckedInputConnection(source->getProxy(), portNumber);
          }

        emit this->prismViewCreatable(input->IsInDomains());          
        }
      }
    }
}

//-----------------------------------------------------------------------------
void PrismCore::onChangePrismViewScale()
{
  //first check if the active view is a prism view.
  pqView *view = pqActiveView::instance().current();
  PrismView *pview = qobject_cast<PrismView*>(view);
  if (!pview)
    {
    return;
    }

  QWidget *mainWindow = pqCoreUtilities::mainWidget();
  PrismScaleViewDialog dialog(mainWindow);
  dialog.setView(pview);
  dialog.exec();
}
