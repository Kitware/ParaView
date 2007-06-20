
#include "MyDisplay.h"

#include <QVBoxLayout>
#include <QLabel>

MyDisplay::MyDisplay(pqRepresentation* d, QWidget* p)
  : pqDisplayPanel(d,p)
{
  // just make a label that shows we made it in the GUI
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(new QLabel("From Plugin", this));
}

MyDisplay::~MyDisplay()
{
}

