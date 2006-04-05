/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

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

    if (parser->GetRootElement())
      {
      vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
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
      }
    else
      {
      //error.
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

