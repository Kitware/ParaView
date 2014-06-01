/*=========================================================================

Program:   ParaView
Module:    vtkPVServerSideAnimationPlayer.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerSideAnimationPlayer.h"

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <string>
#include <vtksys/ios/sstream>

//****************************************************************************
class vtkPVServerSideAnimationPlayer::vtkInternals
{
public:
  vtkInternals(vtkPVServerSideAnimationPlayer* parent)
    : ObserverId(0),
    WatchSessionID(0)
  {
    this->Owner = parent;
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    if (!pm)
      {
      return;
      }

    if (pm->GetPartitionId() == 0)
      {
      // Root node
      // Grab SessionServer and attach the session core to our vtkObject which
      // is a SMSession.
      vtkPVSessionBase* serverSession =
          vtkPVSessionBase::SafeDownCast(pm->GetActiveSession());
      this->WatchSessionID = pm->GetSessionID(serverSession);

      assert("Server session were find" && serverSession);
      this->Session.TakeReference(vtkSMSession::New(serverSession));

      // Make sure the proxy definition manager will use the proper session core
      // Only for Root node, satelites don't have SessionProxyManager
      if(this->Session->GetSessionProxyManager())
        {
        vtkSMProxyDefinitionManager* definitionManager =
            this->Session->GetSessionProxyManager()->GetProxyDefinitionManager();
        definitionManager->SetSession(NULL); // This will force the session to be properly set later on
        definitionManager->SetSession(this->Session.GetPointer());

        // Attach to unregister event, so that when the "active" session is unregistered, we can
        // do the cleanup.
        this->ObserverId = pm->AddObserver(vtkCommand::ConnectionClosedEvent,
          this, &vtkPVServerSideAnimationPlayer::vtkInternals::OnUnRegisterSession);
        }
      }
    else
      {
      // Satellite
      vtkProcessModule::GetProcessModule()->UpdateProcessType(vtkProcessModule::PROCESS_BATCH, false);
      }
  }

  ~vtkInternals()
  {
    if(this->ObserverId != 0)
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      pm->RemoveObserver(this->ObserverId);
      this->ObserverId = 0;
      }
  }

public:
  vtkWeakPointer<vtkPVServerSideAnimationPlayer> Owner;
  vtkSmartPointer<vtkSMSession> Session;
  vtkSmartPointer<vtkSMAnimationSceneImageWriter> Writer;
  vtkSmartPointer<vtkPVXMLElement> XMLState;
private:
  unsigned long ObserverId;
  vtkIdType WatchSessionID;

  void OnUnRegisterSession(vtkObject* caller, unsigned long eventid, void* calldata)
    {
    assert(vtkProcessModule::SafeDownCast(caller) != NULL &&
      eventid == vtkCommand::ConnectionClosedEvent);
    (void)caller;
    (void)eventid;
    vtkIdType sessionId = *(reinterpret_cast<vtkIdType*>(calldata));
    if (sessionId <= 0 || sessionId != this->WatchSessionID)
      {
      return;
      }

    caller->RemoveObserver(this->ObserverId);
    this->ObserverId = 0;
    this->WatchSessionID = 0;
    if (this->Owner)
      {
      this->Owner->TriggerExecution();
      }
    }
};

//****************************************************************************
vtkStandardNewMacro(vtkPVServerSideAnimationPlayer);
//----------------------------------------------------------------------------
vtkPVServerSideAnimationPlayer::vtkPVServerSideAnimationPlayer()
{
  this->Internals = new vtkInternals(this);
}

//----------------------------------------------------------------------------
vtkPVServerSideAnimationPlayer::~vtkPVServerSideAnimationPlayer()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkPVServerSideAnimationPlayer::SetWriter(vtkSMAnimationSceneImageWriter* writer)
{
  this->Internals->Writer = writer;
}

//----------------------------------------------------------------------------
void vtkPVServerSideAnimationPlayer::SetSessionProxyManagerState(const char* xml_state_content)
{
  if(xml_state_content == NULL || strlen(xml_state_content) == 0)
    {
    this->Internals->XMLState = NULL;
    }
  else
    {
    vtkNew<vtkPVXMLParser> parser;
    parser->Parse(xml_state_content);
    this->Internals->XMLState = parser->GetRootElement();
    }
}

//----------------------------------------------------------------------------
void vtkPVServerSideAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVServerSideAnimationPlayer::TriggerExecution()
{
 // Ensure only one execution
  if(this->Internals->Session)
    {
    // Switch process type to Batch mode
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkProcessModule::GetProcessModule()->UpdateProcessType(vtkProcessModule::PROCESS_BATCH, false);

    // Make sure our internal session will be used across rendering calls and
    // proxy management
    vtkIdType tmpSessionID = pm->RegisterSession(this->Internals->Session);
    this->Internals->Session->Activate();

    // Update our local SessionProxyManager state
    vtkSMSessionProxyManager* spxm = this->Internals->Session->GetSessionProxyManager();
    spxm->LoadXMLState(this->Internals->XMLState, NULL, true);

    vtkNew<vtkSMProxyIterator> proxyIter;
    proxyIter->SetSession(this->Internals->Session);

    // Mark all views so that they render offscreen.
    // BUG #10159.
    for (proxyIter->Begin(); !proxyIter->IsAtEnd(); proxyIter->Next())
      {
      vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(proxyIter->GetProxy());
      // We need to ensure that we skip prototypes.
      if (view && !view->IsPrototype())
        {
        if (vtkSMPropertyHelper(view,
                                "UseOffscreenRenderingForScreenshots", true).GetAsInt() == 1)
          {
          vtkSMPropertyHelper(view, "UseOffscreenRendering", true).Set(1);
          view->UpdateVTKObjects();
          }
        }
      }

    // Write any animations.
    for (proxyIter->Begin(); !proxyIter->IsAtEnd(); proxyIter->Next())
      {
      vtkSMAnimationScene* scene =
          vtkSMAnimationScene::SafeDownCast(
            proxyIter->GetProxy()->GetClientSideObject());
      if (scene)
          {
        if (!this->Internals->Writer)
          {
          scene->Play();
          }
        else
          {
          this->Internals->Writer->SetAnimationScene(scene);
          if (!this->Internals->Writer->Save())
            {
            vtkErrorMacro("Failed to save animation.");
            }
          break;
          }
        }
      }

    // Disconnect our session from process module
    this->Internals->Session->DeActivate();
    pm->UnRegisterSession(tmpSessionID);
    this->Internals->Session->GetSessionProxyManager()->UnRegisterProxies();
    }

  // Clean up memory
  this->Internals->Session = NULL;
}
