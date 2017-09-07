/*=========================================================================

   Program: ParaView
   Module:  pqSyncLightToCameraWidget.cxx

   Copyright (c) 2017 Sandia Corporation, Kitware Inc.
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
#include "pqSyncLightToCameraWidget.h"
#include "pqView.h"

#include "vtkCamera.h"
#include "vtkPVLight.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"

#include "pqPVApplicationCore.h"

//-----------------------------------------------------------------------------
pqSyncLightToCameraWidget::pqSyncLightToCameraWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, smproperty, parentObject)
{
  PV_DEBUG_PANELS() << "pqSyncLightToCameraWidget for a property with "
                       "the panel_widget=\"sync_light_to_camera_button\" attribute.";
  // vtkSMSessionProxyManager* pxm = smproxy->GetSessionProxyManager();
}

//-----------------------------------------------------------------------------
pqSyncLightToCameraWidget::~pqSyncLightToCameraWidget()
{
}

void pqSyncLightToCameraWidget::buttonClicked()
{

  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->proxy()).arg("comment", "update a light");

  int type = 0;
  vtkSMPropertyHelper(this->proxy(), "LightType").Get(&type);

  // default camera light params.
  double position[3] = { 0, 0, 1 };
  double focal[3] = { 0, 0, 0 };
  // nothing to do for headlight or ambient light
  if (type == VTK_LIGHT_TYPE_CAMERA_LIGHT)
  {
    // camera light is relative the camera already, this button is just a 'reset' to defaults.
    vtkSMPropertyHelper(this->proxy(), "LightPosition").Modified().Set(position, 3);
    vtkSMPropertyHelper(this->proxy(), "FocalPoint").Modified().Set(focal, 3);
  }
  else if (type == VTK_LIGHT_TYPE_SCENE_LIGHT)
  {
    vtkSMRenderViewProxy* viewProxy =
      this->view() ? vtkSMRenderViewProxy::SafeDownCast(this->view()->getProxy()) : NULL;
    if (viewProxy)
    {
      vtkCamera* camera = viewProxy->GetActiveCamera();
      camera->GetPosition(position);
      camera->GetFocalPoint(focal);
    }

    vtkSMPropertyHelper(this->proxy(), "LightPosition").Modified().Set(position, 3);
    vtkSMPropertyHelper(this->proxy(), "FocalPoint").Modified().Set(focal, 3);
  }
  this->proxy()->UpdateVTKObjects();
  // Trigger a rendering
  pqPVApplicationCore::instance()->render();
}
