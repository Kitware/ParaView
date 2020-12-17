/*=========================================================================

   Program: ParaView
   Module:  vtkVRInteractorStyle.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef vtkVRInteractorStyle_h
#define vtkVRInteractorStyle_h

#include <vtkObject.h>

#include <map>
#include <string>
#include <vector>

class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkSMProxy;
class vtkSMDoubleVectorProperty;
class vtkStringList;
struct vtkVREvent;

class vtkVRInteractorStyle : public vtkObject
{
public:
  static vtkVRInteractorStyle* New();
  vtkTypeMacro(vtkVRInteractorStyle, vtkObject);
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

  virtual bool HandleEvent(const vtkVREvent& event);

  /// Update() called to update all the remote vtkObjects and perhaps even to render.
  ///   Typically processing intensive operations go here. The method should not
  ///   be called from within the handler and is reserved to be called from an
  ///   external interaction style manager.
  virtual bool Update();

  // Description:
  // Get a list of defined roles for each output type.
  void GetAnalogRoles(vtkStringList*);
  void GetButtonRoles(vtkStringList*);
  void GetTrackerRoles(vtkStringList*);

  // Description:
  // Get the number of roles defined for each output type.
  int GetNumberOfAnalogRoles();
  int GetNumberOfButtonRoles();
  int GetNumberOfTrackerRoles();

  // Description:
  // Get the role of the input with the given name. If the name is not
  // set or recognized, an empty string is returned.
  std::string GetAnalogRole(const std::string& name);
  std::string GetButtonRole(const std::string& name);
  std::string GetTrackerRole(const std::string& name);

  // Description:
  // Set/Get the name of the input that fulfills the specified role.
  bool SetAnalogName(const std::string& role, const std::string& name);
  std::string GetAnalogName(const std::string& role);
  bool SetButtonName(const std::string& role, const std::string& name);
  std::string GetButtonName(const std::string& role);
  bool SetTrackerName(const std::string& role, const std::string& name);
  std::string GetTrackerName(const std::string& role);

  /// Load state for the style from XML.
  virtual bool Configure(vtkPVXMLElement* child, vtkSMProxyLocator*);

  /// Save state to xml.
  virtual vtkPVXMLElement* SaveConfiguration() const;

protected:
  vtkVRInteractorStyle();
  virtual ~vtkVRInteractorStyle();

  virtual void HandleButton(const vtkVREvent& event);
  virtual void HandleAnalog(const vtkVREvent& event);
  virtual void HandleTracker(const vtkVREvent& event);

  static std::vector<std::string> Tokenize(std::string input);

  vtkSMProxy* ControlledProxy;
  char* ControlledPropertyName;

  // Description:
  // Add a new input role to the interactor style.
  void AddAnalogRole(const std::string& role);
  void AddButtonRole(const std::string& role);
  void AddTrackerRole(const std::string& role);

  typedef std::map<std::string, std::string> StringMap;
  StringMap Analogs;
  StringMap Buttons;
  StringMap Trackers;
  void MapKeysToStringList(const StringMap& source, vtkStringList* target);
  bool SetValueInMap(StringMap& map_, const std::string& key, const std::string& value);
  std::string GetValueInMap(const StringMap& map_, const std::string& key);
  std::string GetKeyInMap(const StringMap& map_, const std::string& value);

private:
  vtkVRInteractorStyle(const vtkVRInteractorStyle&) = delete;
  void operator=(const vtkVRInteractorStyle&) = delete;
};

#endif
