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
vtkCxxRevisionMacro(vtkPVIceTDisplayRenderModule, "1.3.4.5");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVIceTDisplayRenderModule::vtkPVIceTDisplayRenderModule()
{
  this->CompositeID.ID = 0;
  this->Composite = 0;

  this->DisplayManagerID.ID = 0;

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
    this->Composite = 0;
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

  this->RendererID = pm->NewStreamObject("vtkIceTRenderer");
  this->RenderWindowID = pm->NewStreamObject("vtkRenderWindow");
  
  pm->SendStreamToClientAndServer();
  this->Renderer = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->RendererID));
  this->RenderWindow = 
    vtkRenderWindow::SafeDownCast(
      pm->GetObjectFromID(this->RenderWindowID));
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->RenderWindowID
                  << "FullScreenOn"
                  << vtkClientServerStream::End;
  pm->SendStreamToServer();
  if (pvApp->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }

  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->RenderWindowID << "AddRenderer" << this->RendererID
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();

  this->Composite = NULL;
  this->DisplayManagerID = pm->NewStreamObject("vtkIceTRenderManager");
  pm->SendStreamToClientAndServer();
  int *tileDim = pvApp->GetTileDimensions();
  cout << "Size: " << tileDim[0] << ", " << tileDim[1] << endl;
  pm->GetStream() <<  vtkClientServerStream::Invoke
                  << this->DisplayManagerID
                  << "SetTileDimensions"
                  << tileDim[0] << tileDim[1]
                  << vtkClientServerStream::End;

  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  pm->SendStreamToServer();
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
  this->CompositeID = pm->NewStreamObject("vtkIceTClientCompositeManager");
  pm->SendStreamToClient();
  pm->GetStream() << vtkClientServerStream::New
                  << "vtkIceTClientCompositeManager" << this->CompositeID 
                  << vtkClientServerStream::End;
  pm->SendStreamToServerRoot();

  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  pm->GetStream() << vtkClientServerStream::Invoke << pm->GetApplicationID() << "GetSocketController"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetClientController" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << pm->GetApplicationID() << "GetClientMode"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetClientFlag" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End; 
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
  // copy the stream before it is sent and reset
  vtkClientServerStream copy = pm->GetStream();
  pm->SendStreamToClient(); // send the stream to the client
  pm->GetStream() = copy; // now copy the copy into the current stream
  pm->SendStreamToServerRoot(); // send the same stream to the server root
}

//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModule::StillRender()
{
  // No reduction for still render.
  if (this->PVApplication && this->DisplayManagerID.ID)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke << this->DisplayManagerID
                    << "SetImageReductionFactor" << 1 
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
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
  if (this->PVApplication && this->DisplayManagerID.ID)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke << this->DisplayManagerID
                    << "SetImageReductionFactor" << 1 
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
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

