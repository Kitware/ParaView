/*=========================================================================

  Module:    vtkKWLabeledLabel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledLabel - an entry with a label
// .SECTION Description
// The LabeledLabel creates a label with another label in front of it; both are
// contained in a frame


#ifndef __vtkKWLabeledLabel_h
#define __vtkKWLabeledLabel_h

#include "vtkKWLabeledWidget.h"

class vtkKWLabel;

class VTK_EXPORT vtkKWLabeledLabel : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledLabel* New();
  vtkTypeRevisionMacro(vtkKWLabeledLabel, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal object
  vtkGetObjectMacro(Label2, vtkKWLabel);

  // Description:
  // Convenience method to set the contents label2.
  void SetLabel2(const char *);
  
  // Description:
  // Set the widget packing order to be horizontal (default).
  virtual void SetPackHorizontally(int);
  vtkBooleanMacro(PackHorizontally, int);
  vtkGetMacro(PackHorizontally, int);

  // Description:
  // Set the 2nd label to auto-expand (does not by default).
  virtual void SetExpandLabel2(int);
  vtkBooleanMacro(ExpandLabel2, int);
  vtkGetMacro(ExpandLabel2, int);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Set the anchoring for the first label
  virtual void SetLabelAnchor(int);
  vtkBooleanMacro(LabelAnchor, int);
  vtkGetMacro(LabelAnchor, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLabeledLabel();
  ~vtkKWLabeledLabel();

  int PackHorizontally;
  int ExpandLabel2;
  int LabelAnchor;

  vtkKWLabel *Label2;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWLabeledLabel(const vtkKWLabeledLabel&); // Not implemented
  void operator=(const vtkKWLabeledLabel&); // Not implemented
};

#endif

