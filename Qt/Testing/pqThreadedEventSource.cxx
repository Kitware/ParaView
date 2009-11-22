/*=========================================================================

   Program: ParaView
   Module:    pqThreadedEventSource.cxx

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


#include "pqThreadedEventSource.h"

#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QString>

#include "pqEventDispatcher.h"

class pqThreadedEventSource::pqInternal : public QThread
{
  friend class pqThreadedEventSource;
public:
  pqInternal(pqThreadedEventSource& source)
    : Source(source), 
      ShouldStop(0),
      GotEvent(0)
    {
    }
  
  virtual void run()
    {
    Source.run();
    }
  
  pqThreadedEventSource& Source;

  QWaitCondition WaitCondition;
  int Waiting;

  int ShouldStop;
  int GotEvent;
  QString CurrentObject;
  QString CurrentCommand;
  QString CurrentArgument;

  class ThreadHelper : public QThread
  {
    public:
      static void msleep(int msecs)
        {
        QThread::msleep(msecs);
        }
  };
};

pqThreadedEventSource::pqThreadedEventSource(QObject* p)
  : pqEventSource(p)
{
  this->Internal = new pqInternal(*this);
}

pqThreadedEventSource::~pqThreadedEventSource()
{
  // wait a second for this thread to finish, if it hasn't yet
  this->Internal->wait(1000);
  delete this->Internal;
}


int pqThreadedEventSource::getNextEvent(
    QString& object,
    QString& command,
    QString& arguments)
{

  while(!this->Internal->GotEvent)
    {
    // wait for the other thread to post an event, while
    // we keep the GUI alive.
    pqEventDispatcher::processEventsAndWait(100);
    }

  object = this->Internal->CurrentObject;
  command = this->Internal->CurrentCommand;
  arguments = this->Internal->CurrentArgument;
  this->Internal->GotEvent = 0;
  this->guiAcknowledge();
  
  if(object == QString::null)
    {
    if(arguments == "failure")
      {
      return FAILURE;
      }
    return DONE;
    }

  return SUCCESS;
}

void pqThreadedEventSource::relayEvent(QString object, QString command, QString arguments)
{
  this->Internal->CurrentObject = object;
  this->Internal->CurrentCommand = command;
  this->Internal->CurrentArgument = arguments;
  this->Internal->GotEvent = 1;
}


bool pqThreadedEventSource::postNextEvent(const QString& object,
                   const QString& command,
                   const QString& argument)
{
  QMetaObject::invokeMethod(this, "relayEvent", Qt::QueuedConnection, 
                            Q_ARG(QString, object),
                            Q_ARG(QString, command),
                            Q_ARG(QString, argument));

  return this->waitForGUI();
}

void pqThreadedEventSource::start()
{
  this->Internal->ShouldStop = 0;
  this->Internal->start(QThread::LowestPriority);
}

void pqThreadedEventSource::stop()
{
  this->Internal->ShouldStop = 1;
  this->Internal->wait();
}

bool pqThreadedEventSource::waitForGUI()
{
  this->Internal->Waiting = 1;

  while(this->Internal->Waiting == 1 &&
        this->Internal->ShouldStop == 0)
    {
    pqInternal::ThreadHelper::msleep(50);
    }
  
  this->Internal->Waiting = 0;

  return !this->Internal->ShouldStop;
}

void pqThreadedEventSource::guiAcknowledge()
{
  while(this->Internal->Waiting == 0)
    {
    pqInternal::ThreadHelper::msleep(50);
    }
  
  this->Internal->Waiting = 0;
}

void pqThreadedEventSource::msleep(int msec)
{
  pqInternal::ThreadHelper::msleep(msec);
}

void pqThreadedEventSource::done(int success)
{
  if(success == 0)
    {
    this->postNextEvent(QString(), QString(), QString());
    return;
    }
  this->postNextEvent(QString(), QString(), "failure");
}

