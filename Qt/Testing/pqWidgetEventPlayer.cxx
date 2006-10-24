/*=========================================================================

   Program: ParaView
   Module:    pqWidgetEventPlayer.cxx

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

#include "pqWidgetEventPlayer.h"

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep
#endif
#ifdef Q_OS_UNIX
#include <time.h>
#endif

#include <QCoreApplication>
#include <QTime>
  
pqWidgetEventPlayer::pqWidgetEventPlayer(QObject* p) 
  : QObject(p)  
{
}

pqWidgetEventPlayer::~pqWidgetEventPlayer() 
{
}

void pqWidgetEventPlayer::wait(int ms)
{
  QTime timer;
  timer.start();
  do {
    QCoreApplication::processEvents(QEventLoop::AllEvents, ms);
#ifdef Q_OS_WIN
    Sleep(uint(10));
#else
    struct timespec ts = { 10 / 1000, (10 % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif

  } while (timer.elapsed() < ms);
}

