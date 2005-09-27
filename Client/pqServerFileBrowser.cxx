#include "pqServerFileBrowser.h"

#include <vtkProcessModule.h>

pqServerFileBrowser::pqServerFileBrowser()
{
  this->ui.setupUi(this);
  
  QObject::connect(this->ui.buttonUp, SIGNAL(clicked()), this->ui.fileList, SLOT(upDir()));
  QObject::connect(this->ui.buttonHome, SIGNAL(clicked()), this->ui.fileList, SLOT(home()));
  QObject::connect(this->ui.fileList, SIGNAL(fileSelected(const QString&)), this, SLOT(onFileSelected(const QString&)));
  
  this->ui.fileList->setProcessModule(vtkProcessModule::GetProcessModule());
}

void pqServerFileBrowser::onFileSelected(const QString& File)
{
}



