/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkSourceDialog.cxx

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

=========================================================================*/

/// \file pqLookmarkSourceDialog.cxx
/// \date 12/5/2006

#include "pqLookmarkSourceDialog.h"

#include "pqFlatTreeView.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqServer.h"
#include "pqServerManagerModel2.h"
#include "pqApplicationCore.h"

#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include <QScrollArea>
#include <QSpacerItem>
#include <QString>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QFont>


//----------------------------------------------------------------------------
pqLookmarkSourceDialog::pqLookmarkSourceDialog(QStandardItemModel *lookmarkModel, pqPipelineModel *pipelineModel, QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->LookmarkPipelineModel = 0;
  this->CurrentPipelineModel = 0;
  this->CurrentPipelineView = 0;
  this->CurrentLookmarkItem = 0;
  this->LookmarkPipelineView = 0;
  this->SelectedSource = 0;
  this->CurrentPipelineViewLabel = 0;
  this->OkButton = 0;

  // Set up the base gui elements.
  QGridLayout *baseLayout = new QGridLayout(this);

  // Add labels for the dialog areas.
  this->CurrentPipelineViewLabel = new QLabel("Lookmark Pipeline Preview:", this);
  baseLayout->addWidget(this->CurrentPipelineViewLabel, 0, 0);
  QFrame *divider = new QFrame(this);
  divider->setFrameShadow(QFrame::Sunken);
  divider->setFrameShape(QFrame::VLine);
  baseLayout->addWidget(divider, 0, 1, 3, 1);
  baseLayout->addWidget(new QLabel("Select Source: ", this), 0, 2);

  // Create the preview pane and add it to the right.
  this->LookmarkPipelineView = new pqFlatTreeView(this);
  this->LookmarkPipelineView->setObjectName("Lookmark Pipeline");
  this->LookmarkPipelineView->getHeader()->hide();
  this->LookmarkPipelineView->setMaximumWidth(170);
  this->LookmarkPipelineView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  this->LookmarkPipelineView->setSelectionMode(pqFlatTreeView::NoSelection);
  baseLayout->addWidget(this->LookmarkPipelineView, 1, 0, 2, 1);

  // Create a flat tree view and put it in the middle.
  this->CurrentPipelineView = new pqFlatTreeView(this);
  this->CurrentPipelineView->setObjectName("Current Pipeline");
  this->CurrentPipelineView->getHeader()->hide();
  this->CurrentPipelineView->setMaximumWidth(170);
  this->CurrentPipelineView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  this->CurrentPipelineView->setSelectionMode(pqFlatTreeView::SingleSelection);
  baseLayout->addWidget(this->CurrentPipelineView, 1, 2, 2, 1);

  // Add the separator and the dialog buttons.
  divider = new QFrame(this);
  divider->setFrameShadow(QFrame::Sunken);
  divider->setFrameShape(QFrame::HLine);
  baseLayout->addWidget(divider, 3, 0, 1, 4);

  this->OkButton = new QPushButton("&OK", this);
  this->OkButton->setObjectName("OkButton");
  this->OkButton->setDefault(true);
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  baseLayout->addLayout(buttonLayout, 4, 0, 1, 4);
  buttonLayout->addStretch();
  buttonLayout->addWidget(this->OkButton);

  // Connect the dialog buttons to the appropriate action.
  this->connect(this->OkButton, SIGNAL(clicked()), this, SLOT(accept()));

  this->setWindowTitle("Lookmark Source Chooser");

  // Set up the views using the given models
  this->setModels(lookmarkModel,pipelineModel);
}

void pqLookmarkSourceDialog::setLookmarkSource(QStandardItem *item)
{
  this->CurrentLookmarkItem = item;

  // set the given item's text to bold and the rest to normal
  for(int i=0; i<this->LookmarkPipelineModel->rowCount();i++)
    {
    QFont srcFont = this->LookmarkPipelineModel->item(i)->font();
    QString srcText = this->LookmarkPipelineModel->item(i)->text();
    if(item == this->LookmarkPipelineModel->item(i))
      {
      srcFont.setBold(true);
      }
    else
      {
      srcFont.setBold(false);
      }
    this->LookmarkPipelineModel->item(i)->setFont(srcFont);
    }
}


