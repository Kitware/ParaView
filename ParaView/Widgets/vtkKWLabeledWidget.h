/*=========================================================================

  Module:    vtkKWLabeledWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledWidget - an abstract class widget with a label
// .SECTION Description
// This class creates a superclass for composite widgets associating a label to 
// a widget.
// .SECTION See Also
// vtkKWLabeledEntry

#ifndef __vtkKWLabeledWidget_h
#define __vtkKWLabeledWidget_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWLabel;

class VTK_EXPORT vtkKWLabeledWidget : public vtkKWWidget
{
public:
  static vtkKWLabeledWidget* New();
  vtkTypeRevisionMacro(vtkKWLabeledWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal label
  vtkGetObjectMacro(Label, vtkKWLabel);

  // Description:
  // Convenience method to set the contents label.
  void SetLabel(const char *);
  
  // Description:
  // Convenience method to set/get the label width.
  void SetLabelWidth(int width);
  int GetLabelWidth();
  
  // Description:
  // Show/Hide the label.
  virtual void SetShowLabel(int);
  vtkBooleanMacro(ShowLabel, int);
  vtkGetMacro(ShowLabel, int);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLabeledWidget();
  ~vtkKWLabeledWidget();

  vtkKWLabel      *Label;

  int ShowLabel;

  // Pack or repack the widget. To be implemented by subclasses.

  virtual void Pack() {};

private:
  vtkKWLabeledWidget(const vtkKWLabeledWidget&); // Not implemented
  void operator=(const vtkKWLabeledWidget&); // Not implemented
};


#endif

