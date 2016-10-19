/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidgetCheckHelper.cxx

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

#include "pqTreeWidgetCheckHelper.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>

//-----------------------------------------------------------------------------
pqTreeWidgetCheckHelper::pqTreeWidgetCheckHelper(QTreeWidget* tree, int checkableColumn, QObject* p)
  : QObject(p)
{
  this->Mode = CLICK_IN_ROW;
  this->Tree = tree;
  this->CheckableColumn = checkableColumn;
  this->PressState = -1;
  QObject::connect(this->Tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this,
    SLOT(onItemClicked(QTreeWidgetItem*, int)));
  QObject::connect(this->Tree, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this,
    SLOT(onItemPressed(QTreeWidgetItem*, int)));
}

//-----------------------------------------------------------------------------
void pqTreeWidgetCheckHelper::onItemPressed(QTreeWidgetItem* item, int /*column*/)
{
  this->PressState = item->checkState(this->CheckableColumn);
}

//-----------------------------------------------------------------------------
void pqTreeWidgetCheckHelper::onItemClicked(QTreeWidgetItem* item, int column)
{
  if (this->Mode == CLICK_IN_COLUMN && column != this->CheckableColumn)
  {
    return;
  }
  Qt::CheckState state = item->checkState(this->CheckableColumn);
  if (this->PressState != state)
  {
    // the click was on the check box itself, hence the state is already toggled.
    return;
  }
  if (state == Qt::Unchecked)
  {
    state = Qt::Checked;
  }
  else if (state == Qt::Checked)
  {
    state = Qt::Unchecked;
  }
  item->setCheckState(this->CheckableColumn, state);
  this->PressState = -1;
}

//-----------------------------------------------------------------------------
