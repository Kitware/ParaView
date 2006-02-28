/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotArraySelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPlotArraySelection - array selection widget with additional
// options for each array such as color. Created for use with the Plot Display.
// .SECTION Description
// vtkPVArraySelection with options to choose color per array. In future, we may 
// provide other options such as symbol.

#ifndef __vtkPVPlotArraySelection_h
#define __vtkPVPlotArraySelection_h


#include "vtkPVArraySelection.h"

class vtkCollection;
class vtkSMDoubleVectorProperty;

class VTK_EXPORT vtkPVPlotArraySelection : public vtkPVArraySelection
{
public:
  static vtkPVPlotArraySelection* New();
  vtkTypeRevisionMacro(vtkPVPlotArraySelection, vtkPVArraySelection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Callback when color is changed.
  void ArrayColorCallback(double, double, double);

  // Description:
  // Access to change this widgets state from a script. Used for tracing.
  void SetArrayStatus(const char* array_name, int status, double r, double g, double b);
  void SetArrayStatus(const char *name, int status)
    { this->Superclass::SetArrayStatus(name, status); }

  // Description:
  // Set the property used to pass the color selections to the server.
  void SetColorProperty(vtkSMDoubleVectorProperty*);
  vtkGetObjectMacro(ColorProperty, vtkSMDoubleVectorProperty);

protected:
  vtkPVPlotArraySelection();
  ~vtkPVPlotArraySelection();
  
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  virtual void CreateNewGUI();
  virtual void SetPropertyFromGUI();

  vtkCollection* ArrayColorButtons;
  vtkSMDoubleVectorProperty* ColorProperty;
private:
  vtkPVPlotArraySelection(const vtkPVPlotArraySelection&); // Not implemented.
  void operator=(const vtkPVPlotArraySelection&); // Not implemented.
};

#endif

