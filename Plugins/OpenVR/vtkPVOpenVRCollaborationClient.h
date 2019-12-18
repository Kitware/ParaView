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

class vtkImplicitPlaneWidget2;
class vtkOpenVRRenderer;
class vtkPVOpenVRCollaborationClientInternal;
class vtkPVOpenVRHelper;
class vtkOpenVRModel;

class vtkPVOpenVRCollaborationClient : public vtkObject
{
public:
  static vtkPVOpenVRCollaborationClient* New();
  vtkTypeMacro(vtkPVOpenVRCollaborationClient, vtkObject);

  void SetHelper(vtkPVOpenVRHelper*);
  bool Connect(vtkOpenVRRenderer* ren);
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

  void RemoveAllCropPlanes();
  void UpdateCropPlanes(std::set<vtkImplicitPlaneWidget2*> const&);

  void UpdateRay(vtkOpenVRModel*, vtkEventDataDevice);

  void ShowBillboard(std::vector<std::string> const& vals);

  void AddPointToSource(double const* pt);
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
