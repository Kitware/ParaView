/*=========================================================================

Program: ParaView
Module:    AnnotationManagerPanel.cxx

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

=========================================================================*/
#include "AnnotationManagerPanel.h"
#include "ui_AnnotationManagerPanel.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkAnnotationLink.h"
#include "vtkCommand.h"
#include "vtkEmptyRepresentation.h"
#include "vtkEventQtSlotConnect.h"
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkInformationDoubleVectorKey.h>
#include <vtkInformationIntegerKey.h>
#include <vtkInformationVector.h>
#include "vtkProcessModule.h"
#include "vtkQtAnnotationView.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"

#include "pqAnnotationLayersModel.h"
#include "pqTreeView.h"

#include <QColorDialog>
#include <QHeaderView>
#include <QItemDelegate>
#include <QMessageBox>
#include <QPointer>
#include <QScrollArea>
#include <QtDebug>
#include <QTimer>
#include <QVBoxLayout>

class AnnotationManagerPanelCommand : public vtkCommand
{
public:
  static AnnotationManagerPanelCommand* New()
  { return new AnnotationManagerPanelCommand(); }
  void Execute(vtkObject* caller, unsigned long id, void* callData);
  AnnotationManagerPanel* Target;
};

void AnnotationManagerPanelCommand::Execute(vtkObject* caller, unsigned long, void*)
{
  this->Target->annotationsChanged();
}

//////////////////////////////////////////////////////////////////////////////
// AnnotationManagerPanel::pqImplementation

struct AnnotationManagerPanel::pqImplementation : public Ui::AnnotationManagerPanel
{
public:
  pqImplementation() 
  {
    this->Model = 0;
    this->AnnotationLink = 0;
  }

  ~pqImplementation()
  {
    delete this->Model;
    this->Model = 0;
    if (this->AnnotationLink)
      {
  this->AnnotationLink->Delete();
      }
  }

  pqAnnotationLayersModel *Model;
  vtkSMSourceProxy* AnnotationLink;

};

/////////////////////////////////////////////////////////////////////////////////
              // AnnotationManagerPanel

AnnotationManagerPanel::AnnotationManagerPanel(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->Command = AnnotationManagerPanelCommand::New();
  this->Command->Target = this;

  this->Implementation->setupUi(this);

  this->Implementation->Model = new pqAnnotationLayersModel(this);
  this->Implementation->treeView->setModel(this->Implementation->Model);

  QObject::connect(this->Implementation->Model,
       SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
       this, SLOT(modelChanged()));

  QObject::connect(
       this->Implementation->treeView, SIGNAL(activated(const QModelIndex &)),
       this, SLOT(activateItem(const QModelIndex &)));

  QObject::connect(this->Implementation->upButton, 
       SIGNAL(pressed()), this, SLOT(onUpButtonPressed()));

  QObject::connect(this->Implementation->downButton, 
       SIGNAL(pressed()), this, SLOT(onDownButtonPressed()));

  QObject::connect(this->Implementation->selectButton, 
       SIGNAL(pressed()), this, SLOT(onSelectButtonPressed()));

  QObject::connect(this->Implementation->deleteButton, 
       SIGNAL(pressed()), this, SLOT(onDeleteButtonPressed()));
}

//-----------------------------------------------------------------------------
AnnotationManagerPanel::~AnnotationManagerPanel()
{
  delete this->Implementation;
  this->Command->Delete();
}

