/*=========================================================================

   Program: ParaView
   Module:    pqComboBoxEventTranslator.cxx

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

#include "pqComboBoxEventTranslator.h"

#include <QComboBox>
#include <QEvent>

pqComboBoxEventTranslator::pqComboBoxEventTranslator(QObject* p) 
  : pqWidgetEventTranslator(p),
  CurrentObject(0)
{
}

bool pqComboBoxEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QComboBox* combo = NULL;
  for(QObject* test = Object; combo == NULL && test != NULL; test = test->parent())
    {
    combo = qobject_cast<QComboBox*>(test);
    }

  if(!combo)
    {
    // not for me
    return false;
    }

  if(Event->type() == QEvent::Enter && Object == combo)
    {
    if(this->CurrentObject != Object)
      {
      if(this->CurrentObject)
        {
        disconnect(this->CurrentObject, 0, this, 0);
        }
      
      this->CurrentObject = Object;
      connect(combo, SIGNAL(destroyed(QObject*)), this, SLOT(onDestroyed(QObject*)));
      connect(combo, SIGNAL(activated(const QString&)), this, SLOT(onStateChanged(const QString&)));
      connect(combo, SIGNAL(editTextChanged(const QString&)), this, SLOT(onStateChanged(const QString&)));
      }
    }
  
  return true;
}

void pqComboBoxEventTranslator::onDestroyed(QObject* /*Object*/)
{
  this->CurrentObject = 0;
}

void pqComboBoxEventTranslator::onStateChanged(const QString& State)
{
  emit recordEvent(this->CurrentObject, "set_string", State);
}
