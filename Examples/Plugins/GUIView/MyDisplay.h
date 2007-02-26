
#ifndef MyDisplay_h
#define MyDisplay_h

#include "pqDisplayPanel.h"

class MyDisplay : public pqDisplayPanel
{
  Q_OBJECT
public:
  MyDisplay(pqDisplay* display, QWidget* p = NULL);
  ~MyDisplay();

};

#endif // MyDisplay_h

