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

#include <QComboBox>
#include <QHBoxLayout>

pqVariableSelectorWidget::pqVariableSelectorWidget( QWidget *p ) :
  QWidget( p )
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

  connect(this->Variables, SIGNAL(activated(int)), SLOT(onVariableActivated(int)));
}

pqVariableSelectorWidget::~pqVariableSelectorWidget()
{
  delete this->Layout;
  delete this->Variables;
  
  this->Layout = 0;
  this->Variables = 0;
}

void pqVariableSelectorWidget::clear()
{
  this->Variables->clear();
}

void pqVariableSelectorWidget::addVariable(pqVariableType type, const QString& name)
{
  // Don't allow duplicates to creep in ...
  if(-1 != this->Variables->findData(this->variableData(type, name)))
    return;

  switch(type)
    {
    case VARIABLE_TYPE_NODE:
      this->Variables->addItem("Point " + name, this->variableData(type, name));
      break;
    case VARIABLE_TYPE_CELL:
      this->Variables->addItem("Cell " + name, this->variableData(type, name));
      break;
    }
}

void pqVariableSelectorWidget::chooseVariable(pqVariableType type, const QString& name)
{
  const int row = this->Variables->findData(variableData(type, name));
  if(row != -1)
    {
    this->Variables->setCurrentIndex(row);
    this->onVariableActivated(row);
    }
}

void pqVariableSelectorWidget::onVariableActivated(int row)
{
  const QStringList d = this->Variables->itemData(row).toString().split("|");
  if(d.size() == 2)
    {
    const pqVariableType type = d[1] == "cell" ? VARIABLE_TYPE_CELL : VARIABLE_TYPE_NODE;
    const QString name = d[0];
    
    emit variableChanged(type, name);
    }
}

const QString pqVariableSelectorWidget::variableData(pqVariableType type, const QString& name)
{
  switch(type)
    {
    case VARIABLE_TYPE_NODE:
      return name + "|point";
    case VARIABLE_TYPE_CELL:
      return name + "|cell";
    }
    
  return QString();
}
