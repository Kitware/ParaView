/*=========================================================================

   Program: ParaView
   Module:    pqTestUtility.cxx

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

#include "pqTestUtility.h"

#include <QFileInfo>
#include <QApplication>

#include "pqEventSource.h"
#include "pqEventObserver.h"
#include "pqRecordEventsDialog.h"
#include "QtTestingConfigure.h"

#ifdef QT_TESTING_WITH_XML
#include "pqXMLEventSource.h"
#include "pqXMLEventObserver.h"
#endif

#ifdef QT_TESTING_WITH_PYTHON
#include "pqPythonEventSource.h"
#include "pqPythonEventObserver.h"
#endif


pqTestUtility::pqTestUtility(QObject* p) :
  QObject(p)
{
  QObject::connect(
    &this->Dispatcher,
    SIGNAL(succeeded()),
    this,
    SLOT(testSucceeded()));
    
  QObject::connect(
    &this->Dispatcher,
    SIGNAL(failed()),
    this,
    SLOT(testFailed()));
  
  this->Translator.addDefaultWidgetEventTranslators();
  this->Player.addDefaultWidgetEventPlayers();

#ifdef QT_TESTING_WITH_XML
  // add an XML source
  this->addEventSource("xml", new pqXMLEventSource(this));
  this->addEventObserver("xml", new pqXMLEventObserver(this));
#endif

#ifdef QT_TESTING_WITH_PYTHON
  // add a python event source
  this->addEventSource("py", new pqPythonEventSource(this));
  this->addEventObserver("py", new pqPythonEventObserver(this));
#endif
}

pqTestUtility::~pqTestUtility()
{
}
  
pqEventDispatcher* pqTestUtility::dispatcher()
{
  return &this->Dispatcher;
}

pqEventPlayer* pqTestUtility::eventPlayer()
{
  return &this->Player;
}

pqEventTranslator* pqTestUtility::eventTranslator()
{
  return &this->Translator;
}

void pqTestUtility::addEventSource(const QString& fileExtension, pqEventSource* source)
{
  QMap<QString, pqEventSource*>::iterator iter;
  iter = this->EventSources.find(fileExtension);
  if(iter != this->EventSources.end())
    {
    pqEventSource* src = iter.value();
    this->EventSources.erase(iter);
    delete src;
    }
  this->EventSources.insert(fileExtension, source);
  source->setParent(this);
}

void pqTestUtility::addEventObserver(const QString& fileExtension,
                                     pqEventObserver* observer)
{
  QMap<QString, pqEventObserver*>::iterator iter;
  iter = this->EventObservers.find(fileExtension);
  if(iter != this->EventObservers.end() && iter.value() != observer)
    {
    pqEventObserver* src = iter.value();
    this->EventObservers.erase(iter);
    delete src;
    }
  if(iter != this->EventObservers.end() && iter.value() == observer)
    {
    return;
    }
  this->EventObservers.insert(fileExtension, observer);
  observer->setParent(this);
  

}

void pqTestUtility::playTests(const QString& filename)
{
  QFileInfo info(filename);
  QString suffix = info.completeSuffix();
  QMap<QString, pqEventSource*>::iterator iter;
  iter = this->EventSources.find(suffix);
  if(info.isReadable() && iter != this->EventSources.end())
    {
    iter.value()->setContent(filename);
    this->Dispatcher.playEvents(*iter.value(), this->Player);
    }
}

void pqTestUtility::playTests(const QStringList& filenames)
{
  foreach(QString filename, filenames)
    {
    this->playTests(filename);
    }
}

void pqTestUtility::recordTests(const QString& filename)
{
#if defined(Q_WS_MAC)
  // check for native or non-native menu bar.
  // testing framework doesn't work with native menu bar, so let's warn if we
  // get that.
  if(!getenv("QT_MAC_NO_NATIVE_MENUBAR"))
    {
    qWarning("Recording menu events for native Mac menus doesn't work.\n"
             "Set the QT_MAC_NO_NATIVE_MENUBAR environment variable to"
             " correctly record menus");
    }
#endif

  QMap<QString, pqEventObserver*>::iterator iter;

  QFileInfo info(filename);
  QString suffix = info.completeSuffix();
  pqEventObserver* observer = NULL;
  iter = this->EventObservers.find(suffix);
  if(iter != this->EventObservers.end())
    {
    observer = iter.value();
    }

  if(!observer)
    {
    // cannot find observer for type of file
    return;
    }

  pqRecordEventsDialog* dialog = new pqRecordEventsDialog(this->Translator,
                                          *observer,
                                          filename,
                                          QApplication::activeWindow());
  dialog->setAttribute(Qt::WA_QuitOnClose, false);
  dialog->show();
}

void pqTestUtility::testSucceeded()
{
}

void pqTestUtility::testFailed()
{
}

