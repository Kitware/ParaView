/*=========================================================================

   Program: ParaView
   Module:    pqSpinBoxEventTranslator.cxx

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

#include "pqSpinBoxEventTranslator.h"

#include <QSpinBox>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QStyle>
#include <QStyleOptionSpinBox>

pqSpinBoxEventTranslator::pqSpinBoxEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p)
{
}

bool pqSpinBoxEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QSpinBox* object = qobject_cast<QSpinBox*>(Object);
  
  // consume line edit events if part of spin box
  if(!object && qobject_cast<QSpinBox*>(Object->parent()))
    {
    return true;
    }

  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::MouseButtonPress:
      {
      QMouseEvent* me = static_cast<QMouseEvent*>(Event);
      if(me->button() == Qt::LeftButton)
        {
        QStyle* style = object->style();
        QStyleOptionSpinBox option;
        option.initFrom(object);
        option.subControls = QStyle::SC_All;
        QStyle::SubControl sub = style->hitTestComplexControl(
          QStyle::CC_SpinBox, &option, me->pos(), object);
        if(sub == QStyle::SC_SpinBoxUp)
          {
          emit recordEvent(object, "spin", "up");
          }
        else if(sub == QStyle::SC_SpinBoxDown)
          {
          emit recordEvent(object, "spin", "down");
          }
        }
      }
      break;
    case QEvent::KeyRelease:
      {
      QKeyEvent* ke = static_cast<QKeyEvent*>(Event);
      QString keyText = ke->text();
      if(keyText.length() && keyText.at(0).isLetterOrNumber())
        {
        emit recordEvent(object, "set_int", QString("%1").arg(object->value()));
        }
      else
        {
        emit recordEvent(object, "key", QString("%1").arg(ke->key()));
        }
      }
    default:
      break;
    }
      
  return true;
}

