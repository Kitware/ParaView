/*=========================================================================

   Program: ParaView
   Module:    pqPythonEventSource.h

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

#ifndef _pqPythonEventSource_h
#define _pqPythonEventSource_h

#include "pqThreadedEventSource.h"
#include <QString>

/** Concrete implementation of pqEventSource that retrieves events recorded
by pqPythonEventObserver */
class QTTESTING_EXPORT pqPythonEventSource :
  public pqThreadedEventSource
{
  Q_OBJECT
public:
  pqPythonEventSource(QObject* p = 0);
  ~pqPythonEventSource();

  void setContent(const QString& path);

  static QString getProperty(QString& object, QString& prop);
  static void setProperty(QString& object, QString& prop, const QString& value);
  static QStringList getChildren(QString& object);
  static QString invokeMethod(QString& object, QString& method);

protected:
  virtual void run();
  virtual void start();

protected slots:
  void threadGetProperty();
  void threadSetProperty();
  void threadGetChildren();
  void threadInvokeMethod();

private:
  class pqInternal;
  pqInternal* Internal;

};

#endif // !_pqPythonEventSource_h

