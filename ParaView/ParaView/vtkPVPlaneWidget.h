/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlaneWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPlaneWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// This widget creates and manages its own vtkPlane on each process.
// I could not descide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVPlaneWidget_h
#define __vtkPVPlaneWidget_h

#include "vtkPVImplicitPlaneWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;

class VTK_EXPORT vtkPVPlaneWidget : public vtkPVImplicitPlaneWidget
{
public:
  static vtkPVPlaneWidget* New();
  vtkTypeRevisionMacro(vtkPVPlaneWidget, vtkPVImplicitPlaneWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // This method sets the input to the 3D widget and places the widget.
  virtual void ActualPlaceWidget();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVPlaneWidget* ClonePrototype(vtkPVSource* pvSource,
                                   vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  void SetCenter(float,float,float);
  virtual void SetCenter(float f[3]) { this->SetCenter(f[0], f[1], f[2]); }
  void SetNormal(float,float,float);
  virtual void SetNormal(float f[3]) { this->SetNormal(f[0], f[1], f[2]); }

  // Description:
  // For saving the widget into a VTK tcl script.
  // Saves a plane (one for all parts).
  virtual void SaveInBatchScript(ofstream *file);

protected:
  vtkPVPlaneWidget();
  ~vtkPVPlaneWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVPlaneWidget(const vtkPVPlaneWidget&); // Not implemented
  void operator=(const vtkPVPlaneWidget&); // Not implemented
};

#endif
