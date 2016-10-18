/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyDefinitionManager.h"

#include "vtkClientServerStream.h"
#include "vtkEventForwarderCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkTimerLog.h"

#include <sstream>

vtkStandardNewMacro(vtkSMProxyDefinitionManager);
//----------------------------------------------------------------------------
vtkSMProxyDefinitionManager::vtkSMProxyDefinitionManager()
{
  this->SetGlobalID(vtkSIProxyDefinitionManager::GetReservedGlobalID());
  this->Forwarder = vtkEventForwarderCommand::New();
  this->Forwarder->SetTarget(this);
  this->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
vtkSMProxyDefinitionManager::~vtkSMProxyDefinitionManager()
{
  this->SetSession(NULL);
  this->Forwarder->SetTarget(NULL);
  this->Forwarder->Delete();
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::SetSession(vtkSMSession* session)
{
  if (this->GetSession() == session)
  {
    return;
  }

  if (this->ProxyDefinitionManager)
  {
    this->ProxyDefinitionManager->RemoveObserver(this->Forwarder);
  }
  this->ProxyDefinitionManager = NULL;
  this->Superclass::SetSession(session);

  if (session)
  {
    this->ProxyDefinitionManager = session->GetProxyDefinitionManager();
    this->ProxyDefinitionManager->AddObserver(
      vtkSIProxyDefinitionManager::ProxyDefinitionsUpdated, this->Forwarder);
    this->ProxyDefinitionManager->AddObserver(
      vtkSIProxyDefinitionManager::CompoundProxyDefinitionsUpdated, this->Forwarder);
    this->ProxyDefinitionManager->AddObserver(vtkCommand::RegisterEvent, this->Forwarder);
    this->ProxyDefinitionManager->AddObserver(vtkCommand::UnRegisterEvent, this->Forwarder);
    this->ProxyDefinitionManager->EnableXMLProxyDefnitionUpdate(
      (session->GetProcessRoles() & vtkPVSession::SERVERS) != 0);
    this->SynchronizeDefinitions();
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::SynchronizeDefinitions()
{
  // Pull state from the data-server root and load it locally.
  if (!this->GetSession() || (this->GetSession()->GetProcessRoles() & vtkPVSession::SERVERS) != 0)
  {
    // not running in remote-mode, in that case nothing to update.
    return;
  }

  vtkTimerLog::MarkStartEvent("Process Proxy definitions");
  vtkSMMessage message;
  this->SetLocation(vtkPVSession::SERVERS);
  if (this->PullState(&message) == false)
  {
    this->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
    vtkErrorMacro("Failed to obtain server state.");
    return;
  }

  this->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
  this->ProxyDefinitionManager->Push(&message);
  vtkTimerLog::MarkEndEvent("Process Proxy definitions");
}
//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::LoadState(
  const vtkSMMessage* msg, vtkSMProxyLocator* vtkNotUsed(locator))
{
  vtkSMMessage copy = *msg;
  this->ProxyDefinitionManager->Push(&copy);
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::AddCustomProxyDefinition(
  const char* group, const char* name, vtkPVXMLElement* top)
{
  if (this->ProxyDefinitionManager)
  {
    // Generally such logic is bad, but we can optimizing the parsing when
    // running in built-in mode, so what the heck!
    this->ProxyDefinitionManager->AddCustomProxyDefinition(group, name, top);

    if ((this->GetSession()->GetProcessRoles() & vtkPVSession::SERVERS) != 0)
    {
      return;
    }
    if (this->GetSession() && top && group && name)
    {
      std::ostringstream str_stream;
      top->PrintXML(str_stream, vtkIndent());
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << SIOBJECT(this) << "AddCustomProxyDefinition"
             << group << name << str_stream.str().c_str() << vtkClientServerStream::End;
      this->GetSession()->ExecuteStream(vtkPVSession::SERVERS, stream, false);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::RemoveCustomProxyDefinition(const char* group, const char* name)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << SIOBJECT(this) << "RemoveCustomProxyDefinition"
         << group << name << vtkClientServerStream::End;
  if (this->GetSession())
  {
    this->GetSession()->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, stream, false);
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::ClearCustomProxyDefinitions()
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << SIOBJECT(this) << "ClearCustomProxyDefinitions"
         << vtkClientServerStream::End;
  if (this->GetSession())
  {
    this->GetSession()->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, stream, false);
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::LoadCustomProxyDefinitions(vtkPVXMLElement* root)
{
  if (this->ProxyDefinitionManager)
  {
    // Generally such logic is bad, but we can optimizing the parsing when
    // running in built-in mode, so what the heck!
    this->ProxyDefinitionManager->LoadCustomProxyDefinitions(root);

    if ((this->GetSession()->GetProcessRoles() & vtkPVSession::SERVERS) != 0)
    {
      return;
    }

    if (this->GetSession() && root)
    {
      std::ostringstream str_stream;
      root->PrintXML(str_stream, vtkIndent());
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << SIOBJECT(this)
             << "LoadCustomProxyDefinitionsFromString" << str_stream.str().c_str()
             << vtkClientServerStream::End;
      this->GetSession()->ExecuteStream(vtkPVSession::SERVERS, stream, false);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::LoadCustomProxyDefinitionsFromString(const char* xmlContent)
{
  if (this->GetSession() && xmlContent)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << SIOBJECT(this)
           << "LoadCustomProxyDefinitionsFromString" << xmlContent << vtkClientServerStream::End;
    this->GetSession()->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, stream, false);
  }
}

//----------------------------------------------------------------------------
bool vtkSMProxyDefinitionManager::LoadConfigurationXML(vtkPVXMLElement* root)
{
  if (this->ProxyDefinitionManager)
  {
    // Generally such logic is bad, but we can optimizing the parsing when
    // running in built-in mode, so what the heck!
    this->ProxyDefinitionManager->LoadConfigurationXML(root);

    if ((this->GetSession()->GetProcessRoles() & vtkPVSession::SERVERS) != 0)
    {
      return true;
    }

    if (this->GetSession() && root)
    {
      std::ostringstream str_stream;
      root->PrintXML(str_stream, vtkIndent());
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << SIOBJECT(this) << "LoadConfigurationXMLFromString"
             << str_stream.str().c_str() << vtkClientServerStream::End;
      this->GetSession()->ExecuteStream(vtkPVSession::SERVERS, stream, false);
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMProxyDefinitionManager::LoadConfigurationXMLFromString(const char* xmlContent)
{
  if (this->GetSession() && xmlContent)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << SIOBJECT(this) << "LoadConfigurationXMLFromString"
           << xmlContent << vtkClientServerStream::End;
    this->GetSession()->ExecuteStream(vtkPVSession::CLIENT_AND_SERVERS, stream, false);
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
