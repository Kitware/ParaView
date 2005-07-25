/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPropertyKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPropertyKeyFrame - GUI for a keyframe that animates a property.
// .SECTION Description
// This is the GUI for a key frame that animates a property (as against the one 
// for animating a proxy).
// .SECTION See Also
// vtkPVProxyKeyFrame

#ifndef __vtkPVPropertyKeyFrame_h
#define __vtkPVPropertyKeyFrame_h

#include "vtkPVKeyFrame.h"
class vtkKWLabel;
class vtkKWWidget;
class vtkKWPushButton;
class vtkSMProperty;

class VTK_EXPORT vtkPVPropertyKeyFrame : public vtkPVKeyFrame
{
public:
  vtkTypeRevisionMacro(vtkPVPropertyKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods to get/set key value. 
  void SetKeyValue(double value) { this->SetKeyValue(0, value); }
  void SetKeyValue(int index, double value);
  double GetKeyValue() { return this->GetKeyValue(0); }
  double GetKeyValue(int index);

  // Description:
  // Set the number of key values for this key frame.
  void SetNumberOfKeyValues(int num);
  int GetNumberOfKeyValues();

  // Description:
  // Initialized Key Value using current animated property value.
  virtual void InitializeKeyValueUsingCurrentState();

  // Description:
  // Initilizes the Key Value using the property element at given index.
  virtual void InitializeKeyValueUsingProperty(
    vtkSMProperty* property, int index);
  
  // Description:
  // Initialize the Key Value bounds using current animatied property value
  // and domain state.
  virtual void InitializeKeyValueDomainUsingCurrentState();

  // Description:
  // Callbacks for GUI.
  void ValueChangedCallback();
  void MinimumCallback();
  void MaximumCallback();

  // Description:
  // These methods set the current Key value to min or max if
  // they exist. Otherwise the value remains unchanged.
  virtual void SetValueToMinimum();
  virtual void SetValueToMaximum();

  // Description:
  // Methods that add entries to trace. These are called by
  // *Callback methods.
  void SetKeyValueWithTrace(int index, double val);
  void SetNumberOfKeyValuesWithTrace(int num);

  // Description:
  // Copies the values from argument.
  // If the two differ in type, only corresponding properies are copied over;
  // Note: this method is safe only if both the keyframes are from
  // the same Animation Cue.
  virtual void Copy(vtkPVKeyFrame* fromKF);
  
  // Description:
  // Save state of the GUI.
  virtual void SaveState(ofstream* file);

  virtual void UpdateEnableState();
protected:
  vtkPVPropertyKeyFrame();
  ~vtkPVPropertyKeyFrame();
 
  // Description:
  // Subclasses create the GUI elements they need in this method.
  // Don't forget to call the superclass implementation.
  virtual void ChildCreate(vtkKWApplication* app);

  // Description:
  // Method to create the value 
  // widget according to the animated property.
  // This methods creates the widget to display the keyframe value.
  // It can be of 3 types: vtkPVSelectionList, vtkKWCheckButton, vtkKWThumbWheel.
  // Which to create depends on the domain of the animated property.
  // This merely creates the widget.
  void CreateValueWidget();

  // Description:
  // Update the values from the vtkSMKeyFrameProxy.
  virtual void UpdateValuesFromProxy();

  // Description:
  // Updates the animated property values using the GUI.
  virtual void UpdateValueFromGUI();

  // Description:
  // Updates GUI domain based on vtkSMDomain of the animated property.
  void UpdateDomain();

  vtkKWLabel* ValueLabel;
  vtkKWWidget* ValueWidget; // the type of this widget will be decided at runtime.
  vtkKWPushButton* MinButton;
  vtkKWPushButton* MaxButton;
 
private:
  vtkPVPropertyKeyFrame(const vtkPVPropertyKeyFrame&); // Not implemented.
  void operator=(const vtkPVPropertyKeyFrame&); // Not implemented.
  
};

#endif


