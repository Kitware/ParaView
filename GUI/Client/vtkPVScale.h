/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScale.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVScale -
// .SECTION Description

#ifndef __vtkPVScale_h
#define __vtkPVScale_h

#include "vtkPVObjectWidget.h"

class vtkKWScale;
class vtkKWLabel;

class VTK_EXPORT vtkPVScale : public vtkPVObjectWidget
{
public:
  static vtkPVScale* New();
  vtkTypeRevisionMacro(vtkPVScale, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // This method allows scripts to modify the widgets value.
  void SetValue(double val);
  double GetValue();

  // Description:
  // The label.
  void SetLabel(const char* label);

  // Description:
  // The resolution of the scale
  void SetResolution(float res);

  // Description:
  // Set the range of the scale.
  void SetRange(float min, float max);
  float GetRangeMin();
  float GetRangeMax();
  
  // Description:
  // Turn on display of the entry box widget that lets the user entry
  // an exact value.
  void DisplayEntry();

  // Description:
  // Set whether the entry is displayed to the side of the scale or on
  // top.  Default is 1 for on top.  Set to 0 for side.
  void SetDisplayEntryAndLabelOnTop(int value);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Check if the widget was modified.
  void CheckModifiedCallback();
  void EntryCheckModifiedCallback();
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVScale* ClonePrototype(vtkPVSource* pvSource,
                             vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  //BTX
  // Description:
  // Called when accept button is pushed.
  // Sets objects variable to the widgets value.
  // Side effect is to turn modified flag off.
  virtual void Accept();
  //ETX

  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();

  // Description:
  // Initialize the widget after creation
  virtual void Initialize();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);
  void Trace();

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // Called when menu item (above) is selected.  Neede for tracing.
  // Would not be necessary if menus traced invocations.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai);
  
  // Description:
  // Get/Set whether to round floating point values to integers.
  vtkSetMacro(Round, int);
  vtkGetMacro(Round, int);
  vtkBooleanMacro(Round, int);
  
  // Description:
  // Flags to determine how to display the scale.
  vtkSetMacro(EntryFlag, int);
  vtkSetMacro(EntryAndLabelOnTopFlag, int);
  vtkSetMacro(DisplayValueFlag, int);

  // Description:
  // Flag for whether to save each movement of the slider in a trace file.
  // This is used for vtkPVScales that are not being used to control parameters
  // of a vtkSource.
  vtkSetMacro(TraceSliderMovement, int);
  vtkGetMacro(TraceSliderMovement, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
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
  // Resets the animation entries (start and end) to values obtained
  // from the range domain
  virtual void ResetAnimationRange(vtkPVAnimationInterfaceEntry* ai);
 
protected:
  vtkPVScale();
  ~vtkPVScale();
  
  int RoundValue(float val);

  int EntryFlag;
  int EntryAndLabelOnTopFlag;
  int DisplayValueFlag;
  int Round;
  
  vtkKWLabel *LabelWidget;
  vtkKWScale *Scale;

  vtkPVScale(const vtkPVScale&); // Not implemented
  void operator=(const vtkPVScale&); // Not implemented

  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);
  char* EntryLabel;

  int TraceSliderMovement;
  
//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

};

#endif
