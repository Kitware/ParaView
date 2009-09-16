#include "NetDmfView.h"

#include "GlyphRepresentation.h"
#include "vtkSMGlyphRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqServer.h>

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
  // connect to display creation so we can show them in our view
  this->connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    SLOT(onRepresentationAdded(pqRepresentation*)));
  this->connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    SLOT(onRepresentationRemoved(pqRepresentation*)));
  
  this->set3DMode(true);
}

NetDmfView::~NetDmfView()
{
}

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
