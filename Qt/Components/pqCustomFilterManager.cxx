/*=========================================================================

   Program: ParaView
   Module:    pqCustomFilterManager.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/

/// \file pqCustomFilterManager.cxx
/// \date 6/23/2006

#include "pqCustomFilterManager.h"
#include "ui_pqCustomFilterManager.h"

#include "pqCustomFilterManagerModel.h"
#include "pqFileDialog.h"

#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QStringList>

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include "vtksys/FStream.hxx"

class pqCustomFilterManagerForm : public Ui::pqCustomFilterManager
{
};

//-----------------------------------------------------------------------------
pqCustomFilterManager::pqCustomFilterManager(
  pqCustomFilterManagerModel* model, QWidget* widgetParent)
  : QDialog(widgetParent)
{
  this->Model = model;
  this->Form = new pqCustomFilterManagerForm();
  this->Form->setupUi(this);

  // Initialize the form.
  this->Form->ExportButton->setEnabled(false);
  this->Form->RemoveButton->setEnabled(false);
  this->Form->CustomFilterList->setModel(this->Model);

  // Listen for button clicks.
  QObject::connect(this->Form->ImportButton, SIGNAL(clicked()), this, SLOT(importFiles()));
  QObject::connect(this->Form->ExportButton, SIGNAL(clicked()), this, SLOT(exportSelected()));
  QObject::connect(this->Form->RemoveButton, SIGNAL(clicked()), this, SLOT(removeSelected()));
  QObject::connect(this->Form->CloseButton, SIGNAL(clicked()), this, SLOT(accept()));

  // Listen for selection changes.
  QObject::connect(this->Form->CustomFilterList->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(updateButtons(const QItemSelection&, const QItemSelection&)));

  // Listen for new custom filter additions.
  QObject::connect(this->Model, SIGNAL(customFilterAdded(const QString&)), this,
    SLOT(selectCustomFilter(const QString&)));
}

//-----------------------------------------------------------------------------
pqCustomFilterManager::~pqCustomFilterManager()
{
  delete this->Form;
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::selectCustomFilter(const QString& name)
{
  QModelIndex index = this->Model->getIndexFor(name);
  if (index.isValid())
  {
    this->Form->CustomFilterList->selectionModel()->select(
      index, QItemSelectionModel::SelectCurrent);
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::importFiles(const QStringList& files)
{
  // Clear the current selection. The new custom filter definitions
  // will be selected as they're added.
  this->Form->CustomFilterList->selectionModel()->clear();

  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  QStringList::ConstIterator iter = files.begin();
  for (; iter != files.end(); ++iter)
  {
    // Make sure name is unique among filters
    // Should this be done in vtkSMProxyManager???
    vtkPVXMLParser* parser = vtkPVXMLParser::New();
    parser->SetFileName((*iter).toLocal8Bit().data());
    parser->Parse();
    vtkPVXMLElement* root = parser->GetRootElement();
    if (!root)
    {
      continue;
    }
    unsigned int numElems = root->GetNumberOfNestedElements();
    for (unsigned int i = 0; i < numElems; i++)
    {
      vtkPVXMLElement* currentElement = root->GetNestedElement(i);
      if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "CustomProxyDefinition") == 0)
      {
        const char* name = currentElement->GetAttribute("name");
        const char* group = currentElement->GetAttribute("group");
        if (name && group)
        {
          QString newname = this->getUnusedFilterName(group, name);
          currentElement->SetAttribute("name", newname.toLocal8Bit().data());
        }
      }
    }

    // Load the custom proxy definitions using the server manager.
    // This should trigger some register events, which will update the
    // list of custom filters.
    proxyManager->LoadCustomProxyDefinitions(root);

    parser->Delete();
  }
}

//----------------------------------------------------------------------------
QString pqCustomFilterManager::getUnusedFilterName(const QString& group, const QString& name)
{
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  QString tempName = name;
  int counter = 1;
  while (
    proxyManager->GetProxyDefinition(group.toLocal8Bit().data(), tempName.toLocal8Bit().data()))
  {
    tempName = QString(name + " (" + QString::number(counter) + ")");
    counter++;
  }

  return tempName;
}

//----------------------------------------------------------------------------
void pqCustomFilterManager::exportSelected(const QStringList& files)
{
  // Get the selected custom filters from the list.
  QModelIndexList selection = this->Form->CustomFilterList->selectionModel()->selectedIndexes();
  if (selection.size() == 0 || files.size() == 0)
  {
    return;
  }

  // Create the root xml element for the file.
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("CustomFilterDefinitions");

  QString filter;
  vtkPVXMLElement* element = nullptr;
  vtkPVXMLElement* definition = nullptr;
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  QModelIndexList::Iterator iter = selection.begin();
  for (; iter != selection.end(); ++iter)
  {
    // Get the xml for the custom filter. The xml from the server
    // manager needs to be added to a "CustomProxyDefinition"
    // element. That element needs a name attribute set.
    filter = this->Model->getCustomFilterName(*iter);
    definition = vtkPVXMLElement::New();
    definition->SetName("CustomProxyDefinition");
    definition->AddAttribute("name", filter.toLocal8Bit().data());
    element = proxyManager->GetProxyDefinition("filters", filter.toLocal8Bit().data());
    if (element)
    {
      definition->AddAttribute("group", "filters");
    }
    else
    {
      element = proxyManager->GetProxyDefinition("sources", filter.toLocal8Bit().data());
      definition->AddAttribute("group", "sources");
    }
    definition->AddNestedElement(element);
    root->AddNestedElement(definition);
    definition->Delete();
  }

  // Save the custom filters in the selected files.
  QStringList::ConstIterator jter = files.begin();
  for (; jter != files.end(); ++jter)
  {
    vtksys::ofstream os((*jter).toLocal8Bit().data(), ios::out);
    root->PrintXML(os, vtkIndent());
  }

  root->Delete();
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::importFiles()
{
  // Let the user select a file.
  pqFileDialog* fileDialog = new pqFileDialog(nullptr, this, tr("Open Custom Filter File"),
    QString(), "Custom Filter Files (*.cpd *.xml);;All Files (*)");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("FileOpenDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFile);

  // Listen for the user's selection.
  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList&)), this,
    SLOT(importFiles(const QStringList&)));

  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::exportSelected()
{
  // Let the user select a file to save.
  pqFileDialog* fileDialog = new pqFileDialog(nullptr, this, tr("Save Custom Filter File"),
    QString(), "Custom Filter Files (*.cpd *.xml);;All Files (*)");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("FileSaveDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);

  // Listen for the user's selection.
  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList&)), this,
    SLOT(exportSelected(const QStringList&)));

  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::removeSelected()
{
  // Get the selected custom filters from the list.
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  QModelIndexList selection = this->Form->CustomFilterList->selectionModel()->selectedIndexes();
  QModelIndexList::Iterator iter = selection.begin();
  QStringList filters;
  for (; iter != selection.end(); ++iter)
  {
    filters.append(this->Model->getCustomFilterName(*iter));
  }

  foreach (QString filter, filters)
  {
    // Unregister the custom filter from the server manager.
    if (proxyManager->GetProxyDefinition("filters", filter.toLocal8Bit().data()))
    {
      proxyManager->UnRegisterCustomProxyDefinition("filters", filter.toLocal8Bit().data());
    }
    else if (proxyManager->GetProxyDefinition("sources", filter.toLocal8Bit().data()))
    {
      proxyManager->UnRegisterCustomProxyDefinition("sources", filter.toLocal8Bit().data());
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::updateButtons(const QItemSelection&, const QItemSelection&)
{
  // Enable or disable the buttons based on the selection.
  QItemSelectionModel* selection = this->Form->CustomFilterList->selectionModel();
  bool hasSelected = selection->selection().size() > 0;
  this->Form->ExportButton->setEnabled(hasSelected);
  this->Form->RemoveButton->setEnabled(hasSelected);
}
