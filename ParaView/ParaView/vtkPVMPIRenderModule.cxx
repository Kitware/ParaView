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
vtkCxxRevisionMacro(vtkPVMPIRenderModule, "1.4.2.4");



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
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  // We had trouble with SGI/aliasing with compositing.
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pvApp->GetProcessModule()->GetNumberOfPartitions() > 1))
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->RenderWindowID << "SetMultiSamples" << 0
      << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }

  if (pvApp->GetClientMode() || pvApp->GetServerMode())
    {
    this->Composite = NULL;
    this->CompositeID = pm->NewStreamObject("vtkClientCompositeManager");
    // Clean up this mess !!!!!!!!!!!!!
    // Even a cast to vtkPVClientServerModule would be better than this.
    // How can we syncronize the process modules and render modules?
    pm->GetStream()
      << vtkClientServerStream::Invoke << pm->GetApplicationID()
      << "GetSocketController" << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke << this->CompositeID
      << "SetClientController" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke << pm->GetApplicationID()
      << "GetClientMode" << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke << this->CompositeID << "SetClientFlag"
      << vtkClientServerStream::LastResult << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
  else
    {
    // Create the compositer.
    this->CompositeID = pm->NewStreamObject("vtkPVTreeComposite");
    pm->SendStreamToClientAndServer();
    this->Composite = vtkPVTreeComposite::SafeDownCast(
      pm->GetObjectFromID(this->CompositeID));

    //this->Composite->RemoveObservers(vtkCommand::AbortCheckEvent);
    //vtkCallbackCommand* abc = vtkCallbackCommand::New();
    //abc->SetCallback(PVLODRenderModuleAbortCheck);
    //abc->SetClientData(this);
    //this->AbortCheckTag = 
    //    this->Composite->AddObserver(vtkCommand::AbortCheckEvent, abc);
    //abc->Delete();

    // Try using a more efficient compositer (if it exists).
    // This should be a part of a module.
    vtkClientServerID tmp = pm->NewStreamObject("vtkCompressCompositer");
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetCompositer" << tmp
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Delete << tmp
      << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();

    // If we are using SGI pipes, create a new Controller/Communicator/Group
    // to use for compositing.
    if (pvApp->GetUseRenderingGroup())
      {
      int numPipes = pvApp->GetNumberOfPipes();
      // I would like to create another controller with a subset of world, but...
      // For now, I added it as a hack to the composite manager.
      pm->GetStream()
        << vtkClientServerStream::Invoke << this->CompositeID
        << "SetNumberOfProcesses" << numPipes << vtkClientServerStream::End;
      pm->SendStreamToClientAndServer();
      }
    }

 // pvApp->BroadcastScript("%s SetRenderWindow %s", this->CompositeTclName,
  //                       this->RenderWindowTclName);
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CompositeID << "InitializeRMIs" << vtkClientServerStream::End;
  if ( getenv("PV_DISABLE_COMPOSITE_INTERRUPTS") )
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "EnableAbortOff" << vtkClientServerStream::End;
    }
  if ( pvApp->GetUseOffscreenRendering() )
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke << this->CompositeID
      << "InitializeOffScreen" << vtkClientServerStream::End;
    }
  pm->SendStreamToClientAndServer();
}

//----------------------------------------------------------------------------
void vtkPVMPIRenderModule::SetUseCompositeCompression(int val)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  if (strcmp(pvApp->GetRenderModuleName(),"DeskTopRenderModule") != 0)
    {
    vtkClientServerID tmp;
    if (this->CompositeID.ID)
      {
      if (val)
        {
        tmp = pm->NewStreamObject("vtkCompressCompositer");
        }
      else
        {
        tmp = pm->NewStreamObject("vtkTreeCompositer");
        }
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "SetCompositer" << tmp
        << vtkClientServerStream::End;
      pm->DeleteStreamObject(tmp);
      pm->SendStreamToClientAndServer();
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVMPIRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

