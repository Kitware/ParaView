/*=========================================================================

  Program:   ParaView
  Module:    vtkPVThumbWheel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVThumbWheel - PV version of vtkKWThumbWheel
// .SECTION Description

#ifndef __vtkPVThumbWheel_h
#define __vtkPVThumbWheel_h

#include "vtkPVObjectWidget.h"

class vtkKWLabel;
class vtkKWThumbWheel;

class VTK_EXPORT vtkPVThumbWheel : public vtkPVObjectWidget
{
public:
  static vtkPVThumbWheel* New();
  vtkTypeRevisionMacro(vtkPVThumbWheel, vtkPVObjectWidget);
  void PrintSelf(ostream &os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Allow scripts to set/get the widget's value.
  void SetValue(float val);
  float GetValue();
  
  // Description:
  // Set the minimum value of the thumb wheel
  void SetMinimumValue(float min);
  
  // Description:
  // Set the resolution of the thumb wheel
  void SetResolution(float res);
  
  // Description:
  // Set the label for this widget.
  void SetLabel(const char *label);
  
  // Description:
  // This class redifines SetBalloonHelpString since it has to forward the
  // call to the widget it contains.
  virtual void SetBalloonHelpString(const char *str);
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create a new instance
  // of the same type as the current object using NewInstance() and then copy
  // some necessary state parameters.
  vtkPVThumbWheel* ClonePrototype(vtkPVSource *pvSource,
                               vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);

  // Description:
  // Move widget state to vtk object or back.
  virtual void Accept();
  virtual void ResetInternal();
//ETX

  // Description:
  // For saving state.
  virtual void Trace(ofstream *file);
  
  // Description:
  // Update the "enable" state of the widget and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Save this widget to a file.
  virtual void SaveInBatchScript(ofstream *file);
 
  // Description:
  // Called when menu item (above) is selected.  Neede for tracing.
  // Would not be necessary if menus traced invocations.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(
    vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // Resets the animation entries (start and end) to values obtained
  // from the range domain
  virtual void ResetAnimationRange(vtkPVAnimationInterfaceEntry* ai);

protected:
  vtkPVThumbWheel();
  ~vtkPVThumbWheel();

  vtkKWLabel *Label;
  vtkKWThumbWheel *ThumbWheel;
  
//BTX
  virtual void CopyProperties(vtkPVWidget *clone, vtkPVSource *source,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement *element,
                        vtkPVXMLPackageParser *parser);
  
private:
  vtkPVThumbWheel(const vtkPVThumbWheel&); // Not implemented
  void operator=(const vtkPVThumbWheel&); // Not implemented
};

#endif
