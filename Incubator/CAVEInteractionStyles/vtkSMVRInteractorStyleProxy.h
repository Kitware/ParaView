// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRInteractorStyleProxy_h
#define vtkSMVRInteractorStyleProxy_h

#include "vtkCommand.h"                                // For UserEvent
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include <vtkSMProxy.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

class vtkCamera;
class vtkMatrix4x4;
class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkSMProxy;
class vtkSMDoubleVectorProperty;
class vtkSMRenderViewProxy;
class vtkStringList;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRInteractorStyleProxy : public vtkSMProxy
{
public:
  static vtkSMVRInteractorStyleProxy* New();
  vtkTypeMacro(vtkSMVRInteractorStyleProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Get the vector size of the controlled property this style expects, e.g. a
  // 4x4 matrix will be 16, a 3D vector will be 3, etc. This is used to limit
  // the number of options presented to the user when prompting for a property.
  // This is NOT checked internally by SetControlledPropertyName.
  //
  // A value of -1 means no filtering will be done, and all available properties
  // will be shown.
  virtual int GetControlledPropertySize() { return -1; }

  virtual void SetControlledProxy(vtkSMProxy*);
  vtkGetObjectMacro(ControlledProxy, vtkSMProxy);

  vtkSetStringMacro(ControlledPropertyName);
  vtkGetStringMacro(ControlledPropertyName);

  // An internal interactor style proxy should not be shown in the
  // UI, but can still be used by the CAVEInteraction plugin for
  // internal purposes.
  vtkSetMacro(IsInternal, bool);
  vtkGetMacro(IsInternal, bool);
  vtkBooleanMacro(IsInternal, bool);

  virtual bool HandleEvent(const vtkVREvent& event);

  /// Update() called to update all the remote vtkObjects and perhaps even to render.
  ///   Typically processing intensive operations go here. The method should not
  ///   be called from within the handler and is reserved to be called from an
  ///   external interaction style manager.
  virtual bool Update();

  // Description:
  // Get a list of defined roles for each output type.
  void GetValuatorRoles(vtkStringList*);
  void GetButtonRoles(vtkStringList*);
  void GetTrackerRoles(vtkStringList*);

  // Description:
  // Get the number of roles defined for each output type.
  int GetNumberOfValuatorRoles();
  int GetNumberOfButtonRoles();
  int GetNumberOfTrackerRoles();

  // Description:
  // Get the role of the input with the given name. If the name is not
  // set or recognized, an empty string is returned.
  std::string GetButtonRole(const std::string& name);
  std::string GetTrackerRole(const std::string& name);

  // Description:
  // Valuators are special in that data from all channels is
  // delivered in an array in a single event.  This method allows
  // finding that index for any named role which the user has
  // bound to an event
  unsigned int GetChannelIndexForValuatorRole(const std::string& role);

  // Description:
  // Add a new input role to the interactor style.
  void AddValuatorRole(const std::string& role);
  void AddButtonRole(const std::string& role);
  void AddTrackerRole(const std::string& role);

  void ClearAllRoles();

  // Description:
  // Set/Get the name of the input that fulfills the specified role.
  bool SetValuatorName(const std::string& role, const std::string& name);
  std::string GetValuatorName(const std::string& role);
  bool SetButtonName(const std::string& role, const std::string& name);
  std::string GetButtonName(const std::string& role);
  bool SetTrackerName(const std::string& role, const std::string& name);
  std::string GetTrackerName(const std::string& role);

  /// Load state for the style from XML.
  virtual bool Configure(vtkPVXMLElement* child, vtkSMProxyLocator*);

  /// Save state to xml.
  virtual vtkPVXMLElement* SaveConfiguration();

  enum
  {
    INTERACTOR_STYLE_REQUEST_CONFIGURE = vtkCommand::UserEvent + 7370,
    INTERACTOR_STYLE_NAVIGATION = vtkCommand::UserEvent + 7371
  };

  // Description:
  // Get active objects or return nullptr
  static vtkSMRenderViewProxy* GetActiveViewProxy();
  static vtkCamera* GetActiveCamera();

  // Description:
  // Get/Set the matrix used to navigate the scene.
  vtkMatrix4x4* GetNavigationMatrix();
  void SetNavigationMatrix(vtkMatrix4x4*);

  typedef std::map<std::string, std::string> StringMap;
  typedef std::map<std::string, StringMap> StringMapMap;

  void SetValuatorLookupTable(std::shared_ptr<StringMapMap>);

protected:
  vtkSMVRInteractorStyleProxy();
  virtual ~vtkSMVRInteractorStyleProxy();

  virtual void HandleButton(const vtkVREvent& event);
  virtual void HandleValuator(const vtkVREvent& event);
  virtual void HandleTracker(const vtkVREvent& event);

  static std::vector<std::string> Tokenize(std::string input);

  vtkSMProxy* ControlledProxy;
  char* ControlledPropertyName;

  StringMap Valuators;
  StringMap Buttons;
  StringMap Trackers;
  void MapKeysToStringList(const StringMap& source, vtkStringList* target);
  bool SetValueInMap(StringMap& map_, const std::string& key, const std::string& value);
  std::string GetValueInMap(const StringMap& map_, const std::string& key);
  std::string GetKeyInMap(const StringMap& map_, const std::string& value);

  bool IsInternal;

  std::shared_ptr<StringMapMap> valuatorLookupTable;

private:
  vtkSMVRInteractorStyleProxy(const vtkSMVRInteractorStyleProxy&) = delete;
  void operator=(const vtkSMVRInteractorStyleProxy&) = delete;
};

#endif
