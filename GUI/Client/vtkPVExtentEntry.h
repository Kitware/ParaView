/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtentEntry - Special behavior for animations
//
// .SECTION Description
// Although I could make this a subclass of vtkPVVector Entry,
// Vector entry is too general, and some inherited method may be confusing.
// The reason I created this class is to get a special behavior
// for animations.

#ifndef __vtkPVExtentEntry_h
#define __vtkPVExtentEntry_h

#include "vtkPVObjectWidget.h"

class vtkKWEntry;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkPVInputMenu;
class vtkPVMinMax;

class VTK_EXPORT vtkPVExtentEntry : public vtkPVObjectWidget
{
public:
  static vtkPVExtentEntry* New();
  vtkTypeRevisionMacro(vtkPVExtentEntry, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Methods to set this widgets value from a script.
  void SetValue(int v1, int v2, int v3, int v4, int v5, int v6);
  
  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This method gets called when the user selects this widget to animate.
  // It sets up the script and animation parameters.
  virtual void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai, int Mode);
  virtual void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
    {
      this->AnimationMenuCallback(ai, 0);
    }

  // Description:
  // The label.
  vtkSetStringMacro(Label);
  vtkGetStringMacro(Label);

  // Description:
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

  // Description:
  virtual void Update();

  // Description:
  // The label.
  void SetRange(int v0, int v1, int v2, int v3, int v4, int v5);
  vtkGetVector6Macro(Range,int);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);


//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVExtentEntry* ClonePrototype(vtkPVSource* pvSource,
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
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

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
  virtual void ResetAnimationRange(vtkPVAnimationInterfaceEntry* ai, int idx);
  
protected:
  vtkPVExtentEntry();
  ~vtkPVExtentEntry();

  vtkKWLabeledFrame* LabeledFrame;
  char* Label;

  vtkPVInputMenu* InputMenu;

  int Range[6];
  vtkPVMinMax* MinMax[3];

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVExtentEntry(const vtkPVExtentEntry&); // Not implemented
  void operator=(const vtkPVExtentEntry&); // Not implemented
};

#endif
