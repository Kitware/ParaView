/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODRenderModule.cxx
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
#include "vtkPVLODRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVLODPartDisplay.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkPVPart.h"
#include "vtkPVDataInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODRenderModule);
vtkCxxRevisionMacro(vtkPVLODRenderModule, "1.4");

//int vtkPVLODRenderModuleCommand(ClientData cd, Tcl_Interp *interp,
//                             int argc, char *argv[]);


//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVLODRenderModule::vtkPVLODRenderModule()
{
  this->LODThreshold = 2.0;
  this->LODResolution = 50;

  this->TotalVisibleGeometryMemorySize = 0;
  this->TotalVisibleLODMemorySize = 0;

  this->AbortCheckTag = 0;
}

//----------------------------------------------------------------------------
vtkPVLODRenderModule::~vtkPVLODRenderModule()
{
}

//----------------------------------------------------------------------------
void PVLODRenderModuleAbortCheck(vtkObject*, unsigned long, void* arg, void*)
{
  vtkPVRenderModule *me = (vtkPVRenderModule*)arg;

  if (me->GetRenderInterruptsEnabled())
    {
    // Just forward the event along.
    me->InvokeEvent(vtkCommand::AbortCheckEvent, NULL);  
    }
}


//----------------------------------------------------------------------------
vtkPVPartDisplay* vtkPVLODRenderModule::CreatePartDisplay()
{
  return vtkPVLODPartDisplay::New();
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModule::InteractiveRender()
{
  this->UpdateAllPVData();

  // Used in subclass for window subsampling, but not really necessary here.
  //this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
  this->RenderWindow->SetDesiredUpdateRate(5.0);

  // We need to decide globally whether to use decimated geometry.  
  if (this->GetTotalVisibleGeometryMemorySize() > this->LODThreshold*1000)
    {
    this->GetPVApplication()->SetGlobalLODFlag(1);
    }
  else
    {
    this->GetPVApplication()->SetGlobalLODFlag(0);
    }  

  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");
}







//----------------------------------------------------------------------------
void vtkPVLODRenderModule::SetLODThreshold(float threshold)
{
  this->LODThreshold = threshold;
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModule::SetLODResolution(int resolution)
{
  vtkObject* object;
  vtkPVLODPartDisplay* partDisp;

  this->LODResolution = resolution;

  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    partDisp = vtkPVLODPartDisplay::SafeDownCast(object);
    if (partDisp)
      {
      partDisp->SetLODResolution(resolution);
      }
    } 
}



//-----------------------------------------------------------------------------
void vtkPVLODRenderModule::ComputeTotalVisibleMemorySize()
{
  vtkObject* object;
  vtkPVLODPartDisplay* pDisp;
  vtkPVLODPartDisplayInformation* info;

  this->TotalVisibleGeometryMemorySize = 0;
  this->TotalVisibleLODMemorySize = 0;
  this->PartDisplays->InitTraversal();
  while ( (object=this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVLODPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      info = pDisp->GetInformation();
      this->TotalVisibleGeometryMemorySize += info->GetGeometryMemorySize();
      this->TotalVisibleLODMemorySize += info->GetLODGeometryMemorySize();
      }
    }
}


//-----------------------------------------------------------------------------
unsigned long vtkPVLODRenderModule::GetTotalVisibleGeometryMemorySize()
{
  if (this->GetTotalVisibleMemorySizeValid())
    {
    return this->TotalVisibleGeometryMemorySize;
    }

  this->ComputeTotalVisibleMemorySize();
  return this->TotalVisibleGeometryMemorySize;
}


//----------------------------------------------------------------------------
void vtkPVLODRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LODThreshold: " << this->LODThreshold << endl;
  os << indent << "LODResolution: " << this->LODResolution << endl;
}

