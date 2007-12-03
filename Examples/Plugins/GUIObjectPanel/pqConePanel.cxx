
#include "pqConePanel.h"

#include <QLayout>
#include <QLabel>

#include "vtkSMProxy.h"
#include "pqProxy.h"

pqConePanel::pqConePanel(pqProxy* pxy, QWidget* p)
  : pqLoadedFormObjectPanel(":/MyConePanel/pqConePanel.ui", pxy, p)
{
  this->layout()->addWidget(new QLabel("This is from a plugin", this));
}

pqConePanel::~pqConePanel()
{
}


