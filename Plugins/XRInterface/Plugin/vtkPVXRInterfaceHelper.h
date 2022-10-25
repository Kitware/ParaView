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
 * @class   vtkPVXRInterfaceHelper
 * @brief   support for connecting PV and OpenVR/OpenXR
 *
 * This class handles most of the non GUI related methods
 * for adding OpenVR/OpenXR support to ParaView. It is instantiated
 * by the pqXRInterfaceDockPanel.
 */

#ifndef vtkPVXRInterfaceHelper_h
#define vtkPVXRInterfaceHelper_h

#include "vtkRenderingOpenGLConfigure.h"

#if defined(VTK_USE_X)
// There are compile errors in vtkPVXRInterfaceHelper.cxx if Qt, X, and glew
// are not included here and in just this order.  We have to prevent
// clang-format from "fixing" this for us or compilation will fail.
// clang-format off
#include "vtk_glew.h"
#include "QVTKOpenGLWindow.h"
#include <qdir.h>
#include <qurl.h>
// clang-format on
#endif

#include "vtkNew.h" // for ivars
#include "vtkObject.h"
#include "vtkVRCamera.h"          // for visibility of inner "Pose" class
#include "vtkVRInteractorStyle.h" // for visibility of enum "MovementStyle"

#include <array>  // for method sig
#include <map>    // for ivar
#include <vector> // for ivar

class pqXRInterfaceControls;
class QVTKOpenGLWindow;
class vtkCallbackCommand;
class vtkEventData;
class vtkOpenGLCamera;
class vtkOpenGLRenderer;
class vtkOpenGLRenderWindow;
class vtkXRInterfacePolyfill;
class vtkPropCollection;
class vtkPVXRInterfaceCollaborationClient;
class vtkPVXRInterfaceExporter;
class vtkPVXRInterfaceWidgets;
class vtkPVRenderView;
class vtkPVXMLElement;
class vtkQWidgetWidget;
class vtkVRRenderWindow;
class vtkVRRenderer;
class vtkRenderWindowInteractor;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMViewProxy;
class vtkTransform;

// helper class to store information per location
class vtkPVXRInterfaceHelperLocation
{
public:
  vtkPVXRInterfaceHelperLocation();
  ~vtkPVXRInterfaceHelperLocation();
  int NavigationPanelVisibility;
  std::vector<std::pair<std::array<double, 3>, std::array<double, 3>>> CropPlaneStates;
  std::vector<std::array<double, 16>> ThickCropStates;
  std::map<vtkSMProxy*, bool> Visibility;
  vtkVRCamera::Pose* Pose;
};

