/*=========================================================================

   Program: ParaView
   Module:    pqPVApplicationCore.h

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
#ifndef pqPVApplicationCore_h
#define pqPVApplicationCore_h

#include "pqApplicationCore.h"

#include "pqApplicationComponentsModule.h" // for exports
#include <QList>
#include <QPointer>

class pqAnimationManager;
class pqPythonManager;
class pqSelectionManager;
class pqTestUtility;
class QMenu;
class QWidget;

/**
* pqPVApplicationCore is the application code used by ParaView-based
* applications that use more of ParaView's functionality than that provided by
* pqApplicationCore such as the the selection manager, animation  manager etc.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqPVApplicationCore : public pqApplicationCore
{
  Q_OBJECT
  typedef pqApplicationCore Superclass;

public:
  pqPVApplicationCore(int& argc, char** argv, pqOptions* options = 0);
  ~pqPVApplicationCore() override;

  /**
  * Returns the pqPVApplicationCore instance. If no pqPVApplicationCore has been
  * created then return NULL.
  */
  static pqPVApplicationCore* instance()
  {
    return qobject_cast<pqPVApplicationCore*>(Superclass::instance());
  }

  /**
  * Provides access to the selection manager. Selection manager provides
  * access to the ParaView wide data selection mechanism. This must not be
  * confused with the active-object selection.
  */
  pqSelectionManager* selectionManager() const;

  /**
  * Provides access to the animation manager. Animation manager helps with the
  */
  // animation subsystem -- saving movies, creating scenes etc.
  pqAnimationManager* animationManager() const;

  /**
  * Provides access to the test utility.
  */
  pqTestUtility* testUtility() override;

  /**
  * Provides access to the python manager. This is non-null only when paraview
  * is compiled with python support i.e. PARAVIEW_USE_PYTHON is ON.
  */
  pqPythonManager* pythonManager() const;

  /**
  * ParaView provides a mechanism to trigger menu actions using a quick-launch
  * dialog. Applications can register menus action from which should be
  * launch-able from the quick-launch dialog. Typical candidates are the
  * sources menu, filters menu etc.
  */
  virtual void registerForQuicklaunch(QWidget*);

  void loadStateFromPythonFile(const QString& filename, pqServer* server);

public Q_SLOTS:
  /**
  * Pops-up the quick launch dialog.
  */
  void quickLaunch();
  /**
  * Pops-up the search dialog if the focused widget is
  * QAsbstractItemView type.
  */
  void startSearch();

Q_SIGNALS:
  /**
  * Emitted whenever the quickLaunch dialog is about to show.  This can be used
  * to update the menu items (QActions) that will be shown in the quick-launch
  * dialog.
  */
  void aboutToShowQuickLaunch();

protected:
  /**
  * Override event filter in order to catch file association mechanism
  */
  bool eventFilter(QObject* obj, QEvent* event) override;

  QPointer<pqSelectionManager> SelectionManager;
  QPointer<pqAnimationManager> AnimationManager;

  pqPythonManager* PythonManager;
  QList<QPointer<QWidget> > QuickLaunchMenus;

private:
  Q_DISABLE_COPY(pqPVApplicationCore)
};

#endif
