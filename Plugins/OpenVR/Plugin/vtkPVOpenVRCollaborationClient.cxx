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
#include "vtkOpenVROverlayInternal.h"
#include "vtkPVOpenVRHelper.h"
#include "vtkTransform.h"
#include "vtkVRModel.h"
#include "vtkVRRay.h"
#include "vtkVRRenderWindow.h"
#include <sstream>

#ifdef OPENVR_HAS_COLLABORATION
#include "vtkOpenGLRenderer.h"
#include "vtkOpenVRPolyfill.h"
#include "vtkVRCollaborationClient.h"

class vtkPVOpenVRCollaborationClientInternal : public vtkVRCollaborationClient
{
public:
  static vtkPVOpenVRCollaborationClientInternal* New();
  vtkTypeMacro(vtkPVOpenVRCollaborationClientInternal, vtkVRCollaborationClient);
  void SetHelper(vtkPVOpenVRHelper* l) { this->Helper = l; }

  vtkPVOpenVRCollaborationClientInternal()
  {
    // override the scale callback to use polyfill
    // so that desktop views look reasonable
    this->ScaleCallback = [this]() {
      return this->Helper->GetOpenVRPolyfill()->GetPhysicalScale();
    };
  }

protected:
  void HandleBroadcastMessage(std::string const& otherID, std::string const& type) override
  {
    if (type == "P")
    {
      // change pose.
      std::vector<Argument> args = this->GetMessageArguments();

      int viewIndex = 0;
      std::vector<double> uTrans;
      std::vector<double> uDir;
      if (args.size() != 3 || !args[0].GetInt32(viewIndex) || !args[1].GetDoubleVector(uTrans) ||
        !args[2].GetDoubleVector(uDir))
      {
        this->Log(vtkLogger::VERBOSITY_ERROR,
          "Incorrect arguments for P (avatar pose) collaboration message");
        return;
      }

      double updateTranslation[3] = { 0 };
      double updateDirection[3] = { 0.0 };

      memcpy(&updateTranslation[0], uTrans.data(), sizeof(updateTranslation));
      memcpy(&updateDirection[0], uDir.data(), sizeof(updateDirection));

      std::ostringstream ss;
      ss << "Collab " << otherID << ", Pose: " << viewIndex << " " << updateTranslation[0] << " "
         << updateTranslation[1] << " " << updateTranslation[2] << " " << std::endl;
      this->Log(vtkLogger::VERBOSITY_INFO, ss.str());

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
        std::vector<Argument> args = this->GetMessageArguments();

        std::vector<double> dvec;
        if (args.size() != 1 || !args[0].GetDoubleVector(dvec))
        {
          this->Log(vtkLogger::VERBOSITY_ERROR, "Incorrect arguments for PO collaboration message");
          return;
        }

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
        this->Log(vtkLogger::VERBOSITY_ERROR, "Incorrect arguments for PSA collaboration message");
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
        this->Log(vtkLogger::VERBOSITY_ERROR, "Incorrect arguments for UCP collaboration message");
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
        this->Log(vtkLogger::VERBOSITY_ERROR, "Incorrect arguments for UTC collaboration message");
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
      std::vector<Argument> args = this->GetMessageArguments();

      std::vector<std::string> vals;
      if (args.size() != 1 || !args[0].GetStringVector(vals) || vals.size() < 2)
      {
        this->Log(vtkLogger::VERBOSITY_ERROR,
          "Incorrect arguments for SB (show billboard) collaboration message");
        return;
      }

      std::string text = vals[0];
      std::string update = vals[1];
      std::string file = "";
      if (vals.size() > 2)
      {
        file = vals[2];
      }

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
      this->vtkVRCollaborationClient::HandleBroadcastMessage(otherID, type);
    }
  }

  vtkPVOpenVRHelper* Helper;
};
#else
class vtkPVOpenVRCollaborationClientInternal : public vtkObject
{
public:
  static vtkPVOpenVRCollaborationClientInternal* New();
  vtkTypeMacro(vtkPVOpenVRCollaborationClientInternal, vtkObject);
};
#endif

vtkStandardNewMacro(vtkPVOpenVRCollaborationClientInternal);
vtkStandardNewMacro(vtkPVOpenVRCollaborationClient);

//----------------------------------------------------------------------------
vtkPVOpenVRCollaborationClient::vtkPVOpenVRCollaborationClient()
  : CurrentLocation(-1)
{
  this->Internal = vtkPVOpenVRCollaborationClientInternal::New();
}

