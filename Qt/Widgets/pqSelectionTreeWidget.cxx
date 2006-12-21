/*=========================================================================

   Program: ParaView
   Module:    pqSelectionTreeWidget.cxx

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

#include "pqSelectionTreeWidget.h"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QTimer>
#include <QHeaderView>

pqSelectionTreeWidget::pqSelectionTreeWidget(QWidget* p)
  : QTreeWidget(p)
{
  QStyleOptionButton option;
  QRect r = this->style()->subElementRect(QStyle::SE_CheckBoxIndicator, 
                                          &option, this);
  option.rect = QRect(QPoint(0,0), r.size());
  
  this->CheckPixmap = new QPixmap(r.size());
  this->CheckPixmap->fill(QColor(0,0,0,0));
  QPainter painter1(this->CheckPixmap);
  option.state = QStyle::State_On;
  this->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, 
                       &painter1, this);
  
  this->PartialCheckPixmap = new QPixmap(r.size());
  this->PartialCheckPixmap->fill(QColor(0,0,0,0));
  QPainter painter2(this->PartialCheckPixmap);
  option.state = QStyle::State_NoChange;
  this->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, 
                       &painter2, this);

  this->UnCheckPixmap = new QPixmap(r.size());
  this->UnCheckPixmap->fill(QColor(0,0,0,0));
  QPainter painter3(this->UnCheckPixmap);
  option.state = QStyle::State_Off;
  this->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, 
                       &painter3, this);
  
  this->headerItem()->setCheckState(0, Qt::Checked);
  this->headerItem()->setData(0, Qt::DecorationRole, *this->CheckPixmap);

  QObject::connect(this->header(), SIGNAL(sectionClicked(int)),
                   this, SLOT(doToggle(int)),
                   Qt::QueuedConnection);
  
  this->header()->setClickable(true);
}

pqSelectionTreeWidget::~pqSelectionTreeWidget()
{
  delete this->CheckPixmap;
  delete this->UnCheckPixmap;
  delete this->PartialCheckPixmap;
}

void pqSelectionTreeWidget::dataChanged(const QModelIndex& topLeft,
                                        const QModelIndex& bottomRight)
{
  Superclass::dataChanged(topLeft, bottomRight);
  QTimer::singleShot(0, this, SLOT(updateCheckState()));
}

void pqSelectionTreeWidget::updateCheckState()
{
  Qt::CheckState oldState = this->checkState();
  Qt::CheckState newState = Qt::Checked;
  int numChecked = 0;
  QAbstractItemModel* m = this->model();
  int numRows = m->rowCount(QModelIndex());
  for(int i=0; i<numRows; i++)
    {
    QModelIndex idx = m->index(i, 0);
    QVariant v = m->data(idx, Qt::CheckStateRole);
    if(v == Qt::Checked)
      {
      numChecked++;
      }
    }
  if(numChecked != numRows)
    {
    newState = numChecked == 0 ? Qt::Unchecked : Qt::PartiallyChecked;
    }

  if(newState != oldState)
    {
    this->headerItem()->setCheckState(0, newState);
    switch(newState)
      {
    case Qt::Checked:
      this->headerItem()->setData(0, Qt::DecorationRole, *this->CheckPixmap);
      break;
    case Qt::Unchecked:
      this->headerItem()->setData(0, Qt::DecorationRole, *this->UnCheckPixmap);
      break;
    case Qt::PartiallyChecked:
      this->headerItem()->setData(0, Qt::DecorationRole, *this->PartialCheckPixmap);
      break;
      }
    }
}

Qt::CheckState pqSelectionTreeWidget::checkState() const
{
  return this->headerItem()->checkState(0);
}
  
void pqSelectionTreeWidget::allOn()
{
  QTreeWidgetItem* item;
  int i, end = this->topLevelItemCount();
  for(i=0; i<end; i++)
    {
    item = this->topLevelItem(i);
    item->setCheckState(0, Qt::Checked);
    }
}

void pqSelectionTreeWidget::allOff()
{
  QTreeWidgetItem* item;
  int i, end = this->topLevelItemCount();
  for(i=0; i<end; i++)
    {
    item = this->topLevelItem(i);
    item->setCheckState(0, Qt::Unchecked);
    }
}


void pqSelectionTreeWidget::doToggle(int column)
{
  if(column == 0)
    {
    if(this->checkState() == Qt::Checked)
      {
      this->allOff();
      }
    else
      {
      this->allOn();
      }
    }
}

