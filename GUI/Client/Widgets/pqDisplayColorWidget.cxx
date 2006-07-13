/*=========================================================================

   Program: ParaView
   Module:    pqDisplayColorWidget.cxx

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

#include "pqDisplayColorWidget.h"

#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkEventQtSlotConnect.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QList>
#include <QRegExp>
#include <QtDebug>
#include <QIcon>

#include "pqApplicationCore.h"
#include "pqParts.h"
#include "pqPipelineSource.h"
#include "pqPipelineFilter.h"
#include "pqPipelineDisplay.h"
#include "pqRenderModule.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
pqDisplayColorWidget::pqDisplayColorWidget( QWidget *p ) :
  QWidget( p ),
  BlockEmission(false)
{
  this->CellDataIcon = new QIcon(":/pqWidgets/Icons/pqCellData16.png");
  this->PointDataIcon = new QIcon(":/pqWidgets/Icons/pqPointData16.png");
  this->SolidColorIcon = new QIcon(":/pqWidgets/Icons/pqSolidColor16.png");

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

  this->VTKConnect = vtkEventQtSlotConnect::New();
  this->PendingDisplayPropertyConnections = true;
  this->BlockEmission = false;
}

//-----------------------------------------------------------------------------
pqDisplayColorWidget::~pqDisplayColorWidget()
{
  delete this->Layout;
  delete this->Variables;
  delete this->CellDataIcon;
  delete this->PointDataIcon;
  
  this->Layout = 0;
  this->Variables = 0;
  this->VTKConnect->Delete();
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::clear()
{
  this->BlockEmission = true;
  this->Variables->clear();
  this->BlockEmission = false;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::addVariable(pqVariableType type, 
  const QString& name)
{
  // Don't allow duplicates to creep in ...
  if(-1 != this->Variables->findData(this->variableData(type, name)))
    {
    return;
    }

  bool old_value = this->BlockEmission;
  this->BlockEmission = true;
  switch(type)
    {
    case VARIABLE_TYPE_NONE:
      this->Variables->addItem(*this->SolidColorIcon,
        "Solid Color", this->variableData(type, name));
      break;
    case VARIABLE_TYPE_NODE:
      this->Variables->addItem(*this->PointDataIcon, name, 
        this->variableData(type, name));
      break;
    case VARIABLE_TYPE_CELL:
      this->Variables->addItem(*this->CellDataIcon,
        name, this->variableData(type, name));
      break;
    }
  this->BlockEmission = old_value;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::chooseVariable(pqVariableType type, 
  const QString& name)
{
  const int row = this->Variables->findData(variableData(type, name));
  if(row != -1)
    {
    this->Variables->setCurrentIndex(row);
    }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::onVariableActivated(int row)
{
  if(this->BlockEmission)
    {
    return;
    }

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
const QString pqDisplayColorWidget::variableData(pqVariableType type, 
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
void pqDisplayColorWidget::onVariableChanged(pqVariableType vtkNotUsed(type), 
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
void pqDisplayColorWidget::updateVariableSelector(pqPipelineSource* source)
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
void pqDisplayColorWidget::updateGUI()
{
  if (this->SelectedSource && this->SelectedSource->getDisplayCount())
    {
    this->BlockEmission = true;
    this->Variables->setCurrentIndex(
      this->Variables->findText(pqPart::GetColorField(
          this->SelectedSource->getDisplay(0)->getDisplayProxy())));
    this->BlockEmission = false;
    }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::reloadGUI()
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
      this->addVariable(VARIABLE_TYPE_CELL, arrayName);
      }
    else if (regExpPoint.indexIn(arrayName) != -1)
      {
      this->addVariable(VARIABLE_TYPE_NODE, arrayName);
      }
    }

  QString currentArray = pqPart::GetColorField(displayProxy);
  int index =  this->Variables->findText(currentArray);
  if (index == -1)
    {
    index = 0;
    }

  this->Variables->blockSignals(true);
  this->Variables->setCurrentIndex(index);
  this->Variables->blockSignals(false);

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
