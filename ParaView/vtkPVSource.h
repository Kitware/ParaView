/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPVSource - A super class for filter objects.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.
// This class should probably be merged with vtkPVComposite.


#ifndef __vtkPVSource_h
#define __vtkPVSource_h

#include "vtkKWComposite.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkPVData.h"
#include "vtkSource.h"

class vtkPVWindow;
class vtkPVCommandList;
class vtkKWCheckButton;
class vtkKWScale;
class vtkKWEntry;
class vtkPVSelectionList;

class VTK_EXPORT vtkPVSource : public vtkKWComposite
{
public:
  static vtkPVSource* New();
  vtkTypeMacro(vtkPVSource,vtkKWComposite);
  
  // Description:
  // This duplicates the object in the satellite processes.
  // They will all have the same tcl name.
  virtual void Clone(vtkPVApplication *app);
  
  // Description:
  // Get the Window for this class.
  vtkPVWindow *GetWindow();
  
  // Description:
  // A way to get the output in the superclass.
  vtkPVData *GetPVData() {return this->PVOutput;}
    
  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();
  virtual void ShowProperties();

  void CreateDataPage();
  
  // Description:
  // Methods to indicate when this composite is the selected composite.
  // These methods are used by subclasses to modify the menu bar
  // for example. When a volume composite is selected it might 
  // add an option to the menu bar to view the 2D slices.
  virtual void Select(vtkKWView *);
  virtual void Deselect(vtkKWView *);

  // Description:
  // This flag turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);

  // Description:
  // This name is used in the data list to identify the composite.
  virtual void SetName(const char *name);
  char* GetName();
  
  vtkPVData **GetInputs() { return this->Inputs; };
  vtkPVData *GetNthInput(int idx);
  vtkGetMacro(NumberOfInputs, int);
  void SqueezeInputArray();
  void RemoveAllInputs();
  
  // Description:
  // This just returns the application typecast correctly.
  vtkPVApplication* GetPVApplication();  

  // Description:
  // Sources have no props.
  vtkProp *GetProp() {return NULL;}  

  // Description:
  // Creates the output and assignment.
  // If there is an input, it uses its assignement. 
  // Otherwise, it creates a new one.
  virtual void InitializeOutput() {};
  
  // Description:
  // Called when the accept button is pressed.
  void AcceptCallback();
  
  // Description:
  // Called when the accept button is pressed.
  void CancelCallback();

  // Description:
  // This method resets the UI values (Widgets added with the following methods).
  // It uses the GetCommand supplied to the interface.
  void UpdateParameterWidgets();
  
  // Description:
  // Create an entry for a single value.  Label is put to left of entry.
  // The methods are called on the object (VTKSource if o=NULL).
  vtkKWEntry *AddLabeledEntry(char *label, char *setCmd, char *getCmd,
                              vtkKWObject *o = NULL);
  
  // Description:
  // Create an entry for items with multiple elements.
  // The primary label is put to the left.  The element labels
  // (l1,l2,l3, ...) are put in from of the individual entry boxes.
  // The methods are called on the object (VTKSource if o=NULL).
  void AddVector2Entry(char *label, char *l1, char *l2,
		       char *setCmd, char *getCmd, vtkKWObject *o = NULL);
  void AddVector3Entry(char *label, char *l1, char *l2, char *l3,
		       char *setCmd, char *getCmd, vtkKWObject *o = NULL);
  void AddVector4Entry(char *label, char *l1, char *l2, char *l3, char *l4,
		       char *setCmd, char *getCmd, vtkKWObject *o = NULL);
  void AddVector6Entry(char *label, char *l1, char *l2, char *l3, 
		       char *l4, char *l5, char *l6,
		       char *setCmd, char *getCmd, vtkKWObject *o = NULL);
  
  // Description:
  // Special widget controls (not entries).
  // The methods are called on the object (VTKSource if o=NULL).
  vtkKWCheckButton *AddLabeledToggle(char *label, char *setCmd, char *getCmd,
                                     vtkKWObject *o = NULL);
  vtkKWScale *AddScale(char *label, char *setCmd, char *getCmd, 
                       float min, float max, float resolution,
                       vtkKWObject *o = NULL);

  // Description:
  // Creates a list for delecting a mode.
  // The methods are called on the object (VTKSource if o=NULL).
  vtkPVSelectionList *AddModeList(char *label, char *setCmd, char *getCmd,
                                  vtkKWObject *o = NULL);
  void AddModeListItem(char *name, int value);
  
  // Description:
  // Set the vtk source that will be a part of the pipeline.
  void SetVTKSource(vtkSource *source);
  vtkGetObjectMacro(VTKSource, vtkSource);
  const char *GetVTKSourceTclName();
  vtkSetStringMacro(VTKSourceTclName);

  // Description:
  // A method used to broadcast changes resulting from widgets.
  void AcceptHelper(char *method, char *args);
  void AcceptHelper2(char *tclName, char *method, char *args);  

  // Description:
  // A call back method from the Navigation window.
  // This sets the input source as the selected composite.
  void SelectSource(vtkPVSource *source);

protected:
  vtkPVSource();
  ~vtkPVSource();
  vtkPVSource(const vtkPVSource&) {};
  void operator=(const vtkPVSource&) {};
  
  // Description:
  // Mangages the double pointer and reference counting.
  void SetPVData(vtkPVData *data);
  
  vtkPVData *PVOutput;
  vtkSource *VTKSource;
  char *VTKSourceTclName;

  vtkPVData **Inputs;
  int NumberOfInputs;
  
  // Called to allocate the input array.  Copies old inputs.
  void SetNumberOfInputs(int num);
  
  // protected methods for setting inputs.
  void SetNthInput(int idx, vtkPVData *input);
  void AddInput(vtkPVData *input);
  void RemoveInput(vtkPVData *input);
  
  // The name is just for display.
  char      *Name;

  vtkKWWidget       *Properties;
  vtkKWLabeledFrame *NavigationFrame;
  vtkKWWidget       *NavigationCanvas;
  void              UpdateNavigationCanvas();
  vtkKWLabeledFrame *ParameterFrame;
  
  int DataCreated;

  vtkKWWidgetCollection *Widgets;

  vtkKWPushButton *AcceptButton;
  vtkKWPushButton *CancelButton;

  vtkPVSelectionList *LastSelectionList;

  vtkPVCommandList *AcceptCommands;
  vtkPVCommandList *CancelCommands;
};

#endif






