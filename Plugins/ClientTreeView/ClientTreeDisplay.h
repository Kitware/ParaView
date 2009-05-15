/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef ClientTreeDisplay_h
#define ClientTreeDisplay_h

#include "pqDisplayPanel.h"

class ClientTreeDisplay : public pqDisplayPanel
{
  Q_OBJECT

public:
  ClientTreeDisplay(pqRepresentation* display, QWidget* p);
  ~ClientTreeDisplay();

private:
  class implementation;
  implementation* const Implementation;
};

#endif // ClientTreeDisplay_h

