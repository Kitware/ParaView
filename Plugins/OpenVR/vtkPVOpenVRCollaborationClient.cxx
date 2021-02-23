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

#include "vtkBoxRepresentation.h"
#include "vtkBoxWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRModel.h"
#include "vtkOpenVROverlayInternal.h"
#include "vtkOpenVRRay.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderer.h"
#include "vtkPVOpenVRHelper.h"
#include "vtkTransform.h"

#ifdef OPENVR_HAS_COLLABORATION
#include "mvCollaborationClient.h"
#include "vtkOpenVRPolyfill.h"
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

  vtkPVOpenVRCollaborationClientInternal()
  {
    // override the scale callback to use polyfill
    // so that desktop views look reasonable
    this->ScaleCallback = [this](
      void*) { return this->Helper->GetOpenVRPolyfill()->GetPhysicalScale(); };
  }

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
        // if we are a VR window then use the update values directly
        this->Helper->GoToSavedLocation(viewIndex, updateTranslation, updateDirection);
      }
    }
    // go to pose
    else if (type == "PO")
    {
      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        zmq::message_t update;
        int count = 0;

        this->Subscriber.recv(&update);
        memcpy(&count, update.data(), sizeof(count));

        std::vector<double> dvec;
        dvec.resize(count);
        this->Subscriber.recv(&update);
        memcpy(dvec.data(), update.data(), dvec.size() * sizeof(decltype(dvec)::value_type));

        vtkOpenVRCameraPose pose;
        auto it = dvec.begin();
        std::copy(it, it + 3, pose.Position);
        it += 3;
        std::copy(it, it + 3, pose.PhysicalViewUp);
        it += 3;
        std::copy(it, it + 3, pose.PhysicalViewDirection);
        it += 3;
        std::copy(it, it + 3, pose.ViewDirection);
        it += 3;
        std::copy(it, it + 3, pose.Translation);
        it += 3;
        pose.Distance = *it++;
        pose.MotionFactor = *it++;

        double collabTrans[3];
        std::copy(it, it + 3, collabTrans);
        it += 3;
        double collabDir[3];
        std::copy(it, it + 3, collabDir);
        it += 3;

        this->Helper->collabGoToPose(&pose, collabTrans, collabDir);
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
      std::vector<Argument> args = this->GetMessageArguments();

      std::string name;         // first arg is string name
      std::vector<double> dvec; // second arg is double[3] location
      if (args.size() != 2 || !args[0].GetString(name) || !args[1].GetDoubleVector(dvec))
      {
        mvLog("Incorrect arguments for PSA collaboration message" << std::endl);
        return;
      }

      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        double origin[3];
        origin[0] = dvec[0];
        origin[1] = dvec[1];
        origin[2] = dvec[2];
        this->Helper->collabAddPointToSource(name, origin);
      }
    }
    // crop planes
    else if (type == "RCP")
    {
      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->collabRemoveAllCropPlanes();
      }
    }
    else if (type == "UCP")
    {
      std::vector<Argument> args = this->GetMessageArguments();

      int32_t index;
      std::vector<double> origin;
      std::vector<double> normal;
      if (args.size() != 3 || !args[0].GetInt32(index) || !args[1].GetDoubleVector(origin) ||
        !args[2].GetDoubleVector(normal))
      {
        mvLog("Incorrect arguments for UCP collaboration message" << std::endl);
        return;
      }

      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->collabUpdateCropPlane(index, origin.data(), normal.data());
      }
    }
    // thick crops
    else if (type == "RTC")
    {
      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->collabRemoveAllThickCrops();
      }
    }
    else if (type == "UTC")
    {
      std::vector<Argument> args = this->GetMessageArguments();

      int32_t index;
      std::vector<double> matrix;
      if (args.size() != 2 || !args[0].GetInt32(index) || !args[1].GetDoubleVector(matrix))
      {
        mvLog("Incorrect arguments for UTC collaboration message" << std::endl);
        return;
      }

      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->collabUpdateThickCrop(index, matrix.data());
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
    else if (type == "HB")
    {
      // only want to change if it's from someone else.
      if (otherID != this->CollabID)
      {
        this->Helper->HideBillboard();
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
bool vtkPVOpenVRCollaborationClient::Connect(vtkOpenGLRenderer* ren)
{
  return this->Internal->Initialize(ren);
#else
bool vtkPVOpenVRCollaborationClient::Connect(vtkOpenGLRenderer*)
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
        vtkOpenVRRenderWindow::SafeDownCast(this->Internal->GetRenderer()->GetVTKWindow());
      if (renWin)
      {
        this->Internal->SendMessage(
          "P", index, renWin->GetPhysicalTranslation(), renWin->GetPhysicalViewDirection());
      }
    }
    this->CurrentLocation = index;
  }
}

void vtkPVOpenVRCollaborationClient::SetHelper(vtkPVOpenVRHelper* val)
{
  this->Internal->SetHelper(val);
}

void vtkPVOpenVRCollaborationClient::GoToPose(
  vtkOpenVRCameraPose const& pose, double* collabTrans, double* collabDir)
{
  // store the data as a vector
  std::vector<double> dvec;
  dvec.insert(dvec.end(), pose.Position, pose.Position + 3);
  dvec.insert(dvec.end(), pose.PhysicalViewUp, pose.PhysicalViewUp + 3);
  dvec.insert(dvec.end(), pose.PhysicalViewDirection, pose.PhysicalViewDirection + 3);
  dvec.insert(dvec.end(), pose.ViewDirection, pose.ViewDirection + 3);
  dvec.insert(dvec.end(), pose.Translation, pose.Translation + 3);
  dvec.push_back(pose.Distance);
  dvec.push_back(pose.MotionFactor);
  dvec.insert(dvec.end(), collabTrans, collabTrans + 3);
  dvec.insert(dvec.end(), collabDir, collabDir + 3);

  this->Internal->SendMessage("PO", dvec);
}

void vtkPVOpenVRCollaborationClient::RemoveAllCropPlanes()
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendMessage("RCP");
  }
}

