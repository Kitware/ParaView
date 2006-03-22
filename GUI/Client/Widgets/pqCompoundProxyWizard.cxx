
#include "pqCompoundProxyWizard.h"

#include <QHeaderView>

#include "pqFileDialog.h"
#include "pqLocalFileDialogModel.h"
#include "pqServer.h"

#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkSMProxyManager.h>

pqCompoundProxyWizard::pqCompoundProxyWizard(pqServer* s, QWidget* p, Qt::WFlags f)
  : QDialog(p, f), Server(s)
{
  this->setupUi(this);
  this->connect(this->LoadButton, SIGNAL(clicked()), SLOT(onLoad()));
  this->connect(this->RemoveButton, SIGNAL(clicked()), SLOT(onRemove()));
  
  this->connect(this, SIGNAL(newCompoundProxy(const QString&, const QString&)),
                      SLOT(addToList(const QString&, const QString&)));

  this->TreeWidget->setColumnCount(1);
  this->TreeWidget->header()->hide();
}

pqCompoundProxyWizard::~pqCompoundProxyWizard()
{
}

void pqCompoundProxyWizard::onLoad()
{
  pqFileDialog* fileDialog = new pqFileDialog(
    new pqLocalFileDialogModel(), 
    tr("Open Compound Proxy File:"),
    this,
    "fileOpenDialog");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);

  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList&)),
                this, SLOT(onLoad(const QStringList&)));

  fileDialog->show();
}

void pqCompoundProxyWizard::onLoad(const QStringList& files)
{
  foreach(QString file, files)
    {
    vtkPVXMLParser* parser = vtkPVXMLParser::New();
    parser->SetFileName(file.toAscii().data());
    parser->Parse();

    vtkSMProxyManager* pm = pqServer::GetProxyManager();
    pm->LoadCompoundProxyDefinitions(parser->GetRootElement());

    // get names of all the compound proxies   
    // TODO: proxy manager should probably give us a handle back on newly loaded compound proxies
    unsigned int numElems = parser->GetRootElement()->GetNumberOfNestedElements();
    for (unsigned int i=0; i<numElems; i++)
      {
      vtkPVXMLElement* currentElement = parser->GetRootElement()->GetNestedElement(i);
      if (currentElement->GetName() &&
          strcmp(currentElement->GetName(), "CompoundProxyDefinition") == 0)
        {
        const char* name = currentElement->GetAttribute("name");
        emit this->newCompoundProxy(file, name);
        }
      }

    parser->Delete();
    }
}

void pqCompoundProxyWizard::addToList(const QString& file, const QString& proxy)
{
  int numItems = this->TreeWidget->topLevelItemCount();
  QTreeWidgetItem* item = NULL;
  for(int i=0; i<numItems && item == NULL; i++)
    {
    QTreeWidgetItem* tmp = this->TreeWidget->topLevelItem(i);
    if(tmp->text(0) == file)
      {
      item = tmp;
      }
    }

  if(!item)
    {
    item = new QTreeWidgetItem;
    this->TreeWidget->insertTopLevelItem(numItems, item);
    item->setData(0, Qt::DisplayRole, file);
    }

  QTreeWidgetItem* proxyItem = new QTreeWidgetItem(item);
  proxyItem->setData(0, Qt::DisplayRole, proxy);
}

void pqCompoundProxyWizard::onRemove()
{
}

