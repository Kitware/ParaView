/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This class is either a simple subclass of mvCollaborationClient
// if collaboration is enabled or an empty class if not
// It provides a consistent API so that other classses can use it
// without concern if collaboration is implemented or not.
//
#ifndef vtkPVOpenVRCollaborationClient_h
#define vtkPVOpenVRCollaborationClient_h

#include "vtkEventData.h"
#include "vtkObject.h"
#include <functional> // for method
#include <set>        // for ivar
#include <vector>     // for sig

class vtkBoxWidget2;
class vtkImplicitPlaneWidget2;
class vtkOpenGLRenderer;
class vtkPVOpenVRCollaborationClientInternal;
class vtkPVOpenVRHelper;
class vtkOpenVRCameraPose;
class vtkOpenVRModel;

class vtkPVOpenVRCollaborationClient : public vtkObject
{
public:
  static vtkPVOpenVRCollaborationClient* New();
  vtkTypeMacro(vtkPVOpenVRCollaborationClient, vtkObject);

  void SetHelper(vtkPVOpenVRHelper*);
  bool Connect(vtkOpenGLRenderer* ren);
  bool Disconnect();

  // call frequently to handle messages
  void Render();

  bool SupportsCollaboration();

  void SetCollabHost(std::string const& val);
  void SetCollabSession(std::string const& val);
  void SetCollabName(std::string const& val);
  void SetCollabPort(int val);

  void GoToSavedLocation(int pos);
  void SetCurrentLocation(int val) { this->CurrentLocation = val; }
  void SetLogCallback(std::function<void(std::string const& data, void* cd)> cb, void* clientData);
  void GoToPose(vtkOpenVRCameraPose const& pose, double* collabTrans, double* collabDir);

  void RemoveAllCropPlanes();
  void UpdateCropPlane(size_t i, vtkImplicitPlaneWidget2*);

  void RemoveAllThickCrops();
  void UpdateThickCrop(size_t i, vtkBoxWidget2*);

  void UpdateRay(vtkOpenVRModel*, vtkEventDataDevice);

  void ShowBillboard(std::vector<std::string> const& vals);
  void HideBillboard();

  void AddPointToSource(std::string const& name, double const* pt);
  void ClearPointSource();

protected:
  vtkPVOpenVRCollaborationClient();
  ~vtkPVOpenVRCollaborationClient();
  vtkPVOpenVRCollaborationClientInternal* Internal;
  int CurrentLocation;

private:
  vtkPVOpenVRCollaborationClient(const vtkPVOpenVRCollaborationClient&) = delete;
  void operator=(const vtkPVOpenVRCollaborationClient&) = delete;
};

#endif
