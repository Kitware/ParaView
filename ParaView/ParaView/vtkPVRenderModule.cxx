/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModule.cxx
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
#include "vtkPVRenderModule.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVConfig.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceList.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkFloatArray.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderModule);
vtkCxxRevisionMacro(vtkPVRenderModule, "1.14.4.3");

//int vtkPVRenderModuleCommand(ClientData cd, Tcl_Interp *interp,
//                             int argc, char *argv[]);


//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVRenderModule::vtkPVRenderModule()
{
  this->PVApplication = NULL;
  this->TotalVisibleMemorySizeValid = 0;
  
  this->PartDisplays = vtkCollection::New();

  this->Renderer = 0;
  this->RenderWindow = 0;
  this->RendererID.ID     = 0;
  this->RenderWindowID.ID = 0;
  this->InteractiveRenderTime    = 0;
  this->StillRenderTime          = 0;

  this->ResetCameraClippingRangeTag = 0;

  this->RenderInterruptsEnabled = 1;

}

//----------------------------------------------------------------------------
vtkPVRenderModule::~vtkPVRenderModule()
{
  vtkPVApplication *pvApp = this->PVApplication;
  vtkPVProcessModule* pm = 0;
  if(pvApp)
    {
    pm = pvApp->GetProcessModule();
    }
  
  if (this->Renderer && this->ResetCameraClippingRangeTag > 0)
    {
    this->Renderer->RemoveObserver(this->ResetCameraClippingRangeTag);
    this->ResetCameraClippingRangeTag = 0;
    }

  this->PartDisplays->Delete();
  this->PartDisplays = NULL;

  
  if (this->Renderer)
    {
    if (this->ResetCameraClippingRangeTag > 0)
      {
      this->Renderer->RemoveObserver(this->ResetCameraClippingRangeTag);
      this->ResetCameraClippingRangeTag = 0;
      }

    if ( pm )
      {
      pm->DeleteStreamObject(this->RendererID);
      }
    this->RendererID.ID = 0;
    }
  
  if (this->RenderWindow)
    {
    if ( pm )
      { 
      pm->DeleteStreamObject(this->RenderWindowID);
      }
    this->RenderWindowID.ID = 0;
    }
  if(pm)
    {
    pm->SendStreamToClientAndServer();
    }
  
  this->SetPVApplication(NULL);
}

//----------------------------------------------------------------------------
// This is a bit of a pain.  I do ResetCameraClippingRange as a call back
// because the PVInteractorStyles call ResetCameraClippingRange 
// directly on the renderer.  Since they are PV styles, I might
// have them call the render module directly like they do for render.
void vtkPVRenderModuleResetCameraClippingRange(
  vtkObject *caller, unsigned long vtkNotUsed(event),void *clientData, void *)
{
  float bds[6];
  double range1[2];
  double range2[2];

  vtkPVRenderModule *self = (vtkPVRenderModule *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  // Get default clipping range.
  // Includes 3D widgets but not all processes.
  ren->GetActiveCamera()->GetClippingRange(range1);

  self->ComputeVisiblePropBounds(bds);
  ren->ResetCameraClippingRange(bds);
  // Get part clipping range.
  // Includes all process partitions, but not 3d Widgets.
  ren->GetActiveCamera()->GetClippingRange(range2);

  // Merge
  if (range1[0] < range2[0])
    {
    range2[0] = range1[0];
    }
  if (range1[1] > range2[1])
    {
    range2[1] = range1[1];
    }
  // Includes all process partitions and 3D Widgets.
  ren->GetActiveCamera()->SetClippingRange(range2);
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetPVApplication(vtkPVApplication *pvApp)
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
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  // Maybe I should not reference count this object to avoid
  // a circular reference.
  this->PVApplication = pvApp;
  this->PVApplication->Register(this);

  this->RendererID = pm->NewStreamObject("vtkRenderer");
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
  stream << vtkClientServerStream::Invoke
         << this->RenderWindowID << "AddRenderer" << this->RendererID
         << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  
  // Make sure we have a chance to set the clipping range properly.
  vtkCallbackCommand* cbc;
  cbc = vtkCallbackCommand::New();
  cbc->SetCallback(vtkPVRenderModuleResetCameraClippingRange);
  cbc->SetClientData((void*)this);
  // ren will delete the cbc when the observer is removed.
  this->ResetCameraClippingRangeTag = 
    this->Renderer->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);
  cbc->Delete();
}


