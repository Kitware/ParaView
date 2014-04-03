/*=========================================================================

   Program: ParaView
   Module:    pqPluginSettingsBehavior.cxx

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
#include "pqPluginSettingsBehavior.h"

#include "pqApplicationCore.h"
#include "pqPluginManager.h"
#include "pqServer.h"

#include "vtkNew.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"


//----------------------------------------------------------------------------
pqPluginSettingsBehavior::pqPluginSettingsBehavior(QObject* parent)
{
  pqPluginManager* pluginManager = pqApplicationCore::instance()->getPluginManager();
  this->connect(pluginManager, SIGNAL(pluginsUpdated()), this, SLOT(updateSettings()));
}

//----------------------------------------------------------------------------
void pqPluginSettingsBehavior::updateSettings()
{
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  vtkSMSession* session = server->session();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSMProxyDefinitionManager* pdm = pxm->GetProxyDefinitionManager();

  vtkNew<vtkSMParaViewPipelineController> controller;

  vtkPVProxyDefinitionIterator* iter = pdm->NewSingleGroupIterator( "settings" );
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMProxy* proxy = pxm->GetProxy(iter->GetGroupName(), iter->GetProxyName());
    if (!proxy)
      {
      proxy = pxm->NewProxy(iter->GetGroupName(), iter->GetProxyName());
      if (proxy)
        {
        controller->InitializeProxy(proxy);
        pxm->RegisterProxy(iter->GetGroupName(), iter->GetProxyName(), proxy);
        proxy->UpdateVTKObjects();
        proxy->Delete();
        }
      }
    }

  iter->Delete();
}
