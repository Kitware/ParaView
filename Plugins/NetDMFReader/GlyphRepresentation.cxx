/// \file GlyphRepresentation.cxx
/// \date 4/24/2006

#include "GlyphRepresentation.h"

#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"

#include "vtkSMDataRepresentationProxy.h"
#include "vtkSMGlyphRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkScatterPlotMapper.h"

#include <QMap>
#include <QStringList>

inline void vtkSMProxySetString(
  vtkSMProxy* proxy, const char* pname, const char* pval)
{
  vtkSMStringVectorProperty* ivp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, pval);
    proxy->UpdateProperty(pname);
    }
}

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
  else
    {
    cerr << "Can't find property: " << pname << endl;
    }
}

class GlyphRepresentation::pqInternal
{
public:
  QMap<QString, pqPipelineSource*> GlyphSources;
};
//-----------------------------------------------------------------------------
GlyphRepresentation::GlyphRepresentation(
  const QString& group,
  const QString& name,
  vtkSMProxy* display,
  pqServer* server, QObject* p/*=null*/):
  Superclass(group, name, display, server, p)
{
  this->Internal = new GlyphRepresentation::pqInternal();
}

//-----------------------------------------------------------------------------
GlyphRepresentation::~GlyphRepresentation()
{
  
}

//-----------------------------------------------------------------------------
void GlyphRepresentation::setDefaultPropertyValues()
{
  vtkSMGlyphRepresentationProxy* repr = 
    vtkSMGlyphRepresentationProxy::SafeDownCast(this->getRepresentationProxy());
  if (!repr)
    {
    return;
    }
  if (repr->InputTypeIsA("vtkDirectedGraph"))
    {
    pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
                                        "Glyph Representation");
    vtkSMProxySetString(repr, "GlyphXOrientationArray", "point,Direction,0");
    vtkSMProxySetString(repr, "GlyphYOrientationArray", "point,Direction,1");
    vtkSMProxySetString(repr, "GlyphZOrientationArray", "point,Direction,2");
    vtkSMProxySetInt(repr, "ThreeDMode", 1);
    vtkSMProxySetInt(repr, "OrientationMode", 1);
    repr->UpdateVTKObjects();
    }
  else
    {
    this->Superclass::setDefaultPropertyValues();
    }
}


//-----------------------------------------------------------------------------
void GlyphRepresentation::addRepresentation(pqRepresentation* rep)
{
  pqDataRepresentation* dataRep = dynamic_cast<pqDataRepresentation*>(rep);
  if (!dataRep)
    {
    return;
    }
  this->Internal->GlyphSources[dataRep->getInput()->getSMName()] = 
    dataRep->getInput();
}

//-----------------------------------------------------------------------------
void GlyphRepresentation::removeRepresentation(pqRepresentation* rep)
{
  pqDataRepresentation* dataRep = dynamic_cast<pqDataRepresentation*>(rep);
  if (!dataRep)
    {
    return;
    }
  this->Internal->GlyphSources.remove(
    this->Internal->GlyphSources.key(dataRep->getInput()));
}

//-----------------------------------------------------------------------------
void GlyphRepresentation::setGlyphInput(pqPipelineSource* source)
{
  vtkSMGlyphRepresentationProxy* glyphRepresentation = 
    vtkSMGlyphRepresentationProxy::SafeDownCast(
      this->getRepresentationProxy());
  glyphRepresentation->SetGlyphInput(
    vtkSMSourceProxy::SafeDownCast(source->getProxy()));
}

//-----------------------------------------------------------------------------
void GlyphRepresentation::setGlyphInput(const QString& glyphInput )
{
  vtkSMGlyphRepresentationProxy* repr = 
    vtkSMGlyphRepresentationProxy::SafeDownCast(this->getRepresentationProxy());
  if (!repr)
    {
    return;
    }
  if (this->Internal->GlyphSources.contains(glyphInput))
    {
    this->setGlyphInput(this->Internal->GlyphSources[glyphInput]);
    }
}

//-----------------------------------------------------------------------------
QStringList GlyphRepresentation::getGlyphSources()
{
  QStringList list;
  foreach (QString str, this->Internal->GlyphSources.keys())
    {
    list << str;
    }
  return list;
}
