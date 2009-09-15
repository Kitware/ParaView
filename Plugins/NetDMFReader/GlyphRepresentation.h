/// \file GlyphRepresentation.h
/// \date 9/10/2009

#ifndef __GlyphRepresentation_h
#define __GlyphRepresentation_h

#include "pqPipelineRepresentation.h"
class vtkSMDataRepresentationProxy;

/// This is PQ representation for a single display. A pqRepresentation represents
/// a single vtkSMPropRepresentationProxy. The display can be added to
/// only one render module or more (ofcouse on the same server, this class
/// doesn't worry about that.
class GlyphRepresentation : public pqPipelineRepresentation
{
  Q_OBJECT
  typedef pqPipelineRepresentation Superclass;
public:
  // Constructor.
  // \c group :- smgroup in which the proxy has been registered.
  // \c name  :- smname as which the proxy has been registered.
  // \c repr  :- the representation proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  GlyphRepresentation( const QString& group, 
                       const QString& name,
                       vtkSMProxy* repr, 
                       pqServer* server,
                       QObject* parent=NULL);
  virtual ~GlyphRepresentation();
  
  // Sets the default color mapping for the display.
  // The rules are:
  // If the source created a NEW point scalar array, use it.
  // Else if the source created a NEW cell scalar array, use it.
  // Else if the input color by array exists in this source, use it.
  // Else color by property.
  virtual void setDefaultPropertyValues();
  
  void addRepresentation(pqRepresentation* rep);
  void removeRepresentation(pqRepresentation* rep);
  QStringList getGlyphSources();

public slots:
  void setGlyphInput(const QString& glyphInput);
  void setGlyphInput(pqPipelineSource* source);
private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
