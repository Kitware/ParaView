
#ifndef _pqConePanel_h
#define _pqConePanel_h

#include "pqLoadedFormObjectPanel.h"
#include "pqObjectPanelInterface.h"

class pqConePanel : public pqLoadedFormObjectPanel
{
  Q_OBJECT
public:
  pqConePanel(pqProxy* proxy, QWidget* p);
  ~pqConePanel();
};

#endif

