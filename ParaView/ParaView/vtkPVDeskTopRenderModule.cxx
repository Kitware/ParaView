/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDeskTopRenderModule.cxx
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
#include "vtkPVDeskTopRenderModule.h"

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
vtkStandardNewMacro(vtkPVDeskTopRenderModule);
vtkCxxRevisionMacro(vtkPVDeskTopRenderModule, "1.1.2.1");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVDeskTopRenderModule::vtkPVDeskTopRenderModule()
{
  this->DisplayManagerTclName = 0;
}

//----------------------------------------------------------------------------
vtkPVDeskTopRenderModule::~vtkPVDeskTopRenderModule()
{
  vtkPVApplication *pvApp = this->PVApplication;
  
  vtkPVProcessModule * pm = 0;
  if ( pvApp )
    {
    pm = pvApp->GetProcessModule();
    }

  // Tree Composite
  if (this->DisplayManagerTclName && pvApp && pm)
    {
    pm->ServerScript("%s Delete", this->DisplayManagerTclName);
    this->SetDisplayManagerTclName(NULL);
    }
  if (this->CompositeTclName && pvApp && pm)
    {
    pm->Script("%s Delete", this->CompositeTclName);
    pm->RootScript("%s Delete", this->CompositeTclName);
    this->SetCompositeTclName(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::SetPVApplication(vtkPVApplication *pvApp)
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

  this->Renderer = (vtkRenderer*)pvApp->MakeTclObject("vtkIceTRenderer", "Ren1");
  this->RendererTclName = NULL;
  this->SetRendererTclName("Ren1");

  this->RenderWindow = 
    (vtkRenderWindow*)pvApp->MakeTclObject("vtkRenderWindow", "RenWin1");

  pm->ServerScript("Ren1 Delete");
  pm->ServerScript("vtkIceTRenderer Ren1");

  if (pvApp->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }

  this->SetRenderWindowTclName("RenWin1");

  pm->BroadcastScript("%s AddRenderer %s", this->RenderWindowTclName,
    this->RendererTclName);

  pvApp->MakeTclObject("vtkIceTRenderManager", "TDispManager1");
  pm->ServerScript("TDispManager1 SetTileDimensions 1 1");

  this->DisplayManagerTclName = NULL;
  this->SetDisplayManagerTclName("TDispManager1");

  pm->ServerScript("%s SetRenderWindow %s", this->DisplayManagerTclName,
    this->RenderWindowTclName);
  pm->ServerScript("%s SetController [ [ $Application GetProcessModule ] GetController ]",
    this->DisplayManagerTclName);
  pm->ServerScript("%s InitializeRMIs", this->DisplayManagerTclName);


  // **********************************************************
  this->SetCompositeTclName("CCompositeManager1");
  pm->Script("vtkDesktopDeliveryClient %s", this->CompositeTclName);
  pm->RootScript("vtkDesktopDeliveryServer %s", this->CompositeTclName);
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  pm->Script("%s SetController [$Application GetSocketController]", this->CompositeTclName);
  //pm->Script("%s SetClientFlag [$Application GetClientMode]", this->CompositeTclName);
  pm->RootScript("%s SetController [$Application GetSocketController]", this->CompositeTclName);
  //pm->RootScript("%s SetClientFlag [$Application GetClientMode]", this->CompositeTclName);

  pm->Script("%s SetRenderWindow %s", this->CompositeTclName, this->RenderWindowTclName);
  pm->Script("%s InitializeRMIs", this->CompositeTclName);
  pm->RootScript("%s SetRenderWindow %s", this->CompositeTclName, this->RenderWindowTclName);
  pm->RootScript("%s InitializeRMIs", this->CompositeTclName);

  pm->Script("%s UseCompositingOn", this->CompositeTclName);
  pm->RootScript("%s UseCompositingOn", this->CompositeTclName);

  // The client server manager needs to set parameters on the IceT manager.
  pm->RootScript("%s SetRenderManager %s", 
                 this->CompositeTclName, this->DisplayManagerTclName);

}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::StillRender()
{
  this->Superclass::StillRender();
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::InteractiveRender()
{
  this->Superclass::InteractiveRender();
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

