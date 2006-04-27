/*=========================================================================

   Program:   ParaQ
   Module:    pqTesting.cxx

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

#include "pqTesting.h"

#include <QCoreApplication>
#include <QTime>

#ifdef Q_OS_UNIX
  #include <time.h>
#endif

#ifdef Q_OS_WIN32
  #include <windows.h>
#endif

void pqTesting::NonBlockingSleep(int Milliseconds)
{
  QTime timer;
  timer.start();
  while(timer.elapsed() < Milliseconds)
    {
    QCoreApplication::processEvents(QEventLoop::AllEvents, Milliseconds);
    
#ifdef Q_OS_WIN32
    Sleep(Milliseconds / 10);
#else
    struct timespec ts = { 0, 0 };
    ts.tv_nsec = Milliseconds * 1000 * 1000 / 10;
    nanosleep(&ts, NULL);
#endif
    }
}
