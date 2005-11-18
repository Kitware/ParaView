/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

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
  
  for(QObject* parent = Object->parent(); parent; parent = parent->parent())
    {
    if(parent->objectName().isEmpty())
      {
      qWarning() << "Incompletely-named object " << Object;
//      return;
      }
      
    name = parent->objectName() + "/" + name;
    }
  
  qDebug() << "Event: " << name << " " << Command << " " << Arguments;
  emit recordEvent(name, Command, Arguments);
}

