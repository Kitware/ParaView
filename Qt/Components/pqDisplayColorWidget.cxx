/*=========================================================================

   Program: ParaView
   Module:    pqDisplayColorWidget.cxx

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

#include "pqDisplayColorWidget.h"

#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPVRepresentationProxy.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QRegExp>

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
pqDisplayColorWidget::pqDisplayColorWidget( QWidget *p ) :
  QWidget( p ),
  BlockEmission(0),
  Updating(false)
{
  this->CellDataIcon = new QIcon(":/pqWidgets/Icons/pqCellData16.png");
  this->PointDataIcon = new QIcon(":/pqWidgets/Icons/pqPointData16.png");
  this->SolidColorIcon = new QIcon(":/pqWidgets/Icons/pqSolidColor16.png");

  this->Layout  = new QHBoxLayout( this );
  this->Layout->setMargin( 0 );

  this->Variables = new QComboBox( this );
  this->Variables->setMaxVisibleItems(60);
  this->Variables->setObjectName("Variables");
  this->Variables->setMinimumSize( QSize( 150, 0 ) );
  this->Variables->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  
  this->Components = new QComboBox( this );
  this->Components->setObjectName("Components");

  this->Layout->addWidget(this->Variables);
  this->Layout->addWidget(this->Components);

  QObject::connect(this->Variables, SIGNAL(currentIndexChanged(int)), 
    SLOT(onVariableActivated(int)));
  QObject::connect(this->Components, SIGNAL(currentIndexChanged(int)), 
    SLOT(onComponentActivated(int)));
  QObject::connect(this, 
    SIGNAL(variableChanged(pqVariableType, const QString&)),
    this,
    SLOT(onVariableChanged(pqVariableType, const QString&)));

  this->VTKConnect = vtkEventQtSlotConnect::New();
}

//-----------------------------------------------------------------------------
pqDisplayColorWidget::~pqDisplayColorWidget()
{
  delete this->CellDataIcon;
  delete this->PointDataIcon;
  delete this->SolidColorIcon;
  this->VTKConnect->Delete();
}

//-----------------------------------------------------------------------------
QString pqDisplayColorWidget::getCurrentText() const
{
  return this->Variables->currentText();
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::clear()
{
  this->BlockEmission++;
  this->Variables->clear();
  this->BlockEmission--;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::addVariable(pqVariableType type, 
  const QString& arg_name, bool is_partial)
{
  QString name = arg_name;
  if (is_partial)
    {
    name += " (partial)";
    }

  // Don't allow duplicates to creep in ...
  if(-1 != this->Variables->findData(this->variableData(type, arg_name)))
    {
    return;
    }

  this->BlockEmission++;
  switch(type)
    {
    case VARIABLE_TYPE_NONE:
      this->Variables->addItem(*this->SolidColorIcon,
        "Solid Color", this->variableData(type, arg_name));
      break;

    case VARIABLE_TYPE_NODE:
      this->Variables->addItem(*this->PointDataIcon, name, 
        this->variableData(type, arg_name));
      break;

    case VARIABLE_TYPE_CELL:
      this->Variables->addItem(*this->CellDataIcon,
        name, this->variableData(type, arg_name));
      break;
    }
  this->BlockEmission--;
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
void pqDisplayColorWidget::onComponentActivated(int row)
{
  if(this->BlockEmission)
    {
    return;
    }
  
  pqPipelineRepresentation* display = this->getRepresentation();
  if(display)
    {
    BEGIN_UNDO_SET("Color Component Change");
    pqScalarsToColors* lut = display->getLookupTable();
    if(row == 0)
      {
      lut->setVectorMode(pqScalarsToColors::MAGNITUDE, -1);
      }
    else
      {
      lut->setVectorMode(pqScalarsToColors::COMPONENT, row-1);
      }
    lut->updateScalarBarTitles(this->Components->itemText(row));
    display->resetLookupTableScalarRange();
    END_UNDO_SET();

    display->renderViewEventually();
    }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::onVariableActivated(int row)
{
  if(this->BlockEmission)
    {
    return;
    }

  const QStringList d = this->Variables->itemData(row).toStringList();
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
  emit this->modified();
}

//-----------------------------------------------------------------------------
const QStringList pqDisplayColorWidget::variableData(pqVariableType type, 
                                                     const QString& name)
{
  QStringList result;
  result << name;

  switch(type)
    {
    case VARIABLE_TYPE_NONE:
      result << "none";
      break;
    case VARIABLE_TYPE_NODE:
      result << "point";
      break;
    case VARIABLE_TYPE_CELL:
      result << "cell";
      break;
    default:
      // Return empty list.
      return QStringList();
    }
    
  return result;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::onVariableChanged(pqVariableType type, 
  const QString& name)
{
  pqPipelineRepresentation* display = this->getRepresentation();
  if (display)
    {
    BEGIN_UNDO_SET("Color Change");
    switch(type)
      {
    case VARIABLE_TYPE_NONE:
      display->colorByArray(NULL, 0);
      break;
    case VARIABLE_TYPE_NODE:
      display->colorByArray(name.toAscii().data(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS);
      break;
    case VARIABLE_TYPE_CELL:
      display->colorByArray(name.toAscii().data(), 
        vtkDataObject::FIELD_ASSOCIATION_CELLS);
      break;
      }
    END_UNDO_SET();
    display->renderViewEventually();
    }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::updateGUI()
{
  this->BlockEmission++;
  pqPipelineRepresentation* display = this->getRepresentation();
  if (display)
    {
    int index = this->AvailableArrays.indexOf(display->getColorField());
    if (index < 0)
      {
      index = 0;
      }
    this->Variables->setCurrentIndex(index);
    this->updateComponents();
    }
  this->BlockEmission--;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::updateComponents()
{
  this->BlockEmission++;
  this->Components->clear();

  pqPipelineRepresentation* display = this->getRepresentation();
  if (display)
    {
    pqScalarsToColors* lut = display->getLookupTable();
    int numComponents = display->getColorFieldNumberOfComponents(
        display->getColorField());
    if ( lut && numComponents == 1 )
      {
      //we need to show a single name
      QString compName = display->getColorFieldComponentName( 
        display->getColorField(), 0);
      if ( compName.length() > 0 )
        {
        //only add an item if a component name exists, that way
        //we don't get a drop down box with a single empty item
        this->Components->addItem( compName );          
        }
      }
    else if(lut && numComponents > 1)
      {
      // delayed connection for when lut finally exists
      // remove previous connection, if any
      this->VTKConnect->Disconnect(
        lut->getProxy(), vtkCommand::PropertyModifiedEvent, 
        this, SLOT(updateGUI()), NULL);
      this->VTKConnect->Connect(
        lut->getProxy(), vtkCommand::PropertyModifiedEvent, 
        this, SLOT(updateGUI()), NULL, 0.0);

      this->Components->addItem("Magnitude");
      for(int i=0; i<numComponents; i++)
        {        
        this->Components->addItem(  
          display->getColorFieldComponentName( display->getColorField(), i) );          
        }
      
      if(lut->getVectorMode() == pqScalarsToColors::MAGNITUDE)
        {
        this->Components->setCurrentIndex(0);
        }
      else
        {
        this->Components->setCurrentIndex(lut->getVectorComponent()+1);
        }
      }
    }
  this->BlockEmission--;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::setRepresentation(pqDataRepresentation* display) 
{
  if(display == this->Representation)
    {
    return;
    }

  if (this->Representation)
    {
    QObject::disconnect(this->Representation, 0, this, 0);
    }

  this->VTKConnect->Disconnect();
  this->Representation = qobject_cast<pqPipelineRepresentation*>(display);
  if(this->Representation)
    {
    vtkSMProxy* repr = this->Representation->getProxy();
    this->VTKConnect->Connect(repr->GetProperty("ColorAttributeType"),
      vtkCommand::ModifiedEvent, this, SLOT(updateGUI()),
      NULL, 0.0);
    this->VTKConnect->Connect(repr->GetProperty("ColorArrayName"),
      vtkCommand::ModifiedEvent, this, SLOT(updateGUI()),
      NULL, 0.0);

    if (repr->GetProperty("Representation"))
      {
      this->VTKConnect->Connect(
        repr->GetProperty("Representation"), vtkCommand::ModifiedEvent, 
        this, SLOT(updateGUI()),
        NULL, 0.0);
      }

    // Every time the display updates, it is possible that the arrays available for 
    // coloring have changed, hence we reload the list.
    QObject::connect(this->Representation, SIGNAL(dataUpdated()),
      this, SLOT(reloadGUI()));
    }
  this->reloadGUI();
}

//-----------------------------------------------------------------------------
pqPipelineRepresentation* pqDisplayColorWidget::getRepresentation() const
{
  return this->Representation;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::reloadGUI()
{
  this->reloadGUIInternal();
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::reloadGUIInternal()
{
  this->Updating = false;
  this->BlockEmission++;
  this->clear();

  pqPipelineRepresentation* display = this->getRepresentation();
  if (!display)
    {
    this->addVariable(VARIABLE_TYPE_NONE, "Solid Color", false);
    this->BlockEmission--;
    this->setEnabled(false);
    return;
    }
  this->setEnabled(true);

  this->AvailableArrays = display->getColorFields();
  QRegExp regExpCell(" \\(cell\\)\\w*$");
  QRegExp regExpPoint(" \\(point\\)\\w*$");
  foreach(QString arrayName, this->AvailableArrays)
    {
    if (arrayName == "Solid Color")
      {
      this->addVariable(VARIABLE_TYPE_NONE, arrayName, false);
      }
    else if (regExpCell.indexIn(arrayName) != -1)
      {
      arrayName = arrayName.replace(regExpCell, "");
      this->addVariable(VARIABLE_TYPE_CELL, arrayName, 
        display->isPartial(arrayName, vtkDataObject::FIELD_ASSOCIATION_CELLS));
      }
    else if (regExpPoint.indexIn(arrayName) != -1)
      {
      arrayName = arrayName.replace(regExpPoint, "");
      this->addVariable(VARIABLE_TYPE_NODE, arrayName,
        display->isPartial(arrayName, vtkDataObject::FIELD_ASSOCIATION_POINTS));
      }
    }
    
  this->BlockEmission--;
  this->updateGUI();

  emit this->modified();
}

