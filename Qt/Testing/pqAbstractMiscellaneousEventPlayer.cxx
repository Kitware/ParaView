/*=========================================================================

   Program: ParaView
   Module:    pqAbstractMiscellaneousEventPlayer.cxx

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



#include "pqAbstractMiscellaneousEventPlayer.h"

#include <QAbstractButton>
#include <QtDebug>
#include <QFile>
#include <QThread>

// Class that encapsulates the protected function QThread::msleep
class SleeperThread : public QThread
{
public:
  //Allows for cross platform sleep function
  static bool msleep(unsigned long msecs)
  {
    QThread::msleep(msecs);
    return true;
  }
};

pqAbstractMiscellaneousEventPlayer::pqAbstractMiscellaneousEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

//Allows for execution of testing commands that don't merit their own class
bool pqAbstractMiscellaneousEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{

  if (Command == "pause")
    {
    const int value = Arguments.toInt();
    if(SleeperThread::msleep(value))
      {
      return true;
      }
    Error = true;
    qCritical() << "calling pause on unhandled type " << Object;
    }

  return false;
}


