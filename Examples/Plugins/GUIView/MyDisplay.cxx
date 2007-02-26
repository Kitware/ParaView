
#include "MyDisplay.h"

#include <QVBoxLayout>
#include <QLabel>

MyDisplay::MyDisplay(pqDisplay* d, QWidget* p)
  : pqDisplayPanel(d,p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(new QLabel("From Plugin", this));
}

MyDisplay::~MyDisplay()
{
}

