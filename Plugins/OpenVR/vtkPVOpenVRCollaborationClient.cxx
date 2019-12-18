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

#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRModel.h"
#include "vtkOpenVRRay.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderer.h"
#include "vtkPVOpenVRHelper.h"

#ifdef OPENVR_HAS_COLLABORATION
#include "mvCollaborationClient.h"
#include "zhelpers.hpp"

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
    // point source
    else if (type == "PSC")
    {
      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        //        this->Helper->ClearPointSource();
      }
    }
    else if (type == "PSA")
    {
      zmq::message_t update;
      unsigned int count = 0;

      this->Subscriber.recv(&update);
      memcpy(&count, update.data(), sizeof(count));

      std::vector<double> tmp;
      tmp.resize(count);

      this->Subscriber.recv(&update);
      memcpy(&tmp[0], update.data(), count * sizeof(tmp[0]));

      // only want to change if it's from someone else.
      if (otherID != this->CollabID && count == 3)
      {
        double origin[3];
        origin[0] = tmp[0];
        origin[1] = tmp[1];
        origin[2] = tmp[2];
        this->Helper->AddPointToSource(origin);
      }
    }
    // clip planes
    else if (type == "CPC")
    {
      zmq::message_t update;
      int count = 0;

      this->Subscriber.recv(&update);
      memcpy(&count, update.data(), sizeof(count));

      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->SetNumberOfCropPlanes(count);
      }
    }
    else if (type == "CPU")
    {
      zmq::message_t update;
      int count = 0;
      double origin[3] = { 0.0 };
      double normal[3] = { 0.0 };

      this->Subscriber.recv(&update);
      memcpy(&count, update.data(), sizeof(count));
      this->Subscriber.recv(&update);
      memcpy(&origin[0], update.data(), sizeof(origin));
      this->Subscriber.recv(&update);
      memcpy(&normal[0], update.data(), sizeof(normal));

      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->UpdateCropPlane(count, origin, normal);
      }
    }
    else if (type == "SB")
    {
      std::string text = s_recv(this->Subscriber);
      std::string update = s_recv(this->Subscriber);
      std::string file = s_recv(this->Subscriber);
      std::string end = s_recv(this->Subscriber);

      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->ShowBillboard(text, update == "true", file);
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
      vtkOpenVRRenderWindow* renWin =
        static_cast<vtkOpenVRRenderWindow*>(this->Internal->GetRenderer()->GetVTKWindow());
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

void vtkPVOpenVRCollaborationClient::RemoveAllCropPlanes()
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendMessage("CPC", 0);
  }
}

void vtkPVOpenVRCollaborationClient::UpdateCropPlanes(
  std::set<vtkImplicitPlaneWidget2*> const& planes)
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendMessage("CPC", static_cast<int>(planes.size()));
  }

  int count = 0;
  for (auto const& widget : planes)
  {
    vtkImplicitPlaneRepresentation* rep =
      static_cast<vtkImplicitPlaneRepresentation*>(widget->GetRepresentation());
    double origin[3];
    double normal[3];
    rep->GetOrigin(origin);
    rep->GetNormal(normal);
    this->Internal->SendMessage("CPU", count, origin, normal);
    count++;
  }
}

void vtkPVOpenVRCollaborationClient::UpdateRay(vtkOpenVRModel* model, vtkEventDataDevice dev)
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendMessage(model->GetRay()->GetShow() ? "SR" : "HR", static_cast<int>(dev));
  }
}

void vtkPVOpenVRCollaborationClient::ShowBillboard(std::vector<std::string> const& vals)
{
  this->Internal->SendMessage("SB", vals);
}

void vtkPVOpenVRCollaborationClient::AddPointToSource(double const* pt)
{
  std::vector<double> tmp;
  tmp.push_back(pt[0]);
  tmp.push_back(pt[1]);
  tmp.push_back(pt[2]);
  this->Internal->SendMessage("PSA", tmp);
}

void vtkPVOpenVRCollaborationClient::ClearPointSource()
{
  this->Internal->SendMessage("PSC");
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
void vtkPVOpenVRCollaborationClient::RemoveAllCropPlanes()
{
}
void vtkPVOpenVRCollaborationClient::UpdateCropPlanes(std::set<vtkImplicitPlaneWidget2*> const&)
{
}
void vtkPVOpenVRCollaborationClient::UpdateRay(vtkOpenVRModel*, vtkEventDataDevice)
{
}
void vtkPVOpenVRCollaborationClient::ShowBillboard(std::vector<std::string> const&)
{
}
void vtkPVOpenVRCollaborationClient::AddPointToSource(double const*)
{
}
void vtkPVOpenVRCollaborationClient::ClearPointSource()
{
}
#endif
