/*=========================================================================

   Program: ParaView
Module:    ProcessModuleGUIHelper.cxx

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

#include "ProcessModuleGUIHelper.h"

#include <QTimer>
#include <QBitmap>
#include <QShortcut>

#include "pqApplicationCore.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkSMProxyManager.h"
#include "vtkSMViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMIntVectorProperty.h"
#include "pqAdaptiveMainWindowCore.h"
#include "pqClientMainWindow.h"
#include "pqPluginManager.h"
#include "pqCustomViewModules.h"
#include "pqCustomDisplayPolicy.h"

vtkStandardNewMacro(ProcessModuleGUIHelper);
vtkCxxRevisionMacro(ProcessModuleGUIHelper, "1.1");

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::ProcessModuleGUIHelper()
{
}

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::~ProcessModuleGUIHelper()
{
}


//-----------------------------------------------------------------------------
int ProcessModuleGUIHelper::RunGUIStart(int argc, char** argv,
  int vtkNotUsed(numServerProcs), int vtkNotUsed(myId))
{
  int status = this->preAppExec(argc, argv, int(0), int(0));

  return status;
}

//-----------------------------------------------------------------------------
int ProcessModuleGUIHelper::preAppExec(int argc, char** argv,
  int numServerProcs, int myId)
{
  int result = this->Superclass::preAppExec(argc, argv, numServerProcs, myId);

  vtkSMProxyManager * pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm)
    {
    vtkSMProxy* prototype =
      pxm->GetPrototypeProxy("helpers", "AdaptiveOptions");
    if (!prototype)
      {
      /*
      vtkWarningMacro("Tried and failed to create a adaptive module. "
        << "Make sure the adaptive plugin can be found by ParaView.");
      */
      }
    }  

  return result;
}


//-----------------------------------------------------------------------------
QWidget* ProcessModuleGUIHelper::CreateMainWindow()
{
  pqApplicationCore::instance()->setApplicationName("ParaView" PARAVIEW_VERSION);
  pqApplicationCore::instance()->setOrganizationName("ParaView");

  // Create main window core
  pqAdaptiveMainWindowCore *myCore = new pqAdaptiveMainWindowCore();

  pqClientMainWindow* w = new pqClientMainWindow(myCore);
  QTimer::singleShot(10, this->Splash, SLOT(close()));

  //remove surface selection buttons
  w->disableSelections();

  //remove the view types that do not have adaptive support
  pqPluginManager* plugin_manager =
    pqApplicationCore::instance()->getPluginManager();
  QObjectList ifcs = plugin_manager->interfaces();
  for (int i = 0; i < ifcs.size(); i++)
    {
    QObject *nxt = ifcs.at(i);
    if (nxt->inherits("pqStandardViewModules"))
      {
      plugin_manager->removeInterface(ifcs.at(i));
      break;
      }
    }
  //add back the ones that do
  plugin_manager->addInterface(
    new pqCustomViewModules(plugin_manager));

  //setup custom policy so that we don't show items by default
  pqApplicationCore::instance()->setDisplayPolicy(
    new pqCustomDisplayPolicy(pqApplicationCore::instance()));

  //link up pass number message message alerts
  w->connect(myCore,
    SIGNAL(setMessage(const QString&)),
    SLOT(setMessage(const QString&)));

  //make spacebar tell it to stop what it is doing
  QObject::connect(new QShortcut(Qt::Key_Space, w), SIGNAL(activated()), 
                   myCore, SLOT(stopAdaptive()));

  return w;
}

//-----------------------------------------------------------------------------
void ProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

