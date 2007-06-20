
#ifndef MyDisplay_h
#define MyDisplay_h

#include "pqDisplayPanel.h"

/// a simple display panel widget
class MyDisplay : public pqDisplayPanel
{
  Q_OBJECT
public:

    /// constructor
  MyDisplay(pqRepresentation* display, QWidget* p = NULL);
  ~MyDisplay();

};

#endif // MyDisplay_h

