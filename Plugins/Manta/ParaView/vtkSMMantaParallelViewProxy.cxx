/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMantaParallelViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkSMMantaParallelViewProxy.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkMantaParallelViewProxy - parallel view setup for vtkManta
// .SECTION Description
// A parallel View that sets up the parallel display pipeline so that it
// works with manta.

#include "vtkSMMantaParallelViewProxy.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"

#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

#include "vtkSMClientServerRenderSyncManagerHelper.h"
#include "vtkSMIntVectorProperty.h"

#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMRepresentationProxy.h"

#define DEBUGPRINT_VIEW(arg);

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMMantaParallelViewProxy);


//-----------------------------------------------------------------------------
vtkSMMantaParallelViewProxy::vtkSMMantaParallelViewProxy()
{
  this->UsingIceTRenderers = false;
  this->EnableShadows = 0;
  this->Threads = 1;
  this->Samples = 1 ;
  this->MaxDepth = 1;
}

//-----------------------------------------------------------------------------
vtkSMMantaParallelViewProxy::~vtkSMMantaParallelViewProxy()
{
}

//------------------------------------------------------------------------------
bool vtkSMMantaParallelViewProxy::BeginCreateVTKObjects()
{
  DEBUGPRINT_VIEW(
    cerr 
    << "MPV(" << this << ") Creating parallel view "
    << this->GetXMLName() << endl;
    );

  //kicks off object factory override, that swaps GL classes for manta ones
  //does so ONLY on the server
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID id = pm->NewStreamObject("vtkServerSideFactory", stream);
  stream << vtkClientServerStream::Invoke
         << id << "EnableFactory"
         << vtkClientServerStream::End;
  pm->DeleteStreamObject(id, stream);
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::RENDER_SERVER,
                 stream);

  return this->Superclass::BeginCreateVTKObjects();
}

//------------------------------------------------------------------------------
void vtkSMMantaParallelViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  //change out the default CompressCompositer for a TreeCompositer
  vtkSMProxyProperty *pp = 
    vtkSMProxyProperty::SafeDownCast(this->GetProperty("Compositer"));
  vtkSMProxy *tree = this->GetSubProxy("Tree");
  pp->SetProxy(0, tree);

  //turn off the axes widgets by default
  vtkSMIntVectorProperty *vis;
  vtkSMProxy *annotation;
  annotation = this->GetSubProxy("OrientationWidget");
  vis = vtkSMIntVectorProperty::SafeDownCast(annotation->GetProperty("Visibility"));
  vis->SetElement(0,0);
  annotation = this->GetSubProxy("CenterAxes");
  vis = vtkSMIntVectorProperty::SafeDownCast(annotation->GetProperty("Visibility"));
  vis->SetElement(0,0);

  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMMantaParallelViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSMMantaParallelViewProxy::SetThreads(int newval)
{
  if (newval == this->Threads)
    {
    return;
    }
  this->Threads = newval;
  this->Modified();

  //use streams instead of properties because the client
  //doesn't have a vtkMantaRenderer
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkSMProxy *proxy = this->GetRendererProxy();
  vtkClientServerStream stream;
  vtkClientServerID id = proxy->GetID();
  stream << vtkClientServerStream::Invoke
         << id << "SetNumberOfWorkers"
         << newval
         << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::RENDER_SERVER,
                 stream);
}

//-----------------------------------------------------------------------------
void vtkSMMantaParallelViewProxy::SetEnableShadows(int newval)
{
  if (newval == this->EnableShadows)
    {
    return;
    }
  this->EnableShadows = newval;
  this->Modified();

  //use streams instead of properties because the client
  //doesn't have a vtkMantaRenderer
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkSMProxy *proxy = this->GetRendererProxy();
  vtkClientServerStream stream;
  vtkClientServerID id = proxy->GetID();
  stream << vtkClientServerStream::Invoke
         << id << "SetEnableShadows"
         << newval
         << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::RENDER_SERVER,
                 stream);
}

//-----------------------------------------------------------------------------
void vtkSMMantaParallelViewProxy::SetSamples(int newval)
{
  if (newval == this->Samples)
    {
    return;
    }
  this->Samples = newval;
  this->Modified();

  //use streams instead of properties because the client
  //doesn't have a vtkMantaRenderer
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkSMProxy *proxy = this->GetRendererProxy();
  vtkClientServerStream stream;
  vtkClientServerID id = proxy->GetID();
  stream << vtkClientServerStream::Invoke
         << id << "SetSamples"
         << newval
         << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::RENDER_SERVER,
                 stream);
}

//-----------------------------------------------------------------------------
void vtkSMMantaParallelViewProxy::SetMaxDepth(int newval)
{
  if (newval == this->EnableShadows)
    {
    return;
    }
  this->MaxDepth = newval;
  this->Modified();

  //use streams instead of properties because the client
  //doesn't have a vtkMantaRenderer
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkSMProxy *proxy = this->GetRendererProxy();
  vtkClientServerStream stream;
  vtkClientServerID id = proxy->GetID();
  stream << vtkClientServerStream::Invoke
         << id << "SetMaxDepth"
         << newval
         << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::RENDER_SERVER,
                 stream);
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* 
  vtkSMMantaParallelViewProxy::CreateDefaultRepresentation
    (vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  DEBUGPRINT_VIEW(
    cerr << "MPV(" << this << ") CreateDefaultRepresentation" << endl;
    );

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    sproxy->UpdatePipeline(this->GetViewUpdateTime());
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype;
  prototype = pxm->GetPrototypeProxy("representations",
    "MantaGeometryRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool g = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (g)
    {
    DEBUGPRINT_VIEW(
      cerr << "MPV(" << this << ") Created MantaGeometryRepresentation" << endl;
      );
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "MantaGeometryRepresentation"));
    }

  return 0;
}

//-----------------------------------------------------------------------------
const char* vtkSMMantaParallelViewProxy::GetSuggestedViewType(vtkIdType connectionID)
{
  return "MantaIceTDesktopRenderView";
}
