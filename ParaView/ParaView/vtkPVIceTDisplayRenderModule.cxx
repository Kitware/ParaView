/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTDisplayRenderModule.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVIceTDisplayRenderModule.h"

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkCollection.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkCallbackCommand.h"

#include "vtkCompositeRenderManager.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVIceTDisplayRenderModule);
vtkCxxRevisionMacro(vtkPVIceTDisplayRenderModule, "1.3.4.1");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVIceTDisplayRenderModule::vtkPVIceTDisplayRenderModule()
{
  this->CompositeTclName = 0;
  this->Composite = 0;

  this->DisplayManagerTclName = 0;

  this->ReductionFactor = 2;
}

//----------------------------------------------------------------------------
vtkPVIceTDisplayRenderModule::~vtkPVIceTDisplayRenderModule()
{
  vtkPVApplication *pvApp = this->PVApplication;
  
  vtkPVProcessModule * pm = 0;
  if ( pvApp )
    {
    pm = pvApp->GetProcessModule();
    }

  // Tree Composite
  if (this->CompositeTclName && pvApp && pm)
    {
    pm->ServerScript("%s Delete", this->DisplayManagerTclName);
    this->SetDisplayManagerTclName(NULL);
    }
  if (this->CompositeTclName && pvApp && pm)
    {
    pm->Script("%s Delete", this->CompositeTclName);
    pm->RootScript("%s Delete", this->CompositeTclName);
    this->SetCompositeTclName(NULL);
    this->Composite = NULL;
    }
  else if (this->Composite)
    {
    this->Composite->Delete();
    this->Composite = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModule::SetPVApplication(vtkPVApplication *pvApp)
{
  if (this->PVApplication)
    {
    if (pvApp == NULL)
      {
      this->PVApplication->UnRegister(this);
      this->PVApplication = NULL;
      return;
      }
    vtkErrorMacro("PVApplication already set.");
    return;
    }
  if (pvApp == NULL)
    {
    return;
    }  

  vtkPVProcessModule * pm = pvApp->GetProcessModule();
  // Maybe I should not reference count this object to avoid
  // a circular reference.
  this->PVApplication = pvApp;
  this->PVApplication->Register(this);

  this->Renderer = (vtkRenderer*)pvApp->MakeTclObject("vtkRenderer", "Ren1");
  this->RendererTclName = NULL;
  this->SetRendererTclName("Ren1");

  this->RenderWindow = 
    (vtkRenderWindow*)pvApp->MakeTclObject("vtkRenderWindow", "RenWin1");

  pm->ServerScript("Ren1 Delete");
  pm->ServerScript("RenWin1 FullScreenOn");
  pm->ServerScript("vtkIceTRenderer Ren1");

  if (pvApp->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }


  this->SetRenderWindowTclName("RenWin1");

  pm->BroadcastScript("%s AddRenderer %s", this->RenderWindowTclName,
    this->RendererTclName);

  //cout << "Ren1: " << this->RendererTclName << " " << this->Renderer->GetClassName() << endl;
  //cout << "RenWin1: " << this->RenderWindowTclName << " " << this->RenderWindow->GetClassName() << endl;


  this->Composite = NULL;
  pvApp->MakeTclObject("vtkIceTRenderManager", "TDispManager1");
  int *tileDim = pvApp->GetTileDimensions();
  cout << "Size: " << tileDim[0] << ", " << tileDim[1] << endl;
  pm->ServerScript("TDispManager1 SetTileDimensions %d %d",
    tileDim[0], tileDim[1]);

  this->DisplayManagerTclName = NULL;
  this->SetDisplayManagerTclName("TDispManager1");

  pm->ServerScript("%s SetRenderWindow %s", this->DisplayManagerTclName,
    this->RenderWindowTclName);
  pm->ServerScript("%s SetController [ [ $Application GetProcessModule ] GetController ]",
    this->DisplayManagerTclName);
  pm->ServerScript("%s InitializeRMIs", this->DisplayManagerTclName);


  // **********************************************************
  this->SetCompositeTclName("CCompositeManager1");
  pm->Script("vtkIceTClientCompositeManager %s", this->CompositeTclName);
  pm->RootScript("vtkIceTClientCompositeManager %s", this->CompositeTclName);
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  pm->Script("%s SetClientController [$Application GetSocketController]", this->CompositeTclName);
  pm->Script("%s SetClientFlag [$Application GetClientMode]", this->CompositeTclName);
  pm->RootScript("%s SetClientController [$Application GetSocketController]", this->CompositeTclName);
  pm->RootScript("%s SetClientFlag [$Application GetClientMode]", this->CompositeTclName);

  pm->Script("%s SetRenderWindow %s", this->CompositeTclName, this->RenderWindowTclName);
  pm->Script("%s InitializeRMIs", this->CompositeTclName);
  pm->RootScript("%s SetRenderWindow %s", this->CompositeTclName, this->RenderWindowTclName);
  pm->RootScript("%s InitializeRMIs", this->CompositeTclName);

  pm->Script("%s UseCompositingOn", this->CompositeTclName);
  pm->RootScript("%s UseCompositingOn", this->CompositeTclName);

}

//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModule::StillRender()
{
  // No reduction for still render.
  if (this->PVApplication && this->DisplayManagerTclName)
    {
    this->PVApplication->BroadcastScript("%s SetImageReductionFactor 1",
                                         this->DisplayManagerTclName);
    }

  this->Superclass::StillRender();
  /*
  this->UpdateAllPVData();

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  this->GetPVApplication()->SetGlobalLODFlag(0);
  vtkTimerLog::MarkStartEvent("Still Render");
  vtkPVApplication* pvApp = this->PVApplication;
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->RootScript("RenWin1 Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");
  */
}

//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModule::InteractiveRender()
{
  if (this->PVApplication && this->DisplayManagerTclName)
    {
    this->PVApplication->BroadcastScript("%s SetImageReductionFactor %d",
                                         this->DisplayManagerTclName,
                                         this->ReductionFactor);
    }

  this->Superclass::InteractiveRender();
    /*
  this->UpdateAllPVData();

  vtkTimerLog::MarkStartEvent("Interactive Render");
  vtkPVApplication* pvApp = this->PVApplication;
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->RootScript("RenWin1 Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");
  */
}

//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
}

