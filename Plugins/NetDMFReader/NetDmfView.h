
#ifndef __NetDmfView_h
#define __NetDmfView_h

#include "pqScatterPlotView.h"

/// a simple view that shows a QLabel with the display's name in the view
class NetDmfView : public pqScatterPlotView
{
  Q_OBJECT
public:
  /// constructor takes a bunch of init stuff and must have this signature to 
    /// satisfy pqView
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

