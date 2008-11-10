/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingHelperProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStreamingHelperProxy.h"

#include "vtkStreamingFactory.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"


//-----------------------------------------------------------------------------
int vtkSMStreamingHelperProxy::StreamingFactoryRegistered = 0;

//-----------------------------------------------------------------------------
class vtkSMStreamingHelperProxy::vtkInternal
{
public:
  vtkInternal()
  {
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMStreamingHelperProxy);
vtkCxxRevisionMacro(vtkSMStreamingHelperProxy, "1.2");

//----------------------------------------------------------------------------
vtkSMStreamingHelperProxy::vtkSMStreamingHelperProxy()
{
  this->EnableStreamMessages = false;
  this->StreamedPasses = 16;
  this->UseCulling = true;
  this->UseViewOrdering = true;
  this->PieceCacheLimit = 16;
  this->PieceRenderCutoff = 16;

  this->Internal = new vtkInternal;

  // Register the streaming object factory
  if (!vtkSMStreamingHelperProxy::StreamingFactoryRegistered)
    {
    vtkStreamingFactory* sf = vtkStreamingFactory::New();
    vtkObjectFactory::RegisterFactory(sf);
    vtkSMStreamingHelperProxy::StreamingFactoryRegistered = 1;
    sf->Delete();
    }

}

//----------------------------------------------------------------------------
vtkSMStreamingHelperProxy::~vtkSMStreamingHelperProxy()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSMStreamingHelperProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
const char* vtkSMStreamingHelperProxy::GetInstanceName()
{
  static const char* name = "StreamingHelperInstance";
  return name;
}

//----------------------------------------------------------------------------
vtkSMStreamingHelperProxy * vtkSMStreamingHelperProxy::GetHelper()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMStreamingHelperProxy * helper = 
    vtkSMStreamingHelperProxy::SafeDownCast(
      pxm->GetProxy("helpers", vtkSMStreamingHelperProxy::GetInstanceName()));
  if (!helper)
    {
    helper = vtkSMStreamingHelperProxy::SafeDownCast(
      pxm->NewProxy("helpers", "StreamingHelper")); 
    if (helper)
      {
      helper->SetConnectionID(NULL);
      pxm->RegisterProxy("helpers", vtkSMStreamingHelperProxy::GetInstanceName(), helper);
      helper->Delete();
      }
    }
  return helper;
}
