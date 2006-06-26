/*=========================================================================

   Program: ParaView
   Module:    pqBundleManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqBundleManager.cxx
/// \date 6/23/2006

#include "pqBundleManager.h"
#include "ui_pqBundleManager.h"

#include "pqBundleManagerModel.h"
#include "pqFileDialog.h"
#include "pqLocalFileDialogModel.h"

#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QStringList>

#include "vtkPVXMLElement.h"
#include "vtkSMProxyManager.h"


class pqBundleManagerForm : public Ui::pqBundleManager {};


pqBundleManager::pqBundleManager(pqBundleManagerModel *model,
    QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Model = model;
  this->Form = new pqBundleManagerForm();
  this->Form->setupUi(this);

  // Initialize the form.
  this->Form->ExportButton->setEnabled(false);
  this->Form->RemoveButton->setEnabled(false);
  this->Form->BundleList->setModel(this->Model);

  // Listen for button clicks.
  QObject::connect(this->Form->ImportButton, SIGNAL(clicked()),
      this, SLOT(importFiles()));
  QObject::connect(this->Form->ExportButton, SIGNAL(clicked()),
      this, SLOT(exportSelected()));
  QObject::connect(this->Form->RemoveButton, SIGNAL(clicked()),
      this, SLOT(removeSelected()));
  QObject::connect(this->Form->CloseButton, SIGNAL(clicked()),
      this, SLOT(accept()));

  // Listen for selection changes.
  QObject::connect(this->Form->BundleList->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this,
      SLOT(updateButtons(const QItemSelection &, const QItemSelection &)));

  // Listen for new bundle additions.
  QObject::connect(this->Model, SIGNAL(bundleAdded(const QString &)),
      this, SLOT(selectBundle(const QString &)));
}

pqBundleManager::~pqBundleManager()
{
  delete this->Form;
}

void pqBundleManager::selectBundle(const QString &name)
{
  QModelIndex index = this->Model->getIndexFor(name);
  if(index.isValid())
    {
    this->Form->BundleList->selectionModel()->select(index,
        QItemSelectionModel::SelectCurrent);
    }
}

void pqBundleManager::importFiles(const QStringList &files)
{
  // Clear the current selection. The new bundle definitions will be
  // selected as they're added.
  this->Form->BundleList->selectionModel()->clear();

  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
  QStringList::ConstIterator iter = files.begin();
  for( ; iter != files.end(); ++iter)
    {
    // Load the compound proxy definitions using the server manager.
    // This should trigger some register events, which will update the
    // list of bundles.
    proxyManager->LoadCompoundProxyDefinitions((*iter).toAscii().data());
    }
}

void pqBundleManager::exportSelected(const QStringList &files)
{
  // Get the selected bundles from the list.
  QModelIndexList selection =
      this->Form->BundleList->selectionModel()->selectedIndexes();
  if(selection.size() == 0 || files.size() == 0)
    {
    return;
    }

  // Create the root xml element for the file.
  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->SetName("BundleDefinitions");

  QString bundle;
  vtkPVXMLElement *element = 0;
  vtkPVXMLElement *definition = 0;
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
  QModelIndexList::Iterator iter = selection.begin();
  for( ; iter != selection.end(); ++iter)
    {
    // Get the xml for the bundle. The xml from the server manager
    // needs to be added to a "CompoundProxyDefinition" element. That
    // element needs a name attribute set.
    bundle = this->Model->getBundleName(*iter);
    definition = vtkPVXMLElement::New();
    definition->SetName("CompoundProxyDefinition");
    definition->AddAttribute("name", bundle.toAscii().data());
    element = proxyManager->GetCompoundProxyDefinition(bundle.toAscii().data());
    definition->AddNestedElement(element);
    root->AddNestedElement(definition);
    definition->Delete();
    }

  // Save the bundles in the selected files.
  QStringList::ConstIterator jter = files.begin();
  for( ; jter != files.end(); ++jter)
    {
    ofstream os((*jter).toAscii().data(), ios::out);
    root->PrintXML(os, vtkIndent());
    }

  root->Delete();
}

void pqBundleManager::importFiles()
{
  // Let the user select a file.
  pqFileDialog* fileDialog = new pqFileDialog(
      new pqLocalFileDialogModel(), 
      this,
      tr("Open Bundle File"),
      QString(),
      "Pipeline Bundle Files (*.pbd, *.xml);;All Files (*)");
  fileDialog->setObjectName("FileOpenDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFiles);

  // Listen for the user's selection.
  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList &)),
      this, SLOT(importFiles(const QStringList &)));

  fileDialog->show();
}

void pqBundleManager::exportSelected()
{
  // Let the user select a file to save.
  pqFileDialog* fileDialog = new pqFileDialog(
      new pqLocalFileDialogModel(), 
      this,
      tr("Save Bundle File"),
      QString(),
      "Pipeline Bundle Files (*.pbd, *.xml);;All Files (*)");
  fileDialog->setObjectName("FileSaveDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);

  // Listen for the user's selection.
  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList &)),
      this, SLOT(exportSelected(const QStringList &)));

  fileDialog->show();
}

void pqBundleManager::removeSelected()
{
  // Get the selected bundles from the list.
  QString bundle;
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
  QModelIndexList selection =
      this->Form->BundleList->selectionModel()->selectedIndexes();
  QModelIndexList::Iterator iter = selection.begin();
  for( ; iter != selection.end(); ++iter)
    {
    // Unregister the bundle from the server manager.
    bundle = this->Model->getBundleName(*iter);
    proxyManager->UnRegisterCompoundProxyDefinition(bundle.toAscii().data());
    }
}

void pqBundleManager::updateButtons(const QItemSelection &,
    const QItemSelection &)
{
  // Enable or disable the buttons based on the selection.
  QItemSelectionModel *selection = this->Form->BundleList->selectionModel();
  bool hasSelected = selection->selection().size() > 0;
  this->Form->ExportButton->setEnabled(hasSelected);
  this->Form->RemoveButton->setEnabled(hasSelected);
}


