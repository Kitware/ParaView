/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComponentSelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComponentSelection -
// .SECTION Description

#ifndef __vtkPVComponentSelection_h
#define __vtkPVComponentSelection_h

#include "vtkPVObjectWidget.h"

class VTK_EXPORT vtkPVComponentSelection : public vtkPVObjectWidget
{
public:
  static vtkPVComponentSelection* New();
  vtkTypeRevisionMacro(vtkPVComponentSelection, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual void Create(vtkKWApplication *pvApp);
    
  // Description:
  // Set/Get the state of the ith checkbutton.
  void SetState(int i, int state);
  int GetState(int i);
  
  // Description:
  // Set the number of connected components.
  // This has to be set before calling create.
  vtkSetMacro(NumberOfComponents, int);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVComponentSelection* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  //BTX
  // Description:
  // Called when accept button is pushed.
  // Sets object's variable to the widget's value.
  // Side effect is to turn modified flag off.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // Called then the reset button is pushed.
  // Sets the widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

protected:
  vtkPVComponentSelection();
  ~vtkPVComponentSelection();

  vtkKWWidgetCollection *CheckButtons;
  int Initialized;

  vtkPVComponentSelection(const vtkPVComponentSelection&); // Not implemented
  void operator=(const vtkPVComponentSelection&); // Not implemented

  int NumberOfComponents;

  unsigned char *LastAcceptedState;
  
//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
