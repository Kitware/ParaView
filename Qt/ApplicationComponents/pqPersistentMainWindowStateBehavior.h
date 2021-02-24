/*=========================================================================

   Program: ParaView
   Module:    pqPersistentMainWindowStateBehavior.h

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
#ifndef pqPersistentMainWindowStateBehavior_h
#define pqPersistentMainWindowStateBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QMainWindow;

/**
* @ingroup Behaviors
* pqPersistentMainWindowStateBehavior saves and restores the MainWindow state
* on shutdown and restart. Simply instantiate this behavior if you want your
* main window layout to be persistent.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqPersistentMainWindowStateBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
  * Parent cannot be nullptr.
  */
  pqPersistentMainWindowStateBehavior(QMainWindow* parent);
  ~pqPersistentMainWindowStateBehavior() override;

  static void restoreState(QMainWindow*);
  static void saveState(QMainWindow*);

protected Q_SLOTS:
  void saveState();
  void restoreState();

private:
  Q_DISABLE_COPY(pqPersistentMainWindowStateBehavior)
};

#endif
