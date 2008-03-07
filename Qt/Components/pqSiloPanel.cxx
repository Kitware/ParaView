
#include "pqSiloPanel.h"
#include "pqProxy.h"

#include <QVBoxLayout>
#include <QPushButton>

// This (mostly empty) class is a place holder until development
// of the silo panel is finished.

pqSiloPanel::pqSiloPanel(pqProxy* pxy, QWidget* p)
  : pqObjectPanel(pxy, p)
{
  this->buildPanel();
}

pqSiloPanel::~pqSiloPanel()
{
}

void pqSiloPanel::pushButton()
{
}

void pqSiloPanel::buildPanel()
{
  QVBoxLayout * vbox = new QVBoxLayout;
  QPushButton * button = new QPushButton("Select Subsets");

  vbox->addWidget(button);

  vbox->addStretch(1);
  this->setLayout(vbox);

  connect(button, SIGNAL(clicked()), this, SLOT(pushButton()));
}


