/*=========================================================================

   Program:   ParaQ
   Module:    pqMenuEventTranslator.cxx

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

#include "pqMenuEventTranslator.h"
#include "pqMenuEventTranslatorAdaptor.h"

#include <QEvent>
#include <QMenu>

pqMenuEventTranslator::pqMenuEventTranslator()
{
}

pqMenuEventTranslator::~pqMenuEventTranslator()
{
  clearActions();
}

bool pqMenuEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QMenu* const object = qobject_cast<QMenu*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      {
      this->clearActions();
      QList<QAction*> actions = object->actions();
      for(int i = 0; i != actions.size(); ++i)
        {
        pqMenuEventTranslatorAdaptor* const adaptor = new pqMenuEventTranslatorAdaptor(actions[i]);
        this->Actions.push_back(adaptor);
        QObject::connect(
          adaptor,
          SIGNAL(recordEvent(QObject*, const QString&, const QString&)),
          this,
          SLOT(onRecordEvent(QObject*, const QString&, const QString&)));
        }
      }
      break;
    default:
      break;
    }
      
  return true;
}

void pqMenuEventTranslator::clearActions()
{
  for(int i = 0; i != this->Actions.size(); ++i)
    delete this->Actions[i];
  this->Actions.clear();
}

void pqMenuEventTranslator::onRecordEvent(QObject* Object, const QString& Command, const QString& Arguments)
{
  emit recordEvent(Object, Command, Arguments);
}