void pqLookmarkSourceDialog::setModels(QStandardItemModel *lmkModel, pqPipelineModel *currentModel)
{
  if(this->LookmarkPipelineModel == lmkModel && this->CurrentPipelineModel == currentModel)
    {
    return;
    }

  // Clean up the selection pipeline.
  if(this->CurrentPipelineModel)
    {
    this->CurrentPipelineView->setModel(0);
    delete this->CurrentPipelineModel;
    this->CurrentPipelineModel = 0;
    }

  if(this->LookmarkPipelineModel)
    {
    this->LookmarkPipelineView->setModel(0);
    delete this->LookmarkPipelineModel;
    this->LookmarkPipelineModel = 0;
    }

  // Save the pipeline model pointer. If the model is null, don't save
  // the filter.
  this->LookmarkPipelineModel = lmkModel;

  // Add the model to the tree view.
  this->LookmarkPipelineView->setModel(this->LookmarkPipelineModel);
  if(this->LookmarkPipelineModel)
    {
    // Hide all but the first column.
    int columns = this->LookmarkPipelineModel->columnCount();
    for(int i = 1; i < columns; ++i)
      {
      this->LookmarkPipelineView->getHeader()->hideSection(i);
      }
    }
  this->LookmarkPipelineView->expandAll();

  this->CurrentPipelineModel = currentModel;

  // Add the model to the tree view.
  this->CurrentPipelineView->setModel(this->CurrentPipelineModel);
  if(this->CurrentPipelineModel)
    {
    // Hide all but the first column.
    int columns = this->CurrentPipelineModel->columnCount();
    for(int i = 1; i < columns; ++i)
      {
      this->CurrentPipelineView->getHeader()->hideSection(i);
      }

    this->CurrentPipelineModel->setEditable(false);
    this->connect(this->CurrentPipelineModel, SIGNAL(firstChildAdded(const QModelIndex &)),
        this->CurrentPipelineView, SLOT(expand(const QModelIndex &)));
    }
  this->CurrentPipelineView->expandAll();

  // Find and select an initial source (so something is selected)
  pqServerManagerModel2 *model = pqApplicationCore::instance()->getServerManagerModel2();
  QList<pqPipelineSource*> sources = model->findItems<pqPipelineSource*>();
  foreach (pqPipelineSource* src, sources)
    {
    if(!qobject_cast<pqPipelineFilter*>(src))
      {
      this->CurrentPipelineView->setCurrentIndex(this->CurrentPipelineModel->getIndexFor(src));
      this->SelectedSource = src;
      break;
      }
    }

  // Listen to the new selection model.
  this->connect(this->CurrentPipelineView->getSelectionModel(),
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    this, SLOT(selectSource()));

  this->setWindowTitle("Lookmark Source Chooser");
}

void pqLookmarkSourceDialog::selectSource()
{
  // If the selected item is a server, do not set to current:
  QModelIndexList indices = 
       this->CurrentPipelineView->getSelectionModel()->selectedIndexes();
  if(indices.size()==0)
    {
    return;
    }

  pqServer *server = dynamic_cast<pqServer*>(
      this->CurrentPipelineModel->getItemFor(indices.at(0)));

  //pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter*>(this->CurrentPipelineModel->getItemFor(selected.indexes().at(0)));  
  pqPipelineSource *src = dynamic_cast<pqPipelineSource*>(
      this->CurrentPipelineModel->getItemFor(indices.at(0)));  
  if(server) // || filter)
    {
    if(this->SelectedSource)
      {
      this->CurrentPipelineView->setCurrentIndex(this->CurrentPipelineModel->getIndexFor(this->SelectedSource));
      }
    }
  else if(src)
    {
    this->SelectedSource = src;
    }
}

