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
#include "vtkSMPartDisplay.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSMPlotDisplay.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVProcessModule.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSimpleRenderModule);
vtkCxxRevisionMacro(vtkPVSimpleRenderModule, "1.5");


//----------------------------------------------------------------------------
vtkPVSimpleRenderModule::vtkPVSimpleRenderModule()
{
}

//----------------------------------------------------------------------------
vtkPVSimpleRenderModule::~vtkPVSimpleRenderModule()
{
}

//-----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::CacheUpdate(int idx, int total)
{
  vtkObject* object;
  vtkSMPartDisplay* pDisp;
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      pDisp->CacheUpdate(idx, total);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::InvalidateAllGeometries()
{
  vtkObject* object;
  vtkSMDisplay* pDisp;
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->MarkConsumersAsModified();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::ComputeVisiblePropBounds(double bds[6])
{
  double* tmp;
  vtkObject* object;
  vtkSMPartDisplay* pDisp;

  // Compute the bounds for our sources.
  bds[0] = bds[2] = bds[4] = VTK_DOUBLE_MAX;
  bds[1] = bds[3] = bds[5] = -VTK_DOUBLE_MAX;
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      tmp = pDisp->GetGeometryInformation()->GetBounds();
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
vtkSMPartDisplay* vtkPVSimpleRenderModule::CreatePartDisplay()
{
  vtkSMPartDisplay* pDisp = vtkSMPartDisplay::New();
  pDisp->SetProcessModule(vtkPVProcessModule::SafeDownCast(this->GetProcessModule()));
  return pDisp;
}
//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::AddDisplay(vtkSMDisplay* disp)
{
  if (disp == NULL)
    {
    return;
    }
  this->Displays->AddItem(disp);
  disp->AddToRenderer(this->RendererID);
}

//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::RemoveDisplay(vtkSMDisplay* disp)
{
  if (disp == NULL)
    {
    return;
    }
  this->Displays->RemoveItem(disp);
  disp->RemoveFromRenderer(this->RendererID);
}



//----------------------------------------------------------------------------
void vtkPVSimpleRenderModule::UpdateAllDisplays()
{
  vtkObject* object;
  vtkSMDisplay* pDisp;
  vtkProcessModule* pm = this->GetProcessModule();
  pm->SendPrepareProgress();

  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMDisplay::SafeDownCast(object);
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
  vtkSMPartDisplay* pDisp;
  vtkPVProcessModule *pm;

  pm = this->ProcessModule;

  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMPartDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->SetUseTriangleStrips(val);
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
  vtkSMPartDisplay* pDisp;

  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMPartDisplay::SafeDownCast(object);
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

