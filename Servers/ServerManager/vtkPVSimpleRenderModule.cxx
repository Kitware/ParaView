/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSimpleRenderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSimpleRenderModule.h"
#include "vtkToolkits.h"

#include "vtkCamera.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkPVPartDisplay.h"
//#include "vtkCallbackCommand.h"
//#include "vtkCommand.h"
//#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
//#include "vtkPVProcessModule.h"
//#include "vtkPVConfig.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVProcessModule.h"
//#include "vtkPVSourceCollection.h"
//#include "vtkPVSourceList.h"
//#include "vtkPVWindow.h"
//#include "vtkPVSource.h"
//#include "vtkPolyData.h"
//#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
//#include "vtkRenderWindow.h"
//#include "vtkFloatArray.h"
//#include "vtkString.h"
#include "vtkTimerLog.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSimpleRenderModule);
vtkCxxRevisionMacro(vtkPVSimpleRenderModule, "1.2");


//----------------------------------------------------------------------------
vtkPVSimpleRenderModule::vtkPVSimpleRenderModule()
{
}

//----------------------------------------------------------------------------
vtkPVSimpleRenderModule::~vtkPVSimpleRenderModule()
{
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::AddDisplay(vtkPVDisplay* disp)
{
  this->Displays->AddItem(disp);
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::RemoveDisplay(vtkPVDisplay* disp)
{
  this->Displays->RemoveItem(disp);
}

//-----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::CacheUpdate(int idx, int total)
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      pDisp->CacheUpdate(idx, total);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::InvalidateAllGeometries()
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    pDisp->InvalidateGeometry();
    }
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::ComputeVisiblePropBounds(double bds[6])
{
  double* tmp;
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkSMPart* part;

  // Compute the bounds for our sources.
  bds[0] = bds[2] = bds[4] = VTK_DOUBLE_MAX;
  bds[1] = bds[3] = bds[5] = -VTK_DOUBLE_MAX;
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      part = pDisp->GetPart();
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
vtkPVPartDisplay* vtkPVSimpleRenderModule::CreatePartDisplay()
{
  vtkPVPartDisplay* pDisp = vtkPVPartDisplay::New();
  pDisp->SetProcessModule(this->GetProcessModule());
  return pDisp;
}
//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::AddSource(vtkSMSourceProxy *s)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkSMPart *part;
  vtkPVPartDisplay *pDisp;
  int num, idx;
  
  if (s == NULL)
    {
    return;
    }  
  
  num = s->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = s->GetPart(idx);
    // Create a part display for each part.
    pDisp = this->CreatePartDisplay();
    this->Displays->AddItem(pDisp);
    part->SetPartDisplay(pDisp);
    pDisp->SetPart(part);
   
    if (part && pDisp->GetPropID().ID != 0)
      { 
      vtkClientServerStream& stream = pm->GetStream();
      stream << vtkClientServerStream::Invoke << this->RendererID << "AddProp"
             << pDisp->GetPropID() << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << this->RendererID << "AddProp"
             << pDisp->GetVolumeID() << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    pDisp->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::RemoveSource(vtkSMSourceProxy *s)
{
  int idx, num;
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkSMPart *part;
  vtkPVPartDisplay *pDisp;

  if (s == NULL)
    {
    return;
    }

  num = s->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = s->GetPart(idx);
    pDisp = part->GetPartDisplay();
    if (pDisp)
      {
      this->Displays->RemoveItem(pDisp);
      if (pDisp->GetPropID().ID != 0)
        {  
        vtkClientServerStream& stream = pm->GetStream();
        stream << vtkClientServerStream::Invoke << this->RendererID 
               << "RemoveProp"
               << pDisp->GetPropID() << vtkClientServerStream::End;
        stream << vtkClientServerStream::Invoke << this->RendererID 
               << "RemoveProp"
               << pDisp->GetVolumeID() << vtkClientServerStream::End;
        pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
        }
      part->SetPartDisplay(NULL);
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::UpdateAllDisplays()
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkPVProcessModule* pm = this->GetProcessModule();
  pm->SendPrepareProgress();

  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      pDisp->Update();
      }
    }
  pm->SendCleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::SetUseTriangleStrips(int val)
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkPVProcessModule *pm;

  pm = this->GetProcessModule();

  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetPart())
      {
      vtkClientServerStream& stream = pm->GetStream();
      stream << vtkClientServerStream::Invoke << pDisp->GetGeometryID()
             <<  "SetUseStrips" << val << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
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
void vtkPVSimpleRenderModule::SetUseParallelProjection(int val)
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
void vtkPVSimpleRenderModule::SetUseImmediateMode(int val)
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;

  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
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
void vtkPVSimpleRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

