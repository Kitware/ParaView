/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArraySelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVArraySelection - widget to select a set of data arrays.
// .SECTION Description
// vtkPVArraySelection is used for selecting which set of data arrays to 
// load when a reader has the ability to selectively load arrays.

#ifndef __vtkPVArraySelection_h
#define __vtkPVArraySelection_h

#include "vtkPVWidget.h"

class vtkCollection;
class vtkDataArraySelection;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWPushButton;
class vtkKWRadioButton;
class vtkPVArraySelectionArraySet;
class vtkPVData;

class VTK_EXPORT vtkPVArraySelection : public vtkPVWidget
{
public:
  static vtkPVArraySelection* New();
  vtkTypeRevisionMacro(vtkPVArraySelection, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
//BTX
  // Description:
  // Methods for setting the value of the VTKReader from the widget.
  // User internally when user hits Accept.
  virtual void Accept();
//ETX

  // Description:
  // Methods for setting the value of the widget from the VTKReader.
  // User internally when user hits Reset.
  virtual void ResetInternal();

  // Description:
  // This specifies whether to ues Cell or Point data.
  // Options are "Cell" or "Point".  Possible "Field" in the future.
  vtkSetStringMacro(AttributeName);
  vtkGetStringMacro(AttributeName);

  // Description:
  // Used to change the label of the widget. If not specified,
  // the label is constructed using the AttributeName
  vtkSetStringMacro(LabelText);
  vtkGetStringMacro(LabelText);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Callback for the AllOn and AllOff buttons.
  void AllOnCallback();
  void AllOffCallback();

  // Description:
  // Access to change this widgets state from a script. Used for tracing.
  void SetArrayStatus(const char *name, int status);

  // Description:
  // Get the number of array names listed in this widget.
  int GetNumberOfArrays();
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVArraySelection* ClonePrototype(vtkPVSource* pvSource,
                                      vtkArrayMap<vtkPVWidget*,
                                      vtkPVWidget*>* map);
//ETX
  
  // Description:
  // Save this widget to a file. 
  // Ingore parts for thsi reader specific widget. 
  virtual void SaveInBatchScript(ofstream *file);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVArraySelection();
  ~vtkPVArraySelection();

  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Set/get the SM property to use with this widget..
  vtkSMProperty* GetSMInformationProperty();
  void SetSMInformationProperty(vtkSMProperty* prop);

  // Description:
  // Need access to these so that container-type widgets can set the property
  // name on the widgets they contain (e.g., vtkPVPointSourceWidget).
  vtkSetStringMacro(SMInformationPropertyName);
  vtkGetStringMacro(SMInformationPropertyName);

  char* SMInformationPropertyName;

  vtkSMProperty* SMInformationProperty;

  virtual void UpdateSelections(int fromReader);
  virtual void SetReaderSelectionsFromProperty();

  char* LabelText;
  char* AttributeName;
  
  vtkKWLabeledFrame* LabeledFrame;
  
  vtkKWWidget* ButtonFrame;
  vtkKWPushButton* AllOnButton;
  vtkKWPushButton* AllOffButton;

  vtkKWWidget *CheckFrame;
  vtkCollection* ArrayCheckButtons;
  vtkKWLabel *NoArraysLabel;

  vtkDataArraySelection* Selection;
  
  vtkPVArraySelectionArraySet* ArraySet;

  vtkPVArraySelection(const vtkPVArraySelection&); // Not implemented
  void operator=(const vtkPVArraySelection&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
