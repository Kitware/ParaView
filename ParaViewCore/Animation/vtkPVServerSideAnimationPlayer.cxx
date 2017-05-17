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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSaveAnimationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <cassert>
#include <sstream>

class vtkPVServerSideAnimationPlayerObserver : public vtkCommand
{
public:
  static vtkPVServerSideAnimationPlayerObserver* New()
  {
    return new vtkPVServerSideAnimationPlayerObserver;
  }
  vtkTypeMacro(vtkPVServerSideAnimationPlayerObserver, vtkCommand);

  void SetStateXML(vtkPVXMLElement* xml) { this->StateXML = xml; }
  void SetFileName(const std::string& str) { this->FileName = str; }
  void SetSession(vtkSMSession* session) { this->Session = session; }
  void SetTargetSessionID(vtkIdType id) { this->TargetSessionID = id; }

  void Execute(vtkObject* caller, unsigned long eventid, void* calldata) VTK_OVERRIDE
  {
    assert(vtkProcessModule::SafeDownCast(caller) != NULL);
    if (eventid != vtkCommand::ConnectionClosedEvent)
    {
      return;
    }

    // Check if the session being cleaned up is indeed the one we were
    // observing.
    vtkIdType sessionId = *(reinterpret_cast<vtkIdType*>(calldata));
    if (sessionId <= 0 || sessionId != this->TargetSessionID)
    {
      return;
    }

    this->TargetSessionID = -1;
    this->SaveAnimation();

    // We no longer need to handle any events.
    caller->RemoveObserver(this);
  }

protected:
  vtkPVServerSideAnimationPlayerObserver()
    : TargetSessionID(-1)
  {
  }
  ~vtkPVServerSideAnimationPlayerObserver() {}

  void SaveAnimation()
  {
    assert(!this->FileName.empty() && this->StateXML != NULL && this->Session != NULL);

    // Switch process type to Batch mode
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    assert(pm->GetPartitionId() == 0);

    pm->UpdateProcessType(vtkProcessModule::PROCESS_BATCH, false);

    // Make sure our internal session will be used across rendering calls and
    // proxy management
    vtkIdType tmpSessionID = pm->RegisterSession(this->Session);
    this->Session->Activate();

    // Update our local SessionProxyManager state
    vtkSMSessionProxyManager* spxm = this->Session->GetSessionProxyManager();
    spxm->LoadXMLState(this->StateXML);

    // FIXME:
    // Mark all views so that they render offscreen.
    // BUG #10159.

    // Write any animations.
    if (vtkSMSaveAnimationProxy* options =
          vtkSMSaveAnimationProxy::SafeDownCast(spxm->FindProxy("misc", "misc", "SaveAnimation")))
    {
      vtkSMPropertyHelper(options, "DisconnectAndSave").Set(0);
      options->WriteAnimation(this->FileName.c_str());
    }

    // Disconnect our session from process module
    this->Session->DeActivate();
    pm->UnRegisterSession(tmpSessionID);
    spxm->UnRegisterProxies();
  }

private:
  std::string FileName;
  vtkSmartPointer<vtkPVXMLElement> StateXML;
  vtkSmartPointer<vtkSMSession> Session;
  vtkIdType TargetSessionID;
};

//****************************************************************************
vtkStandardNewMacro(vtkPVServerSideAnimationPlayer);
//----------------------------------------------------------------------------
vtkPVServerSideAnimationPlayer::vtkPVServerSideAnimationPlayer()
  : SessionProxyManagerState(NULL)
  , FileName(NULL)
{
}

//----------------------------------------------------------------------------
vtkPVServerSideAnimationPlayer::~vtkPVServerSideAnimationPlayer()
{
  this->SetSessionProxyManagerState(NULL);
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
void vtkPVServerSideAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVServerSideAnimationPlayer::Activate()
{
  if (!this->SessionProxyManagerState)
  {
    vtkErrorMacro("No `SessionProxyManagerState` specified.");
    return;
  }

  if (!this->FileName)
  {
    vtkErrorMacro("No `FileName` specified.");
    return;
  }

  vtkSmartPointer<vtkPVXMLElement> pxmState =
    vtkPVXMLParser::ParseXML(this->SessionProxyManagerState);
  if (!pxmState)
  {
    vtkErrorMacro("Failed to parse `SessionProxyManagerState`.");
    return;
  }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  assert(pm != NULL);
  if (pm->GetPartitionId() == 0)
  {
    vtkNew<vtkPVServerSideAnimationPlayerObserver> observer;
    observer->SetStateXML(pxmState);
    observer->SetFileName(this->FileName);

    // Root node: monitor the process module till when the active session gets
    // disconnected.
    vtkPVSessionBase* serverSession = vtkPVSessionBase::SafeDownCast(pm->GetActiveSession());
    observer->SetTargetSessionID(pm->GetSessionID(serverSession));
    assert("Server session were found" && serverSession);

    // Create a new session that re-uses the existing session's parallel
    // communication infrastructure.
    vtkSmartPointer<vtkSMSession> session;
    session.TakeReference(vtkSMSession::New(serverSession));
    observer->SetSession(session);

    // Make sure the proxy definition manager will use the proper session core
    // Only for Root node, satellites don't have SessionProxyManager
    if (session->GetSessionProxyManager())
    {
      vtkSmartPointer<vtkSMProxyDefinitionManager> definitionManager;
      definitionManager = session->GetSessionProxyManager()->GetProxyDefinitionManager();

      definitionManager->SetSession(
        NULL); // This will force the session to be properly set later on
      definitionManager->SetSession(session.GetPointer());

      // Attach to unregister event, so that when the "active" session is unregistered, we can
      // do the cleanup.
      pm->AddObserver(vtkCommand::ConnectionClosedEvent, observer.Get());
    }
  }
  else
  {
    // Satellite
    pm->UpdateProcessType(vtkProcessModule::PROCESS_BATCH, false);
  }
}
