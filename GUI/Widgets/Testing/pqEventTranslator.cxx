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

#include "pqAbstractButtonEventTranslator.h"
#include "pqAbstractSliderEventTranslator.h"
#include "pqComboBoxEventTranslator.h"
#include "pqDoubleSpinBoxEventTranslator.h"
#include "pqLineEditEventTranslator.h"
#include "pqMenuEventTranslator.h"
#include "pqSpinBoxEventTranslator.h"

#include "pqEventTranslator.h"

#include <QCoreApplication>
#include <QtDebug>
#include <QSet>
#include <QVector>

////////////////////////////////////////////////////////////////////////////////
// pqEventTranslator::pqImplementation

struct pqEventTranslator::pqImplementation
{
  ~pqImplementation()
  {
  for(int i = 0; i != this->Translators.size(); ++i)
    delete this->Translators[i];
  }

  /// Stores the working set of widget translators  
  QVector<pqWidgetEventTranslator*> Translators;
  /// Stores the set of objects that should be ignored when translating events
  QSet<QObject*> IgnoredObjects;
};

////////////////////////////////////////////////////////////////////////////////
// pqEventTranslator

pqEventTranslator::pqEventTranslator() :
  Implementation(new pqImplementation())
{
  QCoreApplication::instance()->installEventFilter(this);
}

pqEventTranslator::~pqEventTranslator()
{
  QCoreApplication::instance()->removeEventFilter(this);
  
  delete Implementation;
}

void pqEventTranslator::addDefaultWidgetEventTranslators()
{
  addWidgetEventTranslator(new pqAbstractButtonEventTranslator());
  addWidgetEventTranslator(new pqAbstractSliderEventTranslator());
  addWidgetEventTranslator(new pqComboBoxEventTranslator());
  addWidgetEventTranslator(new pqDoubleSpinBoxEventTranslator());
  addWidgetEventTranslator(new pqLineEditEventTranslator());
  addWidgetEventTranslator(new pqMenuEventTranslator());
  addWidgetEventTranslator(new pqSpinBoxEventTranslator());
}

void pqEventTranslator::addWidgetEventTranslator(pqWidgetEventTranslator* Translator)
{
  if(Translator)
    {
    this->Implementation->Translators.push_back(Translator);
    
    QObject::connect(
      Translator,
      SIGNAL(recordEvent(QObject*, const QString&, const QString&)),
      this,
      SLOT(onRecordEvent(QObject*, const QString&, const QString&)));
    }
}

void pqEventTranslator::ignoreObject(QObject* Object)
{
  this->Implementation->IgnoredObjects.insert(Object);
}

bool pqEventTranslator::eventFilter(QObject* Object, QEvent* Event)
{
  for(int i = 0; i != this->Implementation->Translators.size(); ++i)
    {
    bool error = false;
    if(this->Implementation->Translators[i]->translateEvent(Object, Event, error))
      {
      if(error)
        qWarning() << "Error translating an event for object " << Object;
      return false;
      }
    }
    
  return false;
}

void pqEventTranslator::onRecordEvent(QObject* Object, const QString& Command, const QString& Arguments)
{
  if(this->Implementation->IgnoredObjects.contains(Object))
    return;

  QString name = Object->objectName();
  if(name.isEmpty())
    {
    qWarning() << "Cannot record event for unnamed object " << Object;
    return;
    }
  
  for(QObject* p = Object->parent(); p; p = p->parent())
    {
    if(p->objectName().isEmpty())
      {
      qWarning() << "Cannot record event for incompletely-named object " << Object;
      return;
      }
      
    name = p->objectName() + "/" + name;
    }
  
  // qDebug() << "Event: " << name << " " << Command << " " << Arguments;
  emit recordEvent(name, Command, Arguments);
}

