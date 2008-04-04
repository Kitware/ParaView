/*=========================================================================

   Program: ParaView
   Module:    pqBasicWidgetEventTranslator.cxx

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

#include "pqBasicWidgetEventTranslator.h"

#include <QEvent>
#include <QKeyEvent>
#include <QWidget>

pqBasicWidgetEventTranslator::pqBasicWidgetEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p)
{
}

pqBasicWidgetEventTranslator::~pqBasicWidgetEventTranslator()
{
}

bool pqBasicWidgetEventTranslator::translateEvent(QObject* Object, 
                                                  QEvent* Event, 
                                                  bool& /*Error*/)
{
  QWidget* const object = qobject_cast<QWidget*>(Object);
  if(!object)
    return false;

  switch(Event->type())
    {
    case QEvent::ContextMenu:
      {
      emit recordEvent(Object, "contextMenu", "");
      }
      break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
      {
      QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(Event);
      QString info = QString("%1,%2,%3,%4,%5")
        .arg(mouseEvent->button())
        .arg(mouseEvent->buttons())
        .arg(mouseEvent->modifiers())
        .arg(mouseEvent->x())
        .arg(mouseEvent->y());

      if(Event->type() != QEvent::MouseButtonRelease)
        {
        this->LastPos = mouseEvent->pos();
        }

      if(Event->type() == QEvent::MouseButtonPress)
        {
        emit recordEvent(object, "mousePress", info);
        }
      if(Event->type() == QEvent::MouseButtonDblClick)
        {
          emit recordEvent(object, "mouseDblClick", info);
        }
      else if(Event->type() == QEvent::MouseButtonRelease)
        {
        if(this->LastPos != mouseEvent->pos())
          {
          emit recordEvent(object, "mouseMove", info);
          }
        emit recordEvent(object, "mouseRelease", info);
        }
      }
      break;
    default:
      break;
    }
      
  return true;
}

