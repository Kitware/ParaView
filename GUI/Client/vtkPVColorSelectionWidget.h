/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorSelectionWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVColorSelectionWidget - widget to select the array and field 
// to use for color (or volume rendering).
// .SECTION Description
// This widget is nothing but a drop-down menu from which the user can
// select the type field data and array. It is designed to be used
// in vtkPVDisplayGUI to select color/volume rendering array.
// Note that this is not a PVWidget.

#ifndef __vtkPVColorSelectionWidget_h
#define __vtkPVColorSelectionWidget_h

#include "vtkKWOptionMenu.h"
class vtkPVSource;
class vtkPVDataSetAttributesInformation;
class vtkPVArrayInformation;

class VTK_EXPORT vtkPVColorSelectionWidget : public vtkKWOptionMenu
{
public:
  static vtkPVColorSelectionWidget* New();
  vtkTypeRevisionMacro(vtkPVColorSelectionWidget, vtkKWOptionMenu);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Need the source to get the input.
  vtkPVSource* GetPVSource() { return this->PVSource; }
  void SetPVSource(vtkPVSource* src) { this->PVSource = src; }

  // Description:
  // This is the command that is called on the Target when
  // array is selected. This command is passed two arguments
  // (arrayname , field type);
  vtkGetStringMacro(ColorSelectionCommand);
  vtkSetStringMacro(ColorSelectionCommand);

  void SetTarget(vtkKWWidget* t) { this->Target = t; }
  vtkGetObjectMacro(Target, vtkKWWidget);

  // Description:
  // Update the menu from PVSource.
  // When remove_all=0, Update methods does not remove the
  // entires already added to the widget. This provides for a means
  // to explicitly add entries.
  void Update(int remove_all=1);

  // Description:
  // Sets the current array.
  void SetValue(const char* arrayname, int field);
  void SetValue(const char* label);

protected:
  vtkPVColorSelectionWidget();
  ~vtkPVColorSelectionWidget();

  vtkPVSource* PVSource;
  vtkKWWidget* Target;
  char* ColorSelectionCommand;

  void AddArray(vtkPVDataSetAttributesInformation* attrInfo, int field_type);


  int FormLabel(vtkPVArrayInformation* arrayInfo, int field, char *label);
private:
  vtkPVColorSelectionWidget(const vtkPVColorSelectionWidget&); // Not implemented.
  void operator=(const vtkPVColorSelectionWidget&); // Not implemented.
  
};

#endif