class vtkPVXRInterfaceHelper : public vtkObject
{
public:
  static vtkPVXRInterfaceHelper* New();
  vtkTypeMacro(vtkPVXRInterfaceHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Re-initializes the priority queue using the amr structure given to the most
   * recent call to Initialize().
   */
  virtual void SendToXR(vtkSMViewProxy* view);

  /**
   * Instead of sending to VR hardware, attach to the current
   * view and use that for display.
   */
  virtual void AttachToCurrentView(vtkSMViewProxy* view);

  /**
   * Display a window showing a stabilized view of what the headset is showing
   * Useful for screen sharing/presentations/video
   */
  void ShowXRView();

  // called when a view is removed from PV
  void ViewRemoved(vtkSMViewProxy* view);

  // if in VR close out the event loop
  void Quit();

  // reset all prop positions
  void ResetPositions();

  // if running update the props to the current props
  // on the View
  void UpdateProps();

  // use multisampling
  vtkSetMacro(MultiSample, bool);
  vtkGetMacro(MultiSample, bool);

  // Use OpenXR backend instead of OpenVR
  void SetUseOpenXR(bool useOpenXr);
  vtkGetMacro(UseOpenXR, bool);

  // hide base stations
  virtual void SetBaseStationVisibility(bool);
  vtkGetMacro(BaseStationVisibility, bool);

  // set the initial thickness in world coordinates for
  // thick crop planes. 0 indicates automatic
  // setting. It defaults to 0
  void SetDefaultCropThickness(double);
  double GetDefaultCropThickness();

  // Save/Load the state for VR
  void LoadState(vtkPVXMLElement*, vtkSMProxyLocator*);
  void SaveState(vtkPVXMLElement*);

  // export the data for each saved location
  // as a skybox
  void ExportLocationsAsSkyboxes(vtkSMViewProxy* view);

  // export the data for each saved location
  // in a form mineview can load. Bacially
  // as imple XML format with the surface geometry
  // stored as vtp files.
  void ExportLocationsAsView(vtkSMViewProxy* view);

  // support for collaboration. The collaboration client
  // will always be set even when collaboration is not
  // enabled.
  vtkPVXRInterfaceCollaborationClient* GetCollaborationClient()
  {
    return this->CollaborationClient;
  }
  bool CollaborationConnect();
  bool CollaborationDisconnect();
  void GoToSavedLocation(int, double*, double*);

  // bring other collaborators to you
  void ComeToMe();
  void collabGoToPose(vtkVRCamera::Pose* pose, double* collabTrans, double* collabDir);

  // are we currently in VR
  bool InVR() { return this->Interactor != nullptr; }

  //@{
  /**
   * Add/remove crop planes and thick crops
   */
  void AddACropPlane(double* origin, double* normal);
  void collabRemoveAllCropPlanes();
  void collabUpdateCropPlane(int count, double* origin, double* normal);
  void AddAThickCrop(vtkTransform* t);
  void collabRemoveAllThickCrops();
  void collabUpdateThickCrop(int count, double* matrix);
  void SetCropSnapping(int val);
  void RemoveAllCropPlanesAndThickCrops();
  //@}

  // show the billboard with the provided text
  void ShowBillboard(std::string const& text, bool updatePosition, std::string const& tfile);
  void HideBillboard();

  // add a point to the currently selected source in PV
  // if it accepts points
  void AddPointToSource(double const* pnt);
  void collabAddPointToSource(std::string const& name, double const* pnt);

  //@{
  // set/show the pqOpenVRControls GUI elements
  void SetXRInterfaceControls(pqXRInterfaceControls* val) { this->XRInterfaceControls = val; }
  void ToggleShowControls();
  //@}

  // additional widgets in VR
  void SetDrawControls(bool);
  void SetShowNavigationPanel(bool);
  void TakeMeasurement();
  void RemoveMeasurement();

  // set what the right trigger will do when pressed
  void SetRightTriggerMode(std::string const& mode);

  /**
   * Set the movement style of the interactor style.
   */
  void SetMovementStyle(vtkVRInteractorStyle::MovementStyle style);

  vtkGetObjectMacro(Renderer, vtkOpenGLRenderer);

  /**
   * Set the physical up direction of the render window.
   */
  void SetViewUp(const std::string& axis);

  void SaveCameraPose(int loc);
  void LoadCameraPose(int loc);
  void SetScaleFactor(float val);
  void SetMotionFactor(float val);

  void SetHoverPick(bool);

  // allow the user to edit a scalar field
  // in VR
  void SetEditableField(std::string);
  std::string GetEditableField();

  void SetEditableFieldValue(std::string name);

  void LoadLocationState(int slot);

  vtkGetObjectMacro(AddedProps, vtkPropCollection);

  vtkGetObjectMacro(SMView, vtkSMViewProxy);

  vtkSetMacro(NeedStillRender, bool);

  // forward to widgets helper
  vtkGetObjectMacro(Widgets, vtkPVXRInterfaceWidgets);

  // get polyfill
  vtkGetObjectMacro(XRInterfacePolyfill, vtkXRInterfacePolyfill);

protected:
  vtkPVXRInterfaceHelper();
  ~vtkPVXRInterfaceHelper();

  vtkPVXRInterfaceCollaborationClient* CollaborationClient;

  vtkQWidgetWidget* QWidgetWidget;
  pqXRInterfaceControls* XRInterfaceControls;

  // state settings that the helper loads
  bool MultiSample;
  bool BaseStationVisibility;

  std::string RightTriggerMode;

  void ApplyState();
  void RecordState();

  std::string FieldValues;

  bool InteractorEventCallback(vtkObject* object, unsigned long event, void* calldata);
  bool EventCallback(vtkObject* object, unsigned long event, void* calldata);

  void HandleDeleteEvent(vtkObject* caller);
  void UpdateBillboard(bool updatePosition);

  vtkPVRenderView* View;
  vtkSMViewProxy* SMView;
  vtkOpenGLRenderer* Renderer;
  vtkOpenGLRenderWindow* RenderWindow;
  vtkRenderWindowInteractor* Interactor;
  vtkPropCollection* AddedProps;
  vtkTimeStamp PropUpdateTime;

  vtkXRInterfacePolyfill* XRInterfacePolyfill;

  bool NeedStillRender;

  // quit the event loop
  bool Done;

  bool UseOpenXR = false;

  std::map<int, vtkVRCamera::Pose> SavedCameraPoses;
  int LastCameraPoseIndex = 0;

  void SaveLocationState(int slot);

  void SavePoseInternal(vtkVRRenderWindow* vr_rw, int slot);
  void LoadPoseInternal(vtkVRRenderWindow* vr_rw, int slot);
  void LoadNextCameraPose();

  std::map<int, vtkPVXRInterfaceHelperLocation> Locations;
  int LoadLocationValue;

  void DoOneEvent();

  QVTKOpenGLWindow* ObserverWidget = nullptr;
  vtkNew<vtkOpenGLCamera> ObserverCamera;
  void RenderXRView();

  vtkNew<vtkPVXRInterfaceExporter> Exporter;
  vtkNew<vtkPVXRInterfaceWidgets> Widgets;

  // To simulate dpad with a trackpad on OpenXR we need to
  // store the last position
  double LeftTrackPadPosition[2];

private:
  vtkPVXRInterfaceHelper(const vtkPVXRInterfaceHelper&) = delete;
  void operator=(const vtkPVXRInterfaceHelper&) = delete;
};

#endif
