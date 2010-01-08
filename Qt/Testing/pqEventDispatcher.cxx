/*=========================================================================

   Program: ParaView
   Module:    pqEventDispatcher.cxx

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

#include "pqEventDispatcher.h"

#include "pqEventPlayer.h"
#include "pqEventSource.h"

#include <QAbstractEventDispatcher>
#include <QtDebug>
#include <QTime>
#include <QTimer>
#include <QApplication>
#include <QEventLoop>
#include <QThread>
#include <QDialog>
#include <QMainWindow>

#include <iostream>
using namespace std;

bool pqEventDispatcher::DeferMenuTimeouts = false;
//-----------------------------------------------------------------------------
pqEventDispatcher::pqEventDispatcher(QObject* parentObject) :
  Superclass(parentObject)
{
  this->ActiveSource = NULL;
  this->ActivePlayer = NULL;
  this->PlayBackStatus = false;
  this->PlayBackFinished = false;
  this->AdhocMenuTimer.setInterval(1000);
  this->AdhocMenuTimer.setSingleShot(true);
  QObject::connect(&this->AdhocMenuTimer, SIGNAL(timeout()),
    this, SLOT(onMenuTimerTimeout()));
  QObject::connect(this, SIGNAL(triggerPlayEventStack(void*)),
    this, SLOT(playEventStack(void*)), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqEventDispatcher::~pqEventDispatcher()
{
}

//-----------------------------------------------------------------------------
bool pqEventDispatcher::playEvents(pqEventSource& source, pqEventPlayer& player)
{
  if (this->ActiveSource || this->ActivePlayer)
    {
    qCritical() << "Event dispatcher is already playing";
    return false;
    }

  this->ActiveSource = &source;
  this->ActivePlayer = &player;

  QApplication::setEffectEnabled(Qt::UI_General, false);

  QApplication::instance()->installEventFilter(this);

  this->PlayBackStatus = true; // success.
  this->PlayBackFinished = false;
  this->playEventStack(NULL);
  this->ActiveSource = NULL;
  this->ActivePlayer = NULL;
  
  QApplication::instance()->removeEventFilter(this);
  return this->PlayBackStatus;
}

//-----------------------------------------------------------------------------
void pqEventDispatcher::playEventStack(void* activeWidget)
{
  QWidget* activePopup = QApplication::activePopupWidget();
  QWidget* activeModal = QApplication::activeModalWidget();

  if (activeWidget != activePopup && activeWidget != activeModal)
    {
    return;
    }

  if (this->PlayBackFinished)
    {
    return;
    }

  if (!this->ActiveSource)
    {
    qCritical("Internal error: playEventStack Ecalled without valid source.");
    return;
    }

  QString object;
  QString command;
  QString arguments;
  
  int result = this->ActiveSource->getNextEvent(object, command, arguments);
  if (result == pqEventSource::DONE)
    {
    this->PlayBackFinished = true;
    return;
    }
  else if(result == pqEventSource::FAILURE)
    {
    this->PlayBackFinished = true;
    this->PlayBackStatus = false; // failure.
    return;
    }
    
  QApplication::syncX();
  static unsigned long counter=0;
  unsigned long local_counter = counter++;
  int indent = this->ActiveModalWidgetStack.size();
  QString pretty_name = object.mid(object.lastIndexOf('/'));
  bool print_debug = getenv("PV_DEBUG_TEST") != NULL;
  if (print_debug)
    {
    cout  << QString().fill(' ', 4*indent).toStdString().c_str()
          << local_counter << ": Test (" << indent << "): "
          << pretty_name.toStdString().c_str() << ": "
          << command.toStdString().c_str() << " : "
          << arguments.toStdString().c_str() << endl;
    }

  bool error = false;
  this->ActivePlayer->playEvent(object, command, arguments, error);
  this->processEventsAndWait(100); // let what's going to happen after the
  if (print_debug)
    {
    cout << QString().fill(' ', 4*indent).toStdString().c_str()
      << local_counter << ": Done" << endl;
    }
  if (error)
    {
    this->PlayBackStatus  = false;
    this->PlayBackFinished = true;
    return;
    }

  if (QApplication::activeModalWidget() != activeWidget)
    {
    // done.
    return;
    }

  this->playEventStack(activeWidget);
}

//-----------------------------------------------------------------------------
void pqEventDispatcher::processEventsAndWait(int ms)
{
  bool prev = pqEventDispatcher::DeferMenuTimeouts;
  pqEventDispatcher::DeferMenuTimeouts = true;
  if (ms > 0)
    {
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, SLOT(quit()));
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  pqEventDispatcher::DeferMenuTimeouts = prev;
}

//-----------------------------------------------------------------------------
void pqEventDispatcher::onMenuTimerTimeout()
{
  // this is used to capture cases where are popup-menu (or any modal dialog in
  // case of APPLE) is active in its event loop. However, we don't want the
  // test to proceed if this timeout happened while we were in the
  // processEventsAndWait() loop at any stage in executing an event. So we have
  // this trap.
  if (pqEventDispatcher::DeferMenuTimeouts)
    {
    this->AdhocMenuTimer.start();
    return;
    }

  QWidget* currentPopup = QApplication::activePopupWidget();
#if defined(__APPLE__)
 if (!currentPopup)
    {
    currentPopup = QApplication::activeModalWidget();
    }
#endif
 
  if (currentPopup)
    {
    this->playEventStack(currentPopup);
    }
}

//-----------------------------------------------------------------------------
bool pqEventDispatcher::eventFilter(QObject *obj, QEvent *ev)
{
  QWidget* currentPopup = QApplication::activePopupWidget();
#if defined(__APPLE__)
  if (!currentPopup)
    {
    currentPopup = QApplication::activeModalWidget();
    }
#endif
  if (currentPopup && !this->AdhocMenuTimer.isActive())
    {
    // it's possible that this is temporary popup (eg. standard menus), so we do
    // a deferred handling for this event (I hate these menus in tests, btw).
    // cout << "Start Menu Timer" << endl;
    this->AdhocMenuTimer.start();
    }
  if (!currentPopup && this->AdhocMenuTimer.isActive())
    {
    // cout << "Stop Menu Timer" << endl;
    this->AdhocMenuTimer.stop();
    }

#if defined(__APPLE__)
  return this->Superclass::eventFilter(obj, ev);
#endif

  QWidget* currentWidget = QApplication::activeModalWidget();

  if (
    (this->ActiveModalWidgetStack.size() == 0 && currentWidget == 0) ||
    (this->ActiveModalWidgetStack.size() > 0 && this->ActiveModalWidgetStack.back() ==
     currentWidget))
    {
    return this->Superclass::eventFilter(obj, ev);
    }

  if (currentWidget && this->ActiveModalWidgetStack.contains(currentWidget))
    {
    // a modal dialog was closed.
    this->ActiveModalWidgetStack = this->ActiveModalWidgetStack.mid(0,
      this->ActiveModalWidgetStack.indexOf(currentWidget)+1);
    }
  else if ((currentWidget && this->ActiveModalWidgetStack.size() == 0) ||
    (currentWidget && !this->ActiveModalWidgetStack.contains(currentWidget)) )
    {
    // new modal dialog,
    this->ActiveModalWidgetStack.push_back(currentWidget);
    emit this->triggerPlayEventStack(this->ActiveModalWidgetStack.back());
    }
  else if (!currentWidget)
    {
    // all modal dialogs were closed.
    this->ActiveModalWidgetStack.clear();
    }
  return this->Superclass::eventFilter(obj, ev);
}
