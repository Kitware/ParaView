/*=========================================================================

   Program:   ParaQ
   Module:    pqElementInspectorWidget.cxx

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

#include "pqDataSetModel.h"
#include "pqElementInspectorWidget.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

#include <vtkAppendFilter.h>
#include <vtkUnstructuredGrid.h>

//////////////////////////////////////////////////////////////////////////////
// pqElementInspectorWidget::pqImplementation

struct pqElementInspectorWidget::pqImplementation
{
public:
  pqImplementation() :
    ClearButton(tr("Clear")),
    Data(vtkUnstructuredGrid::New())
  {
  }
  
  ~pqImplementation()
  {
    this->Data->Delete();
  }

  void clearData()
  {
    this->Data->Reset();
  }

  void addData(vtkUnstructuredGrid* data)
  {
    vtkAppendFilter* const append_filter = vtkAppendFilter::New();
    append_filter->AddInput(this->Data);
    append_filter->AddInput(data);
    
    vtkUnstructuredGrid* const output = append_filter->GetOutput();
    output->Update();
    
    this->Data->DeepCopy(output);

    append_filter->Delete();
  }
  
  void setData(vtkUnstructuredGrid* data)
  {
    this->Data->DeepCopy(data);
  }

  QPushButton ClearButton;
  QTreeView TreeView;
  vtkUnstructuredGrid* const Data;
};

/////////////////////////////////////////////////////////////////////////////////
// pqElementInspectorWidget

pqElementInspectorWidget::pqElementInspectorWidget(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->setObjectName("ElementInspectorWidget");
  
  this->Implementation->ClearButton.hide();
  connect(&this->Implementation->ClearButton, SIGNAL(clicked()), SLOT(clear()));
  
  this->Implementation->TreeView.setRootIsDecorated(false);
  this->Implementation->TreeView.setAlternatingRowColors(true);
  this->Implementation->TreeView.setModel(new pqDataSetModel(&this->Implementation->TreeView));

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->addWidget(&this->Implementation->ClearButton);
  hbox->setMargin(0);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox);
  vbox->addWidget(&this->Implementation->TreeView);
  this->setLayout(vbox);
}

pqElementInspectorWidget::~pqElementInspectorWidget()
{
  delete this->Implementation;
}

void pqElementInspectorWidget::clear()
{
  this->Implementation->clearData();
  onElementsChanged();    
}

void pqElementInspectorWidget::addElements(vtkUnstructuredGrid* Elements)
{
  this->Implementation->addData(Elements);
  onElementsChanged();
}

void pqElementInspectorWidget::setElements(vtkUnstructuredGrid* Elements)
{
  this->Implementation->setData(Elements);
  onElementsChanged();  
}

void pqElementInspectorWidget::onElementsChanged()
{
  if(this->Implementation->Data->GetNumberOfCells())
    this->Implementation->ClearButton.show();
  else
    this->Implementation->ClearButton.hide();
    
  qobject_cast<pqDataSetModel*>(this->Implementation->TreeView.model())->setDataSet(this->Implementation->Data);
  this->Implementation->TreeView.reset();
  this->Implementation->TreeView.update();
  emit elementsChanged(this->Implementation->Data);
}
