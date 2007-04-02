/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkInspector.cxx

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

#include "pqLookmarkInspector.h"
#include "ui_pqLookmarkInspector.h"

// Qt includes
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QTabWidget>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QMessageBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QIcon>

// ParaView includes
#include "pqApplicationCore.h"
#include <QItemSelectionModel>
#include "pqLookmarkManagerModel.h"
#include "pqLookmarkModel.h"
#include "vtkPVXMLElement.h"

class pqLookmarkInspectorForm : public Ui::pqLookmarkInspector {};


//-----------------------------------------------------------------------------
pqLookmarkInspector::pqLookmarkInspector(pqLookmarkManagerModel *model, QWidget *p)
  : QWidget(p)
{
  this->setObjectName("lookmarkInspector");

  this->Model = model;
  this->Form = new pqLookmarkInspectorForm();
  this->Form->setupUi(this);
  this->Form->PropertiesFrame->hide();
  this->Form->ControlsFrame->hide();
  this->CurrentLookmark = NULL;
  //this->PipelineModel = new QStandardItemModel();
  this->Form->PipelineView->getHeader()->hide();
  this->Form->PipelineView->setSelectionMode(pqFlatTreeView::NoSelection);
  //this->Form->PipelineView->setModel(this->PipelineModel);

  this->connect(this->Form->SaveButton, SIGNAL(clicked()), SLOT(save()));
  this->connect(this->Form->LoadButton, SIGNAL(clicked()), SLOT(load()));
  this->connect(this->Form->DeleteButton, SIGNAL(clicked()), SLOT(remove()));

  this->Form->SaveButton->setEnabled(false);
  this->Form->LoadButton->setEnabled(false);
  this->Form->DeleteButton->setEnabled(false);

  // Disable the restore data button until a fix can be made for the crash that's ocurring as ParaView closes when a lookmark that has this option turned off has been loaded.
  //this->Form->RestoreData->setEnabled(false);

  //this->connect(this->Form->RestoreData, 
  //              SIGNAL(stateChanged(int)),
  //              SIGNAL(modified()));

  this->connect(this->Form->RestoreCamera, 
                SIGNAL(stateChanged(int)),
                SIGNAL(modified()));

  this->connect(this->Form->RestoreTime, 
                SIGNAL(stateChanged(int)),
                SIGNAL(modified()));

  this->connect(this->Form->LookmarkName, 
                SIGNAL(textChanged(const QString &)),
                SIGNAL(modified()));

  this->connect(this->Form->LookmarkComments, 
                SIGNAL(textChanged()),
                SIGNAL(modified()));

  this->connect(this, 
                SIGNAL(modified()),
                SLOT(onModified()));

}

//-----------------------------------------------------------------------------
pqLookmarkInspector::~pqLookmarkInspector()
{
  //delete this->PipelineModel;
  delete this->Form;
}


void pqLookmarkInspector::load()
{
  if(this->CurrentLookmark)
    {
    emit this->loadLookmark(this->CurrentLookmark->getName());
    }
}

//-----------------------------------------------------------------------------
void pqLookmarkInspector::remove()
{ 
  if(this->CurrentLookmark) //this->CurrentSelection.at(0).isValid())
    {
    emit this->removeLookmark(this->CurrentLookmark); //this->BrowserModel->getLookmarkName(this->CurrentSelection.at(0)));
    }

  // this should change the selection in the browser model which will call this->onLookmarkSelectionChanged()
}


