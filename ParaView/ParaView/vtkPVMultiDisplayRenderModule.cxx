/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayRenderModule.cxx
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
#include "vtkPVMultiDisplayRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayRenderModule);
vtkCxxRevisionMacro(vtkPVMultiDisplayRenderModule, "1.1");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVMultiDisplayRenderModule::vtkPVMultiDisplayRenderModule()
{
}

//----------------------------------------------------------------------------
vtkPVMultiDisplayRenderModule::~vtkPVMultiDisplayRenderModule()
{

}




//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::SetPVApplication(vtkPVApplication *pvApp)
{
  this->Superclass::SetPVApplication(pvApp);
  if (pvApp == NULL)
    {
    return;
    }
  // We had trouble with SGI/aliasing with compositing.
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pvApp->GetProcessModule()->GetNumberOfPartitions() > 1))
    {
    pvApp->BroadcastScript("%s SetMultiSamples 0", this->RenderWindowTclName);
    }

  this->Composite = NULL;
  pvApp->MakeTclObject("vtkPVTiledDisplayManager", "TDispManager1");
  int *tileDim = pvApp->GetTileDimensions();
  pvApp->BroadcastScript("TDispManager1 SetTileDimensions %d %d",
                         tileDim[0], tileDim[1]);
  pvApp->BroadcastScript("TDispManager1 InitializeSchedule");

  this->CompositeTclName = NULL;
  this->SetCompositeTclName("TDispManager1");

  pvApp->BroadcastScript("%s SetRenderWindow %s", this->CompositeTclName,
                         this->RenderWindowTclName);
  pvApp->BroadcastScript("%s InitializeRMIs", this->CompositeTclName);

}


//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

