/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVOpenVRHelper
 * @brief   support for connecting PV and OpenVR
 *
*/

#ifndef vtkPVOpenVRHelper_h
#define vtkPVOpenVRHelper_h

#include "vtkNew.h" // for ivars
#include "vtkObject.h"
#include <map> // for ivar
#include <set> // for ivar

class vtkCallbackCommand;
class vtkDistanceWidget;
class vtkImplicitPlaneWidget2;
class vtkOpenVRInteractorStyle;
class vtkOpenVRMenuWidget;
class vtkOpenVRMenuRepresentation;
class vtkOpenVRPanelWidget;
class vtkOpenVRPanelRepresentation;
class vtkOpenVRRenderWindowInteractor;
class vtkOpenVRRenderer;
class vtkOpenVRRenderWindow;
class vtkPropCollection;
class vtkPVRenderView;
class vtkSMProxy;
class vtkSMViewProxy;

class vtkPVOpenVRHelper : public vtkObject
{
public:
  static vtkPVOpenVRHelper* New();
  vtkTypeMacro(vtkPVOpenVRHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Re-initializes the priority queue using the amr structure given to the most
   * recent call to Initialize().
   */
  void SendToOpenVR(vtkSMViewProxy* view);

  // called when a view is removed from PV
  void ViewRemoved(vtkSMViewProxy* view);

  std::string GetNextXML() { return this->NextXML; }

  void SetNextXML(std::string& pos) { this->NextXML = pos; }

  // if in VR close out the event loop
  void Quit();

  // if running update the props to the current props
  // on the View
  void UpdateProps();

protected:
  vtkPVOpenVRHelper();
  ~vtkPVOpenVRHelper();

  std::string NextXML;

  void GetScalars();
  void BuildScalarMenu();
  void SelectScalar(std::string name);

  vtkNew<vtkOpenVRPanelWidget> NavWidget;
  vtkNew<vtkOpenVRPanelRepresentation> NavRepresentation;
  void ToggleNavigationPanel();
  unsigned long NavigationTag;

  void AddACropPlane();
  void RemoveAllCropPlanes();
  std::set<vtkImplicitPlaneWidget2*> CropPlanes;

  vtkOpenVRInteractorStyle* Style;
  vtkOpenVRRenderWindowInteractor* Interactor;

  std::map<std::string, int> ScalarMap;
  vtkNew<vtkOpenVRMenuWidget> ScalarMenu;
  vtkNew<vtkOpenVRMenuRepresentation> ScalarMenuRepresentation;
  vtkCallbackCommand* EventCommand;
  static void EventCallback(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  void HandleMenuEvent(vtkOpenVRMenuWidget* menu, vtkObject* object, unsigned long event,
    void* clientdata, void* calldata);
  void HandleInteractorEvent(vtkOpenVRRenderWindowInteractor* iren, vtkObject* object,
    unsigned long event, void* clientdata, void* calldata);

  vtkDistanceWidget* DistanceWidget;
  vtkPVRenderView* View;
  vtkSMViewProxy* SMView;
  vtkOpenVRRenderer* Renderer;
  vtkOpenVRRenderWindow* RenderWindow;
  vtkPropCollection* AddedProps;
  vtkTimeStamp PropUpdateTime;

private:
  vtkPVOpenVRHelper(const vtkPVOpenVRHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVOpenVRHelper&) VTK_DELETE_FUNCTION;
};

#endif
