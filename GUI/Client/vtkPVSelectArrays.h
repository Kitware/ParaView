/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectArrays.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSelectArrays - Widget for selecting a set of cell arrays.
// .SECTION Description
// This widget started for selecting volume fraction arrays in CTH data sets.
// I am generalizing it to select any arrays.
// I may generalize it further to select point arrays as well.

#ifndef __vtkPVSelectArrays_h
#define __vtkPVSelectArrays_h

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


class VTK_EXPORT vtkPVSelectArrays : public vtkPVWidget
{
public:
  static vtkPVSelectArrays* New();
  vtkTypeRevisionMacro(vtkPVSelectArrays, vtkPVWidget);
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
  vtkPVSelectArrays* ClonePrototype(vtkPVSource* pvSource,
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
  vtkPVSelectArrays();
  ~vtkPVSelectArrays();

  vtkKWWidget*      ButtonFrame;
  vtkKWLabel*       ShowAllLabel;
  vtkKWCheckButton* ShowAllCheck;

  vtkKWListBox* ArraySelectionList;
  // Labels get substituted for list box after accept is called.
  vtkCollection* ArrayLabelCollection;

  // This is set to vtkDataSet::CELL_DATA_FIELD if the widget is selecting
  // cell arrays.  It is set to vtkDataSet::POINT_DATA_FIELD if the widget is being
  // used for selecting point arrays.  It is set through the XML
  // description of the widget: field="Cell".
  // This defaults to Cell.
  int Field;

  // This flag is set when the widget deactivates after the first selection.
  // The default value is 0 (off).  XML: Deactivate="On".
  // I expect this option to go awway after we start using multiblock data sets
  // for ParaView's group.
  int Deactivate;

  // This flag adds the option of filtering the arrays based on their names.
  // Right now it is hard coded for CTH voloume fractions, but I see no reason
  // it cold not be extended to a general pattern matching in the future.
  // This defaults to 0 (off): XML: FilterArrays="On"
  int FilterArrays;

  // Called to inactivate widget (after accept is called).
  void Inactivate();
  int Active;
  vtkStringList* SelectedArrayNames;

  vtkPVInputMenu* InputMenu;

  int StringMatch(const char* arrayName);
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
  
  vtkPVSelectArrays(const vtkPVSelectArrays&); // Not implemented
  void operator=(const vtkPVSelectArrays&); // Not implemented
};

#endif
