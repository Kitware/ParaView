
#include "MyView.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <vtkSMProxy.h>

#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqServer.h>

MyView::MyView(const QString& viewmoduletype, 
       const QString& group, 
       const QString& name, 
       vtkSMViewProxy* viewmodule, 
       pqServer* server, 
       QObject* p)
 : pqView(viewmoduletype, group, name, viewmodule, server, p)
{
  // our view is just a simple QWidget
  this->MyWidget = new QWidget;
  this->MyWidget->setAutoFillBackground(true);
  new QVBoxLayout(this->MyWidget);

  // connect to display creation so we can show them in our view
  this->connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    SLOT(onRepresentationAdded(pqRepresentation*)));
  this->connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    SLOT(onRepresentationRemoved(pqRepresentation*)));
}

MyView::~MyView()
{
  delete this->MyWidget;
}


QWidget* MyView::getWidget()
{
  return this->MyWidget;
}

void MyView::onRepresentationAdded(pqRepresentation* d)
{
  // add a label with the display id
  QLabel* l = new QLabel(
    QString("Display (%1)").arg(d->getProxy()->GetSelfIDAsString()), 
    this->MyWidget);
  this->MyWidget->layout()->addWidget(l);
  this->Labels.insert(d, l);
}

void MyView::onRepresentationRemoved(pqRepresentation* d)
{
  // remove the label
  QLabel* l = this->Labels.take(d);
  if(l)
    {
    this->MyWidget->layout()->removeWidget(l);
    delete l;
    }
}

void MyView::setBackground(const QColor& c)
{
  QPalette p = this->MyWidget->palette();
  p.setColor(QPalette::Window, c);
  this->MyWidget->setPalette(p);
}

QColor MyView::background() const
{
  return this->MyWidget->palette().color(QPalette::Window);
}

bool MyView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() : 0;
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


