/*=========================================================================

   Program: ParaView
   Module:    pqMultiViewFrame.cxx

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

#include "pqMultiViewFrame.h"

#include <QStyle>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>
#include <QMenu>
#include <QUuid>
#include <QApplication>
#include <QMouseEvent>

static int gPenWidth = 2;

pqMultiViewFrame::pqMultiViewFrame(QWidget* p)
  : QWidget(p), EmptyMainWidget(new QWidget(this)), AutoHide(false), Active(false), 
    Color(QColor("blue"))
{
  QVBoxLayout* boxLayout = new QVBoxLayout(this);
  boxLayout->setMargin(gPenWidth);
  boxLayout->setSpacing(gPenWidth);

  this->Menu = new QWidget(this);
  this->setupUi(Menu);
  boxLayout->addWidget(this->Menu);
  this->Menu->installEventFilter(this);

  QVBoxLayout* sublayout = new QVBoxLayout();
  boxLayout->addLayout(sublayout);
  sublayout->addStretch();
  
  this->CloseButton->setIcon(
    QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarCloseButton)));
  this->MaximizeButton->setIcon(
    QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarMaxButton)));
  this->RestoreButton->setIcon(
    QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarNormalButton)));

  // set up actions
  QAction* a = new QAction(this->ActiveButton->icon(), tr("Active"), this->Menu);
  a->setObjectName("ActiveAction");
  a->setCheckable(true);
  this->ActiveButton->setDefaultAction(a);
  a = new QAction(this->SplitHorizontalButton->icon(), 
                  this->SplitHorizontalButton->text(), 
                  this->Menu);
  a->setObjectName("SplitHorizontalAction");
  this->SplitHorizontalButton->setDefaultAction(a);
  a = new QAction(this->SplitVerticalButton->icon(), 
                  this->SplitVerticalButton->text(), 
                  this->Menu);
  a->setObjectName("SplitVerticalAction");
  this->SplitVerticalButton->setDefaultAction(a);
  a = new QAction(this->MaximizeButton->icon(), 
                  this->MaximizeButton->text(), 
                  this->Menu);
  a->setObjectName("MaximizeAction");
  this->MaximizeButton->setDefaultAction(a);
  a = new QAction(this->RestoreButton->icon(), 
                  this->RestoreButton->text(), 
                  this->Menu);
  a->setObjectName("RestoreAction");
  this->RestoreButton->setDefaultAction(a);
  a = new QAction(this->CloseButton->icon(), 
                  this->CloseButton->text(), 
                  this->Menu);
  a->setObjectName("CloseAction");
  this->CloseButton->setDefaultAction(a);

  this->connect(this->ActiveButton->defaultAction(), SIGNAL(triggered(bool)), 
                SLOT(setActive(bool)));
  this->connect(this->CloseButton->defaultAction(), SIGNAL(triggered(bool)), 
                SLOT(close()), Qt::QueuedConnection);
  this->connect(this->MaximizeButton->defaultAction(), 
                SIGNAL(triggered(bool)), 
                SLOT(maximize()), Qt::QueuedConnection);
  this->connect(this->RestoreButton->defaultAction(), 
                SIGNAL(triggered(bool)), 
                SLOT(restore()), Qt::QueuedConnection);
  this->connect(this->SplitVerticalButton->defaultAction(), 
                SIGNAL(triggered(bool)), 
                SLOT(splitVertical()), Qt::QueuedConnection);
  this->connect(this->SplitHorizontalButton->defaultAction(), 
                SIGNAL(triggered(bool)), 
                SLOT(splitHorizontal()), Qt::QueuedConnection);

  // setup the context menu
  this->Menu->setContextMenuPolicy(Qt::CustomContextMenu);
  this->Menu->setAcceptDrops(true);
  this->setAcceptDrops(true);
  QObject::connect(this->Menu,  
    SIGNAL(customContextMenuRequested(const QPoint&)),
    this, SLOT(onCustomContextMenuRequested(const QPoint&)));

  this->ContextMenu = new QMenu(this->Menu);
  this->ContextMenu->setObjectName("FrameContextMenu");
  this->ContextMenu->addAction(this->SplitHorizontalButton->defaultAction());
  this->ContextMenu->addAction(this->SplitVerticalButton->defaultAction());
  this->ContextMenu->addAction(this->CloseButton->defaultAction());

  this->MenuHidden=false;
  // TODO: temporary until they can be implemented or wanted
  this->RestoreButton->hide();
  this->ActiveButton->hide();
  this->MaximizeButton->hide();
  this->CloseButton->hide();
  this->SplitVerticalButton->hide();
  this->SplitHorizontalButton->hide();


  this->UniqueID=QUuid::createUuid();

  // to allow an empty frame to work with the focus stuff
  this->setFocusPolicy(Qt::ClickFocus);

  this->setMainWidget(NULL);
}

pqMultiViewFrame::~pqMultiViewFrame()
{
}


void pqMultiViewFrame::addTitlebarAction(QAction* action)
{
  this->TitlebarActions.append(action);
  QToolButton* button = new QToolButton(this);
  button->setDefaultAction(action);
  button->setObjectName(action->objectName());
  this->hboxLayout->insertWidget(0,button);
}
void pqMultiViewFrame::removeTitlebarAction(QAction* action)
{
  this->TitlebarActions.removeAll(action);
  QToolButton* button=this->Menu->findChild<QToolButton*>(action->objectName());
  
  if(button)
    delete button;

}


QAction* pqMultiViewFrame::getAction(QString name)
{
  QList<QAction*>::iterator i;
  for (i = this->TitlebarActions.begin(); i !=  this->TitlebarActions.end(); ++i)
    {
    QAction *action= *i;
    if(!QString::compare(action->objectName(),name))
      return action;
    }


  return NULL;
}
void pqMultiViewFrame::onCustomContextMenuRequested(const QPoint& point)
{
  // get the focus before showing the context menu.
  // When the context menu closes, it returns the focus to the wiget with the previous
  // focus which can cause issues.
  this->setFocus(Qt::OtherFocusReason);
  this->setActive(true);

  emit this->contextMenuRequested();
  this->ContextMenu->exec(this->Menu->mapToGlobal(point));
}


void pqMultiViewFrame::setTitle(const QString& title)
{
  this->WindowCaption->setText(title);
}

void pqMultiViewFrame::hideMenu(bool vis)
{
  if(vis && !this->MenuHidden)
  {
    this->MenuHidden=true;
    QLayout* l=this->layout();
    this->Menu->hide();
    l->removeWidget(this->Menu);
  }
  else if(!vis && this->MenuHidden)
  {
    this->MenuHidden=false;
    QLayout* l=this->layout();
    l->addWidget(this->Menu);
    this->Menu->show();
  }
}
bool pqMultiViewFrame::menuAutoHide() const
{
  return this->AutoHide;
}

void pqMultiViewFrame::setMenuAutoHide(bool autohide)
{
  this->AutoHide = autohide;
}

bool pqMultiViewFrame::active() const
{
  return this->Active;
}

void pqMultiViewFrame::setActive(bool a)
{
  if (this->Active == a)
    {
    return;
    }
  this->Active = a;

  if(this->ActiveButton->defaultAction()->isChecked() != a)
    {
    this->ActiveButton->defaultAction()->setChecked(a);
    }

  emit this->activeChanged(a);
  this->update();
}

QColor pqMultiViewFrame::borderColor() const
{
  return this->Color;
}

void pqMultiViewFrame::setBorderColor(QColor c)
{
  this->Color = c;
}

//-----------------------------------------------------------------------------
void pqMultiViewFrame::setMainWidget(QWidget* w)
{
  this->mainWidgetPreChange(this);
  QLayout* l;
  if(this->MenuHidden)
    {
    l = this->layout()->itemAt(0)->layout();
    }
  else
    {
    l = this->layout()->itemAt(1)->layout();
    }
  QLayoutItem* item = l->takeAt(0);
  if(item)
    {
    delete item;
    }

  if(w)
    {
    l->addWidget(w);
    this->WindowCaption->setText(w->windowTitle());
    this->EmptyMainWidget->hide();
    }
  else
    {
    l->addWidget(this->EmptyMainWidget);
    this->EmptyMainWidget->show();
    //static_cast<QBoxLayout*>(l)->addStretch();
    }

  this->mainWidgetChanged(this);
}

//-----------------------------------------------------------------------------
QWidget* pqMultiViewFrame::mainWidget()
{
  QLayout* l;
  if(this->MenuHidden)
    {
    l = this->layout()->itemAt(0)->layout();
    }
  else
    {
    l = this->layout()->itemAt(1)->layout();
    }

  if (l->itemAt(0))
    {
    return l->itemAt(0)->widget();
    }
  return NULL;

}

//-----------------------------------------------------------------------------
void pqMultiViewFrame::paintEvent(QPaintEvent* e)
{
  QWidget::paintEvent(e);
  if(this->Active)
    {
    QPainter painter(this);
    QPen pen;
    pen.setColor(this->Color);
    pen.setWidth(gPenWidth);
    painter.setPen(pen);
    if(this->MenuHidden)
      {
      QRect r = contentsRect();
      r.adjust(-gPenWidth/2+2, 
              gPenWidth/2, 
              gPenWidth/2-2, 
              gPenWidth/2-2);
      painter.drawRect(r);
     }
    else
      {
      QLayoutItem* i = this->layout()->itemAt(0);
      QRect r = contentsRect();
      r.adjust(-gPenWidth/2+2, 
              i->geometry().height()+4-gPenWidth/2, 
              gPenWidth/2-2, 
              gPenWidth/2-2);
      painter.drawRect(r);
      }
    }
}

void pqMultiViewFrame::close()
{
  emit this->closePressed();
}

void pqMultiViewFrame::maximize()
{
  emit this->maximizePressed();
}
void pqMultiViewFrame::restore()
{
  emit this->restorePressed();
}
void pqMultiViewFrame::splitVertical()
{
  emit this->splitVerticalPressed();
}

void pqMultiViewFrame::splitHorizontal()
{
  emit this->splitHorizontalPressed();
}

QUuid pqMultiViewFrame::uniqueID() const
{
  return this->UniqueID;
}

bool pqMultiViewFrame::event(QEvent* e)
{
  if(e->type() == QEvent::DragEnter)
    {
    QDragEnterEvent* de=reinterpret_cast<QDragEnterEvent*>(e);
    emit(dragEnter(this,de));
    }
  else if(e->type() == QEvent::DragMove)
    {
    QDragMoveEvent* de=reinterpret_cast<QDragMoveEvent*>(e);
    emit(dragMove(this,de));
    }
  else if(e->type() == QEvent::Drop)
    {
    QDropEvent* de=reinterpret_cast<QDropEvent*>(e);
    emit(drop(this,de));
    }
  return QWidget::event(e);
}

bool pqMultiViewFrame::eventFilter(QObject* caller, QEvent* e)
{
  if(e->type() == QEvent::MouseButtonPress)
    {     
    QMouseEvent *mouseEvent=(QMouseEvent*)e;
    if (mouseEvent->button() == Qt::LeftButton)
      {
      DragStartPosition = mouseEvent->pos();
      }
    return caller->event(e);
    }
  else if(e->type() == QEvent::MouseMove)
    {
    QMouseEvent *mouseEvent=(QMouseEvent*)e;
    if (!(mouseEvent->buttons() & Qt::LeftButton))
      {
      return caller->event(e);
      }
    if ((mouseEvent->pos() - this->DragStartPosition).manhattanLength()
      < QApplication::startDragDistance())
      {
      return caller->event(e);
      }

    emit(dragStart(this));

    }
  else if(e->type() == QEvent::DragEnter)
    {
    QDragEnterEvent* de=reinterpret_cast<QDragEnterEvent*>(e);
    emit(dragEnter(this,de));
    }
  else if(e->type() == QEvent::DragMove)
    {
    QDragMoveEvent* de=reinterpret_cast<QDragMoveEvent*>(e);
    emit(dragMove(this,de));
    }
  else if(e->type() == QEvent::Drop)
    {
    QDropEvent* de=reinterpret_cast<QDropEvent*>(e);
    emit(drop(this,de));
    }

  return caller->event(e);
}

//-----------------------------------------------------------------------------
void pqMultiViewFrame::hideDecorations()
{
//  this->hideMenu(true);
  this->Menu->hide();
}

//-----------------------------------------------------------------------------
void pqMultiViewFrame::showDecorations()
{
//  this->hideMenu(false);
  this->Menu->show();
}

