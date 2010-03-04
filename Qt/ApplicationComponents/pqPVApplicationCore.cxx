/*=========================================================================

   Program: ParaView
   Module:    pqPVApplicationCore.cxx

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
#include "pqPVApplicationCore.h"
#include "vtkPVConfig.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqComponentsInit.h"
#include "pqComponentsTestUtility.h"
#include "pqCoreUtilities.h"
#include "pqPQLookupTableManager.h"
#include "pqQuickLaunchDialog.h"
#include "pqSelectionManager.h"
#include "pqSetName.h"

#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonManager.h"
#endif

#include <QAction>
#include <QShortcut>

//-----------------------------------------------------------------------------
pqPVApplicationCore::pqPVApplicationCore(
  int& argc, char** argv, pqOptions* options)
: Superclass(argc, argv, options)
{
  // Initialize pqComponents resources.
  pqComponentsInit();

  this->AnimationManager = new pqAnimationManager(this);
  this->SelectionManager = new pqSelectionManager(this);

  this->PythonManager = 0;
#ifdef PARAVIEW_ENABLE_PYTHON
  this->PythonManager = new pqPythonManager(this);
#endif
  
  // Lookuptable management will soon enough move to the server manager.
  this->setLookupTableManager(new pqPQLookupTableManager(this));

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(serverChanged(pqServer*)),
    this->AnimationManager, SLOT(onActiveServerChanged(pqServer*)));
}

//-----------------------------------------------------------------------------
pqPVApplicationCore::~pqPVApplicationCore()
{
  delete this->AnimationManager;
  delete this->SelectionManager;
#ifdef PARAVIEW_ENABLE_PYTHON
  delete this->PythonManager;
#endif
}

//-----------------------------------------------------------------------------
void pqPVApplicationCore::registerForQuicklaunch(QWidget* menu)
{
  if (menu)
    {
    this->QuickLaunchMenus.push_back(menu);
    }
}

//-----------------------------------------------------------------------------
void pqPVApplicationCore::quickLaunch()
{
  if (this->QuickLaunchMenus.size() > 0)
    {
    pqQuickLaunchDialog dialog(pqCoreUtilities::mainWidget());
    foreach (QWidget* menu, this->QuickLaunchMenus)
      {
      if (menu)
        {
        // don't use QMenu::actions() since that includes only the top-level
        // actions.
        dialog.addActions(menu->findChildren<QAction*>());
        }
      }
    dialog.exec();
    }
}

//-----------------------------------------------------------------------------
pqSelectionManager* pqPVApplicationCore::selectionManager() const
{
  return this->SelectionManager;
}

//-----------------------------------------------------------------------------
pqAnimationManager* pqPVApplicationCore::animationManager() const
{
  return this->AnimationManager;
}

//-----------------------------------------------------------------------------
pqPythonManager* pqPVApplicationCore::pythonManager() const
{
#ifdef PARAVIEW_ENABLE_PYTHON
  return this->PythonManager;
#else
  return 0;
#endif
}

//-----------------------------------------------------------------------------
pqTestUtility* pqPVApplicationCore::testUtility()
{
  if (!this->TestUtility)
    {
    this->TestUtility = new pqComponentsTestUtility(this);
    }
  return this->TestUtility;
}
