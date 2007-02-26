
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
  this->MyWidget = new QWidget;
  new QVBoxLayout(this->MyWidget);

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
  QLabel* l = new QLabel(
    QString("Display (%1)").arg(d->getProxy()->GetSelfIDAsString()), 
    this->MyWidget);
  this->MyWidget->layout()->addWidget(l);
  this->Labels.insert(d, l);
}

void MyView::onDisplayRemoved(pqDisplay* d)
{
  QLabel* l = this->Labels.take(d);
  if(l)
    {
    this->MyWidget->layout()->removeWidget(l);
    delete l;
    }
}

bool MyView::canDisplaySource(pqPipelineSource* source) const
{
  // always check valid server connections
  if(!source ||
     this->getServer()->GetConnectionID() !=
     source->getServer()->GetConnectionID())
    {
    return false;
    }

  if(QString("MyExtractEdges") == source->getProxy()->GetXMLName())
    {
    return true;
    }

  return false;
}


