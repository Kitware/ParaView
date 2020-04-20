/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqTriggerOnIdleHelper_h
#define pqTriggerOnIdleHelper_h

#include <QObject>
#include <QPointer>

#include "pqComponentsModule.h"
#include "pqTimer.h"

class pqServer;

/**
* Often times we need to call certain slots on idle. However, just relying on
* Qt's idle is not safe since progress events often result in processing of
* pending events, which may not be a safe place for such slots to be called.
* In that such cases this class can be used. It emits the triggered() signal
* when the server (set using setServer()) is indeed idle. Otherwise it simply
* reschedules the triggering of another time.
*/
class PQCOMPONENTS_EXPORT pqTriggerOnIdleHelper : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTriggerOnIdleHelper(QObject* parent = 0);
  ~pqTriggerOnIdleHelper() override;

  /**
  * get the server
  */
  pqServer* server() const;

Q_SIGNALS:
  void triggered();

public Q_SLOTS:
  void setServer(pqServer*);
  void trigger();

protected Q_SLOTS:
  void triggerInternal();

private:
  Q_DISABLE_COPY(pqTriggerOnIdleHelper)

  QPointer<pqServer> Server;
  pqTimer Timer;
};

#endif
