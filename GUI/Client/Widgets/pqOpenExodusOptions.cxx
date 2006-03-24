/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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


#include "pqOpenExodusOptions.h"
#include <QCheckBox>

#include "pqSMAdaptor.h"

#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMArraySelectionDomain.h>


pqOpenExodusOptions::pqOpenExodusOptions(vtkSMSourceProxy* exodusReader, QWidget* p)
  : QDialog(p), ExodusReader(exodusReader)
{
  this->setupUi(this);

  if(!this->ExodusReader)
    return;

  this->ExodusReader->UpdateVTKObjects();
  this->ExodusReader->UpdatePipelineInformation();

  pqSMAdaptor* PropertyAdaptor = pqSMAdaptor::instance();
  
  int i;
  
  // get and populate block ids
  QList<QVariant> blocks = PropertyAdaptor->getProperty(this->ExodusReader->GetProperty("BlockArrayStatus")).toList();
  for(i=0; i<blocks.size(); i++)
    {
    QList<QVariant> block = blocks[i].toList();
    QCheckBox* cb = new QCheckBox(this->BlocksGroup);
    cb->setText(block[0].toString());
    cb->setChecked(block[1].toBool());
    this->BlocksGroup->layout()->addWidget(cb);
    }

  // get and populate cell arrays
  QList<QVariant> cellArrays = PropertyAdaptor->getProperty(this->ExodusReader->GetProperty("CellArrayStatus")).toList();
  for(i=0; i<cellArrays.size(); i++)
    {
    QList<QVariant> cellArray = cellArrays[i].toList();
    QCheckBox* cb = new QCheckBox(this->ElementVariablesGroup);
    cb->setText(cellArray[0].toString());
    cb->setChecked(cellArray[1].toBool());
    this->ElementVariablesGroup->layout()->addWidget(cb);
    }

  // get and populate point arrays
  QList<QVariant> pointArrays = PropertyAdaptor->getProperty(this->ExodusReader->GetProperty("PointArrayStatus")).toList();
  for(i=0; i<pointArrays.size(); i++)
    {
    QList<QVariant> pointArray = pointArrays[i].toList();
    QCheckBox* cb = new QCheckBox(this->NodeVariablesGroup);
    cb->setText(pointArray[0].toString());
    cb->setChecked(pointArray[1].toBool());
    this->NodeVariablesGroup->layout()->addWidget(cb);
    }
  
  // get and populate side sets
  QList<QVariant> sideSetArrays = PropertyAdaptor->getProperty(this->ExodusReader->GetProperty("SideSetArrayStatus")).toList();
  for(i=0; i<sideSetArrays.size(); i++)
    {
    QList<QVariant> sideSetArray = sideSetArrays[i].toList();
    QCheckBox* cb = new QCheckBox(this->SideSetGroup);
    cb->setText(sideSetArray[0].toString());
    cb->setChecked(sideSetArray[1].toBool());
    this->SideSetGroup->layout()->addWidget(cb);
    }
  
  // get and populate node sets
  QList<QVariant> nodeSetArrays = PropertyAdaptor->getProperty(this->ExodusReader->GetProperty("NodeSetArrayStatus")).toList();
  for(i=0; i<nodeSetArrays.size(); i++)
    {
    QList<QVariant> nodeSetArray = nodeSetArrays[i].toList();
    QCheckBox* cb = new QCheckBox(this->NodeSetGroup);
    cb->setText(nodeSetArray[0].toString());
    cb->setChecked(nodeSetArray[1].toBool());
    this->NodeSetGroup->layout()->addWidget(cb);
    }
}

pqOpenExodusOptions::~pqOpenExodusOptions()
{
}

void pqOpenExodusOptions::accept()
{
  int i;
  pqSMAdaptor* PropertyAdaptor = pqSMAdaptor::instance();

  // set blocks
  vtkSMProperty* prop = this->ExodusReader->GetProperty("BlockArrayStatus");
  QList<QVariant> blocks = PropertyAdaptor->getProperty(prop).toList();
  for(i=0; i<blocks.size(); i++)
    {
    QLayout* l = this->BlocksGroup->layout();
    QCheckBox* cb = qobject_cast<QCheckBox*>(l->itemAt(l->count() - blocks.size() + i)->widget());
    PropertyAdaptor->setProperty(this->ExodusReader, prop, i, cb->isChecked());
    this->ExodusReader->UpdateVTKObjects();  // TODO: get rid of this call
    }

  // set element arrays
  prop = this->ExodusReader->GetProperty("CellArrayStatus");
  QList<QVariant> cellArrays = PropertyAdaptor->getProperty(prop).toList();
  for(i=0; i<cellArrays.size(); i++)
    {
    QLayout* l = this->ElementVariablesGroup->layout();
    QCheckBox* cb = qobject_cast<QCheckBox*>(l->itemAt(l->count() - cellArrays.size() + i)->widget());
    PropertyAdaptor->setProperty(this->ExodusReader, prop, i, cb->isChecked());
    this->ExodusReader->UpdateVTKObjects();  // TODO: get rid of this call
    }
  
  // set point arrays
  prop = this->ExodusReader->GetProperty("PointArrayStatus");
  QList<QVariant> pointArrays = PropertyAdaptor->getProperty(prop).toList();
  for(i=0; i<pointArrays.size(); i++)
    {
    QLayout* l = this->NodeVariablesGroup->layout();
    QCheckBox* cb = qobject_cast<QCheckBox*>(l->itemAt(l->count() - pointArrays.size() + i)->widget());
    PropertyAdaptor->setProperty(this->ExodusReader, prop, i, cb->isChecked());
    this->ExodusReader->UpdateVTKObjects();  // TODO: get rid of this call
    }
  
  // set sideset arrays
  prop = this->ExodusReader->GetProperty("SideSetArrayStatus");
  QList<QVariant> sideSetArrays = PropertyAdaptor->getProperty(prop).toList();
  for(i=0; i<sideSetArrays.size(); i++)
    {
    QLayout* l = this->SideSetGroup->layout();
    QCheckBox* cb = qobject_cast<QCheckBox*>(l->itemAt(l->count() - sideSetArrays.size() + i)->widget());
    PropertyAdaptor->setProperty(this->ExodusReader, prop, i, cb->isChecked());
    this->ExodusReader->UpdateVTKObjects();  // TODO: get rid of this call
    }
  
  // set nodeset arrays
  prop = this->ExodusReader->GetProperty("NodeSetArrayStatus");
  QList<QVariant> nodeSetArrays = PropertyAdaptor->getProperty(prop).toList();
  for(i=0; i<nodeSetArrays.size(); i++)
    {
    QLayout* l = this->NodeSetGroup->layout();
    QCheckBox* cb = qobject_cast<QCheckBox*>(l->itemAt(l->count() - nodeSetArrays.size() + i)->widget());
    PropertyAdaptor->setProperty(this->ExodusReader, prop, i, cb->isChecked());
    this->ExodusReader->UpdateVTKObjects();  // TODO: get rid of this call
    }

  QDialog::accept();
}

