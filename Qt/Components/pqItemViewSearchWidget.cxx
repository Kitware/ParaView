/*=========================================================================

Program: ParaView
Module:    pqItemViewSearchWidget.cxx

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

#include "pqItemViewSearchWidget.h"

#include "pqWaitCursor.h"
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QModelIndex>
#include <QPointer>
#include <QShowEvent>
#include <QStyle>
#include <QTimer>

#include "ui_pqItemViewSearchWidget.h"

class pqItemViewSearchWidget::PIMPL : public Ui::pqItemViewSearchWidget
{
public:
  PIMPL(QWidget* parentW) 
  {
  this->RedPal.setColor(QPalette::Base, Qt::red);
  this->WhitePal.setColor(QPalette::Base, Qt::white);
  this->DlgBackPal.setColor(QPalette::Background, Qt::darkYellow);
  }
  ~PIMPL() {}
  QString SearchString;
  QModelIndex CurrentFound;
  QPointer<QAbstractItemView> BaseWidget;
  QPalette RedPal;
  QPalette WhitePal;
  QPalette DlgBackPal;
};

pqItemViewSearchWidget::pqItemViewSearchWidget(QWidget* parentW):
  Superclass(parentW->parentWidget(), Qt::Dialog|Qt::FramelessWindowHint)
{
  this->Private = new pqItemViewSearchWidget::PIMPL(parentW);
  this->Private->setupUi(this);
  this->setBaseWidget(parentW); 
  QObject::connect(this->Private->lineEditSearch, SIGNAL(textEdited(QString)),
    this, SLOT(updateSearch(QString)));
  QObject::connect(this->Private->checkBoxMattchCase, SIGNAL(toggled(bool)),
    this, SLOT(updateSearch()));
  QObject::connect(this->Private->nextButton, SIGNAL(clicked()),
    this, SLOT(findNext()));
  QObject::connect(this->Private->previousButton, SIGNAL(clicked()),
    this, SLOT(findPrevious()));
  this->installEventFilter(this);
  this->Private->lineEditSearch->installEventFilter(this);
}

// -------------------------------------------------------------------------
pqItemViewSearchWidget::~pqItemViewSearchWidget()
{
  this->Private->lineEditSearch->removeEventFilter(this);
  if(this->Private->CurrentFound.isValid() &&
    this->Private->BaseWidget)
    {
    this->Private->BaseWidget->model()->setData(
      this->Private->CurrentFound, Qt::white, Qt::BackgroundColorRole);
    }
  delete this->Private;
}

// -------------------------------------------------------------------------
void pqItemViewSearchWidget::setBaseWidget(QWidget* widget)
{
  this->Private->BaseWidget = widget ? 
    qobject_cast<QAbstractItemView*>(widget) : NULL;;
}

// -------------------------------------------------------------------------
void pqItemViewSearchWidget::showSearchWidget()
{
  if(!this->Private->BaseWidget)
    {
    return;
    }
  this->setPalette(this->Private->DlgBackPal);
  QPoint mappedPoint = this->Private->BaseWidget->geometry().topLeft();
  mappedPoint = this->Private->BaseWidget->mapToGlobal(mappedPoint);
  mappedPoint = this->mapFromGlobal(mappedPoint);
  this->move(mappedPoint.x(), mappedPoint.y()-2*this->height());
  this->exec();
}

// --------------------------------------------------------------------------
void pqItemViewSearchWidget::closeSearchWidget()
{
  this->close();
}

//-----------------------------------------------------------------------------
bool pqItemViewSearchWidget::eventFilter(QObject* obj, QEvent* event)
{ 
 if(event->type()== QEvent::KeyPress)
   {
   QKeyEvent *e = dynamic_cast<QKeyEvent*>(event);
   if(e && e->modifiers()==Qt::AltModifier)
     {
     this->keyPressEvent(e);
     return true;
     }
   }
  return this->Superclass::eventFilter(obj, event);
}

//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::keyPressEvent(QKeyEvent *e)
{
  if ((e->key()==Qt::Key_Escape))
    {
    e->accept();
    this->close();
    }
  else if((e->modifiers()==Qt::AltModifier))
    {
    e->accept();
    if(e->key()==Qt::Key_N)
      {
      this->findNext();
      }
    else if(e->key()==Qt::Key_P)
      {
      this->findPrevious();
      }
    }
}
//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::showEvent(QShowEvent* e)
{
  this->activateWindow();
  this->Private->lineEditSearch->setFocus();
  this->Superclass::showEvent(e);
}

//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::updateSearch(QString searchText)
{
  this->Private->SearchString = searchText;
  QModelIndex current;  
  if(this->Private->CurrentFound.isValid())
    {
    this->Private->BaseWidget->model()->setData(
      this->Private->CurrentFound, Qt::white, Qt::BackgroundColorRole);
    }
  this->Private->CurrentFound = current;
  QAbstractItemView* theView = this->Private->BaseWidget;
  if (!theView || this->Private->SearchString.size() == 0)
    {
    this->Private->lineEditSearch->setPalette(this->Private->WhitePal);
    return;
    }
  const QString searchString =this->Private->SearchString;
  // Loop through all the model indices in the model
  QAbstractItemModel* viewModel = theView->model();

  for( int r = 0; r < viewModel->rowCount(); r++ )
    {
    for( int c = 0; c < viewModel->columnCount( ); c++ )
      {
      current = viewModel->index( r, c );
      if (this->searchModel( viewModel, current, searchString ))
        {
        return;
        }
      }
    }

  this->Private->lineEditSearch->setPalette(this->Private->RedPal);
}
//-----------------------------------------------------------------------------
bool pqItemViewSearchWidget::searchModel( const QAbstractItemModel * M,
  const QModelIndex & curIdx, const QString & searchString ) const
{
  bool found=false;
  if( !curIdx.isValid() )
    {
    return found;
    }
  pqWaitCursor wCursor;

  // Try to match the curIdx index itself
  if (this->matchSearchString(M, curIdx, searchString))
    {
    found = true;
    this->Private->BaseWidget->model()->setData(
      curIdx, Qt::green, Qt::BackgroundColorRole);
    this->Private->BaseWidget->scrollTo(curIdx);
    this->Private->CurrentFound = curIdx;
    this->Private->lineEditSearch->setPalette(this->Private->WhitePal);
    return found;
    }

  if(M->hasChildren(curIdx))
    {
    QModelIndex current;
    // Search curIdx index's children
    for( int r = 0; r < M->rowCount( curIdx )&& !found;r ++ )
      {
      for( int c = 0; c < M->columnCount( curIdx )&& !found;c ++ )
        {
        current = M->index( r, c, curIdx );
        found = this->searchModel( M, current, searchString );
        }
      }
    }

  return found;
}
//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::findNext()
{
  QAbstractItemView* theView = this->Private->BaseWidget;
  if (!theView || this->Private->SearchString.size() == 0)
    {
    return;
    }
  const QString searchString =this->Private->SearchString;

  // Loop through all the model indices in the model
  QModelIndexList match;
  QAbstractItemModel* viewModel = theView->model();
  QModelIndex current, firstmactch, start=this->Private->CurrentFound;
  if(start.isValid())
    {
    this->Private->BaseWidget->model()->setData(
      start, Qt::white, Qt::BackgroundColorRole);

    // search the rest of this index
    int r = start.row();
    for( int c = start.column()+1; c < viewModel->columnCount( ); c++ )
      {
      current = start.sibling(r, c);
      if (this->searchModel( viewModel, current, searchString ))
        {
        return;
        }
      }
    // If not found, start from next row
    for( r = start.row()+1; r < viewModel->rowCount(); r++ )
      {
      for( int c = 0; c < viewModel->columnCount( ); c++ )
        {
        current = viewModel->index( r, c );
        if (this->searchModel( viewModel, current, searchString ))
          {
          return;
          }
        }
      }
    // If still not found, start from (0,0)
    for( r = 0; r <= start.row(); r++ )
      {
      for( int c = 0; c < viewModel->columnCount( ); c++ )
        {
        current = viewModel->index( r, c );
        if (this->searchModel( viewModel, current, searchString ))
          {
          return;
          }
        }
      }
    this->Private->lineEditSearch->setPalette(this->Private->RedPal);
    }
  else
    {
    this->updateSearch();
    }
}
//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::updateSearch()
{
  this->updateSearch(this->Private->SearchString);
}
//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::findPrevious()
{
  QAbstractItemView* theView = this->Private->BaseWidget;
  if (!theView || this->Private->SearchString.size() == 0)
    {
    return;
    }
  const QString searchString =this->Private->SearchString;

  // Loop through all the model indices in the model
  QModelIndexList match;
  QAbstractItemModel* viewModel = theView->model();
  QModelIndex current, firstmactch, start=this->Private->CurrentFound;
  if(start.isValid())
    {
    this->Private->BaseWidget->model()->setData(
      start, Qt::white, Qt::BackgroundColorRole);
    // search the rest of this index
    int r = start.row();
    for( int c = start.column()-1; c >=0; c-- )
      {
      current = start.sibling(r, c);
      if (this->searchModel( viewModel, current, searchString ))
        {
        return;
        }
      }
    // If not found, start from previous row
    for( r = start.row()-1; r >=0; r-- )
      {
      for( int c = viewModel->columnCount()-1; c >=0; c-- )
        {
        current = viewModel->index( r, c );
        if (this->searchModel( viewModel, current, searchString ))
          {
          return;
          }
        }
      }
    // If still not found, start from the end(rowCount,columnCount)
    for( r = viewModel->rowCount()-1; r >= start.row(); r-- )
      {
      for( int c =viewModel->columnCount( )-1; c>=0 ; c-- )
        {
        current = viewModel->index( r, c );
        if (this->searchModel( viewModel, current, searchString ))
          {
          return;
          }
        }
      }
    this->Private->lineEditSearch->setPalette(this->Private->RedPal);
    }
  else
    {
    this->updateSearch();
    }
}

//-----------------------------------------------------------------------------
bool pqItemViewSearchWidget::matchSearchString(
  const QAbstractItemModel * M,
  const QModelIndex &idx, const QString &searchString) const
{
  Qt::CaseSensitivity cs = this->Private->checkBoxMattchCase->isChecked() ?
    Qt::CaseSensitive : Qt::CaseInsensitive;
  QVariant v = M->data(idx, Qt::DisplayRole);
  QString t = v.toString();
  if (t.contains(searchString, cs))
    {
    return true;
    }
  return false;
}
