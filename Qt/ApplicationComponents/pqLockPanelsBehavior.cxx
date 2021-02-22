/*=========================================================================

   Program: ParaView
   Module:  pqLockPanelsBehavior.cxx

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
#include "pqLockPanelsBehavior.h"

#include <QDockWidget>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqSettings.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"

namespace
{

//-----------------------------------------------------------------------------
class vtkLockDockWidgetsCallback : public vtkCommand
{
public:
  static vtkLockDockWidgetsCallback* New() { return new vtkLockDockWidgetsCallback(); }

  void Execute(vtkObject*, unsigned long, void*) override
  {
    this->Behavior->generalSettingsChanged();
  }

  pqLockPanelsBehavior* Behavior;

protected:
  vtkLockDockWidgetsCallback() = default;
  ~vtkLockDockWidgetsCallback() override = default;
};

} // anonymous namespace

//-----------------------------------------------------------------------------
class pqLockPanelsBehavior::pqInternals
{
public:
  pqInternals(pqLockPanelsBehavior* behavior)
    : Callback(vtkLockDockWidgetsCallback::New())
    , ObserverID(0)
  {
    this->Callback->Behavior = behavior;
  }

  ~pqInternals()
  {
    this->Callback->Delete();
    this->Callback = nullptr;
  }

  vtkLockDockWidgetsCallback* Callback;
  unsigned long ObserverID;
};

//-----------------------------------------------------------------------------
pqLockPanelsBehavior::pqLockPanelsBehavior(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals(this))
{
  vtkNew<vtkPVGeneralSettings> settings;
  ;

  // Lock or unlock the panels as dictated by the settings
  this->lockPanels(settings->GetLockPanels());

  // Add observer for the settings
  this->Internals->ObserverID =
    settings->AddObserver(vtkCommand::ModifiedEvent, this->Internals->Callback);
}

//-----------------------------------------------------------------------------
pqLockPanelsBehavior::~pqLockPanelsBehavior()
{
  vtkNew<vtkPVGeneralSettings> settings;
  settings->RemoveObserver(this->Internals->ObserverID);
}

//-----------------------------------------------------------------------------
void pqLockPanelsBehavior::generalSettingsChanged()
{
  vtkNew<vtkPVGeneralSettings> settings;
  this->lockPanels(settings->GetLockPanels());
}

//-----------------------------------------------------------------------------
void pqLockPanelsBehavior::toggleLockPanels()
{
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  if (!pxm)
  {
    return;
  }

  vtkSMProxy* lockPanelsProxy = pxm->GetProxy("settings", "GeneralSettings");
  vtkSMIntVectorProperty* lockPanelsProperty =
    vtkSMIntVectorProperty::SafeDownCast(lockPanelsProxy->GetProperty("LockPanels"));

  int previousValue = lockPanelsProperty->GetElement(0);
  lockPanelsProperty->SetElement(0, !previousValue);
  lockPanelsProxy->UpdateVTKObjects();

  // We also need to set the value in pqSettings because this is a
  // Qt-only setting.
  pqSettings* qSettings = pqApplicationCore::instance()->settings();
  qSettings->saveInQSettings("GeneralSettings.LockPanels", lockPanelsProperty);
}

//-----------------------------------------------------------------------------
void pqLockPanelsBehavior::lockPanels(bool lock)
{
  QDockWidget::DockWidgetFeatures features = QDockWidget::AllDockWidgetFeatures;
  if (lock)
  {
    features = QDockWidget::DockWidgetClosable;
  }

  // Note: iterate over the children of the parent of this object, which is
  // expected to be a QMainWindow or subclass.
  QList<QDockWidget*> dockWidgets = this->parent()->findChildren<QDockWidget*>();

  for (int i = 0; i < dockWidgets.size(); ++i)
  {
    QDockWidget* widget = dockWidgets[i];
    if (widget)
    {
      widget->setFeatures(features);
    }
  }
}
