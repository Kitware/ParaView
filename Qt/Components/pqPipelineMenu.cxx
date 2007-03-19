/*=========================================================================

   Program: ParaView
   Module:    pqPipelineMenu.cxx

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

========================================================================*/

/// \file pqPipelineMenu.cxx
/// \date 6/5/2006

#include "pqPipelineMenu.h"

#include "pqPipelineModel.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"

#include <QAction>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QtDebug>



//----------------------------------------------------------------------------
pqPipelineMenu::pqPipelineMenu(QObject *parentObject)
  : QObject(parentObject)
{
  this->Model = 0;
  this->Selection = 0;
  this->MenuList = new QAction *[pqPipelineMenu::LastAction + 1];
  for(int i = 0; i <= pqPipelineMenu::LastAction; i++)
    {
    this->MenuList[i] = 0;
    }
}

pqPipelineMenu::~pqPipelineMenu()
{
  delete [] this->MenuList;
}

void pqPipelineMenu::setModels(pqPipelineModel *model,
    QItemSelectionModel *selection)
{
  if(this->Model == model && this->Selection == selection)
    {
    return;
    }

  if(this->Model)
    {
    this->disconnect(this->Model, 0, this, 0);
    }

  if(this->Selection)
    {
    // Disconnect from the previous selection model.
    this->disconnect(this->Selection, 0, this, 0);
    }

  this->Model = selection ? model : 0;
  this->Selection = this->Model ? selection : 0;
  if(this->Selection)
    {
    // Listen to the new selection model's changes.
    this->connect(this->Selection,
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this, SLOT(updateActions()));
    this->connect(this->Selection, SIGNAL(destroyed()),
        this, SLOT(handleDeletion()));
    this->connect(this->Model, SIGNAL(destroyed()),
        this, SLOT(handleDeletion()));
    }

  this->updateActions();
}

void pqPipelineMenu::setMenuAction(ActionName name, QAction *action)
{
  if(name != pqPipelineMenu::InvalidAction)
    {
    this->MenuList[name] = action;
    }
}

QAction *pqPipelineMenu::getMenuAction(pqPipelineMenu::ActionName name) const
{
  if(name != pqPipelineMenu::InvalidAction)
    {
    return this->MenuList[name];
    }

  return 0;
}

bool pqPipelineMenu::isActionEnabled(ActionName name) const
{
  QAction *action = this->getMenuAction(name);
  if(action)
    {
    return action->isEnabled();
    }

  return true;
}

void pqPipelineMenu::updateActions()
{
  // Get the current selection from the model.
  QModelIndexList indexes;
  if(this->Selection)
    {
    indexes = this->Selection->selectedIndexes();
    }

  bool enabled = true;
  if(this->MenuList[pqPipelineMenu::AddFilterAction] != 0)
    {
    // Allow for multi-input filters.
    enabled = indexes.size() >= 1;
    if(enabled)
      {
      pqPipelineSource *source = 0;
      QModelIndexList::Iterator index = indexes.begin();
      for( ; index != indexes.end(); ++index)
        {
        source = dynamic_cast<pqPipelineSource *>(
            this->Model->getItemFor(*index));
        if(!source)
          {
          enabled = false;
          break;
          }
        }
      }

    this->MenuList[pqPipelineMenu::AddFilterAction]->setEnabled(enabled);
    }

  if(this->MenuList[pqPipelineMenu::ChangeInputAction] != 0)
    {
    enabled = indexes.size() == 1;
    if(enabled)
      {
      pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter *>(
          this->Model->getItemFor(indexes.first()));
      enabled = filter != 0;
      }

    this->MenuList[pqPipelineMenu::ChangeInputAction]->setEnabled(enabled);
    }

  if(this->MenuList[pqPipelineMenu::DeleteAction] != 0)
    {
    // TODO: Allow for deleting multiple items at once.
    enabled = indexes.size() == 1;
    if(enabled)
      {
      // TODO: If the item is a link, it can always be removed.
      pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(
          this->Model->getItemFor(indexes.first()));
      if(source)
        {
        enabled = source->getNumberOfConsumers() == 0;
        }
      }

    this->MenuList[pqPipelineMenu::DeleteAction]->setEnabled(enabled);
    }
}

void pqPipelineMenu::handleDeletion()
{
  this->Model = 0;
  this->Selection = 0;
  this->updateActions();
}


