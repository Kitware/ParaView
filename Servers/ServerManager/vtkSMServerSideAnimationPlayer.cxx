/*=========================================================================

  Program:   ParaView
  Module:    vtkSMServerSideAnimationPlayer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMServerSideAnimationPlayer.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMProxyIterator.h"

#include <vtkstd/string>

//-----------------------------------------------------------------------------
class vtkSMServerSideAnimationPlayerObserver : public vtkCommand
{
public:
  static vtkSMServerSideAnimationPlayerObserver* New()
    { return new vtkSMServerSideAnimationPlayerObserver; }

  void SetTarget(vtkSMServerSideAnimationPlayer* t)
    {
    this->Target = t;
    }

  virtual void Execute(vtkObject *obj, unsigned long event, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(obj, event, data);
      }
    }
protected:
  vtkSMServerSideAnimationPlayerObserver()
    {
    this->Target = 0;
    }
  ~vtkSMServerSideAnimationPlayerObserver()
    {
    this->Target = 0;
    }
  vtkSMServerSideAnimationPlayer* Target;
};
//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMServerSideAnimationPlayer);
vtkCxxRevisionMacro(vtkSMServerSideAnimationPlayer, "1.1");
//-----------------------------------------------------------------------------
vtkSMServerSideAnimationPlayer::vtkSMServerSideAnimationPlayer()
{
  this->ConnectionID = 0;
  this->Observer = vtkSMServerSideAnimationPlayerObserver::New();
  this->Observer->SetTarget(this);
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->AddObserver(vtkCommand::ConnectionClosedEvent, this->Observer);

  this->AnimationFileName =0;
  this->Size[0] = 400;
  this->Size[1] = 400;
  this->FrameRate = 1;
  this->Quality = 1;
}

//-----------------------------------------------------------------------------
vtkSMServerSideAnimationPlayer::~vtkSMServerSideAnimationPlayer()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm)
    {
    pm->RemoveObserver(this->Observer);
    }
  this->Observer->SetTarget(0);
  this->Observer->Delete();
  this->SetAnimationFileName(0);
}

//-----------------------------------------------------------------------------
void vtkSMServerSideAnimationPlayer::ExecuteEvent(vtkObject* vtkNotUsed(obj), 
  unsigned long event, void* data)
{
  if (event == vtkCommand::ConnectionClosedEvent)
    {
    vtkIdType cid = *(static_cast<vtkIdType*>(data));
    if (cid == this->ConnectionID)
      {
      this->PerformActions();
      }
    }
}
//-----------------------------------------------------------------------------
void vtkSMServerSideAnimationPlayer::PerformActions()
{
  cout << "Performing ServerSide Actions" << endl;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("No proxy manager located.");
    return;
    }

  pxm->UpdateRegisteredProxies(0);
  pxm->SaveState("/tmp/serverstate.xml");

  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();

  // Render any views.
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMRenderModuleProxy* ren = 
      vtkSMRenderModuleProxy::SafeDownCast(iter->GetProxy());
    if (ren)
      {
      ren->StillRender();
      }
    }

  // Play any animations.
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMAnimationSceneProxy* scene = 
      vtkSMAnimationSceneProxy::SafeDownCast(iter->GetProxy());
    if (scene)
      {
      if (!this->AnimationFileName)
        {
        scene->Play();
        }
      else
        {
        vtkstd::string name = this->AnimationFileName; 
        vtkstd::string ext;
        vtkstd::string::size_type dot_pos = name.rfind(".");
        if(dot_pos != vtkstd::string::npos)
          {
          ext = name.substr(dot_pos+1);
          name = name.substr(0, dot_pos);
          }

        scene->SaveImages(name.c_str(), ext.c_str(),
          this->Size[0], this->Size[1],
          this->FrameRate,
          this->Quality);
        }
      }
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->StopAcceptingAllConnections();

  // Essential to unregister all proxies. 
  // Note that this call will make *this* invalid.
  pxm->UnRegisterProxies();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMServerSideAnimationPlayer::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConnectionID: " << this->ConnectionID << endl;
}
