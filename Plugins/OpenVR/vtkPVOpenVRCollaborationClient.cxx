/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOpenVRCollaborationClient.h"

#include "vtkObjectFactory.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderer.h"
#include "vtkPVOpenVRHelper.h"

#ifdef OPENVR_HAS_COLLABORATION
#include "mvCollaborationClient.h"

#define mvLog(x)                                                                                   \
  if (this->Callback)                                                                              \
  {                                                                                                \
    std::ostringstream ss;                                                                         \
    ss << x;                                                                                       \
    this->Callback(ss.str(), this->CallbackClientData);                                            \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    std::cout << x;                                                                                \
  }

class vtkPVOpenVRCollaborationClientInternal : public mvCollaborationClient
{
public:
  void SetHelper(vtkPVOpenVRHelper* l) { this->Helper = l; }

protected:
  void HandleBroadcastMessage(std::string const& otherID, std::string const& type) override
  {
    if (type == "P")
    {
      // change pose.
      zmq::message_t update;
      int viewIndex = 0;
      double updateTranslation[3] = { 0 };
      double updateDirection[3] = { 0.0 };

      this->Subscriber.recv(&update);
      memcpy(&viewIndex, update.data(), sizeof(viewIndex));
      this->Subscriber.recv(&update);
      memcpy(&updateTranslation[0], update.data(), sizeof(updateTranslation));
      this->Subscriber.recv(&update);
      memcpy(&updateDirection[0], update.data(), sizeof(updateDirection));
      mvLog("Collab " << otherID << ", Pose: " << viewIndex << " " << updateTranslation[0] << " "
                      << updateTranslation[1] << " " << updateTranslation[2] << " " << std::endl);

      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->GoToSavedLocation(viewIndex, updateTranslation, updateDirection);
      }
    }
    else
    {
      this->mvCollaborationClient::HandleBroadcastMessage(otherID, type);
    }
  }

  vtkPVOpenVRHelper* Helper;
};
#else
class vtkPVOpenVRCollaborationClientInternal
{
};
#endif

vtkStandardNewMacro(vtkPVOpenVRCollaborationClient);

//----------------------------------------------------------------------------
vtkPVOpenVRCollaborationClient::vtkPVOpenVRCollaborationClient()
  : CurrentLocation(-1)
{
  this->Internal = new vtkPVOpenVRCollaborationClientInternal();
}

//----------------------------------------------------------------------------
vtkPVOpenVRCollaborationClient::~vtkPVOpenVRCollaborationClient()
{
  delete this->Internal;
}

bool vtkPVOpenVRCollaborationClient::SupportsCollaboration()
{
#ifdef OPENVR_HAS_COLLABORATION
  return true;
#else
  return false;
#endif
}

#ifdef OPENVR_HAS_COLLABORATION
bool vtkPVOpenVRCollaborationClient::Connect(vtkOpenVRRenderer* ren)
{
  return this->Internal->Initialize(ren);
#else
bool vtkPVOpenVRCollaborationClient::Connect(vtkOpenVRRenderer*)
{
  return true;
#endif
}

bool vtkPVOpenVRCollaborationClient::Disconnect()
{
#ifdef OPENVR_HAS_COLLABORATION
  this->Internal->Disconnect();
#endif
  return true;
}

void vtkPVOpenVRCollaborationClient::Render()
{
#ifdef OPENVR_HAS_COLLABORATION
  this->Internal->Render();
#endif
}

#ifdef OPENVR_HAS_COLLABORATION
void vtkPVOpenVRCollaborationClient::SetCollabHost(std::string const& val)
{
  this->Internal->SetCollabHost(val);
}
void vtkPVOpenVRCollaborationClient::SetCollabSession(std::string const& val)
{
  this->Internal->SetCollabSession(val);
}
void vtkPVOpenVRCollaborationClient::SetCollabName(std::string const& val)
{
  this->Internal->SetCollabName(val);
}
void vtkPVOpenVRCollaborationClient::SetCollabPort(int val)
{
  this->Internal->SetCollabPort(val);
}
void vtkPVOpenVRCollaborationClient::SetLogCallback(
  std::function<void(std::string const& data, void* cd)> cb, void* clientData)
{
  this->Internal->SetLogCallback(cb, clientData);
}
void vtkPVOpenVRCollaborationClient::GoToSavedLocation(int index)
{
  // only send if we need to
  if (index != this->CurrentLocation)
  {
    if (this->Internal->GetConnected())
    {
      vtkOpenVRRenderer* ren = this->Internal->GetRenderer();
      vtkOpenVRRenderWindow* renWin = static_cast<vtkOpenVRRenderWindow*>(ren->GetVTKWindow());
      this->Internal->SendMessage(
        "P", index, renWin->GetPhysicalTranslation(), renWin->GetPhysicalViewDirection());
    }
    this->CurrentLocation = index;
  }
}

void vtkPVOpenVRCollaborationClient::SetHelper(vtkPVOpenVRHelper* val)
{
  this->Internal->SetHelper(val);
}

#else
void vtkPVOpenVRCollaborationClient::SetCollabHost(std::string const&)
{
}
void vtkPVOpenVRCollaborationClient::SetCollabSession(std::string const&)
{
}
void vtkPVOpenVRCollaborationClient::SetCollabName(std::string const&)
{
}
void vtkPVOpenVRCollaborationClient::SetCollabPort(int)
{
}
void vtkPVOpenVRCollaborationClient::SetLogCallback(
  std::function<void(std::string const&, void*)>, void*)
{
}
void vtkPVOpenVRCollaborationClient::GoToSavedLocation(int)
{
}
void vtkPVOpenVRCollaborationClient::SetHelper(vtkPVOpenVRHelper*)
{
}
#endif
