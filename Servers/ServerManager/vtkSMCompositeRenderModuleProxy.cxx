/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositeRenderModuleProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageWriter.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkPVServerInformation.h"
#include "vtkPVTreeComposite.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkSMCompositeDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkToolkits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"

vtkCxxRevisionMacro(vtkSMCompositeRenderModuleProxy, "1.20");
//-----------------------------------------------------------------------------
vtkSMCompositeRenderModuleProxy::vtkSMCompositeRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMCompositeRenderModuleProxy::~vtkSMCompositeRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::CreateVTKObjects(int numObjects)
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
void vtkSMCompositeRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
