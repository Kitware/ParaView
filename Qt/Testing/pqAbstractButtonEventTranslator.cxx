/*=========================================================================

   Program: ParaView
   Module:    pqAbstractButtonEventTranslator.cxx

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

#include "pqAbstractButtonEventTranslator.h"

#include <QAbstractButton>
#include <QAction>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>

#include <iostream>

pqAbstractButtonEventTranslator::pqAbstractButtonEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p)
{
}

bool pqAbstractButtonEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QAbstractButton* const object = qobject_cast<QAbstractButton*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::KeyPress:
      {
      QKeyEvent* const e = static_cast<QKeyEvent*>(Event);
      if(e->key() == Qt::Key_Space)
        {
        onActivate(object);
        }
      }
      break;
    case QEvent::MouseButtonPress:
      {
      QMouseEvent* const e = static_cast<QMouseEvent*>(Event);
      QPushButton* pushButton = qobject_cast<QPushButton*>(object);
      if(pushButton && 
         e->button() == Qt::LeftButton && 
         object->rect().contains(e->pos()) &&
         pushButton->menu())
        {
        onActivate(object);
        }
      }
    break;
    case QEvent::MouseButtonRelease:
      {
      QMouseEvent* const e = static_cast<QMouseEvent*>(Event);
      if(e->button() == Qt::LeftButton && object->rect().contains(e->pos()))
        {
        onActivate(object);
        }
      }
      break;
    default:
      break;
    }
      
  return true;
}

void pqAbstractButtonEventTranslator::onActivate(QAbstractButton* actualObject)
{
  QObject* object = actualObject;
  QToolButton* tb = qobject_cast<QToolButton*>(object);
  if (tb && tb->defaultAction())
    {
    object = tb->defaultAction();
    }
  if(actualObject->isCheckable())
    {
    const bool new_value = !actualObject->isChecked();
    emit recordEvent(object, "set_boolean", new_value ? "true" : "false");
    }
  else
    {
    emit recordEvent(object, "activate", "");
    }
}
