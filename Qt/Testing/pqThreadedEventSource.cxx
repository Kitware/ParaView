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
#include <QThread>
#include <QString>
#include <QEventLoop>
#include <QTimer>
#include <QAbstractEventDispatcher>

class pqThreadedEventSource::pqInternal : public QThread
{
  friend class pqThreadedEventSource;
public:
  pqInternal(pqThreadedEventSource& source)
    : Source(source), 
      MainThread(*QThread::currentThread())
    {
    }
  
  virtual void run()
    {
    Source.run();
    }
  
  pqThreadedEventSource& Source;

  QMutex Mutex1;
  QMutex Mutex2;
  QMutex Mutex3;
  QThread& MainThread;
  QEventLoop Loop;

  QString CurrentObject;
  QString CurrentCommand;
  QString CurrentArgument;
};

pqThreadedEventSource::pqThreadedEventSource()
{
  this->Internal = new pqInternal(*this);


  // lock the mutex
  // only when we unlock it, does the other thread
  // have a chance to interact with us
  this->Internal->Mutex1.lock();
}

pqThreadedEventSource::~pqThreadedEventSource()
{
  this->Internal->Mutex1.unlock();
  delete this->Internal;
}


bool pqThreadedEventSource::getNextEvent(
    QString& object,
    QString& command,
    QString& arguments)
{
  this->Internal->CurrentObject = QString::null;
  this->Internal->CurrentCommand = QString::null;
  this->Internal->CurrentArgument = QString::null;

  // unlock the mutex
  this->Internal->Mutex2.lock();

  // unlock Mutex1 when this loop is running
  QTimer::singleShot(0, this, SLOT(unlockTestingMutex()));

  // wait for the other thread to interrupt us, while
  // we keep the GUI alive.
  this->Internal->Loop.exec();
  
  // lock the mutex again
  this->Internal->Mutex1.lock();
  this->Internal->Mutex2.unlock();

  object = this->Internal->CurrentObject;
  command = this->Internal->CurrentCommand;
  arguments = this->Internal->CurrentArgument;

  if(object == QString::null)
    {
    return false;
    }

  return true;
}

void pqThreadedEventSource::unlockTestingMutex()
{
  this->Internal->Mutex1.unlock();
}


void pqThreadedEventSource::postNextEvent(const QString& object,
                   const QString& command,
                   const QString& argument)
{
  this->Internal->Mutex1.lock();
  
  this->Internal->CurrentObject = object;
  this->Internal->CurrentCommand = command;
  this->Internal->CurrentArgument = argument;

  // stop the event loop on the main thread
  this->Internal->Loop.quit();

  this->Internal->Mutex1.unlock();

  // wait until the other thread has locked Mutex1, so we don't come around too
  // quick and try to lock Mutex1 again.
  this->Internal->Mutex2.lock();
  this->Internal->Mutex2.unlock();
}

void pqThreadedEventSource::start()
{
  this->Internal->start();
}
  
void pqThreadedEventSource::done()
{
  this->postNextEvent(QString(), QString(), QString());
}


