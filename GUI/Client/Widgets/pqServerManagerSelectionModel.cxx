/*=========================================================================

   Program:   ParaQ
   Module:    pqServerManagerSelectionModel.cxx

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
#include "pqServerManagerSelectionModel.h"

#include <QPointer>

#include "pqServerManagerModelItem.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
class pqServerManagerSelectionModelInternal
{
public:
  QPointer<pqServerManagerModel> Model;
  pqServerManagerSelection Selection;
  QPointer<pqServerManagerModelItem> Current;
};

//-----------------------------------------------------------------------------
pqServerManagerSelectionModel::pqServerManagerSelectionModel(
  pqServerManagerModel* _model, QObject* _parent /*=null*/) :QObject(_parent)
{
  this->Internal = new pqServerManagerSelectionModelInternal;
  this->Internal->Model = _model;
}

//-----------------------------------------------------------------------------
pqServerManagerSelectionModel::~pqServerManagerSelectionModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqServerManagerSelectionModel::currentItem() const
{
  return this->Internal->Current;
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::setCurrentItem(
  pqServerManagerModelItem* item,
  pqServerManagerSelectionModel::SelectionFlags command)
{
  this->purge();
  if (this->Internal->Current != item)
    {
    this->Internal->Current = item;
    this->select(item, command);

    emit this->currentChanged(item);
    }
}

//-----------------------------------------------------------------------------
pqServerManagerModel* pqServerManagerSelectionModel::model() const
{
  return this->Internal->Model;
}

//-----------------------------------------------------------------------------
bool pqServerManagerSelectionModel::isSelected(
  pqServerManagerModelItem* item) const
{
  return this->Internal->Selection.contains(item);
}

//-----------------------------------------------------------------------------
const pqServerManagerSelection* 
pqServerManagerSelectionModel::selectedItems() const
{
  return &this->Internal->Selection;
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::select(pqServerManagerModelItem* item,
  pqServerManagerSelectionModel::SelectionFlags command)
{
  pqServerManagerSelection sel;
  sel.push_back(item);
  this->select(sel, command);
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::select(
  const pqServerManagerSelection& items,
  pqServerManagerSelectionModel::SelectionFlags command)
{
  this->purge();

  if (command == NoUpdate)
    {
    return;
    }

  bool changed = false;
  
  pqServerManagerSelection selected;
  pqServerManagerSelection deselected;

  if (command & Clear)
    {
    deselected = this->Internal->Selection;
    this->Internal->Selection.clear();
    changed = true;
    }

  foreach(pqServerManagerModelItem* item, items)
    {
    if (command & Select && !this->Internal->Selection.contains(item))
      {
      this->Internal->Selection.push_back(item);
      if (!selected.contains(item))
        {
        selected.push_back(item);
        changed = true;
        }
      }

    if (command & Deselect && this->Internal->Selection.contains(item)) 
      {
      this->Internal->Selection.removeAll(item);
      if (!deselected.contains(item))
        {
        deselected.push_back(item);
        changed = true;
        }
      }
    }

  if (changed)
    {
    emit this->selectionChanged(selected, deselected);
    }
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::purge()
{
  this->Internal->Selection.removeAll(0);
}


