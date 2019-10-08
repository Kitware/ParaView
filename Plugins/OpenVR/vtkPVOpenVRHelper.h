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
 * This class handles most of the non GUI related methods
 * for adding OpenVR support to ParaView. It is instantiated
 * by the pqOpenVRDockPanel.
*/

#ifndef vtkPVOpenVRHelper_h
#define vtkPVOpenVRHelper_h

#include "vtkNew.h" // for ivars
#include "vtkObject.h"
#include <array>  // for method sig
#include <map>    // for ivar
#include <set>    // for ivar
#include <vector> // for ivar

class pqOpenVRControls;
class vtkActor;
class vtkBoxWidget2;
class vtkCallbackCommand;
class vtkDataSet;
class vtkDistanceWidget;
class vtkEventData;
class vtkImplicitPlaneWidget2;
class vtkOpenVRCameraPose;
class vtkOpenVRInteractorStyle;
class vtkOpenVRPanelWidget;
class vtkOpenVRPanelRepresentation;
class vtkOpenVRRenderWindowInteractor;
class vtkOpenVRRenderer;
class vtkOpenVRRenderWindow;
class vtkPlaneSource;
class vtkProp;
class vtkPropCollection;
class vtkPVOpenVRCollaborationClient;
class vtkPVDataRepresentation;
class vtkPVRenderView;
class vtkPVXMLElement;
class vtkQWidgetWidget;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMViewProxy;
class vtkTextActor3D;
class vtkTexture;
class vtkTransform;

// helper class to store information per location
class vtkPVOpenVRHelperLocation
{
public:
  vtkPVOpenVRHelperLocation();
  ~vtkPVOpenVRHelperLocation();
  int NavigationPanelVisibility;
  std::vector<std::pair<std::array<double, 3>, std::array<double, 3> > > CropPlaneStates;
  std::vector<std::array<double, 16> > ThickCropStates;
  std::map<vtkSMProxy*, bool> Visibility;
  vtkOpenVRCameraPose* Pose;
};

class vtkPVOpenVRHelper : public vtkObject
{
public:
  static vtkPVOpenVRHelper* New();
  vtkTypeMacro(vtkPVOpenVRHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Re-initializes the priority queue using the amr structure given to the most
   * recent call to Initialize().
   */
  virtual void SendToOpenVR(vtkSMViewProxy* view);

  // called when a view is removed from PV
  void ViewRemoved(vtkSMViewProxy* view);

  // if in VR close out the event loop
  void Quit();

  // if running update the props to the current props
  // on the View
  void UpdateProps();

  // use multisampling
  vtkSetMacro(MultiSample, bool);
  vtkGetMacro(MultiSample, bool);

  // set the initial thickness in world coordinates for
  // thick crop planes. 0 indicates automatic
  // setting. It defaults to 0
  vtkSetMacro(DefaultCropThickness, double);
  vtkGetMacro(DefaultCropThickness, double);

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
  vtkPVOpenVRCollaborationClient* GetCollaborationClient() { return this->CollaborationClient; }
  bool CollaborationConnect();
  bool CollaborationDisconnect();
  void GoToSavedLocation(int, double*, double*);

  // are we currently in VR
  bool InVR() { return this->Interactor != nullptr; }

  //@{
  /**
   * Add/remove crop planes and thick crops
   */
  void AddACropPlane(double* origin, double* normal);
  void RemoveAllCropPlanes();
  void AddAThickCrop(vtkTransform* t);
  void SetNumberOfCropPlanes(int);
  void UpdateCropPlane(int count, double* origin, double* normal);
  void SetCropSnapping(int val);
  //@}

  // show the billboard with the provided text
  void ShowBillboard(std::string const& text, bool updatePosition, vtkTexture* t = nullptr);

  // add a point to the currently selected source in PV
  // if it accepts points
  void AddPointToSource(double const* pnt);

  //@{
  // set/show the pqOpenVRControls GUI elements
  void SetOpenVRControls(pqOpenVRControls* val) { this->OpenVRControls = val; }
  void ToggleShowControls();
  //@}

  // additional widgets in VR
  void SetDrawControls(bool);
  void SetShowNavigationPanel(bool);
  void TakeMeasurement();
  void RemoveMeasurement();

  vtkGetObjectMacro(Style, vtkOpenVRInteractorStyle);

  // set what the right trigger will do when pressed
  void SetRightTriggerMode(std::string const& mode);

protected:
  vtkPVOpenVRHelper();
  ~vtkPVOpenVRHelper();

  vtkPVOpenVRCollaborationClient* CollaborationClient;

  vtkQWidgetWidget* QWidgetWidget;
  pqOpenVRControls* OpenVRControls;

  // state settings that the helper loads
  bool CropSnapping;
  bool MultiSample;
  double DefaultCropThickness;

  std::string RightTriggerMode;

  void ApplyState();
  void RecordState();

  vtkDataSet* LastPickedDataSet;
  vtkIdType LastPickedCellId;
  vtkPVDataRepresentation* LastPickedRepresentation;
  vtkProp* LastPickedProp;
  vtkDataSet* PreviousPickedDataSet;
  vtkIdType PreviousPickedCellId;
  vtkPVDataRepresentation* PreviousPickedRepresentation;
  std::vector<vtkIdType> SelectedCells;

  vtkNew<vtkOpenVRPanelWidget> NavWidget;
  vtkNew<vtkOpenVRPanelRepresentation> NavRepresentation;
  vtkNew<vtkTextActor3D> TextActor3D;
  vtkNew<vtkPlaneSource> ImagePlane;
  vtkNew<vtkActor> ImageActor;

  std::set<vtkImplicitPlaneWidget2*> CropPlanes;
  std::set<vtkBoxWidget2*> ThickCrops;

  vtkOpenVRInteractorStyle* Style;
  vtkOpenVRRenderWindowInteractor* Interactor;
  bool InteractorEventCallback(vtkObject* object, unsigned long event, void* calldata);
  bool EventCallback(vtkObject* object, unsigned long event, void* calldata);

  void HideBillboard();
  void HandlePickEvent(vtkObject* caller, void* calldata);
  void MoveToNextImage();
  void MoveToNextCell();
  void UpdateBillboard(bool updatePosition);

  vtkDistanceWidget* DistanceWidget;
  vtkPVRenderView* View;
  vtkSMViewProxy* SMView;
  vtkOpenVRRenderer* Renderer;
  vtkOpenVRRenderWindow* RenderWindow;
  vtkPropCollection* AddedProps;
  vtkTimeStamp PropUpdateTime;

  bool NeedStillRender;

  void SaveLocationState(int slot);
  void LoadLocationState();
  int LoadLocationValue;

  std::map<int, vtkPVOpenVRHelperLocation> Locations;

  vtkEventData* LastEventData;

private:
  vtkPVOpenVRHelper(const vtkPVOpenVRHelper&) = delete;
  void operator=(const vtkPVOpenVRHelper&) = delete;
};

#endif
