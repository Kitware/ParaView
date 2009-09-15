
#ifndef __NetDmfDisplay_h
#define __NetDmfDisplay_h

#include "pqDisplayPanel.h"

/// a simple display panel widget
class NetDmfDisplay : public pqDisplayPanel
{
  Q_OBJECT
public:
  /// constructor
  NetDmfDisplay(pqRepresentation* display, QWidget* p = NULL);
  ~NetDmfDisplay();

public slots:
  void zoomToData();
  void cubeAxesVisibilityChanged();
  void editCubeAxes();

protected:
  class pqInternal;
  pqInternal* Internal;
};

#endif // __NetDmfDisplay_h

