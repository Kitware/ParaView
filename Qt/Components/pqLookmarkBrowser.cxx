/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkBrowser.cxx

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

/// \file pqLookmarkBrowser.cxx
/// \date 6/23/2006

#include "pqLookmarkBrowser.h"
#include "ui_pqLookmarkBrowser.h"

#include "pqLookmarkBrowserModel.h"
#include "pqFileDialog.h"
#include "pqApplicationCore.h"

#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QStringList>


class pqLookmarkBrowserForm : public Ui::pqLookmarkBrowser {};


pqLookmarkBrowser::pqLookmarkBrowser(pqLookmarkBrowserModel *model,
    QWidget *widgetParent)
  : QWidget(widgetParent)
{
  this->Model = model;
  this->Form = new pqLookmarkBrowserForm();
  this->Form->setupUi(this);

  // Initialize the form.
  this->Form->ImportButton->setEnabled(true);
  this->Form->ExportButton->setEnabled(false);
  this->Form->RemoveButton->setEnabled(false);
  this->Form->LookmarkList->setModel(this->Model);

  // Listen for button clicks.
  QObject::connect(this->Form->ImportButton, SIGNAL(clicked()),
      this, SLOT(importFiles()));
  QObject::connect(this->Form->ExportButton, SIGNAL(clicked()),
      this, SLOT(exportSelected()));
  QObject::connect(this->Form->RemoveButton, SIGNAL(clicked()),
      this, SLOT(removeSelected()));

  // Listen for selection changes.
  QObject::connect(this->Form->LookmarkList->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this,
      SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &)));

  // Listen for a lookmark to load.
  QObject::connect(this->Form->LookmarkList,
      SIGNAL(doubleClicked(const QModelIndex &)),
      this,
      SLOT(loadLookmark(const QModelIndex &)));

  // Listen for new lookmark additions.
  QObject::connect(this->Model, SIGNAL(lookmarkAdded(const QString &)),
      this, SLOT(selectLookmark(const QString &)));
}

pqLookmarkBrowser::~pqLookmarkBrowser()
{ 
  delete this->Form;
}

void pqLookmarkBrowser::selectLookmark(const QString &name)
{
  this->Form->LookmarkList->selectionModel()->clearSelection();

  QModelIndex index = this->Model->getIndexFor(name);
  if(index.isValid())
    {
    this->Form->LookmarkList->selectionModel()->select(index,
        QItemSelectionModel::Select);
    }
}


void pqLookmarkBrowser::loadLookmark(const QModelIndex &index)
{
  if(!index.isValid())
    {
    return;
    }
  emit this->loadLookmark(this->Model->getNameFor(index));
}

void pqLookmarkBrowser::importFiles()
{
  // Let the user select a file.
  pqFileDialog* fileDialog = new pqFileDialog(
      NULL,
      this,
      tr("Open Lookmark File"),
      QString(),
      "Lookmark Files (*.lmk *.xml);;All Files (*)");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("FileOpenDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFiles);

  this->Form->LookmarkList->selectionModel()->clear();

  // Listen for the user's selection.
  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList &)),
      this->Model, SIGNAL(importLookmarks(const QStringList &)));

  fileDialog->show();
}

void pqLookmarkBrowser::exportSelected()
{
  // Let the user select a file to save.
  pqFileDialog* fileDialog = new pqFileDialog(
      NULL,
      this,
      tr("Save Lookmark File"),
      QString(),
      "Lookmark Files (*.lmk *.xml);;All Files (*)");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("FileSaveDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);

  // Listen for the user's selection.
  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList &)),
      this, SLOT(exportSelected(const QStringList &)));

  fileDialog->show();
}


void pqLookmarkBrowser::exportSelected(const QStringList &files)
{
  // Get the selected lookmarks from the list.
  QModelIndexList selection =
      this->Form->LookmarkList->selectionModel()->selection().indexes();
  if(selection.size() == 0 || files.size() == 0)
    {
    return;
    }

  this->Model->exportLookmarks(selection, files);
}

void pqLookmarkBrowser::removeSelected()
{
  // Get the selected lookmarks from the list.
  QModelIndexList selection =this->Form->LookmarkList->selectionModel()->selectedIndexes();
  this->Model->removeLookmarks(selection);
}


void pqLookmarkBrowser::updateButtons()
{
  QItemSelectionModel *selection = this->Form->LookmarkList->selectionModel();
  bool hasSelected = selection->selection().size() > 0;

  // Enable or disable the buttons based on the selection.
  this->Form->RemoveButton->setEnabled(hasSelected);
  this->Form->ExportButton->setEnabled(hasSelected);

}

void pqLookmarkBrowser::onSelectionChanged(const QItemSelection &selection,
    const QItemSelection &)
{
  this->updateButtons();

  QStringList names;
  for(int i=0;i<selection.indexes().size();i++)
    {
    names.push_back(this->Model->getNameFor(selection.indexes().at(i)));
    }
  emit this->selectedLookmarksChanged(names);
}

QItemSelectionModel* pqLookmarkBrowser::getSelectionModel()
{ 
  return this->Form->LookmarkList->selectionModel();
}


