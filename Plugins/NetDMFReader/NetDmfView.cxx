
#include "NetDmfView.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <vtkSMProxy.h>

#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqServer.h>
#include "vtkSMGlyphRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "GlyphRepresentation.h"

inline void vtkSMProxySetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    proxy->UpdateProperty(pname);
    }
}

NetDmfView::NetDmfView(const QString& viewmoduletype, 
       const QString& group, 
       const QString& name, 
       vtkSMViewProxy* viewmodule, 
       pqServer* server, 
       QObject* p)
 : pqScatterPlotView(group, name, viewmodule, server, p)
{
  /*
  // our view is just a simple QWidget
  this->MyWidget = new QWidget;
  this->MyWidget->setAutoFillBackground(true);
  new QVBoxLayout(this->MyWidget);
  */
  // connect to display creation so we can show them in our view
  this->connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    SLOT(onRepresentationAdded(pqRepresentation*)));
  this->connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    SLOT(onRepresentationRemoved(pqRepresentation*)));
  
  this->set3DMode(true);
}

NetDmfView::~NetDmfView()
{
  //delete this->MyWidget;
}

/*
QWidget* NetDmfView::getWidget()
{
  return this->MyWidget;
}
*/

void NetDmfView::onRepresentationAdded(pqRepresentation* d)
{
  // add representations to each GlyphRepresentation
  GlyphRepresentation* glyphRepresentation = 0;
  foreach (pqRepresentation* rep, this->getRepresentations())
    {
    if (dynamic_cast<GlyphRepresentation*>(rep) != 0)
      {
      glyphRepresentation = dynamic_cast<GlyphRepresentation*>(rep);
      foreach (pqRepresentation* rep, this->getRepresentations())
        {
        if (rep != glyphRepresentation)
          {
          glyphRepresentation->addRepresentation(rep);
          }
        }
      }
    }
}

void NetDmfView::onRepresentationRemoved(pqRepresentation* d)
{
  // add representations to each GlyphRepresentation
  GlyphRepresentation* glyphRepresentation = 0;
  foreach (pqRepresentation* rep, this->getRepresentations())
    {
    if (rep != d && dynamic_cast<GlyphRepresentation*>(rep) != 0)
      {
      glyphRepresentation = dynamic_cast<GlyphRepresentation*>(rep);
      glyphRepresentation->removeRepresentation(d);
      }
    }
}

/*
void NetDmfView::setBackground(const QColor& c)
{
  QPalette p = this->MyWidget->palette();
  p.setColor(QPalette::Window, c);
  this->MyWidget->setPalette(p);
}

QColor NetDmfView::background() const
{
  return this->MyWidget->palette().color(QPalette::Window);
}

bool NetDmfView::canDisplay(pqOutputPort* opPort) const
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


*/
