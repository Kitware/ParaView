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
// It provides a consistent API so that other classes can use it
// without concern if collaboration is implemented or not.
//
#ifndef vtkPVXRInterfaceCollaborationClient_h
#define vtkPVXRInterfaceCollaborationClient_h

#if defined(VTK_USE_X)
// There are compile errors in vtkPVXRInterfaceCollaborationClient.cxx if Qt, X, and glew
// are not included here and in just this order.  We have to prevent clang-format from
// "fixing" this for us or compilation will fail.
// clang-format off
#include "vtk_glew.h"
#include "QVTKOpenGLWindow.h"
// clang-format on
#endif

#include "vtkEventData.h"
#include "vtkLogger.h" // for Verbosity enum
#include "vtkObject.h"
#include "vtkVRCamera.h" // For visibility of inner Pose class

#include <functional> // for method
#include <set>        // for ivar
#include <vector>     // for sig

class vtkBoxWidget2;
class vtkImplicitPlaneWidget2;
class vtkOpenGLRenderer;
class vtkPVOpenVRCollaborationClientInternal;
class vtkPVXRInterfaceHelper;
class vtkVRModel;

class vtkPVXRInterfaceCollaborationClient : public vtkObject
{
public:
  static vtkPVXRInterfaceCollaborationClient* New();
  vtkTypeMacro(vtkPVXRInterfaceCollaborationClient, vtkObject);

  void SetHelper(vtkPVXRInterfaceHelper*);
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
  void SetLogCallback(
    std::function<void(std::string const& data, vtkLogger::Verbosity verbosity)> cb);
  void GoToPose(vtkVRCamera::Pose const& pose, double* collabTrans, double* collabDir);

  void RemoveAllCropPlanes();
  void UpdateCropPlane(size_t i, vtkImplicitPlaneWidget2*);

  void RemoveAllThickCrops();
  void UpdateThickCrop(size_t i, vtkBoxWidget2*);

  void UpdateRay(vtkVRModel*, vtkEventDataDevice);

  void ShowBillboard(std::vector<std::string> const& vals);
  void HideBillboard();

  void AddPointToSource(std::string const& name, double const* pt);
  void ClearPointSource();

protected:
  vtkPVXRInterfaceCollaborationClient();
  ~vtkPVXRInterfaceCollaborationClient();
  vtkPVOpenVRCollaborationClientInternal* Internal;
  int CurrentLocation;

private:
  vtkPVXRInterfaceCollaborationClient(const vtkPVXRInterfaceCollaborationClient&) = delete;
  void operator=(const vtkPVXRInterfaceCollaborationClient&) = delete;
};

#endif