void pqLookmarkInspector::save()
{
  if(!this->CurrentLookmark)
    {
    return;
    }

  // make sure the new name is not already taken
  bool nameTaken = false;
  for(int i=0; i<this->Model->getNumberOfLookmarks(); i++)
    {
    pqLookmarkModel *lmk = this->Model->getLookmark(i);
    if(lmk!=this->CurrentLookmark && QString::compare(lmk->getName(),this->Form->LookmarkName->text())==0)
      {
      nameTaken = true;
      break;
      }
    }
  if(nameTaken)
    {
    QMessageBox::warning(this, "Duplicate Name",
          "The lookmark name already exists.\n"
          "Please choose a different one.");

    return;
    }

  this->CurrentLookmark->setName(this->Form->LookmarkName->text());
  this->CurrentLookmark->setDescription(this->Form->LookmarkComments->toPlainText());
  //this->CurrentLookmark->setRestoreDataFlag(this->Form->RestoreData->isChecked());
  this->CurrentLookmark->setRestoreCameraFlag(this->Form->RestoreCamera->isChecked());
  this->CurrentLookmark->setRestoreTimeFlag(this->Form->RestoreTime->isChecked());

  this->Form->SaveButton->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqLookmarkInspector::onModified()
{
  this->Form->SaveButton->setEnabled(true);
}

//-----------------------------------------------------------------------------
void pqLookmarkInspector::onLookmarkSelectionChanged(const QStringList &selected)
{

  if(selected.isEmpty())
    {
    this->CurrentLookmark = 0;
    // don't display anything if nothing is selected
    this->Form->PropertiesFrame->hide();
    this->Form->ControlsFrame->hide();
    this->Form->LoadButton->setEnabled(false);
    this->Form->SaveButton->setEnabled(false);
    this->Form->DeleteButton->setEnabled(false);
    }
  else if(selected.count()>1)
    {
    // only display the lookmark settings that are applicable to multiple lookmarks
    this->Form->PropertiesFrame->hide();
    this->Form->ControlsFrame->show();
    this->Form->LoadButton->setEnabled(false);
    this->Form->SaveButton->setEnabled(false);
    this->Form->DeleteButton->setEnabled(true);
    }  
  else if(selected.count()==1)
    {
    //this->CurrentSelection = selected.indexes();
    this->CurrentLookmark = this->Model->getLookmark(selected.at(0));

    this->Form->LookmarkName->setText(this->CurrentLookmark->getName());
  //  this->Form->LookmarkData->setText(this->BrowserModel->getLookmarkDataName(this->CurrentSelection.at(0)));
    this->Form->LookmarkComments->setText(this->CurrentLookmark->getDescription());
    QImage img;
    img = this->CurrentLookmark->getIcon();
    if(!img.isNull())
      {
      this->Form->LookmarkIcon->setPixmap(QPixmap::fromImage(img));
      }

    this->generatePipelineView();

    //this->Form->RestoreData->setChecked(this->CurrentLookmark->getRestoreDataFlag());
    this->Form->RestoreCamera->setChecked(this->CurrentLookmark->getRestoreCameraFlag());
    this->Form->RestoreTime->setChecked(this->CurrentLookmark->getRestoreTimeFlag());

    this->Form->PropertiesFrame->show();
    this->Form->ControlsFrame->show();
    this->Form->LoadButton->setEnabled(true);
    this->Form->SaveButton->setEnabled(false);
    this->Form->DeleteButton->setEnabled(true);
    }
}


void pqLookmarkInspector::generatePipelineView()
{
  if(!this->CurrentLookmark || !this->CurrentLookmark->getPipelineHierarchy())
    {
    this->Form->PipelineView->hide();
    return;
    }

  //this->PipelineModel->clear();
  //delete this->PipelineModel;
  QStandardItemModel *pipelineModel = new QStandardItemModel();
  //QStandardItem *root = new QStandardItem("server");
  //this->PipelineModel->setItem(0,root);
  this->addChildItems(this->CurrentLookmark->getPipelineHierarchy(),pipelineModel->invisibleRootItem());
  this->Form->PipelineView->setModel(pipelineModel);
  this->Form->PipelineView->expandAll();
  this->Form->PipelineView->show();
/*
  // Hide all but the first column.
  int columns = this->PipelineModel->columnCount();
  for(int i = 1; i < columns; ++i)
    {
    this->Form->PipelineView->getHeader()->hideSection(i);
    }
*/
/*
  if(!this->PipelineView)
    {
    this->PipelineView = new pqFlatTreeView(this);
    }
  this->PipelineView->setObjectName("PipelinePreview");
  this->PipelineView->getHeader()->hide();
  this->PipelineView->setSelectionMode(pqFlatTreeView::NoSelection);
  this->PipelineView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  baseLayout->addWidget(this->PipelineView);
*/

  //delete model;
}


void pqLookmarkInspector::addChildItems(vtkPVXMLElement *elem, QStandardItem *item)
{
  for(unsigned int i=0; i<elem->GetNumberOfNestedElements(); i++)
    {
    vtkPVXMLElement *childElem = elem->GetNestedElement(i);
    // determine icon type:
/*
    QIcon icon = QIcon();
    if(strcmp(childElem->GetName(),"Server")==0)
      {
      icon.addFile(":/pqWidgets/Icons/pqServer16.png");
      }
    if(strcmp(childElem->GetName(),"Source")==0)
      {
      icon.addFile(":/pqWidgets/Icons/pqSource16.png");
      }
    if(strcmp(childElem->GetName(),"Filter")==0)
      {
      icon.addFile(":/pqWidgets/Icons/pqFilter16.png");
      }
*/
    QStandardItem *childItem = new QStandardItem(QIcon(":/pqWidgets/Icons/pqBundle32.png"),QString(childElem->GetAttribute("Name")));
    item->setChild(i,0,childItem);
    this->addChildItems(childElem,childItem);
    }
}


QSize pqLookmarkInspector::sizeHint() const
{
  // return a size hint that would reasonably fit several properties
  ensurePolished();
  QFontMetrics fm(font());
  int h = 20 * (qMax(fm.lineSpacing(), 14));
  int w = fm.width('x') * 25;
  QStyleOptionFrame opt;
  opt.rect = rect();
  opt.palette = palette();
  opt.state = QStyle::State_None;
  return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                    expandedTo(QApplication::globalStrut()), this));
}
