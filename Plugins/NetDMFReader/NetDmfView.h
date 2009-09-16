
#ifndef __NetDmfView_h
#define __NetDmfView_h

#include "pqScatterPlotView.h"

/// a simple view that shows GlyphRepresentation representations
class NetDmfView : public pqScatterPlotView
{
  Q_OBJECT
public:
  NetDmfView(const QString& viewtypemodule, 
             const QString& group, 
             const QString& name, 
             vtkSMViewProxy* viewmodule, 
             pqServer* server, 
             QObject* p);
  ~NetDmfView();

protected slots:
  void onRepresentationAdded(pqRepresentation* d);
  void onRepresentationRemoved(pqRepresentation* d);
};

#endif // __NetDmfView_h

