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

#include "pqSpinBoxEventTranslator.h"

#include <QSpinBox>
#include <QEvent>

pqSpinBoxEventTranslator::pqSpinBoxEventTranslator() :
  CurrentObject(0)
{
}

bool pqSpinBoxEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QSpinBox* const object = qobject_cast<QSpinBox*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      this->CurrentObject = Object;
      connect(object, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
      break;
    case QEvent::Leave:
      disconnect(Object, 0, this, 0);
      this->CurrentObject = 0;
      break;
    default:
      break;
    }
      
  return true;
}

void pqSpinBoxEventTranslator::onValueChanged(int Value)
{
  emit recordEvent(this->CurrentObject, "set_int", QString().setNum(Value));
}
