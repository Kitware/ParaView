/*=========================================================================

   Program: ParaView
   Module:    pqDoubleSpinBoxEventTranslator.cxx

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

#include "pqDoubleSpinBoxEventTranslator.h"

#include <QDoubleSpinBox>
#include <QEvent>

pqDoubleSpinBoxEventTranslator::pqDoubleSpinBoxEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p),
  CurrentObject(0)
{
}

bool pqDoubleSpinBoxEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QDoubleSpinBox* const object = qobject_cast<QDoubleSpinBox*>(Object);
  if(!object)
    return false;
  
  // consume line edit events if part of spin box
  if(!object && qobject_cast<QDoubleSpinBox*>(Object->parent()))
    {
    return true;
    }
    
  switch(Event->type())
    {
    case QEvent::FocusIn:
      this->CurrentObject = Object;
      connect(object, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
      break;
    case QEvent::FocusOut:
      disconnect(Object, 0, this, 0);
      this->CurrentObject = 0;
      break;
    default:
      break;
    }
      
  return true;
}

void pqDoubleSpinBoxEventTranslator::onValueChanged(double Value)
{
  emit recordEvent(this->CurrentObject, "set_double", QString().setNum(Value));
}
