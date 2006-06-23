/*=========================================================================

   Program: ParaView
   Module:    pqArrayMenu.cxx

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

#include "pqArrayMenu.h"

#include <vtkPVArrayInformation.h>
#include <vtkPVDataInformation.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkSMSourceProxy.h>

#include <QComboBox>
#include <QHBoxLayout>

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
  
  switch(type)
    {
    case VARIABLE_TYPE_NONE:
      break;
    case VARIABLE_TYPE_NODE:
      this->Implementation->Arrays.setItemIcon(
        this->Implementation->Arrays.count() - 1, QIcon(":/pqWidgets/pqPointData16.png"));
      break;
    case VARIABLE_TYPE_CELL:
      this->Implementation->Arrays.setItemIcon(
        this->Implementation->Arrays.count() - 1, QIcon(":/pqWidgets/pqCellData16.png"));
      break;
    }
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
