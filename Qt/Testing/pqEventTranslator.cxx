/*=========================================================================

   Program: ParaView
   Module:    pqEventTranslator.cxx

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

#include "pqEventTranslator.h"

#include "pqAbstractButtonEventTranslator.h"
#include "pqAbstractItemViewEventTranslator.h"
#include "pqAbstractSliderEventTranslator.h"
#include "pqBasicWidgetEventTranslator.h"
#include "pqComboBoxEventTranslator.h"
#include "pqDoubleSpinBoxEventTranslator.h"
#include "pqLineEditEventTranslator.h"
#include "pqMenuEventTranslator.h"
#include "pqObjectNaming.h"
#include "pqSpinBoxEventTranslator.h"
#include "pqTabBarEventTranslator.h"
#include "pqTreeViewEventTranslator.h"

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
  }

  /// Stores the working set of widget translators  
  QVector<pqWidgetEventTranslator*> Translators;
  /// Stores the set of objects that should be ignored when translating events
  QSet<QObject*> IgnoredObjects;

  // list of widgets for which mouse propagation will happen
  // we'll only translate the first and ignore the rest
  QList<QWidget*> MouseParents;
};

////////////////////////////////////////////////////////////////////////////////
// pqEventTranslator

pqEventTranslator::pqEventTranslator(QObject* p)
 : QObject(p),
  Implementation(new pqImplementation())
{
}

pqEventTranslator::~pqEventTranslator()
{
  this->stop();
  delete Implementation;
}

void pqEventTranslator::start()
{
  QCoreApplication::instance()->installEventFilter(this);
}

void pqEventTranslator::stop()
{
  QCoreApplication::instance()->removeEventFilter(this);
}

void pqEventTranslator::addDefaultWidgetEventTranslators()
{
  addWidgetEventTranslator(new pqBasicWidgetEventTranslator());
  addWidgetEventTranslator(new pqAbstractButtonEventTranslator());
  addWidgetEventTranslator(new pqAbstractItemViewEventTranslator());
  addWidgetEventTranslator(new pqAbstractSliderEventTranslator());
  addWidgetEventTranslator(new pqComboBoxEventTranslator());
  addWidgetEventTranslator(new pqDoubleSpinBoxEventTranslator());
  addWidgetEventTranslator(new pqLineEditEventTranslator());
  addWidgetEventTranslator(new pqMenuEventTranslator());
  addWidgetEventTranslator(new pqSpinBoxEventTranslator());
  addWidgetEventTranslator(new pqTabBarEventTranslator());
  addWidgetEventTranslator(new pqTreeViewEventTranslator());
}

void pqEventTranslator::addWidgetEventTranslator(pqWidgetEventTranslator* Translator)
{
  if(Translator)
    {
    this->Implementation->Translators.push_front(Translator);
    Translator->setParent(this);
    
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

  // mouse events are propagated to parents
  // our event translators/players don't quite like that,
  // so lets consume those extra ones
  if(Event->type() == QEvent::MouseButtonPress ||
     Event->type() == QEvent::MouseButtonDblClick ||
     Event->type() == QEvent::MouseButtonRelease)
    {
    if(!this->Implementation->MouseParents.empty() &&
      this->Implementation->MouseParents.first() == Object)
      {
      // right on track
      this->Implementation->MouseParents.removeFirst();
      return false;
      }

    QWidget* widget = qobject_cast<QWidget*>(Object);
    
    // find the chain of parent that will get this mouse event
    this->Implementation->MouseParents.clear();
    for(QWidget* w = widget->parentWidget(); w; w = w->parentWidget())
      {
      this->Implementation->MouseParents.append(w);
      if(w->isWindow() || w->testAttribute(Qt::WA_NoMousePropagation))
        {
        break;
        }
      }
    }

  for(int i = 0; i != this->Implementation->Translators.size(); ++i)
    {
    bool error = false;
    if(this->Implementation->Translators[i]->translateEvent(Object, Event, error))
      {
      if(error)
        {
        qWarning() << "Error translating an event for object " << Object;
        }
      return false;
      }
    }

  return false;
}

void pqEventTranslator::onRecordEvent(QObject* Object, const QString& Command, const QString& Arguments)
{
  if(this->Implementation->IgnoredObjects.contains(Object))
    return;

  const QString name = pqObjectNaming::GetName(*Object);
  if(name.isEmpty())
    return;
    
//  qDebug() << "Event: " << name << " " << Command << " " << Arguments;
  emit recordEvent(name, Command, Arguments);
}