//----------------------------------------------------------------------------
vtkPVOpenVRCollaborationClient::~vtkPVOpenVRCollaborationClient()
{
  this->Internal->Delete();
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
  std::function<void(std::string const& data, vtkLogger::Verbosity verbosity)> cb)
{
  this->Internal->SetLogCallback(cb);
}
void vtkPVOpenVRCollaborationClient::GoToSavedLocation(int index)
{
  // only send if we need to
  if (index != this->CurrentLocation)
  {
    if (this->Internal->GetConnected())
    {
      vtkVRRenderWindow* renWin =
        vtkVRRenderWindow::SafeDownCast(this->Internal->GetRenderer()->GetVTKWindow());
      if (renWin)
      {
        this->Internal->SendPoseMessage(
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

  std::vector<vtkVRCollaborationClient::Argument> args;
  args.resize(1);
  args[0].SetDoubleVector(dvec.data(), static_cast<uint16_t>(dvec.size()));
  this->Internal->SendAMessage("PO", args);
}

void vtkPVOpenVRCollaborationClient::RemoveAllCropPlanes()
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendAMessage("RCP");
  }
}

void vtkPVOpenVRCollaborationClient::RemoveAllThickCrops()
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendAMessage("RTC");
  }
}

void vtkPVOpenVRCollaborationClient::UpdateCropPlane(size_t i, vtkImplicitPlaneWidget2* widget)
{
  if (this->Internal->GetConnected())
  {
    vtkImplicitPlaneRepresentation* rep =
      static_cast<vtkImplicitPlaneRepresentation*>(widget->GetRepresentation());

    std::vector<vtkVRCollaborationClient::Argument> args;
    args.resize(3);
    args[0].SetInt32(static_cast<int32_t>(i));
    args[1].SetDoubleVector(rep->GetOrigin(), 3);
    args[2].SetDoubleVector(rep->GetNormal(), 3);
    this->Internal->SendAMessage("UCP", args);
  }
}

void vtkPVOpenVRCollaborationClient::UpdateThickCrop(size_t i, vtkBoxWidget2* widget)
{
  if (this->Internal->GetConnected())
  {
    vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(widget->GetRepresentation());
    vtkNew<vtkTransform> t;
    rep->GetTransform(t);

    std::vector<vtkVRCollaborationClient::Argument> args;
    args.resize(2);
    args[0].SetInt32(static_cast<int32_t>(i));
    args[1].SetDoubleVector(t->GetMatrix()->GetData(), 16);
    this->Internal->SendAMessage("UTC", args);
  }
}

void vtkPVOpenVRCollaborationClient::UpdateRay(vtkVRModel* model, vtkEventDataDevice dev)
{
  if (this->Internal->GetConnected())
  {
    std::vector<vtkVRCollaborationClient::Argument> args;
    args.resize(1);
    args[0].SetInt32(static_cast<int32_t>(dev));
    this->Internal->SendAMessage(model->GetRay()->GetShow() ? "SR" : "HR", args);
  }
}

void vtkPVOpenVRCollaborationClient::ShowBillboard(std::vector<std::string> const& vals)
{
  std::vector<vtkVRCollaborationClient::Argument> args;
  args.resize(1);
  args[0].SetStringVector(vals);
  this->Internal->SendAMessage("SB", args);
}

void vtkPVOpenVRCollaborationClient::HideBillboard()
{
  this->Internal->SendAMessage("HB");
}

void vtkPVOpenVRCollaborationClient::AddPointToSource(std::string const& name, double const* pt)
{
  if (this->Internal->GetConnected())
  {
    std::vector<vtkVRCollaborationClient::Argument> args;
    args.resize(2);
    args[0].SetString(name);
    args[1].SetDoubleVector(pt, 3);
    this->Internal->SendAMessage("PSA", args);
  }
}

void vtkPVOpenVRCollaborationClient::ClearPointSource()
{
  if (this->Internal->GetConnected())
  {
    this->Internal->SendAMessage("PSC");
  }
}

#else
void vtkPVOpenVRCollaborationClient::GoToPose(vtkOpenVRCameraPose const&, double*, double*) {}
void vtkPVOpenVRCollaborationClient::SetCollabHost(std::string const&) {}
void vtkPVOpenVRCollaborationClient::SetCollabSession(std::string const&) {}
void vtkPVOpenVRCollaborationClient::SetCollabName(std::string const&) {}
void vtkPVOpenVRCollaborationClient::SetCollabPort(int) {}
void vtkPVOpenVRCollaborationClient::SetLogCallback(
  std::function<void(std::string const& data, vtkLogger::Verbosity verbosity)>)
{
}
void vtkPVOpenVRCollaborationClient::GoToSavedLocation(int) {}
void vtkPVOpenVRCollaborationClient::SetHelper(vtkPVOpenVRHelper*) {}
void vtkPVOpenVRCollaborationClient::RemoveAllCropPlanes() {}
void vtkPVOpenVRCollaborationClient::RemoveAllThickCrops() {}
void vtkPVOpenVRCollaborationClient::UpdateCropPlane(size_t, vtkImplicitPlaneWidget2*) {}
void vtkPVOpenVRCollaborationClient::UpdateThickCrop(size_t, vtkBoxWidget2*) {}
void vtkPVOpenVRCollaborationClient::UpdateRay(vtkVRModel*, vtkEventDataDevice) {}
void vtkPVOpenVRCollaborationClient::ShowBillboard(std::vector<std::string> const&) {}
void vtkPVOpenVRCollaborationClient::HideBillboard() {}
void vtkPVOpenVRCollaborationClient::AddPointToSource(std::string const&, double const*) {}
void vtkPVOpenVRCollaborationClient::ClearPointSource() {}
#endif
