// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
#include <qdir.h>
#include <qurl.h>
// clang-format on
#endif

#include "vtkNew.h" // for ivars
#include "vtkObject.h"
#include "vtkVRCamera.h"       // for visibility of enum Pose
#include "vtkOpenGLRenderer.h" // TODO should not be needed

#include <array>  // for method sig
#include <map>    // for map
#include <memory> // for unique_ptr
#include <vector> // for vector

#include <QStringList>

class pqXRInterfaceControls;
class vtkOpenGLRenderer;
class vtkPVXMLElement;
class vtkPVXRInterfaceCollaborationClient;
class vtkPVXRInterfaceWidgets;
class vtkPropCollection;
class vtkSMProxyLocator;
class vtkSMProxy;
class vtkSMViewProxy;
class vtkTransform;
class vtkVRRenderWindow;
class vtkVRRenderer;
class vtkXRInterfacePolyfill;

// helper class to store information per location
struct vtkPVXRInterfaceHelperLocation
{
  vtkPVXRInterfaceHelperLocation()
    : Pose{ new vtkVRCamera::Pose{} }
  {
  }

  int NavigationPanelVisibility;
  std::vector<std::pair<std::array<double, 3>, std::array<double, 3>>> CropPlaneStates;
  std::vector<std::array<double, 16>> ThickCropStates;
  std::map<vtkSMProxy*, bool> Visibility;
  std::unique_ptr<vtkVRCamera::Pose> Pose;
};

