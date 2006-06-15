/*=========================================================================

   Program:   ParaQ
   Module:    pqLineEditEventTranslator.cxx

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

#include "pqLineEditEventTranslator.h"

#include <QEvent>
#include <QLineEdit>
#include <QSpinBox>

pqLineEditEventTranslator::pqLineEditEventTranslator()
{
}

bool pqLineEditEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QLineEdit* const object = qobject_cast<QLineEdit*>(Object);
  if(!object)
    return false;
  
  // If this line edit is part of a spinbox, don't translate events (the spinbox translator will receive the final value directly)
  if(qobject_cast<QSpinBox*>(Object->parent()))
    return false;

  switch(Event->type())
    {
    case QEvent::KeyRelease:
      {
      emit recordEvent(Object, "set_string", object->text());
      }
      break;
    default:
      break;
    }

  return true;
}
