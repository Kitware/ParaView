/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIRenderModule.cxx
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
#include "vtkPVMPIRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVTreeComposite.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMPIRenderModule);
vtkCxxRevisionMacro(vtkPVMPIRenderModule, "1.3.2.4");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVMPIRenderModule::vtkPVMPIRenderModule()
{
}

//----------------------------------------------------------------------------
vtkPVMPIRenderModule::~vtkPVMPIRenderModule()
{
}


//----------------------------------------------------------------------------
void vtkPVMPIRenderModule::SetPVApplication(vtkPVApplication *pvApp)
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

  if (pvApp->GetClientMode() || pvApp->GetServerMode())
    {
    this->Composite = NULL;
    pvApp->MakeTclObject("vtkClientCompositeManager", "CCompositeManager1");
    // Clean up this mess !!!!!!!!!!!!!
    // Even a cast to vtkPVClientServerModule would be better than this.
    // How can we syncronize the process modules and render modules?
    pvApp->BroadcastScript("CCompositeManager1 SetClientController [$Application GetSocketController]");
    pvApp->BroadcastScript("CCompositeManager1 SetClientFlag [$Application GetClientMode]");

    this->CompositeTclName = NULL;
    this->SetCompositeTclName("CCompositeManager1");    
    }
  else
    {
    // Create the compositer.
    this->Composite = static_cast<vtkPVTreeComposite*>
      (pvApp->MakeTclObject("vtkPVTreeComposite", "TreeComp1"));

    //this->Composite->RemoveObservers(vtkCommand::AbortCheckEvent);
    //vtkCallbackCommand* abc = vtkCallbackCommand::New();
    //abc->SetCallback(PVLODRenderModuleAbortCheck);
    //abc->SetClientData(this);
    //this->AbortCheckTag = 
    //    this->Composite->AddObserver(vtkCommand::AbortCheckEvent, abc);
    //abc->Delete();

    // Try using a more efficient compositer (if it exists).
    // This should be a part of a module.
    if (strcmp(pvApp->GetRenderModuleName(),"DeskTopRenderModule") != 0)
      {
      pvApp->BroadcastScript("if {[catch {vtkCompressCompositer pvTmp}] == 0} "
                             "{TreeComp1 SetCompositer pvTmp; pvTmp Delete}");
      }

    this->CompositeTclName = NULL;
    this->SetCompositeTclName("TreeComp1");

    // If we are using SGI pipes, create a new Controller/Communicator/Group
    // to use for compositing.
    if (pvApp->GetUseRenderingGroup())
      {
      int numPipes = pvApp->GetNumberOfPipes();
      // I would like to create another controller with a subset of world, but...
      // For now, I added it as a hack to the composite manager.
      pvApp->BroadcastScript("%s SetNumberOfProcesses %d",
                             this->CompositeTclName, numPipes);
      }
    }

  pvApp->BroadcastScript("%s SetRenderWindow %s", this->CompositeTclName,
                         this->RenderWindowTclName);
  pvApp->BroadcastScript("%s InitializeRMIs", this->CompositeTclName);


  if ( getenv("PV_DISABLE_COMPOSITE_INTERRUPTS") )
    {
    pvApp->BroadcastScript("%s EnableAbortOff", this->CompositeTclName);
    }

  if ( pvApp->GetUseOffscreenRendering() )
    {
    pvApp->BroadcastScript("%s InitializeOffScreen", this->CompositeTclName);
    }
}

//----------------------------------------------------------------------------
void vtkPVMPIRenderModule::SetUseCompositeCompression(int val)
{

    vtkPVApplication *pvApp = this->GetPVApplication();
 if (strcmp(pvApp->GetRenderModuleName(),"DeskTopRenderModule") != 0)
  {

  if (this->CompositeTclName)
    {
    if (val)
      {
      pvApp->BroadcastScript("vtkCompressCompositer pvTemp");
      }
    else
      {
      pvApp->BroadcastScript("vtkTreeCompositer pvTemp");
      }
    pvApp->BroadcastScript("%s SetCompositer pvTemp", this->CompositeTclName);
    pvApp->BroadcastScript("pvTemp Delete");
    }
   }
}


//----------------------------------------------------------------------------
void vtkPVMPIRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

