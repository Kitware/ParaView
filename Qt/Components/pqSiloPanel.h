
#ifndef _pqSiloPanel_h
#define _pqSiloPanel_h

#include "pqObjectPanel.h"

// This (mostly empty) class is a place holder until development
// of the silo panel is finished.

class pqSiloPanel : public pqObjectPanel
{
  Q_OBJECT
public:
  pqSiloPanel(pqProxy* proxy, QWidget* p);
  ~pqSiloPanel();

protected slots:

  void pushButton();

protected:

private:

  void buildPanel();

};

#endif

