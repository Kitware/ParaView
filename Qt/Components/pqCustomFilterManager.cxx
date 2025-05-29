// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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

  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

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
    parser->SetFileName((*iter).toUtf8().data());
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
          currentElement->SetAttribute("name", newname.toUtf8().data());
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
  while (proxyManager->GetProxyDefinition(group.toUtf8().data(), tempName.toUtf8().data()))
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
  if (selection.empty() || files.empty())
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
    definition->AddAttribute("name", filter.toUtf8().data());
    element = proxyManager->GetProxyDefinition("filters", filter.toUtf8().data());
    if (element)
    {
      definition->AddAttribute("group", "filters");
    }
    else
    {
      element = proxyManager->GetProxyDefinition("sources", filter.toUtf8().data());
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
    vtksys::ofstream os((*jter).toUtf8().data(), ios::out);
    root->PrintXML(os, vtkIndent());
  }

  root->Delete();
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::importFiles()
{
  // Let the user select a file.
  pqFileDialog* fileDialog =
    new pqFileDialog(nullptr, this, tr("Open Custom Filter File"), QString(),
      tr("Custom Filter Files") + QString(" (*.cpd *.xml);;") + tr("All Files") + QString(" (*)"),
      false);
  QObject::connect(fileDialog, &QWidget::close, fileDialog, &QObject::deleteLater);
  fileDialog->setObjectName("FileOpenDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFile);

  // Listen for the user's selection.
  QObject::connect(fileDialog, QOverload<const QStringList&>::of(&pqFileDialog::filesSelected),
    this, QOverload<const QStringList&>::of(&pqCustomFilterManager::importFiles));

  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::exportSelected()
{
  // Let the user select a file to save.
  pqFileDialog* fileDialog =
    new pqFileDialog(nullptr, this, tr("Save Custom Filter File"), QString(),
      tr("Custom Filter Files") + QString(" (*.cpd *.xml);;") + tr("All Files") + QString(" (*)"),
      false);
  QObject::connect(fileDialog, &QWidget::close, fileDialog, &QObject::deleteLater);
  fileDialog->setObjectName("FileSaveDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);

  // Listen for the user's selection.
  QObject::connect(fileDialog, QOverload<const QStringList&>::of(&pqFileDialog::filesSelected),
    this, QOverload<const QStringList&>::of(&pqCustomFilterManager::exportSelected));

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

  Q_FOREACH (QString filter, filters)
  {
    // Unregister the custom filter from the server manager.
    if (proxyManager->GetProxyDefinition("filters", filter.toUtf8().data()))
    {
      proxyManager->UnRegisterCustomProxyDefinition("filters", filter.toUtf8().data());
    }
    else if (proxyManager->GetProxyDefinition("sources", filter.toUtf8().data()))
    {
      proxyManager->UnRegisterCustomProxyDefinition("sources", filter.toUtf8().data());
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterManager::updateButtons(const QItemSelection&, const QItemSelection&)
{
  // Enable or disable the buttons based on the selection.
  QItemSelectionModel* selection = this->Form->CustomFilterList->selectionModel();
  bool hasSelected = !selection->selection().empty();
  this->Form->ExportButton->setEnabled(hasSelected);
  this->Form->RemoveButton->setEnabled(hasSelected);
}
