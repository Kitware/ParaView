/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositeRenderViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkSMCompositeRenderViewProxy, "1.1");
//-----------------------------------------------------------------------------
vtkSMCompositeRenderViewProxy::vtkSMCompositeRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMCompositeRenderViewProxy::~vtkSMCompositeRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderViewProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated )
    {
    return;
    }
  this->RenderWindowProxy = this->GetSubProxy("RenderWindow");

  if (!this->RenderWindowProxy)
    {
    vtkErrorMacro("RenderWindow subproxy must be defined.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  this->Superclass::CreateVTKObjects(numObjects);

  // Anti-aliasing generally screws up compositing.  Turn it off.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
    (pm->GetNumberOfPartitions(this->ConnectionID) > 1))
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->RenderWindowProxy->GetID(0) << "SetMultiSamples" << 0
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
    }

  // Give subclasses a chance to initialize there compositing.
  this->InitializeCompositingPipeline();
}


//-----------------------------------------------------------------------------
void vtkSMCompositeRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