void vtkPVOpenVRCollaborationClient::RemoveAllThickCrops()
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendMessage("RTC");
  }
}

void vtkPVOpenVRCollaborationClient::UpdateCropPlane(size_t i, vtkImplicitPlaneWidget2* widget)
{
  if (this->Internal->GetConnected())
  {
    vtkImplicitPlaneRepresentation* rep =
      static_cast<vtkImplicitPlaneRepresentation*>(widget->GetRepresentation());

    std::vector<mvCollaborationClient::Argument> args;
    args.resize(3);
    args[0].SetInt32(static_cast<int32_t>(i));
    args[1].SetDoubleVector(rep->GetOrigin(), 3);
    args[2].SetDoubleVector(rep->GetNormal(), 3);
    this->Internal->SendMessage("UCP", args);
  }
}

void vtkPVOpenVRCollaborationClient::UpdateThickCrop(size_t i, vtkBoxWidget2* widget)
{
  if (this->Internal->GetConnected())
  {
    vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(widget->GetRepresentation());
    vtkNew<vtkTransform> t;
    rep->GetTransform(t);

    std::vector<mvCollaborationClient::Argument> args;
    args.resize(2);
    args[0].SetInt32(static_cast<int32_t>(i));
    args[1].SetDoubleVector(t->GetMatrix()->GetData(), 16);
    this->Internal->SendMessage("UTC", args);
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

void vtkPVOpenVRCollaborationClient::HideBillboard()
{
  this->Internal->SendMessage("HB");
}

void vtkPVOpenVRCollaborationClient::AddPointToSource(std::string const& name, double const* pt)
{
  if (this->Internal->GetConnected())
  {
    std::vector<mvCollaborationClient::Argument> args;
    args.resize(2);
    args[0].SetString(name);
    args[1].SetDoubleVector(pt, 3);
    this->Internal->SendMessage("PSA", args);
  }
}

void vtkPVOpenVRCollaborationClient::ClearPointSource()
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendMessage("PSC");
  }
}

#else
void vtkPVOpenVRCollaborationClient::GoToPose(vtkOpenVRCameraPose const&, double*, double*)
{
}
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
void vtkPVOpenVRCollaborationClient::RemoveAllThickCrops()
{
}
void vtkPVOpenVRCollaborationClient::UpdateCropPlane(size_t, vtkImplicitPlaneWidget2*)
{
}
void vtkPVOpenVRCollaborationClient::UpdateThickCrop(size_t, vtkBoxWidget2*)
{
}
void vtkPVOpenVRCollaborationClient::UpdateRay(vtkOpenVRModel*, vtkEventDataDevice)
{
}
void vtkPVOpenVRCollaborationClient::ShowBillboard(std::vector<std::string> const&)
{
}
void vtkPVOpenVRCollaborationClient::HideBillboard()
{
}
void vtkPVOpenVRCollaborationClient::AddPointToSource(std::string const&, double const*)
{
}
void vtkPVOpenVRCollaborationClient::ClearPointSource()
{
}
#endif
