/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectCTHArrays.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSelectCTHArrays - Widget for ExtractCTHPart filter.
// .SECTION Description
// This widget is for the ExtractCTHPart filter.  It allows the user
// to select multiple volume-fraction cell arrays.

#ifndef __vtkPVSelectCTHArrays_h
#define __vtkPVSelectCTHArrays_h

#include "vtkPVWidget.h"
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWListBox;
class vtkCollection;
class vtkPVInputMenu;
class vtkPVSource;
class vtkKWLabel;
class vtkKWCheckButton;
class vtkStringList;


class VTK_EXPORT vtkPVSelectCTHArrays : public vtkPVWidget
{
public:
  static vtkPVSelectCTHArrays* New();
  vtkTypeRevisionMacro(vtkPVSelectCTHArrays, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void Create(vtkKWApplication *app);

  // Description:
  // Save this source to a file.
  void SaveInBatchScript(ofstream *file);

  // Description:
  // Button callbacks.
  void ShowAllArraysCheckCallback();

  // Description:
  // Access metod necessary for scripting.
  void ClearAllSelections();
  void SetSelectState(const char* arrayName, int val);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  //BTX
  // Description:
  // Called when the Accept button is pressed.  It moves the widget values to the 
  // VTK calculator filter.
  virtual void Accept();
  //ETX

  // Description:
  // This method resets the widget values from the VTK filter.
  virtual void ResetInternal();

  // Description:
  // Initialize after creation.
  virtual void Initialize();

  // Description:
  // This input menu supplies the array options.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

  // Description:
  // This is called to update the menus if something (InputMenu) changes.
  virtual void Update();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // parameters.
  vtkPVSelectCTHArrays* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVSelectCTHArrays();
  ~vtkPVSelectCTHArrays();

  vtkKWWidget*      ButtonFrame;
  vtkKWLabel*       ShowAllLabel;
  vtkKWCheckButton* ShowAllCheck;

  vtkKWListBox* ArraySelectionList;
  // Labels get substituted for list box after accept is called.
  vtkCollection* ArrayLabelCollection;

  // Called to inactivate widget (after accept is called).
  void Inactivate();
  int Active;
  vtkStringList* SelectedArrayNames;

  vtkPVInputMenu* InputMenu;

  int StringMatch(const char* arrayName);
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
  
  vtkPVSelectCTHArrays(const vtkPVSelectCTHArrays&); // Not implemented
  void operator=(const vtkPVSelectCTHArrays&); // Not implemented
};

#endif