//-----------------------------------------------------------------------------
void AnnotationManagerPanel::setAnnotationLink(vtkSMSourceProxy* link)
{
  if (this->Implementation->AnnotationLink != link)
    {
      vtkSMSourceProxy* tempSGMacroVar = this->Implementation->AnnotationLink;
      this->Implementation->AnnotationLink = link;
      if (this->Implementation->AnnotationLink != NULL) 
  { 
    this->Implementation->AnnotationLink->Register(0); 
    vtkAnnotationLink* clientLink = static_cast<vtkAnnotationLink*>(link->GetClientSideObject());
    this->Implementation->Model->setAnnotationLink(clientLink);
  }
      if (tempSGMacroVar != NULL)
  {
    tempSGMacroVar->UnRegister(0);
  }
      this->Implementation->AnnotationLink->AddObserver(vtkCommand::ModifiedEvent, this->Command);
      this->Implementation->AnnotationLink->AddObserver(vtkCommand::UpdateDataEvent, this->Command);
      this->annotationsChanged();
    }
}

//-----------------------------------------------------------------------------
vtkSMSourceProxy* AnnotationManagerPanel::getAnnotationLink()
{
  return this->Implementation->AnnotationLink;
}

int AnnotationManagerPanel::getNumberOfAnnotations()
{
  return this->Implementation->Model->rowCount();
}
int AnnotationManagerPanel::getRowFromName(QString myName)
{
  int ii;
  for(ii = 0; ii<this->Implementation->Model->rowCount(); ii++)
    {
      if(this->Implementation->Model->getAnnotationName(ii) ==
   myName.trimmed())
  return ii;
    }
  return -1;
}
void AnnotationManagerPanel::activateAnnotation(int row, bool onOrOff)
{
  if(row > -1 && row < this->Implementation->Model->rowCount())
    this->Implementation->Model->setAnnotationEnabled(row, onOrOff);
}
void AnnotationManagerPanel::modelChanged()
{
  vtkSMSourceProxy* link = this->getAnnotationLink();
  if (!link)
    {
      return;
    }

  link->RemoveObserver(this->Command);

  link->MarkModified(0);

  link->AddObserver(vtkCommand::ModifiedEvent, this->Command);
  link->AddObserver(vtkCommand::UpdateDataEvent, this->Command);
}

void AnnotationManagerPanel::annotationsChanged()
{
  vtkAnnotationLink* link = static_cast<vtkAnnotationLink*>(this->Implementation->AnnotationLink->GetClientSideObject());

  QObject::disconnect(this->Implementation->Model,
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          this, SLOT(modelChanged()));

  //this->Implementation->Model->setAnnotationLayers(link->GetAnnotationLayers());
  this->Implementation->Model->setAnnotationLink(link);

  QObject::connect(this->Implementation->Model,
       SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
       this, SLOT(modelChanged()));
}

void AnnotationManagerPanel::onUpButtonPressed()
{
  if (!this->getAnnotationLink())
    {
      return;
    }

  vtkAnnotationLink* link = static_cast<vtkAnnotationLink*>(this->Implementation->AnnotationLink->GetClientSideObject());
  vtkAnnotationLayers* annotations = link->GetAnnotationLayers();
  
  QItemSelectionModel *model = this->Implementation->treeView->selectionModel();
  QModelIndexList indexes = model->selectedIndexes();
  if(indexes.size()==0)
    return;
  int removedRow = indexes[0].row();
  if(removedRow == 0)
    return;
  vtkSmartPointer<vtkAnnotationLayers> newAnnotations = vtkSmartPointer<vtkAnnotationLayers>::New();
  vtkSmartPointer<vtkAnnotation> a = vtkSmartPointer<vtkAnnotation>::New();

  a->DeepCopy(annotations->GetAnnotation(removedRow));
  annotations->RemoveAnnotation(annotations->GetAnnotation(removedRow));
  for(int i=0; i<removedRow-1; ++i)
    {
      newAnnotations->AddAnnotation(annotations->GetAnnotation(i)); 
    }
  newAnnotations->AddAnnotation(a);
  for(int i=removedRow-1; i<annotations->GetNumberOfAnnotations(); ++i)
    {
      newAnnotations->AddAnnotation(annotations->GetAnnotation(i)); 
    }
  link->SetAnnotationLayers(newAnnotations);
  this->getAnnotationLink()->MarkModified(0);
}