//----------------------------------------------------------------------------
vtkRenderer *vtkPVRenderModule::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkPVRenderModule::GetRenderWindow()
{
  return this->RenderWindow;
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetBackgroundColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->RendererID << "SetBackground"
         << r << g << b << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::ComputeVisiblePropBounds(float bds[6])
{
  double* tmp;
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkPVPart* part;

  // Compute the bounds for our sources.
  bds[0] = bds[2] = bds[4] = VTK_LARGE_FLOAT;
  bds[1] = bds[3] = bds[5] = -VTK_LARGE_FLOAT;
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    part = pDisp->GetPart();
    if (part && pDisp->GetVisibility())
      {
      tmp = part->GetDataInformation()->GetBounds();
      if (tmp[0] < bds[0]) { bds[0] = tmp[0]; }  
      if (tmp[1] > bds[1]) { bds[1] = tmp[1]; }  
      if (tmp[2] < bds[2]) { bds[2] = tmp[2]; }  
      if (tmp[3] > bds[3]) { bds[3] = tmp[3]; }  
      if (tmp[4] < bds[4]) { bds[4] = tmp[4]; }  
      if (tmp[5] > bds[5]) { bds[5] = tmp[5]; }  
      }
    }

  if ( bds[0] > bds[1])
    {
    bds[0] = bds[2] = bds[4] = -1.0;
    bds[1] = bds[3] = bds[5] = 1.0;
    }
}


//----------------------------------------------------------------------------
vtkPVPartDisplay* vtkPVRenderModule::CreatePartDisplay()
{
  return vtkPVPartDisplay::New();
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::AddPVSource(vtkPVSource *pvs)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  vtkPVPartDisplay *pDisp;
  int num, idx;
  
  if (pvs == NULL)
    {
    return;
    }  
  
  // I would like to move the addition of the prop into vtkPVPart sometime.
  num = pvs->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = pvs->GetPart(idx);
    // Create a part display for each part.
    pDisp = this->CreatePartDisplay();
    this->PartDisplays->AddItem(pDisp);
    pDisp->SetPVApplication(pvApp);
    part->SetPartDisplay(pDisp);
    pDisp->SetPart(part);

    if (part && pDisp->GetPropID().ID != 0)
      { 
      vtkPVProcessModule *pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      stream << vtkClientServerStream::Invoke << this->RendererID << "AddProp"
             << pDisp->GetPropID() << vtkClientServerStream::End;
      pm->SendStreamToClientAndServer();
      }
    pDisp->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::RemovePVSource(vtkPVSource *pvs)
{
  int idx, num;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  vtkPVPartDisplay *pDisp;

  if (pvs == NULL)
    {
    return;
    }

  num = pvs->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = pvs->GetPart(idx);
    pDisp = part->GetPartDisplay();
    if (pDisp)
      {
      this->PartDisplays->RemoveItem(pDisp);
      if (pDisp->GetPropID().ID != 0)
        {  
        vtkPVProcessModule *pm = pvApp->GetProcessModule();
        vtkClientServerStream& stream = pm->GetStream();
        stream << vtkClientServerStream::Invoke << this->RendererID << "RemoveProp"
               << pDisp->GetPropID() << vtkClientServerStream::End;
        pm->SendStreamToClientAndServer();
        }
      part->SetPartDisplay(NULL);
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::UpdateAllPVData()
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;

  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      pDisp->Update();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::InteractiveRender()
{
  this->UpdateAllPVData();

  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::StillRender()
{
  this->UpdateAllPVData();

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  this->GetPVApplication()->SetGlobalLODFlag(0);
  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");
}



//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseTriangleStrips(int val)
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkPVApplication *pvApp;

  pvApp = this->GetPVApplication();

  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetPart())
      {
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      stream << vtkClientServerStream::Invoke << pDisp->GetPart()->GetGeometryID()
             <<  "SetUseStrips" << val << vtkClientServerStream::End;
      pm->SendStreamToClientAndServer();
      }
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable triangle strips.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable triangle strips.");
    }

}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseParallelProjection(int val)
{
  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable parallel projection.");
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable parallel projection.");
    this->Renderer->GetActiveCamera()->ParallelProjectionOff();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseImmediateMode(int val)
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;

  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->SetUseImmediateMode(val);
      }
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Disable display lists.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Enable display lists.");
    }
}

//----------------------------------------------------------------------------
float vtkPVRenderModule::GetZBufferValue(int x, int y)
{
  vtkFloatArray *array = vtkFloatArray::New();
  float val;

  array->SetNumberOfTuples(1);
  this->RenderWindow->GetZbufferData(x,y, x, y,
                                     array);
  val = array->GetValue(0);
  array->Delete();
  return val;  
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InteractiveRenderTime: " 
     << this->InteractiveRenderTime << endl;
//   os << indent << "RenderWindowTclName: " 
//      << (this->GetRenderWindowTclName()?this->GetRenderWindowTclName():"<none>") << endl;
//   os << indent << "RendererTclName: " 
//      << (this->GetRendererTclName()?this->GetRendererTclName():"<none>") << endl;
  os << indent << "StillRenderTime: " << this->StillRenderTime << endl;
  os << indent << "RenderInterruptsEnabled: " 
     << (this->RenderInterruptsEnabled ? "on" : "off") << endl;

  if (this->PVApplication)
    {
    os << indent << "PVApplication: " << this->PVApplication << endl;
    }
  else
    {
    os << indent << "PVApplication: NULL" << endl;
    }


  os << indent << "TotalVisibleMemorySizeValid: " 
     << this->TotalVisibleMemorySizeValid << endl;
}

