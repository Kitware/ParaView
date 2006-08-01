/*=========================================================================

   Program: ParaView
   Module:    pqCollapsedGroup.cxx

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

#include "pqCollapsedGroup.h"

#include <QApplication>
#include <QResizeEvent>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QVBoxLayout>
#include <QtDebug>

#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// pqCollapsedGroup::pqImplementation

class pqCollapsedGroup::pqImplementation
{
public:
  pqImplementation(const QString& title) :
    Expanded(true),
    Title(title),
    Indent(30),
    Inside(false),
    Pressed(false)
  {
  }

  bool Expanded;
  QString Title;
  int Indent;
  QRect ButtonRect;
  bool Inside;
  bool Pressed;
};

pqCollapsedGroup::pqCollapsedGroup(QWidget* parent_widget) :
  QWidget(parent_widget),
  Implementation(new pqImplementation(""))
{
  QApplication::instance()->installEventFilter(this);
}

pqCollapsedGroup::pqCollapsedGroup(const QString& title, QWidget* parent_widget) :
  QWidget(parent_widget),
  Implementation(new pqImplementation(title))
{
  QApplication::instance()->installEventFilter(this);
}

pqCollapsedGroup::~pqCollapsedGroup()
{
  QApplication::instance()->removeEventFilter(this);
  delete this->Implementation;
}

void pqCollapsedGroup::setWidget(QWidget* child)
{
  QVBoxLayout* const layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(child);
  this->setLayout(layout);
}

const bool pqCollapsedGroup::isExpanded()
{
  return this->Implementation->Expanded;
}

void pqCollapsedGroup::expand()
{
  if(!this->Implementation->Expanded)
    this->toggle();
}

void pqCollapsedGroup::collapse()
{
  if(this->Implementation->Expanded)
    this->toggle();
}

void pqCollapsedGroup::toggle()
{
  this->Implementation->Expanded = 
    !this->Implementation->Expanded;
   
  const QObjectList& children = this->children();
  for(int i = 0; i != children.size(); ++i)
    {
    if(QWidget* const widget = qobject_cast<QWidget*>(children[i]))
      {
      widget->setVisible(this->Implementation->Expanded);
      }
    }

  this->update();
}

void pqCollapsedGroup::setTitle(const QString& txt)
{
  this->Implementation->Title = txt;
  this->update();
}

QString pqCollapsedGroup::title() const
{
  return this->Implementation->Title;
}

void pqCollapsedGroup::setIndent(int indent)
{
  this->Implementation->Indent = indent;
  
  // This hack forces a layout update so changes in
  // Designer are shown immediately
  const QSize old_size = this->size();
  this->resize(old_size + QSize(1, 1));
  this->resize(old_size);
}

int pqCollapsedGroup::indent() const
{
  return this->Implementation->Indent;
}
  
void pqCollapsedGroup::resizeEvent(QResizeEvent* event)
{
  // Base the height of the button on the height of the font,
  // and ensure that the result is an odd number (so the icon centers nicely)
  const int height =
    static_cast<int>(this->fontMetrics().height() * 1.3) | 0x01;

  this->Implementation->ButtonRect =
    QRect(0, 0, event->size().width(), height);
  this->setContentsMargins(this->Implementation->Indent, this->Implementation->ButtonRect.height(), 0, 0);
  this->update();
}

void pqCollapsedGroup::paintEvent(QPaintEvent* event)
{
  const QRect button_rect = this->Implementation->ButtonRect;

  QStylePainter painter(this);

  QStyleOptionButton button_options;
  button_options.initFrom(this);
  button_options.features = QStyleOptionButton::None;
  button_options.rect = button_rect;

  button_options.state = QStyle::State_Enabled;

  if(this->Implementation->Inside && !this->Implementation->Pressed)
    button_options.state |= QStyle::State_MouseOver;
/*
  else
    button_options.state &= ~QStyle::State_Default;
*/

  if(this->Implementation->Pressed && this->Implementation->Inside)
    button_options.state |= QStyle::State_Sunken;
  else
    button_options.state |= QStyle::State_Raised;

  painter.drawControl(QStyle::CE_PushButton, button_options);

  const int icon_size = 9; // hardcoded in qcommonstyle.cpp

  QStyleOption icon_options;
  icon_options.rect = QRect(
    button_rect.left() + icon_size / 2,
    button_rect.top() + (button_rect.height() - icon_size) / 2,
    icon_size,
    icon_size);
  icon_options.palette = button_options.palette;
  icon_options.state = QStyle::State_Children;

  if(this->Implementation->Expanded)
    icon_options.state |= QStyle::State_Open;

  painter.drawPrimitive(QStyle::PE_IndicatorBranch, icon_options);
  
  const QRect text_rect = QRect(
    button_rect.left() + icon_size * 2,
    button_rect.top(),
    button_rect.width() - icon_size * 2,
    button_rect.height());
    
  const QString text = this->Implementation->Title;
  painter.drawItemText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, button_options.palette, true, text);
}

bool pqCollapsedGroup::eventFilter(QObject* target, QEvent* event)
{
  if(event->type() == QEvent::MouseMove)
    {
    for(QObject* parent = target; parent; parent = parent->parent())
      {
      if(parent == this)
        {
        const QPoint mouse = this->mapFromGlobal(qobject_cast<QWidget*>(target)->mapToGlobal(static_cast<QMouseEvent*>(event)->pos()));
        const bool inside = this->Implementation->ButtonRect.contains(mouse);
        if(inside != this->Implementation->Inside)
          {
          this->Implementation->Inside = inside;
          this->update();
          }
        break;
        }
      }
    }
    
  return QWidget::eventFilter(target, event);
}

void pqCollapsedGroup::mousePressEvent(QMouseEvent* event)
{
  if(this->Implementation->ButtonRect.contains(event->pos()))
    {
    this->Implementation->Pressed = true;
    this->update();
    }
}

void pqCollapsedGroup::mouseReleaseEvent(QMouseEvent* event)
{
  if(this->Implementation->Pressed)
    {
    this->Implementation->Pressed = false;
    this->update();

    if(this->Implementation->ButtonRect.contains(event->pos()))
      {
      this->toggle();
      }
    }
}

void pqCollapsedGroup::leaveEvent(QEvent*)
{
  if(this->Implementation->Inside)
    {
    this->Implementation->Inside = false;
    this->update();
    }
}
