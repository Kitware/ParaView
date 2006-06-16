/*=========================================================================

   Program:   ParaQ
   Module:    pqArrayMenu.cxx

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

//#include "pqParts.h"
//#include "pqPipelineDisplay.h"
//#include "pqPipelineSource.h"
#include "pqArrayMenu.h"

#include <vtkPVArrayInformation.h>
#include <vtkPVDataInformation.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkSMSourceProxy.h>

#include <QComboBox>
#include <QHBoxLayout>

/*
#include "vtkPVArrayInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkEventQtSlotConnect.h"

#include <QList>
#include <QRegExp>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqRenderModule.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
*/

///////////////////////////////////////////////////////////////////////////////
// pqArrayMenu::pqImplementation

class pqArrayMenu::pqImplementation
{
public:
  const QString arrayData(pqVariableType type, 
    const QString& name)
  {
    switch(type)
      {
      case VARIABLE_TYPE_NONE:
        return name + "|none";
      case VARIABLE_TYPE_NODE:
        return name + "|point";
      case VARIABLE_TYPE_CELL:
        return name + "|cell";
      }
      
    return QString();
  }

  /// Lists available variables in a drop-down list
  QComboBox Arrays;

/*
  /// Converts a variable type and name into a packed string representation 
  /// that can be used with a combo box.
  static const QString arrayData(pqVariableType, const QString& name);
  
  bool PendingDisplayPropertyConnections;
  QPointer<pqPipelineSource> SelectedSource;
  vtkEventQtSlotConnect* VTKConnect;
*/
};

///////////////////////////////////////////////////////////////////////////////
// pqArrayMenu

pqArrayMenu::pqArrayMenu(QWidget* p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  QHBoxLayout* const widget_layout = new QHBoxLayout(this);
  widget_layout->setMargin(0);
  widget_layout->setSpacing(1);
  widget_layout->addWidget(&this->Implementation->Arrays);
  
  this->setLayout(widget_layout);

  QObject::connect(
    &this->Implementation->Arrays,
    SIGNAL(currentIndexChanged(int)),
    this,
    SLOT(onArrayActivated(int)));
}

pqArrayMenu::~pqArrayMenu()
{
  delete this->Implementation;
}

/*
//-----------------------------------------------------------------------------
pqArrayMenu::pqArrayMenu( QWidget *p ) :
  QWidget( p ),
  BlockEmission(false)
{
  this->Layout  = new QHBoxLayout( this );
  this->Layout->setMargin(0);
  this->Layout->setSpacing(6);

  this->Arrays = new QComboBox( this );
  this->Arrays->setObjectName("Arrays");
  this->Arrays->setMinimumSize( QSize( 150, 20 ) );

  this->Layout->setMargin( 0 );
  this->Layout->setSpacing( 1 );
  this->Layout->addWidget(this->Arrays);


  QObject::connect(this, 
    SIGNAL(variableChanged(pqVariableType, const QString&)),
    this,
    SLOT(onVariableChanged(pqVariableType, const QString&)));

  this->VTKConnect = vtkEventQtSlotConnect::New();
  this->PendingDisplayPropertyConnections = true;
  this->BlockEmission = false;
}

//-----------------------------------------------------------------------------
pqArrayMenu::~pqArrayMenu()
{
  delete this->Layout;
  delete this->Arrays;
  
  this->Layout = 0;
  this->Arrays = 0;
  this->VTKConnect->Delete();
}
*/

//-----------------------------------------------------------------------------
void pqArrayMenu::clear()
{
  this->Implementation->Arrays.clear();
}

//-----------------------------------------------------------------------------
void pqArrayMenu::add(pqVariableType type, const QString& name)
{
  // Don't allow duplicates to creep in ...
  if(-1 != this->Implementation->Arrays.findData(
    this->Implementation->arrayData(type, name)))
    {
    return;
    }

  this->Implementation->Arrays.addItem(
    name, this->Implementation->arrayData(type, name));
}

void pqArrayMenu::add(vtkSMSourceProxy* source)
{
  if(!source)
    {
    return;
    }
  
  source->UpdateDataInformation();
  vtkPVDataInformation* const information = source->GetDataInformation();
  if(!information)
    {
    return;
    }
    
  if(vtkPVDataSetAttributesInformation* const point_info =
    information->GetPointDataInformation())
    {
    for(int i = 0; i != point_info->GetNumberOfArrays(); ++i)
      {
      if(vtkPVArrayInformation* const array_info =
        point_info->GetArrayInformation(i))
        {
        this->add(VARIABLE_TYPE_NODE, array_info->GetName());
        }
      }
    }
    
  if(vtkPVDataSetAttributesInformation* const cell_info =
    information->GetCellDataInformation())
    {
    for(int i = 0; i != cell_info->GetNumberOfArrays(); ++i)
      {
      if(vtkPVArrayInformation* const array_info =
        cell_info->GetArrayInformation(i))
        {
        this->add(VARIABLE_TYPE_CELL, array_info->GetName());
        }
      }
    }
}

