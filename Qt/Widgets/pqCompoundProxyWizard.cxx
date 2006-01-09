
#include "pqCompoundProxyWizard.h"
#include "pqFileDialog.h"
#include "pqServerFileDialogModel.h"
#include "pqServer.h"


pqCompoundProxyWizard::pqCompoundProxyWizard(pqServer* s, QWidget* p, Qt::WFlags f)
  : QDialog(p, f), Server(s)
{
  this->setupUi(this);
  this->connect(this->LoadButton, SIGNAL(clicked()), SLOT(onLoad()));
  this->connect(this->RemoveButton, SIGNAL(clicked()), SLOT(onRemove()));
}

pqCompoundProxyWizard::~pqCompoundProxyWizard()
{
  printf("deleted\n");
}

void pqCompoundProxyWizard::onLoad()
{
  pqFileDialog* fileDialog = new pqFileDialog(
    new pqServerFileDialogModel(this->Server->GetProcessModule()), 
    tr("Open Compound Proxy File:"),
    this,
    "fileOpenDialog");

  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList&)),
                this, SLOT(onLoad(const QStringList&)));

  fileDialog->show();
}

void pqCompoundProxyWizard::onLoad(const QStringList& files)
{
}

void pqCompoundProxyWizard::onRemove()
{
}