void AnnotationManagerPanel::onDownButtonPressed()
{
  if (!this->getAnnotationLink())
    {
      return;
    }

  vtkAnnotationLink* link = static_cast<vtkAnnotationLink*>(this->Implementation->AnnotationLink->GetClientSideObject());
  vtkAnnotationLayers* annotations = link->GetAnnotationLayers();
  
  QItemSelectionModel *model = this->Implementation->treeView->selectionModel();
  QModelIndexList indexes = model->selectedIndexes();
  if(indexes.size()==0)
    return;
  int removedRow = indexes[0].row();
  if(removedRow == annotations->GetNumberOfAnnotations()-1)
    return;
  vtkSmartPointer<vtkAnnotationLayers> newAnnotations = vtkSmartPointer<vtkAnnotationLayers>::New();
  vtkSmartPointer<vtkAnnotation> a = vtkSmartPointer<vtkAnnotation>::New();

  a->DeepCopy(annotations->GetAnnotation(removedRow));
  annotations->RemoveAnnotation(annotations->GetAnnotation(removedRow));
  for(int i=0; i<removedRow+1; ++i)
    {
      newAnnotations->AddAnnotation(annotations->GetAnnotation(i));
    }
  newAnnotations->AddAnnotation(a);
  for(int i=removedRow+1; i<annotations->GetNumberOfAnnotations(); ++i)
    {
      newAnnotations->AddAnnotation(annotations->GetAnnotation(i));
    }
  link->SetAnnotationLayers(newAnnotations);
  this->getAnnotationLink()->MarkModified(0);
}

void AnnotationManagerPanel::onSelectButtonPressed()
{
  if (!this->getAnnotationLink())
    {
      return;
    }

  vtkAnnotationLink* link = static_cast<vtkAnnotationLink*>(this->Implementation->AnnotationLink->GetClientSideObject());
  vtkAnnotationLayers* annotations = link->GetAnnotationLayers();
  vtkSmartPointer<vtkSelection> s = vtkSmartPointer<vtkSelection>::New(); 
  QItemSelectionModel *model = this->Implementation->treeView->selectionModel();
  QModelIndexList indexes = model->selectedIndexes();
  foreach (QModelIndex index, indexes)
    {
      vtkSmartPointer<vtkSelection> temp = vtkSmartPointer<vtkSelection>::New(); 
      vtkAnnotation* a = annotations->GetAnnotation(index.row()); 
      temp->DeepCopy(a->GetSelection());
      s->Union(temp);
    }
  
  link->SetCurrentSelection(s);
  this->getAnnotationLink()->MarkModified(0);
}

void AnnotationManagerPanel::onDeleteButtonPressed()
{
  if (!this->getAnnotationLink())
    {
      return;
    }

  vtkAnnotationLink* link = static_cast<vtkAnnotationLink*>(this->Implementation->AnnotationLink->GetClientSideObject());
  vtkAnnotationLayers* annotations = link->GetAnnotationLayers();
  QItemSelectionModel *model = this->Implementation->treeView->selectionModel();
  QModelIndexList indexes = model->selectedIndexes();
  if(indexes.size()==0)
    return;
  vtkAnnotation* a = annotations->GetAnnotation(indexes[0].row());
  annotations->RemoveAnnotation(a);
  this->getAnnotationLink()->MarkModified(0);
}


//-----------------------------------------------------------------------------
void AnnotationManagerPanel::activateItem(const QModelIndex &index)
{
  if (!index.isValid() || index.column() != 1)
    {
      // We are interested in clicks on the color swab alone.
      return;
    }

  // Get current color
  QColor color = this->Implementation->Model->getAnnotationColor(index.row());

  // Show color selector dialog to get a new color
  color = QColorDialog::getColor(color, this);
  if (color.isValid())
    {
      // Set the new color
      this->Implementation->Model->setAnnotationColor(index.row(), color);
    }
}