class vtkPVXRInterfaceHelper : public vtkObject
{
public:
  static vtkPVXRInterfaceHelper* New();
  vtkTypeMacro(vtkPVXRInterfaceHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actions produced when pushing the right controller trigger.
   */
  enum RightTriggerAction
  {
    ADD_POINT_TO_SOURCE = 0,
    GRAB,
    PICK,
    INTERACTIVE_CROP,
    PROBE,
    TELEPORTATION
  };

  /**
   * Get OpenXR Runtime version.
   * This function is affected by `UseOpenXR` and `UseOpenXRRemoting`
   * If `UseOpenXR` is `false`, this function return an empty string.
   * `UseOpenXRRemoting` is used to determine OpenXR connection strategy.
   */
  std::string GetOpenXRRuntimeVersionString() const;

  /**
   * Create an XR View (e.g. displayed in an HMD) with the actors
   * present in the current Render View.
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

  /**
   * Called when a view is removed.
   */
  void ViewRemoved(vtkSMViewProxy* view);

  /**
   * Stop the event loop.
   */
  void Quit();

  /**
   * Reset camera with the center of the dataset bounding box as focal point.
   */
  void ResetCamera();

  /**
   * Reset all prop positions.
   */
  void ResetPositions();

  /**
   * If running, update the props to the current props in the view.
   */
  void UpdateProps();

  /**
   * Save VR state.
   */
  void SaveState(vtkPVXMLElement*);

  /**
   * Load VR state.
   */
  void LoadState(vtkPVXMLElement*, vtkSMProxyLocator*);

  /**
   * Export the data for each saved location as a skybox.
   */
  void ExportLocationsAsSkyboxes(vtkSMViewProxy* view);

  /**
   * Export saved locations as .vtp files in a folder called pv-view.
   * This option is meant to be used for Mineview.
   */
  void ExportLocationsAsView(vtkSMViewProxy* view);

  ///@{
  /**
   * Support for collaboration. The collaboration client
   * will always be set even when collaboration is not enabled.
   */
  vtkPVXRInterfaceCollaborationClient* GetCollaborationClient()
  {
    return this->CollaborationClient;
  }
  bool CollaborationConnect();
  bool CollaborationDisconnect();
  void GoToSavedLocation(std::size_t, double*, double*);
  ///@}

  ///@{
  /**
   * Bring other collaborators to the user.
   */
  void ComeToMe();
  void collabGoToPose(vtkVRCamera::Pose* pose, double* collabTrans, double* collabDir);
  ///@}

  /**
   * Return true if XR is currently running.
   */
  bool InVR();

  ///@{
  /**
   * Add/remove crop planes and thick crops
   */
  void AddACropPlane(double* origin, double* normal);
  void collabRemoveAllCropPlanes();
  void collabUpdateCropPlane(int count, double* origin, double* normal);
  void AddAThickCrop(vtkTransform* t);
  void collabRemoveAllThickCrops();
  void collabUpdateThickCrop(int count, double* matrix);
  void SetCropSnapping(bool val);
  void RemoveAllCropPlanesAndThickCrops();
  void ShowCropPlanes(bool visible);
  ///@}

  ///@{
  /**
   * Show/hide a billboard with the provided text.
   */
  void ShowBillboard(std::string const& text, bool updatePosition, std::string const& tfile);
  void HideBillboard();
  ///@}

  ///@{
  /**
   * Add a point to the currently selected source if it accepts points.
   */
  void AddPointToSource(double const* pnt);
  void collabAddPointToSource(std::string const& name, double const* pnt);
  ///@}

  ///@{
  /**
   * Set/show the pqOpenVRControls GUI elements.
   */
  void SetXRInterfaceControls(pqXRInterfaceControls* val) { this->XRInterfaceControls = val; }
  void ToggleShowControls();
  ///@}

  /**
   * Add measuring widget to the view.
   */
  void TakeMeasurement();

  /**
   * Remove measuring widget from the view.
   */
  void RemoveMeasurement();

  ///@{
  /**
   * Save/load/remove a camera position.
   */
  QStringList GetCustomViewpointToolTips();
  std::size_t GetNextPoseIndex();
  void SaveCameraPose(std::size_t index);
  void LoadCameraPose(std::size_t index);
  void ClearCameraPoses();
  ///@}

  /**
   * Load a saved location with the given index.
   */
  void LoadLocationState(std::size_t slot);

  /**
   * Set whether to display the XR menu.
   */
  void SetDrawControls(bool);

  /**
   * Set whether to display the navigation panel.
   */
  void SetShowNavigationPanel(bool);

  /**
   * Set whether to display a white cross marker at the tip of the right controller.
   */
  void SetShowRightControllerMarker(bool visibility);

  /**
   * Set the right trigger action.
   */
  void SetRightTriggerMode(int index);

  /**
   * Set the movement style of the interactor style.
   */
  void SetMovementStyle(int index);

  /**
   * Set the physical up direction of the render window.
   */
  void SetViewUp(const std::string& axis);

  /**
   * Set the scale factor determining the speed of scaling.
   */
  void SetScaleFactor(float val);

  /**
   * Set the motion factor determining the speed of joystick-based movement.
   */
  void SetMotionFactor(float val);

  /**
   * Indicates if picking should be updated every frame. If so, the interaction
   * picker will try to pick a prop and rays will be updated accordingly.
   */
  void SetHoverPick(bool);

  /**
   * Set custom value chosen in the XR menu on the selected cell and given array.
   */
  void SetEditableFieldValue(std::string name);

  ///@{
  /**
   * Set/get whether multisampled framebuffers are used.
   * Default is false.
   */
  vtkSetMacro(MultiSample, bool);
  vtkGetMacro(MultiSample, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to use the OpenXR backend instead of OpenVR.
   * Default is false.
   */
  void SetUseOpenXR(bool useOpenXr);
  vtkGetMacro(UseOpenXR, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to use OpenXR Remoting for Hololens2.
   * Default is false.
   *
   * @note as the remoting depends on OpenXR, this feature do nothing if the
   * UseOpenXR isn't enabled.
   *
   * @warning This feature is currently experimental.
   */
  void SetUseOpenXRRemoting(bool useOpenXrRemoting);
  vtkGetMacro(UseOpenXRRemoting, bool);
  ///@}

  ///@{
  /**
   * Set/get port used to link with the correct remote devices.
   * Default is 127.0.0.1.
   *
   * Note that UseOpenXRRemoting needs to be set to true.
   */
  vtkSetMacro(RemotingAddress, std::string);
  vtkGetMacro(RemotingAddress, std::string);
  ///@}

  ///@{
  /**
   * Set/get whether to display base stations in the XR View.
   * Default is false.
   */
  virtual void SetBaseStationVisibility(bool);
  vtkGetMacro(BaseStationVisibility, bool);
  ///@}

  ///@{
  /**
   * Set/get the initial thickness for thick crop planes in world coordinates.
   * Default is 0 for automatic setting.
   */
  void SetDefaultCropThickness(double);
  double GetDefaultCropThickness();
  ///@}

  ///@{
  /**
   * Set/get name of the array to edit.
   */
  void SetEditableField(std::string name);
  std::string GetEditableField();
  ///@}

  ///@{
  /**
   * Set/get whether a high resolution render is needed.
   */
  vtkSetMacro(NeedStillRender, bool);
  vtkGetMacro(NeedStillRender, bool);
  ///@}

  /**
   * Get the renderer.
   */
  vtkGetObjectMacro(Renderer, vtkOpenGLRenderer);

  /**
   * Get the added props.
   */
  vtkGetObjectMacro(AddedProps, vtkPropCollection);

  /**
   * Get the view.
   */
  vtkGetObjectMacro(SMView, vtkSMViewProxy);

  /**
   * Get the widgets.
   */
  vtkGetObjectMacro(Widgets, vtkPVXRInterfaceWidgets);

  /**
   * Get the polyfill.
   */
  vtkGetObjectMacro(XRInterfacePolyfill, vtkXRInterfacePolyfill);

protected:
  vtkPVXRInterfaceHelper();
  ~vtkPVXRInterfaceHelper() override;

  void ApplyState();
  bool InteractorEventCallback(vtkObject* object, unsigned long event, void* calldata);
  bool EventCallback(vtkObject* object, unsigned long event, void* calldata);

  void HandleDeleteEvent(vtkObject* caller);
  void UpdateBillboard(bool updatePosition);

  void LoadNextCameraPose();

  void DoOneEvent();
  void RenderXRView();

private:
  vtkPVXRInterfaceHelper(const vtkPVXRInterfaceHelper&) = delete;
  void operator=(const vtkPVXRInterfaceHelper&) = delete;

  void SaveLocationState(std::size_t slot);

  void SavePoseInternal(vtkVRRenderWindow* vr_rw, std::size_t slot);
  void LoadPoseInternal(vtkVRRenderWindow* vr_rw, std::size_t slot);

  // state settings that the helper loads
  bool MultiSample = false;
  bool BaseStationVisibility = false;
  bool NeedStillRender = false;
  bool UseOpenXR = false;
  bool UseOpenXRRemoting = false;
  std::string RemotingAddress = "127.0.0.1";
  RightTriggerAction RightTriggerMode = vtkPVXRInterfaceHelper::PICK;

  vtkSMViewProxy* SMView = nullptr;
  pqXRInterfaceControls* XRInterfaceControls = nullptr;
  vtkSmartPointer<vtkOpenGLRenderer> Renderer;
  vtkNew<vtkPVXRInterfaceCollaborationClient> CollaborationClient;
  vtkNew<vtkPropCollection> AddedProps;
  vtkNew<vtkXRInterfacePolyfill> XRInterfacePolyfill;
  vtkNew<vtkPVXRInterfaceWidgets> Widgets;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
