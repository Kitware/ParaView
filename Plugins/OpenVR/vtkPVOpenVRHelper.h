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
#include <array>  // for method sig
#include <map>    // for ivar
#include <set>    // for ivar
#include <vector> // for ivar

class vtkBoxWidget2;
class vtkCallbackCommand;
class vtkDataSet;
class vtkDistanceWidget;
class vtkImplicitPlaneWidget2;
class vtkOpenVRCameraPose;
class vtkOpenVRInteractorStyle;
class vtkOpenVRMenuWidget;
class vtkOpenVRMenuRepresentation;
class vtkOpenVRPanelWidget;
class vtkOpenVRPanelRepresentation;
class vtkOpenVRRenderWindowInteractor;
class vtkOpenVRRenderer;
class vtkOpenVRRenderWindow;
class vtkPropCollection;
class vtkPVDataRepresentation;
class vtkPVRenderView;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMViewProxy;
class vtkTransform;

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

  // allow the user to edit a scalar field
  // in VR
  vtkSetMacro(EditableField, std::string);
  vtkGetMacro(EditableField, std::string);

  void SetFieldValues(const char*);
  vtkGetMacro(FieldValues, std::string);

  void ExportLocationsAsSkyboxes(vtkSMViewProxy* view);
  void ExportLocationsAsView(vtkSMViewProxy* view);

protected:
  vtkPVOpenVRHelper();
  ~vtkPVOpenVRHelper();

  // state settings that the helper loads
  // These are typically not exposed in the GUI
  // state exposed inthe GUI is handled by the DockPanel
  // gui class.
  int NavigationPanelVisibility;
  std::vector<std::pair<std::array<double, 3>, std::array<double, 3> > > CropPlaneStates;
  std::vector<std::array<double, 16> > ThickCropStates;
  bool CropSnapping;

  std::string EditableField;
  std::string FieldValues;
  bool MultiSample;

  void ApplyState();
  void RecordState();

  double DefaultCropThickness;

  std::map<int, std::string> EditFieldMap;
  vtkOpenVRMenuWidget* EditFieldMenu;
  vtkOpenVRMenuRepresentation* EditFieldMenuRepresentation;
  void EditField(std::string name);

  vtkDataSet* LastPickedDataSet;
  vtkIdType LastPickedCellId;
  vtkPVDataRepresentation* LastPickedRepresentation;
  vtkDataSet* PreviousPickedDataSet;
  vtkIdType PreviousPickedCellId;
  vtkPVDataRepresentation* PreviousPickedRepresentation;
  std::vector<vtkIdType> SelectedCells;

  void GetScalars();
  void BuildScalarMenu();
  void SelectScalar();
  std::string SelectedScalar;

  vtkNew<vtkOpenVRPanelWidget> NavWidget;
  vtkNew<vtkOpenVRPanelRepresentation> NavRepresentation;
  void ToggleNavigationPanel();
  unsigned long NavigationTag;

  std::set<vtkImplicitPlaneWidget2*> CropPlanes;
  std::set<vtkBoxWidget2*> ThickCrops;
  void AddACropPlane(double* origin, double* normal);
  void RemoveAllCropPlanes();
  void AddAThickCrop(vtkTransform* t);
  void RemoveAllThickCrops();
  void ToggleCropSnapping();
  vtkNew<vtkOpenVRMenuRepresentation> CropMenuRepresentation;
  vtkNew<vtkOpenVRMenuWidget> CropMenu;
  unsigned long CropTag;

  vtkOpenVRInteractorStyle* Style;
  vtkOpenVRRenderWindowInteractor* Interactor;

  std::map<std::string, int> ScalarMap;
  vtkOpenVRMenuWidget* ScalarMenu;
  vtkOpenVRMenuRepresentation* ScalarMenuRepresentation;

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

  bool NeedStillRender;

  void SaveLocationState(int slot);
  void LoadLocationState();
  class SlotData
  {
  public:
    std::map<vtkSMProxy*, bool> Visibility;
  };
  std::map<int, SlotData> SlotValues;
  int LoadSlotValue;

  std::map<int, vtkOpenVRCameraPose> SavedCameraPoses;

private:
  vtkPVOpenVRHelper(const vtkPVOpenVRHelper&) = delete;
  void operator=(const vtkPVOpenVRHelper&) = delete;
};

#endif
