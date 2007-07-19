/*=========================================================================

   Program:   ParaQ
   Module:    pqKeyFrameEditor.cxx

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

#include "pqKeyFrameEditor.h"
#include "ui_pqKeyFrameEditor.h"

#include <QPointer>
#include <QStandardItemModel>
#include <QHeaderView>

#include "pqAnimationCue.h"
#include "pqSMAdaptor.h"
#include "pqApplicationCore.h"
#include "pqUndoStack.h"

class pqKeyFrameEditor::pqInternal : public Ui::pqKeyFrameEditor
{
public:
  QPointer<pqAnimationCue> Cue;
  QStandardItemModel Model;
};

pqKeyFrameEditor::pqKeyFrameEditor(pqAnimationCue* cue, QWidget* p)
  : QWidget(p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);
  this->Internal->Cue = cue;

  this->Internal->tableView->setModel(&this->Internal->Model);
  this->Internal->tableView->horizontalHeader()->setStretchLastSection(true);

  this->readKeyFrameData();
}

pqKeyFrameEditor::~pqKeyFrameEditor()
{
  delete this->Internal;
}

void pqKeyFrameEditor::readKeyFrameData()
{
  this->Internal->Model.clear();

  if(!this->Internal->Cue)
    {
    return;
    }

  int numberKeyFrames = this->Internal->Cue->getNumberOfKeyFrames();

  this->Internal->Model.setColumnCount(3);
  this->Internal->Model.setRowCount(numberKeyFrames);

  // set the header data
  QStringList headerLabels;
  headerLabels << tr("Time") << tr("Interpolation") << tr("Value");
  this->Internal->Model.setHorizontalHeaderLabels(headerLabels);

  for(int i=0; i<numberKeyFrames; i++)
    {
    vtkSMProxy* keyFrame = this->Internal->Cue->getKeyFrame(i);
    
    QModelIndex idx = this->Internal->Model.index(i, 0);
    this->Internal->Model.setData(idx,
      pqSMAdaptor::getElementProperty(keyFrame->GetProperty("KeyTime")),
      Qt::DisplayRole);
    
    idx = this->Internal->Model.index(i, 2);
    this->Internal->Model.setData(idx,
      pqSMAdaptor::getElementProperty(keyFrame->GetProperty("KeyValues")),
      Qt::DisplayRole);
    }

}

void pqKeyFrameEditor::writeKeyFrameData()
{
  if(!this->Internal->Cue)
    {
    return;
    }

  int oldNumber = this->Internal->Cue->getNumberOfKeyFrames();
  int newNumber = this->Internal->Model.rowCount();

  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();

  if(stack)
    {
    stack->beginUndoSet("Edit Keyframes");
    }

  for(int i=0; i<oldNumber-newNumber; i++)
    {
    this->Internal->Cue->insertKeyFrame(0);
    }
  for(int i=0; i<newNumber-oldNumber; i++)
    {
    this->Internal->Cue->deleteKeyFrame(0);
    }

  for(int i=0; i<newNumber; i++)
    {
    vtkSMProxy* keyFrame = this->Internal->Cue->getKeyFrame(i);

    QModelIndex idx = this->Internal->Model.index(i, 0);
    QVariant newData = this->Internal->Model.data(idx, Qt::DisplayRole);
    pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyTime"), newData);
    
    idx = this->Internal->Model.index(i, 2);
    newData = this->Internal->Model.data(idx, Qt::DisplayRole);
    pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyValues"), newData);
    keyFrame->UpdateVTKObjects();
    }
  
  if(stack)
    {
    stack->endUndoSet();
    }
}

