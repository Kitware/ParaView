/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLabeledToggle.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLabeledToggle -
// .SECTION Description

#ifndef __vtkPVLabeledToggle_h
#define __vtkPVLabeledToggle_h

#include "vtkPVObjectWidget.h"

class vtkKWApplication;
class vtkKWLabel;
class vtkKWCheckButton;
class vtkPVIndexWidgetProperty;

class VTK_EXPORT vtkPVLabeledToggle : public vtkPVObjectWidget
{
public:
  static vtkPVLabeledToggle* New();
  vtkTypeRevisionMacro(vtkPVLabeledToggle, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Setting the label also sets the name.
  void SetLabel(const char *str);
  const char* GetLabel();

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // This method allows scripts to modify the widgets value.
  void SetState(int val);
  int GetState();

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Disables the checkbutton.
  void Disable();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVLabeledToggle* ClonePrototype(vtkPVSource* pvSource,
                                     vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();

protected:
  vtkPVLabeledToggle();
  ~vtkPVLabeledToggle();
  
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Side effect is to turn modified flag off.Resources/
  virtual void AcceptInternal(vtkClientServerID);
  
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();
  
  // This serves a dual purpose.  For tracing and Resources/for saving state.
  virtual void Trace(ofstream *file);


  vtkKWLabel *Label;
  vtkKWCheckButton *CheckButton;

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
  
  int DefaultValue;
  vtkSetMacro(DefaultValue, int);

  vtkPVIndexWidgetProperty *Property;
  
private:
  vtkPVLabeledToggle(const vtkPVLabeledToggle&); // Not implemented
  void operator=(const vtkPVLabeledToggle&); // Not implemented
};

#endif
