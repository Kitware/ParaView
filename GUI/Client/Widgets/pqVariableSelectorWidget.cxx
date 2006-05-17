/*=========================================================================

   Program:   ParaQ
   Module:    pqVariableSelectorWidget.cxx

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

#include "pqVariableSelectorWidget.h"

#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"



#include <QComboBox>
#include <QHBoxLayout>
#include <QList>

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqPipelineFilter.h"
#include "pqPipelineDisplay.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
pqVariableSelectorWidget::pqVariableSelectorWidget( QWidget *p ) :
  QWidget( p ),
  BlockEmission(false)
{
  this->Layout  = new QHBoxLayout( this );
  this->Layout->setMargin(0);
  this->Layout->setSpacing(6);

  this->Variables = new QComboBox( this );
  this->Variables->setObjectName("Variables");
  this->Variables->setMinimumSize( QSize( 150, 20 ) );

  this->Layout->setMargin( 0 );
  this->Layout->setSpacing( 1 );
  this->Layout->addWidget(this->Variables);

  QObject::connect(this->Variables, SIGNAL(currentIndexChanged(int)), 
    SLOT(onVariableActivated(int)));
  QObject::connect(this, 
    SIGNAL(variableChanged(pqVariableType, const QString&)),
    this,
    SLOT(onVariableChanged(pqVariableType, const QString&)));
}

//-----------------------------------------------------------------------------
pqVariableSelectorWidget::~pqVariableSelectorWidget()
{
  delete this->Layout;
  delete this->Variables;
  
  this->Layout = 0;
  this->Variables = 0;
  this->IgnoreWidgetChanges = false;
}

//-----------------------------------------------------------------------------
void pqVariableSelectorWidget::clear()
{
  this->BlockEmission = true;
  this->Variables->clear();
  this->BlockEmission = false;
}

//-----------------------------------------------------------------------------
void pqVariableSelectorWidget::addVariable(pqVariableType type, 
  const QString& name)
{
  // Don't allow duplicates to creep in ...
  if(-1 != this->Variables->findData(this->variableData(type, name)))
    return;

  this->BlockEmission = true;
  switch(type)
    {
    case VARIABLE_TYPE_NONE:
      this->Variables->addItem(name, this->variableData(type, name));
      break;
    case VARIABLE_TYPE_NODE:
      this->Variables->addItem("Point " + name, 
        this->variableData(type, name));
      break;
    case VARIABLE_TYPE_CELL:
      this->Variables->addItem("Cell " + name, this->variableData(type, name));
      break;
    }
  this->BlockEmission = false;
}

//-----------------------------------------------------------------------------
void pqVariableSelectorWidget::chooseVariable(pqVariableType type, 
  const QString& name)
{
  const int row = this->Variables->findData(variableData(type, name));
  if(row != -1)
    {
    this->Variables->setCurrentIndex(row);
    }
}

//-----------------------------------------------------------------------------
void pqVariableSelectorWidget::onVariableActivated(int row)
{
  if(this->BlockEmission)
    return;

  const QStringList d = this->Variables->itemData(row).toString().split("|");
  if(d.size() != 2)
    return;
    
  pqVariableType type = VARIABLE_TYPE_NONE;
  if(d[1] == "cell")
    {
    type = VARIABLE_TYPE_CELL;
    }
  else if(d[1] == "point")
    {
    type = VARIABLE_TYPE_NODE;
    }
    
  const QString name = d[0];
  
  emit variableChanged(type, name);
}

//-----------------------------------------------------------------------------
const QString pqVariableSelectorWidget::variableData(pqVariableType type, 
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

//-----------------------------------------------------------------------------
void pqVariableSelectorWidget::onVariableChanged(pqVariableType type, 
  const QString& name)
{
  if (!this->SelectedSource || !this->SelectedSource->getDisplayCount())
    {
    return;
    }
  if (this->IgnoreWidgetChanges)
    {
    return;
    }

  // I cannot decide if we should use signals here of directly 
  // call the appropriate methods on undo stack.
  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();

  stack->BeginOrContinueUndoSet("Color Change");
  pqPipelineDisplay* display = this->SelectedSource->getDisplay(0);
   switch(type)
    {
  case VARIABLE_TYPE_NONE:
    display->colorByArray(0, 0);
    break;

  case VARIABLE_TYPE_CELL:
    display->colorByArray(name.toStdString().c_str(), 
      vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
    break;

  case VARIABLE_TYPE_NODE:
    display->colorByArray(name.toStdString().c_str(), 
      vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
    break;
    }
   stack->EndUndoSet();
}

//-----------------------------------------------------------------------------
void pqVariableSelectorWidget::updateVariableSelector(pqPipelineSource* source)
{
  this->IgnoreWidgetChanges = true;
  this->clear();
  this->addVariable(VARIABLE_TYPE_NONE, "");

  this->SelectedSource = source;

  if (!source || source->getDisplayCount() == 0)
    {
    this->IgnoreWidgetChanges = false;
    // nothing more to do.
    return;
    }

  // pqVariableSelectorWidget must actually be listening to color change
  // on the proxy, so that if during undo/redo the coloring changes,
  // the toolbar will get updated.

  // This code must be reorganized, there is duplication here and in
  // pqParts.
  pqPipelineDisplay* display = source->getDisplay(0);
  vtkSMDataObjectDisplayProxy* displayProxy = display->getProxy();

  pqPipelineSource* reader = source;
  pqPipelineFilter* filter = dynamic_cast<pqPipelineFilter*>(reader);
  while(filter && filter->getInputCount() > 0)
    {
    reader = filter->getInput(0);
    filter = dynamic_cast<pqPipelineFilter*>(reader);
    }

  vtkPVDataInformation* geomInfo = displayProxy->GetGeometryInformation();
  // No point arrays?
  vtkPVDataSetAttributesInformation* cellinfo = 
    geomInfo->GetCellDataInformation();
  int i;
  for(i=0; i<cellinfo->GetNumberOfArrays(); i++)
    {
    vtkPVArrayInformation* info = cellinfo->GetArrayInformation(i);
    this->addVariable(VARIABLE_TYPE_CELL, info->GetName());
    }

  // also include unloaded arrays if any
  QList<QList<QVariant> > extraCellArrays = 
    pqSMAdaptor::getSelectionProperty(reader->getProxy(), 
      reader->getProxy()->GetProperty("CellArrayStatus"));
  for(i=0; i<extraCellArrays.size(); i++)
    {
    QList<QVariant> cell = extraCellArrays[i];
    if(cell[1] == false)
      {
      this->addVariable(VARIABLE_TYPE_CELL, cell[0].toString());
      }
    }

  vtkPVDataSetAttributesInformation* pointinfo = 
    geomInfo->GetPointDataInformation();
  for(i=0; i<pointinfo->GetNumberOfArrays(); i++)
    {
    vtkPVArrayInformation* info = pointinfo->GetArrayInformation(i);
    this->addVariable(VARIABLE_TYPE_NODE, info->GetName());
    }

  // also include unloaded arrays if any
  QList<QList<QVariant> > extraPointArrays = 
    pqSMAdaptor::getSelectionProperty(reader->getProxy(), 
      reader->getProxy()->GetProperty("PointArrayStatus"));
  for(i=0; i<extraPointArrays.size(); i++)
    {
    QList<QVariant> cell = extraPointArrays[i];
    if(cell[1] == false)
      {
      this->addVariable(VARIABLE_TYPE_NODE, cell[0].toString());
      }
    }

  // Update the variable selector to display the current proxy color
  vtkSMIntVectorProperty* scalar_visibility = 
    vtkSMIntVectorProperty::SafeDownCast(
      displayProxy->GetProperty("ScalarVisibility"));
  if(scalar_visibility && scalar_visibility->GetElement(0))
    {
    vtkSMStringVectorProperty* d_svp = vtkSMStringVectorProperty::SafeDownCast(
      displayProxy->GetProperty("ColorArray"));
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      displayProxy->GetProperty("ScalarMode"));
    int fieldtype = ivp->GetElement(0);
    if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
      {
      this->chooseVariable(VARIABLE_TYPE_CELL, d_svp->GetElement(0));
      }
    else if(fieldtype == vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)
      {
      this->chooseVariable(VARIABLE_TYPE_NODE, d_svp->GetElement(0));
      }
    }
  else
    {
    this->chooseVariable(VARIABLE_TYPE_NONE, "");
    }
  this->IgnoreWidgetChanges = false;
}
