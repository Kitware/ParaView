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
vtkCxxRevisionMacro(vtkPVDeskTopRenderModule, "1.4");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVDeskTopRenderModule::vtkPVDeskTopRenderModule()
{
  this->DisplayManagerID.ID = 0;
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
  if (this->DisplayManagerID.ID && pvApp && pm)
    {
    pm->DeleteStreamObject(this->DisplayManagerID);
    pm->SendStreamToServer(); 
    this->DisplayManagerID.ID = 0;
    }
  if (this->CompositeID.ID && pvApp && pm)
    {
    pm->DeleteStreamObject(this->CompositeID);
    pm->SendStreamToClient();
    pm->DeleteStreamObject(this->CompositeID);
    pm->SendStreamToServerRoot();
    this->CompositeID.ID = 0;
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

  this->RendererID = pm->NewStreamObject("vtkIceTRenderer");
  this->RenderWindowID = pm->NewStreamObject("vtkRenderWindow");
  pm->SendStreamToClientAndServer();
  this->Renderer = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->RendererID));
  this->RenderWindow = 
    vtkRenderWindow::SafeDownCast(
      pm->GetObjectFromID(this->RenderWindowID));
  
  if (pvApp->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }
  
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->RenderWindowID << "AddRenderer" << this->RendererID
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  
  this->DisplayManagerID = pm->NewStreamObject("vtkIceTRenderManager");
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetTileDimensions"
                  << 1 << 1 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  
  pm->GetStream() << vtkClientServerStream::Invoke
                  << pm->GetProcessModuleID()
                  << "GetController"
                  << vtkClientServerStream::End;
  
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetController"
                  << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID 
                  << "InitializeRMIs"
                  << vtkClientServerStream::End;
  pm->SendStreamToServer();

  // **********************************************************

  // create a vtkDesktopDeliveryClient on the client
  this->CompositeID = pm->NewStreamObject("vtkDesktopDeliveryClient");
  pm->SendStreamToClient();
  // create a vtkDesktopDeliveryServer on the server, but use
  // the same id
  pm->GetStream() << vtkClientServerStream::New << "vtkDesktopDeliveryServer"
                  <<  this->CompositeID <<  vtkClientServerStream::End;
  pm->SendStreamToServerRoot();

  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  pm->GetStream() << vtkClientServerStream::Invoke << pm->GetApplicationID() << "GetSocketController"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetController" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  vtkClientServerStream tmp = pm->GetStream();
  pm->SendStreamToClient();
  pm->GetStream() = tmp;
  pm->SendStreamToServerRoot();
  

  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetParallelRenderManager" << this->DisplayManagerID
                  << vtkClientServerStream::End;
  pm->SendStreamToServerRoot();

  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "InitializeRMIs"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "UseCompositingOn"
                  << vtkClientServerStream::End;
  tmp = pm->GetStream();
  pm->SendStreamToClient();
  pm->GetStream() = tmp;
  pm->SendStreamToServerRoot();
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

