/*=========================================================================

   Program: ParaView
   Module:  pqLockPanelsBehavior.h

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
#ifndef pqLockPanelsBehavior_h
#define pqLockPanelsBehavior_h

#include <QObject>
#include <QScopedPointer>

#include "pqApplicationComponentsModule.h"

class QAction;

/**
* @ingroup Behaviors
* Central location for controlling whether dock widgets are locked down
* or movable.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqLockPanelsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
  * Note: parent is assumed to be a subclass of QMainWindow, e.g.,
  * ParaViewMainWindow.
  */
  pqLockPanelsBehavior(QObject* parent = 0);
  virtual ~pqLockPanelsBehavior();

  /**
  * Invoked when the vtkPVGeneralSettings singleton is modified.
  */
  void generalSettingsChanged();

  /**
  * Toggles the dock panel locking state.
  */
  static void toggleLockPanels();

private:
  Q_DISABLE_COPY(pqLockPanelsBehavior);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  /**
  * Set the dock panel locking state to locked or unlocked.
  */
  void lockPanels(bool lock);
};

#endif // pqLockPanelsBehavior_h
