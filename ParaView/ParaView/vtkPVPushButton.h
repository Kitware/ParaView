/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPushButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPushButton -
// .SECTION Description

#ifndef __vtkPVPushButton_h
#define __vtkPVPushButton_h

#include "vtkPVObjectWidget.h"

class vtkKWPushButton;

class VTK_EXPORT vtkPVPushButton : public vtkPVObjectWidget
{
public:
  static vtkPVPushButton* New();
  vtkTypeRevisionMacro(vtkPVPushButton, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // The label.
  void SetLabel(const char* label);

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
  vtkPVPushButton* ClonePrototype(vtkPVSource* pvSource,
                                  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Set the label of the button.
  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);

  // Description:
  // This method is called when button is pressed.
  void ExecuteCommand();

  // Description:
  // Empty method to keep superclass from complaining.
  virtual void Trace(ofstream *) {};

protected:
  vtkPVPushButton();
  ~vtkPVPushButton();
  
  vtkKWPushButton *Button;
  char *EntryLabel;

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVPushButton(const vtkPVPushButton&); // Not implemented
  void operator=(const vtkPVPushButton&); // Not implemented
};

#endif
