/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTRenderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVIceTRenderModule.h"

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkCollection.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkCallbackCommand.h"
#include "vtkClientServerStream.h"
#include "vtkCompositeRenderManager.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVIceTRenderModule);
vtkCxxRevisionMacro(vtkPVIceTRenderModule, "1.11.2.1");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVIceTRenderModule::vtkPVIceTRenderModule()
{
  this->DisplayManagerID.ID = 0;

  this->ReductionFactor = 2;
}

//----------------------------------------------------------------------------
vtkPVIceTRenderModule::~vtkPVIceTRenderModule()
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
     pm->SendStreamToRenderServer();
     this->DisplayManagerID.ID = 0;
     }
   if (this->CompositeID.ID && pvApp && pm)
     {
     pm->DeleteStreamObject(this->CompositeID);
     pm->SendStreamToClient();
     pm->DeleteStreamObject(this->CompositeID);
     pm->SendStreamToRenderServerRoot();
     this->CompositeID.ID = 0;
     }
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::SetPVApplication(vtkPVApplication *pvApp)
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
  this->Renderer2DID = pm->NewStreamObject("vtkRenderer");
  this->RenderWindowID = pm->NewStreamObject("vtkRenderWindow");
  pm->SendStreamToClientAndRenderServer();
  this->Renderer = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->RendererID));
  this->Renderer2D = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->Renderer2DID));  
  this->RenderWindow = 
    vtkRenderWindow::SafeDownCast(
      pm->GetObjectFromID(this->RenderWindowID));
  pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
                  << "FullScreenOn" 
                  << vtkClientServerStream::End;

  if (pvApp->GetUseStereoRendering())
    {
    pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
                    << "StereoCapableWindowOn" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
                    << "StereoRenderOn" 
                    << vtkClientServerStream::End;
    //pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
    //                << "SetStereoTypeToCrystalEyes" 
    //                << vtkClientServerStream::End;
    }
  pm->SendStreamToRenderServer();
  
  // Why have this on the client?
  if (pvApp->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }
  
  // We cannot erase the zbuffer.  We need it for picking.  
  pm->GetStream() << vtkClientServerStream::Invoke
         << this->Renderer2DID << "EraseOff" 
         << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
         << this->Renderer2DID << "SetLayer" << 2 
         << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
         << this->RenderWindowID << "SetNumberOfLayers" << 3
         << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
         << this->RenderWindowID << "AddRenderer" << this->RendererID
         << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
         << this->RenderWindowID << "AddRenderer" << this->Renderer2DID
         << vtkClientServerStream::End;
  pm->SendStreamToClientAndRenderServer();
    

  this->DisplayManagerID = pm->NewStreamObject("vtkIceTRenderManager");
  pm->SendStreamToRenderServer();
  int *tileDim = pvApp->GetTileDimensions();
  cout << "Size: " << tileDim[0] << ", " << tileDim[1] << endl;
  pm->GetStream() <<  vtkClientServerStream::Invoke
                  << this->DisplayManagerID
                  << "SetTileDimensions"
                  << tileDim[0] << tileDim[1]
                  << vtkClientServerStream::End;
  pm->SendStreamToRenderServer();

  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->DisplayManagerID << "SetRenderWindow"
                  << this->RenderWindowID
                  << vtkClientServerStream::End;
  pm->SendStreamToRenderServer();

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
  pm->SendStreamToRenderServer();


  // **********************************************************
  this->CompositeID = pm->NewStreamObject("vtkIceTClientCompositeManager");
  vtkClientServerStream tmp = pm->GetStream();
  pm->SendStreamToClient();
  pm->GetStream() = tmp;
  pm->SendStreamToRenderServerRoot();
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  pm->GetStream() << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
                  << "GetRenderServerSocketController"
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
  pm->SendStreamToRenderServerRoot(); // send the same stream to the server root

  // The client server manager needs to set parameters on the IceT manager.
  pm->GetStream() << vtkClientServerStream::Invoke << this->CompositeID
                  << "SetIceTManager" << this->DisplayManagerID
                  << vtkClientServerStream::End;
  pm->SendStreamToRenderServerRoot();
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::StillRender()
{
  // No reduction for still render.
  /*
  if (this->PVApplication && this->DisplayManagerTclName)
    {
    this->PVApplication->GetProcessModule()->RootScript("%s SetImageReductionFactor 1",
                                         this->DisplayManagerTclName);
    this->PVApplication->BroadcastScript("%s ParallelRenderingOn",
                                         this->DisplayManagerTclName);
    }
  */

  this->Superclass::StillRender();
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::InteractiveRender()
{
  /*
  if (this->PVApplication && this->DisplayManagerTclName)
    {
    this->PVApplication->GetProcessModule()->RootScript("%s SetImageReductionFactor %d",
                                         this->DisplayManagerTclName,
                                         this->ReductionFactor);
    this->PVApplication->BroadcastScript("%s ParallelRenderingOn",
                                         this->DisplayManagerTclName);
    }
  */
  this->Superclass::InteractiveRender();
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::SetUseCompositeCompression(int)
{
  // IceT does not have this option.
}


//----------------------------------------------------------------------------
void vtkPVIceTRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
}

