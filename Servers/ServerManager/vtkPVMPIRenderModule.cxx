/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIRenderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMPIRenderModule.h"

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkPVProcessModule.h"
#include "vtkPVTreeComposite.h"
#include "vtkClientServerStream.h"
#include "vtkPVOptions.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMPIRenderModule);
vtkCxxRevisionMacro(vtkPVMPIRenderModule, "1.6");


//----------------------------------------------------------------------------
vtkPVMPIRenderModule::vtkPVMPIRenderModule()
{
}

//----------------------------------------------------------------------------
vtkPVMPIRenderModule::~vtkPVMPIRenderModule()
{
}


//----------------------------------------------------------------------------
void vtkPVMPIRenderModule::SetProcessModule(vtkProcessModule *pm)
{
  this->Superclass::SetProcessModule(pm);
  if (this->ProcessModule == NULL)
    {
    return;
    }

  vtkClientServerStream stream;
  // We had trouble with SGI/aliasing with compositing.
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pm->GetNumberOfPartitions() > 1))
    {
    stream << vtkClientServerStream::Invoke
           << this->RenderWindowID << "SetMultiSamples" << 0
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
    }

  if (this->ProcessModule->GetOptions()->GetClientMode() || this->ProcessModule->GetOptions()->GetServerMode())
    {
    this->CompositeID = 
      pm->NewStreamObject("vtkClientCompositeManager", stream);
    // Clean up this mess !!!!!!!!!!!!!
    // Even a cast to vtkPVClientServerModule would be better than this.
    // How can we syncronize the process modules and render modules?
    stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
           << "GetRenderServerSocketController" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->CompositeID
           << "SetClientController" << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
           << "GetClientMode" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->CompositeID << "SetClientFlag"
           << vtkClientServerStream::LastResult << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
    }
  else
    {
    // Create the compositer.
    this->CompositeID = pm->NewStreamObject("vtkPVTreeComposite", stream);
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
    //this->Composite = vtkPVTreeComposite::SafeDownCast(
    //  pm->GetObjectFromID(this->CompositeID));

    //this->Composite->RemoveObservers(vtkCommand::AbortCheckEvent);
    //vtkCallbackCommand* abc = vtkCallbackCommand::New();
    //abc->SetCallback(PVLODRenderModuleAbortCheck);
    //abc->SetClientData(this);
    //this->AbortCheckTag = 
    //    this->Composite->AddObserver(vtkCommand::AbortCheckEvent, abc);
    //abc->Delete();

    // Try using a more efficient compositer (if it exists).
    // This should be a part of a module.
    vtkClientServerID tmp = 
      pm->NewStreamObject("vtkCompressCompositer", stream);
    stream << vtkClientServerStream::Invoke
           << this->CompositeID << "SetCompositer" << tmp
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Delete << tmp
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);

    // If we are using SGI pipes, create a new Controller/Communicator/Group
    // to use for compositing.
    //if (pvApp->GetUseRenderingGroup())
    //  {
    //  int numPipes = pvApp->GetNumberOfPipes();
    //  // I would like to create another controller with a subset of world, but...
    //  // For now, I added it as a hack to the composite manager.
    //  stream 
    //    << vtkClientServerStream::Invoke << this->CompositeID
    //    << "SetNumberOfProcesses" << numPipes << vtkClientServerStream::End;
    //  pm->SendStream(
    //    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
    //  }
    }

  stream << vtkClientServerStream::Invoke
         << this->CompositeID 
         << "SetRenderWindow"
         << this->RenderWindowID
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->CompositeID << "InitializeRMIs" << vtkClientServerStream::End;
  if ( getenv("PV_DISABLE_COMPOSITE_INTERRUPTS") )
    {
    stream << vtkClientServerStream::Invoke
           << this->CompositeID << "EnableAbortOff" 
           << vtkClientServerStream::End;
    }
  if ( this->ProcessModule->GetOptions()->GetUseOffscreenRendering() )
    {
    stream << vtkClientServerStream::Invoke << this->CompositeID
           << "InitializeOffScreen" << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkPVMPIRenderModule::SetUseCompositeCompression(int val)
{
  vtkPVProcessModule *pm = this->ProcessModule;
  vtkClientServerID tmp;
  if (this->CompositeID.ID)
    {
    vtkClientServerStream stream;
    if (val)
      {
      tmp = pm->NewStreamObject("vtkCompressCompositer", stream);
      }
    else
      {
      tmp = pm->NewStreamObject("vtkTreeCompositer", stream);
      }
    stream << vtkClientServerStream::Invoke
           << this->CompositeID << "SetCompositer" << tmp
           << vtkClientServerStream::End;
    pm->DeleteStreamObject(tmp, stream);
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
    }
}


//----------------------------------------------------------------------------
void vtkPVMPIRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

