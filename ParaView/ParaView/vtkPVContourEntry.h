/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContourEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVContourEntry maintains a list of floats for contouring.
// .SECTION Description
// This widget lets the user add or delete floats from a list.
// It is used for contours.

#ifndef __vtkPVContourEntry_h
#define __vtkPVContourEntry_h

#include "vtkPVValueList.h"

class vtkPVArrayMenu;
class vtkPVWidgetProperty;

class VTK_EXPORT vtkPVContourEntry : public vtkPVValueList
{
public:
  static vtkPVContourEntry* New();
  vtkTypeRevisionMacro(vtkPVContourEntry, vtkPVValueList);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // We need to make the callback here so the animation selection
  // can be traced properly.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai);

  //BTX
  // Description:
  // Gets called when the accept button is pressed.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // Gets called when the reset button is pressed.
  virtual void ResetInternal();

  // Description:
  // ArrayMenu is used to obtain the scalar range (it contains an array
  // information object)
  virtual void SetArrayMenu(vtkPVArrayMenu*);
  vtkGetObjectMacro(ArrayMenu, vtkPVArrayMenu);

  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
  // Description:
  // Get the VTK commands.
  vtkSetStringMacro(SetNumberCommand);
  vtkSetStringMacro(SetContourCommand);
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVContourEntry();
  ~vtkPVContourEntry();
  
  vtkPVArrayMenu *ArrayMenu;
  
  void UpdateProperty();

  char *SetNumberCommand;
  char *SetContourCommand;
  
  virtual int ComputeWidgetRange();
  
  vtkPVContourEntry(const vtkPVContourEntry&); // Not implemented
  void operator=(const vtkPVContourEntry&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,      
                        vtkPVXMLPackageParser* parser);

  // Description:
  // The widget saves it state/command in the vtk tcl script.
  virtual void SaveInBatchScriptForPart(ofstream *file, 
                                        vtkClientServerID);
};

#endif
