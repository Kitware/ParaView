/*=========================================================================

   Program: ParaView
   Module:    pqThreadedEventSource.cxx

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


#include "pqThreadedEventSource.h"

#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QString>
#include <QEventLoop>
#include <QEvent>
#include <QApplication>

namespace
{
  class pqPlayCommandEvent : public QEvent
    {
  public:
    pqPlayCommandEvent(const QString& o, const QString& c, const QString& a)
      : QEvent(QEvent::User),
        Object(o),
        Command(c),
        Arguments(a)
    {
    }
    QString Object;
    QString Command;
    QString Arguments;
    };

}


class pqThreadedEventSource::pqInternal : public QThread
{
  friend class pqThreadedEventSource;
public:
  pqInternal(pqThreadedEventSource& source)
    : Source(source), 
      GotEvent(false)
    {
    }
  
  virtual void run()
    {
    Source.run();
    }
  
  pqThreadedEventSource& Source;

  QWaitCondition WaitCondition;
  QEventLoop Loop;

  bool GotEvent;
  QString CurrentObject;
  QString CurrentCommand;
  QString CurrentArgument;
};

pqThreadedEventSource::pqThreadedEventSource(QObject* p)
  : pqEventSource(p)
{
  this->Internal = new pqInternal(*this);
}

pqThreadedEventSource::~pqThreadedEventSource()
{
  delete this->Internal;
}


int pqThreadedEventSource::getNextEvent(
    QString& object,
    QString& command,
    QString& arguments)
{
  if(!this->Internal->GotEvent == true)
  {
    // wait for the other thread to post an event, while
    // we keep the GUI alive.
    this->Internal->Loop.exec();
  }

  object = this->Internal->CurrentObject;
  command = this->Internal->CurrentCommand;
  arguments = this->Internal->CurrentArgument;
  this->Internal->GotEvent = false;
  this->Internal->WaitCondition.wakeAll();

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

bool pqThreadedEventSource::event(QEvent* e)
{
  pqPlayCommandEvent* pe = dynamic_cast<pqPlayCommandEvent*>(e);
  if(pe)
  {
    this->Internal->CurrentObject = pe->Object;
    this->Internal->CurrentCommand = pe->Command;
    this->Internal->CurrentArgument = pe->Arguments;
    this->Internal->GotEvent = true;
    this->Internal->Loop.quit();
    return true;
  }

  return pqEventSource::event(e);
}


void pqThreadedEventSource::postNextEvent(const QString& object,
                   const QString& command,
                   const QString& argument)
{
  QApplication::postEvent(this, new pqPlayCommandEvent(object, command, argument));
  // wait for the GUI thread to take the event and wake us up
  QMutex mut;
  mut.lock();
  this->Internal->WaitCondition.wait(&mut);
}

void pqThreadedEventSource::start()
{
  this->Internal->start();
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

