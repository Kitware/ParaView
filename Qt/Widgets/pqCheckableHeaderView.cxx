/*=========================================================================

   Program: ParaView
   Module:    pqCheckableHeaderView.cxx

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

/// \file pqCheckableHeaderView.cxx
/// \date 8/17/2007

#include "pqCheckableHeaderView.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QEvent>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QStyle>


class pqCheckableHeaderViewItem
{
public:
  pqCheckableHeaderViewItem(bool checkable, int state);
  pqCheckableHeaderViewItem(const pqCheckableHeaderViewItem &other);
  ~pqCheckableHeaderViewItem() {}

  pqCheckableHeaderViewItem &operator=(const pqCheckableHeaderViewItem &other);

  int State;
  bool Checkable;
};


class pqCheckableHeaderViewInternal
{
public:
  enum PixmapStateIndex
    {
    Checked                 = 0,
    PartiallyChecked        = 1,
    UnChecked               = 2,
    
    // All active states in lower half
    Checked_Active          = 3,
    PartiallyChecked_Active = 4,
    UnChecked_Active        = 5,
   
    PixmapCount             = 6
    };

public:
  pqCheckableHeaderViewInternal();
  ~pqCheckableHeaderViewInternal();

  QPixmap getPixmap(int state, bool active) const;

  QList<pqCheckableHeaderViewItem> Items;
  QPixmap **Pixmaps;
  bool IgnoreChange;
};


//----------------------------------------------------------------------------
pqCheckableHeaderViewItem::pqCheckableHeaderViewItem(bool checkable, int state)
{
  this->Checkable = checkable;
  this->State = state;
}

pqCheckableHeaderViewItem::pqCheckableHeaderViewItem(
    const pqCheckableHeaderViewItem &other)
{
  this->Checkable = other.Checkable;
  this->State = other.State;
}

pqCheckableHeaderViewItem &pqCheckableHeaderViewItem::operator=(
    const pqCheckableHeaderViewItem &other)
{
  this->Checkable = other.Checkable;
  this->State = other.State;
  return *this;
}


//----------------------------------------------------------------------------
pqCheckableHeaderViewInternal::pqCheckableHeaderViewInternal()
  : Items()
{
  this->Pixmaps = new QPixmap*[PixmapCount];
  this->IgnoreChange = false;
}

pqCheckableHeaderViewInternal::~pqCheckableHeaderViewInternal()
{
  for(int i = 0; i < PixmapCount; i++)
    {
    delete this->Pixmaps[i];
    }

  delete [] this->Pixmaps;
}

QPixmap pqCheckableHeaderViewInternal::getPixmap(int state, bool active) const
{
  int offset = active ? 3 : 0;
  switch(state)
    {
    case Qt::Checked:
      {
      return *this->Pixmaps[offset + Checked];
      }
    case Qt::Unchecked:
      {
      return *this->Pixmaps[offset + UnChecked];
      }
    case Qt::PartiallyChecked:
      {
      return *this->Pixmaps[offset + PartiallyChecked];
      }
    }
  return QPixmap();
}


//----------------------------------------------------------------------------
pqCheckableHeaderView::pqCheckableHeaderView(Qt::Orientation orient,
    QWidget *widgetParent)
  : QHeaderView(orient, widgetParent)
{
  this->Internal = new pqCheckableHeaderViewInternal();

  // Initialize the pixmaps. The following style array should
  // correspond to the PixmapStateIndex enum.
  const QStyle::State PixmapStyle[] =
    {
    QStyle::State_On | QStyle::State_Enabled,
    QStyle::State_NoChange | QStyle::State_Enabled,
    QStyle::State_Off | QStyle::State_Enabled,
    QStyle::State_On | QStyle::State_Enabled | QStyle::State_Active,
    QStyle::State_NoChange | QStyle::State_Enabled | QStyle::State_Active,
    QStyle::State_Off | QStyle::State_Enabled | QStyle::State_Active
    };

  QStyleOptionButton option;
  QRect r = this->style()->subElementRect(
      QStyle::SE_CheckBoxIndicator, &option, this);
  option.rect = QRect(QPoint(0,0), r.size());
  for(int i = 0; i < pqCheckableHeaderViewInternal::PixmapCount; i++)
    {
    this->Internal->Pixmaps[i] = new QPixmap(r.size());
    this->Internal->Pixmaps[i]->fill(QColor(0, 0, 0, 0));
    QPainter painter(this->Internal->Pixmaps[i]);
    option.state = PixmapStyle[i];
    
    this->style()->drawPrimitive(
        QStyle::PE_IndicatorCheckBox, &option, &painter, this);
    }

  // Listen for user clicks.
  this->connect(this, SIGNAL(sectionClicked(int)),
      this, SLOT(toggleCheckState(int)), Qt::QueuedConnection);
  if(widgetParent)
    {
    // Listen for focus change events.
    widgetParent->installEventFilter(this);
    }
}

pqCheckableHeaderView::~pqCheckableHeaderView()
{
  delete this->Internal;
}

bool pqCheckableHeaderView::eventFilter(QObject *object, QEvent *e)
{
  if(e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut)
    {
    QAbstractItemModel *current = this->model();
    if(current)
      {
      bool active = e->type() == QEvent::FocusIn;
      this->Internal->IgnoreChange = true;
      for(int i = 0; i < this->Internal->Items.size(); i++)
        {
        pqCheckableHeaderViewItem *item = &this->Internal->Items[i];
        if(item->Checkable)
          {
          current->setHeaderData(i, this->orientation(),
              this->Internal->getPixmap(item->State, active),
              Qt::DecorationRole);
          }
        }

      this->Internal->IgnoreChange = false;
      }
    }

  return false;
}

void pqCheckableHeaderView::setModel(QAbstractItemModel *newModel)
{
  QAbstractItemModel *current = this->model();
  if(current)
    {
    this->Internal->Items.clear();
    this->disconnect(current, 0, this, 0);
    }

  QHeaderView::setModel(newModel);
  if(newModel)
    {
    this->connect(
        newModel, SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
        this, SLOT(updateHeaderData(Qt::Orientation, int, int)));
    this->connect(newModel, SIGNAL(modelReset()),
        this, SLOT(initializeIcons()));
    if(this->orientation() == Qt::Horizontal)
      {
      this->connect(newModel,
          SIGNAL(columnsInserted(const QModelIndex &, int, int)),
          this, SLOT(insertHeaderSection(const QModelIndex &, int, int)));
      this->connect(newModel,
          SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
          this, SLOT(removeHeaderSection(const QModelIndex &, int, int)));
      }
    else
      {
      this->connect(newModel,
          SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          this, SLOT(insertHeaderSection(const QModelIndex &, int, int)));
      this->connect(newModel,
          SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
          this, SLOT(removeHeaderSection(const QModelIndex &, int, int)));
      }
    }

  // Determine which sections are clickable and setup the icons.
  this->initializeIcons();
}

void pqCheckableHeaderView::setRootIndex(const QModelIndex &index)
{
  QHeaderView::setRootIndex(index);
  this->initializeIcons();
}

void pqCheckableHeaderView::toggleCheckState(int section)
{
  // If the section is checkable, toggle the check state.
  QAbstractItemModel *current = this->model();
  if(current && section >= 0 && section < this->Internal->Items.size())
    {
    const pqCheckableHeaderViewItem &item = this->Internal->Items[section];
    if(item.Checkable)
      {
      // If the state is unchecked or partially checked, the state
      // should be changed to checked.
      current->setHeaderData(section, this->orientation(),
          item.State == Qt::Checked ? Qt::Unchecked : Qt::Checked,
          Qt::CheckStateRole);
      }
    }
}

void pqCheckableHeaderView::initializeIcons()
{
  this->Internal->Items.clear();
  QAbstractItemModel *current = this->model();
  if(current)
    {
    bool active = true;
    if(this->parentWidget())
      {
      active = this->parentWidget()->hasFocus();
      }

    this->Internal->IgnoreChange = true;
    int total = this->orientation() == Qt::Horizontal ?
        current->columnCount() : current->rowCount();
    for(int i = 0; i < total; i++)
      {
      bool checkable = false;
      int state = current->headerData(
          i, this->orientation(), Qt::CheckStateRole).toInt(&checkable);
      this->Internal->Items.append(
          pqCheckableHeaderViewItem(checkable, state));
      if(checkable)
        {
        current->setHeaderData(i, this->orientation(),
            this->Internal->getPixmap(state, active), Qt::DecorationRole);
        }
      else
        {
        current->setHeaderData(i, this->orientation(), QVariant(),
            Qt::DecorationRole);
        }
      }

    this->Internal->IgnoreChange = false;
    }
}

void pqCheckableHeaderView::updateHeaderData(Qt::Orientation orient,
    int first, int last)
{
  if(this->Internal->IgnoreChange)
    {
    return;
    }

  QAbstractItemModel *current = this->model();
  if(!current)
    {
    return;
    }

  bool active = true;
  if(this->parentWidget())
    {
    active = this->parentWidget()->hasFocus();
    }

  // If the check state has changed, update the icons.
  this->Internal->IgnoreChange = true;
  for(int i = first; i <= last; i++)
    {
    pqCheckableHeaderViewItem *item = &this->Internal->Items[i];
    if(item->Checkable)
      {
      int state = current->headerData(
          i, this->orientation(), Qt::CheckStateRole).toInt(&item->Checkable);
      if(!item->Checkable)
        {
        // Clear the check box pixmap.
        current->setHeaderData(i, this->orientation(), QVariant(),
            Qt::DisplayRole);
        }
      else if(state != item->State)
        {
        item->State = state;
        current->setHeaderData(i, this->orientation(),
            this->Internal->getPixmap(state, active), Qt::DecorationRole);
        }
      }
    }

  this->Internal->IgnoreChange = false;
}

void pqCheckableHeaderView::insertHeaderSection(const QModelIndex &parentIndex,
    int first, int last)
{
  QAbstractItemModel *current = this->model();
  if(current && parentIndex == this->rootIndex() && first >= 0)
    {
    bool active = true;
    if(this->parentWidget())
      {
      active = this->parentWidget()->hasFocus();
      }

    bool doAdd = first >= this->Internal->Items.size();
    this->Internal->IgnoreChange = true;
    for(int i = first; i <= last; i++)
      {
      bool checkable = false;
      int state = current->headerData(
          i, this->orientation(), Qt::CheckStateRole).toInt(&checkable);
      if(doAdd)
        {
        this->Internal->Items.append(
            pqCheckableHeaderViewItem(checkable, state));
        }
      else
        {
        this->Internal->Items.insert(i,
            pqCheckableHeaderViewItem(checkable, state));
        }

      if(checkable)
        {
        current->setHeaderData(i, this->orientation(),
            this->Internal->getPixmap(state, active), Qt::DecorationRole);
        }
      }

    this->Internal->IgnoreChange = false;
    }
}

void pqCheckableHeaderView::removeHeaderSection(const QModelIndex &parentIndex,
    int first, int last)
{
  if(parentIndex == this->rootIndex())
    {
    if(last >= this->Internal->Items.size())
      {
      last = this->Internal->Items.size() - 1;
      }

    if(first <= last && first >= 0)
      {
      for(int i = last; i >= first; i--)
        {
        this->Internal->Items.removeAt(i);
        }
      }
    }
}


