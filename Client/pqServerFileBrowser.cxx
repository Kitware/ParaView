#include "pqServer.h"
#include "pqServerFileBrowser.h"

pqServerFileBrowser::pqServerFileBrowser(pqServer& Server, QWidget* Parent, const char* const Name) :
  QDialog(Parent)
{
  this->ui.setupUi(this);
  this->setName(Name);
  
  QObject::connect(this->ui.buttonUp, SIGNAL(clicked()), this->ui.fileList, SLOT(upDir()));
  QObject::connect(this->ui.buttonHome, SIGNAL(clicked()), this->ui.fileList, SLOT(home()));
  QObject::connect(this->ui.fileList, SIGNAL(fileSelected(const QString&)), this, SLOT(onFileSelected(const QString&)));
  
  this->ui.fileList->setProcessModule(Server.GetProcessModule());
}

void pqServerFileBrowser::onFileSelected(const QString& File)
{
  accept();
}

void pqServerFileBrowser::accept()
{
  emit fileSelected(this->ui.fileList->getCurrentPath());
  delete this;
}

void pqServerFileBrowser::reject()
{
  delete this;
}
