
#include "MyView.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <vtkSMProxy.h>
#include <pqDisplay.h>
#include <pqServer.h>
#include <pqPipelineSource.h>

MyView::MyView(const QString& viewmoduletype, 
       const QString& group, 
       const QString& name, 
       vtkSMAbstractViewModuleProxy* viewmodule, 
       pqServer* server, 
       QObject* p)
 : pqGenericViewModule(viewmoduletype, group, name, viewmodule, server, p)
{
  // our view is just a simple QWidget
  this->MyWidget = new QWidget;
  new QVBoxLayout(this->MyWidget);

  // connect to display creation so we can show them in our view
  this->connect(this, SIGNAL(displayAdded(pqDisplay*)),
    SLOT(onDisplayAdded(pqDisplay*)));
  this->connect(this, SIGNAL(displayRemoved(pqDisplay*)),
    SLOT(onDisplayRemoved(pqDisplay*)));
}

MyView::~MyView()
{
  delete this->MyWidget;
}


QWidget* MyView::getWidget()
{
  return this->MyWidget;
}

void MyView::onDisplayAdded(pqDisplay* d)
{
  // add a label with the display id
  QLabel* l = new QLabel(
    QString("Display (%1)").arg(d->getProxy()->GetSelfIDAsString()), 
    this->MyWidget);
  this->MyWidget->layout()->addWidget(l);
  this->Labels.insert(d, l);
}

void MyView::onDisplayRemoved(pqDisplay* d)
{
  // remove the label
  QLabel* l = this->Labels.take(d);
  if(l)
    {
    this->MyWidget->layout()->removeWidget(l);
    delete l;
    }
}

bool MyView::canDisplaySource(pqPipelineSource* source) const
{
  // check valid source and server connections
  if(!source ||
     this->getServer()->GetConnectionID() !=
     source->getServer()->GetConnectionID())
    {
    return false;
    }

  // we can show MyExtractEdges as defined in the server manager xml
  if(QString("MyExtractEdges") == source->getProxy()->GetXMLName())
    {
    return true;
    }

  return false;
}


