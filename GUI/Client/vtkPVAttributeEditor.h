/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAttributeEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAttributeEditor - A special PVSource.
// .SECTION Description
// This class controls interaction with the vtkAttributeEditor

#ifndef __vtkPVAttributeEditor_h
#define __vtkPVAttributeEditor_h

#include "vtkPVSource.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWPushButton;
class vtkKWEntry;
class vtkCallbackCommand;


class VTK_EXPORT vtkPVAttributeEditor : public vtkPVSource
{
public:
  static vtkPVAttributeEditor* New();
  vtkTypeRevisionMacro(vtkPVAttributeEditor, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  // In the case of exodus data, widgets that allow the user to save their
  // edits are packed.
  virtual void CreateProperties();

  // Description: 
  // Called when the vtkPVSelectWidget is modified that contains options to
  // pick by a box, sphere, or point widget.
  // When the sphere widget is active, we want auto-accept to be turned on so
  // it can be dragged.
  void PickMethodObserver();

  // Description:
  // Called when the browse button is pressed.
  void BrowseCallback();

  // Description:
  // Called when the save button is pressed.
  // Sends commands to writer via clientserverstream.
  // Currently only exodus data can be saved.
  void SaveCallback();
 
  // Description:
  // These must be made available to the ProcessEvents function so that 
  // it can make decisions on what action to take when events occur.
  void OnChar();
  void OnTimestepChange();
  vtkGetMacro(EditedFlag,int);
  vtkSetMacro(EditedFlag,int);
  vtkSetMacro(IsScalingFlag,int);
  vtkSetMacro(IsMovingFlag,int);

  // Description:
  // Handles the events
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);

protected:
  vtkPVAttributeEditor();
  ~vtkPVAttributeEditor();

  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  virtual void Select();
  void UpdateGUI();

  vtkClientServerID WriterID;

  // Listens for keyboard and mouse events
  vtkCallbackCommand* EventCallbackCommand; 

  int IsScalingFlag;
  int IsMovingFlag;
  int EditedFlag;
  int ForceEdit;
  int ForceNoEdit;
  
  vtkKWFrame *Frame;
  vtkKWFrame *DataFrame;
  vtkKWLabel *Label;
  vtkKWPushButton *BrowseButton;
  vtkKWPushButton *SaveButton;
  vtkKWEntry *Entry;

  vtkPVAttributeEditor(const vtkPVAttributeEditor&); // Not implemented
  void operator=(const vtkPVAttributeEditor&); // Not implemented
};

#endif
