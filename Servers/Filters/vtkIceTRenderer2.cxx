/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIceTRenderer2.h"

#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"

#include <vtkgl.h>
#include <GL/ice-t.h>

namespace
{
  static vtkIceTRenderer2* IceTDrawCallbackHandle = 0;
  void vtkIceTRenderer2DrawCallback()
    {
    if (IceTDrawCallbackHandle)
      {
      IceTDrawCallbackHandle->Draw();
      }
    }
};

vtkStandardNewMacro(vtkIceTRenderer2);
//----------------------------------------------------------------------------
vtkIceTRenderer2::vtkIceTRenderer2()
{
  this->IceTCompositePass = vtkIceTCompositePass::New();
  this->IceTCompositePass->SetController(
    vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkIceTRenderer2::~vtkIceTRenderer2()
{
  this->IceTCompositePass->Delete();
  this->IceTCompositePass = 0;
}

//----------------------------------------------------------------------------
void vtkIceTRenderer2::DeviceRender()
{
  // CODE COPIED FROM SUPERCLASS
  // Oh! How I hate such kind of copying, sigh :(.

  vtkTimerLog::MarkStartEvent("OpenGL Dev Render");

  // Do not remove this MakeCurrent! Due to Start / End methods on
  // some objects which get executed during a pipeline update,
  // other windows might get rendered since the last time
  // a MakeCurrent was called.
  this->RenderWindow->MakeCurrent();

  // standard render method
  this->ClearLights();

  this->UpdateCamera();

  this->IceTDeviceRender();

  //// clean up the model view matrix set up by the camera
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  vtkTimerLog::MarkEndEvent("OpenGL Dev Render");
}

//----------------------------------------------------------------------------
void vtkIceTRenderer2::IceTDeviceRender()
{
  vtkRenderState ren_state(this);
  ren_state.SetPropArrayAndCount(this->PropArray,this->PropArrayCount);
  ren_state.SetFrameBuffer(0);

  this->IceTCompositePass->SetupContext(&ren_state);

  // FIXME: No need to display unless we are in tile-display mode.
  // icetDisable(ICET_DISPLAY);
  // icetDisable(ICET_DISPLAY_INFLATE);

  icetDrawFunc(vtkIceTRenderer2DrawCallback);
  IceTDrawCallbackHandle = this;
  icetDrawFrame();
  IceTDrawCallbackHandle = NULL;

  this->IceTCompositePass->CleanupContext(&ren_state);
}

//----------------------------------------------------------------------------
void vtkIceTRenderer2::Draw()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  this->UpdateLightGeometry();
  this->UpdateLights();

  //// set matrix mode for actors
  glMatrixMode(GL_MODELVIEW);

  this->UpdateGeometry();
}

//----------------------------------------------------------------------------
void vtkIceTRenderer2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
