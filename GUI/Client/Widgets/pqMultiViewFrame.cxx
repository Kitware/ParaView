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

#include "pqMultiViewFrame.h"

#include <QStyle>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>
#include <QMenu>

static int gPenWidth = 2;

pqMultiViewFrame::pqMultiViewFrame(QWidget* p)
  : QWidget(p), MainWidget(0), AutoHide(false), Active(false), Color(QColor("red"))
{
  QVBoxLayout* boxLayout = new QVBoxLayout(this);
  boxLayout->setMargin(gPenWidth);
  boxLayout->setSpacing(gPenWidth);

  this->Menu = new QWidget(this);
  this->setupUi(Menu);
  boxLayout->addWidget(this->Menu);

  QVBoxLayout* sublayout = new QVBoxLayout();
  boxLayout->addLayout(sublayout);
  sublayout->addStretch();

  this->CloseButton->setIcon(QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarCloseButton)));
  this->MaximizeButton->setIcon(QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarMaxButton)));

  // set up actions
  QAction* a = new QAction(this->ActiveButton->icon(), tr("Active"), this->Menu);
  a->setCheckable(true);
  this->ActiveButton->setDefaultAction(a);
  a = new QAction(this->SplitHorizontalButton->icon(), this->SplitHorizontalButton->text(), this->Menu);
  this->SplitHorizontalButton->setDefaultAction(a);
  a = new QAction(this->SplitVerticalButton->icon(), this->SplitVerticalButton->text(), this->Menu);
  this->SplitVerticalButton->setDefaultAction(a);
  a = new QAction(this->MaximizeButton->icon(), this->MaximizeButton->text(), this->Menu);
  this->MaximizeButton->setDefaultAction(a);
  a = new QAction(this->CloseButton->icon(), this->CloseButton->text(), this->Menu);
  this->CloseButton->setDefaultAction(a);


  this->connect(this->ActiveButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(setActive(bool)));
  this->connect(this->CloseButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(close()));
  this->connect(this->MaximizeButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(maximize()));
  this->connect(this->SplitVerticalButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(splitVertical()));
  this->connect(this->SplitHorizontalButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(splitHorizontal()));
  
  // setup the context menu
  this->Menu->setContextMenuPolicy(Qt::ActionsContextMenu);
  this->Menu->addAction(this->SplitHorizontalButton->defaultAction());
  this->Menu->addAction(this->SplitVerticalButton->defaultAction());
  this->Menu->addAction(this->CloseButton->defaultAction());
  
  // TODO: temporary until they can be implemented or wanted
  this->MaximizeButton->hide();
  this->ActiveButton->hide();
}

pqMultiViewFrame::~pqMultiViewFrame()
{
}

void pqMultiViewFrame::setTitle(const QString& title)
{
  this->WindowCaption->setText(title);
  if(this->MainWidget)
    {
    this->MainWidget->setWindowTitle(title);
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
  if(this->ActiveButton->isChecked() != a)
    {
    this->ActiveButton->blockSignals(true);
    this->ActiveButton->setChecked(a);
    this->ActiveButton->blockSignals(false);
    }

  if(this->Active != a)
    {
    this->Active = a;
    emit this->activeChanged(a);
    this->update();
    }
}

QColor pqMultiViewFrame::borderColor() const
{
  return this->Color;
}

void pqMultiViewFrame::setBorderColor(QColor c)
{
  this->Color = c;
}

void pqMultiViewFrame::setMainWidget(QWidget* w)
{
  QLayout* l = this->layout()->itemAt(1)->layout();
  l->removeItem(l->itemAt(0));
  if(w)
    {
    l->addWidget(w);
    this->WindowCaption->setText(w->windowTitle());
    }
  else
    {
    static_cast<QBoxLayout*>(l)->addStretch();
    }
}

QWidget* pqMultiViewFrame::mainWidget()
{
  return this->layout()->itemAt(1)->layout()->itemAt(0)->widget();
}

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
    QLayoutItem* i = this->layout()->itemAt(0);
    QRect r = contentsRect();
    r.adjust(-gPenWidth/2+2, i->geometry().height()+4-gPenWidth/2, gPenWidth/2-2, gPenWidth/2-2);
    painter.drawRect(r);
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

void pqMultiViewFrame::splitVertical()
{
  emit this->splitVerticalPressed();
}

void pqMultiViewFrame::splitHorizontal()
{
  emit this->splitHorizontalPressed();
}