void pqArrayMenu::getCurrent(pqVariableType& type, QString& name)
{
  const int current_index = this->Implementation->Arrays.currentIndex();
  if(current_index < 0)
    {
    return;
    }
    
  const QStringList d =
    this->Implementation->Arrays.itemData(current_index).toString().split("|");
    
  if(d.size() != 2)
    {
    return;
    }
    
  type = VARIABLE_TYPE_NONE;
  if(d[1] == "cell")
    {
    type = VARIABLE_TYPE_CELL;
    }
  else if(d[1] == "point")
    {
    type = VARIABLE_TYPE_NODE;
    }
    
  name = d[0];
}

//-----------------------------------------------------------------------------
void pqArrayMenu::onArrayActivated(int /*row*/)
{
  emit arrayChanged();
}

/*
//-----------------------------------------------------------------------------
void pqArrayMenu::chooseVariable(pqVariableType type, 
  const QString& name)
{
  const int row = this->Arrays->findData(arrayData(type, name));
  if(row != -1)
    {
    this->Arrays->setCurrentIndex(row);
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void pqArrayMenu::onVariableChanged(pqVariableType vtkNotUsed(type), 
  const QString& name)
{
  if (!this->SelectedSource || !this->SelectedSource->getDisplayCount())
    {
    return;
    }

  // I cannot decide if we should use signals here of directly 
  // call the appropriate methods on undo stack.
  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();

  stack->BeginOrContinueUndoSet("Color Change");
  pqPipelineDisplay* display = this->SelectedSource->getDisplay(0);
  pqPart::SetColorField(display->getDisplayProxy(), name);
  stack->EndUndoSet();
  pqApplicationCore::instance()->getActiveRenderModule()->render();
}

//-----------------------------------------------------------------------------
void pqArrayMenu::updateVariableSelector(pqPipelineSource* source)
{
  this->VTKConnect->Disconnect();
  this->SelectedSource = source;
  this->PendingDisplayPropertyConnections = true;
  this->BlockEmission = true;
  this->clear();
  this->addVariable(VARIABLE_TYPE_NONE, "Solid Color");
  this->BlockEmission = false;

  if (!source || source->getDisplayCount() == 0)
    {
    // nothing more to do.
    return;
    }

  this->reloadGUI();
}

//-----------------------------------------------------------------------------
void pqArrayMenu::updateGUI()
{
  if (this->SelectedSource && this->SelectedSource->getDisplayCount())
    {
    this->BlockEmission = true;
    this->Arrays->setCurrentIndex(
      this->Arrays->findText(pqPart::GetColorField(
          this->SelectedSource->getDisplay(0)->getDisplayProxy())));
    this->BlockEmission = false;
    }
}

//-----------------------------------------------------------------------------
void pqArrayMenu::reloadGUI()
{
  pqPipelineSource* source = this->SelectedSource;
  if (!source || source->getDisplayCount() == 0)
    {
    return;
    }

  this->BlockEmission = true;
  this->clear();
  this->addVariable(VARIABLE_TYPE_NONE, "Solid Color");

  pqPipelineDisplay* display = source->getDisplay(0);
  vtkSMDataObjectDisplayProxy* displayProxy = display->getDisplayProxy();

  QList<QString> arrayList = pqPart::GetColorFields(displayProxy);
  QRegExp regExpCell("\\(cell\\)\\w*$");
  QRegExp regExpPoint("\\(point\\)\\w*$");
  foreach(QString arrayName, arrayList)
    {
    if (arrayName == "Solid Color")
      {
      this->addVariable(VARIABLE_TYPE_NONE, arrayName);
      }
    else if (regExpCell.indexIn(arrayName) != -1)
      {
      this->addVariable(VARIABLE_TYPE_NODE, arrayName);
      }
    else if (regExpPoint.indexIn(arrayName) != -1)
      {
      this->addVariable(VARIABLE_TYPE_CELL, arrayName);
      }
    }

  QString currentArray = pqPart::GetColorField(displayProxy);
  int index =  this->Arrays->findText(currentArray);
  if (index == -1)
    {
    index = 0;
    }

  this->Arrays->blockSignals(true);
  this->Arrays->setCurrentIndex(index);
  this->Arrays->blockSignals(false);

  if (this->PendingDisplayPropertyConnections)
    {
    this->VTKConnect->Connect(displayProxy->GetProperty("ScalarVisibility"),
      vtkCommand::ModifiedEvent, this, SLOT(updateGUI()));
    this->VTKConnect->Connect(displayProxy->GetProperty("ScalarMode"),
      vtkCommand::ModifiedEvent, this, SLOT(updateGUI()));
    this->VTKConnect->Connect(displayProxy->GetProperty("ColorArray"),
      vtkCommand::ModifiedEvent, this, SLOT(updateGUI()));
    this->VTKConnect->Connect(
      displayProxy->GetProperty("Representation"), vtkCommand::ModifiedEvent, 
      this, SLOT(reloadGUI()),
      NULL, 0.0,
      Qt::QueuedConnection);
    this->PendingDisplayPropertyConnections = false;
    }
  this->BlockEmission = false;
}
*/
